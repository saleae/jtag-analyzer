#include <AnalyzerChannelData.h>

#include <algorithm>

#include "JtagAnalyzer.h"
#include "JtagAnalyzerSettings.h"

JtagAnalyzer::JtagAnalyzer() : mSimulationInitilized( false )
{
    UseFrameV2();
    SetAnalyzerSettings( &mSettings );
}

JtagAnalyzer::~JtagAnalyzer()
{
    KillThread();
}

void JtagAnalyzer::SyncToSample( U64 to_sample )
{
    mTms->AdvanceToAbsPosition( to_sample );
    mTck->AdvanceToAbsPosition( to_sample );
    if( mTdi != NULL )
        mTdi->AdvanceToAbsPosition( to_sample );
    if( mTdo != NULL )
        mTdo->AdvanceToAbsPosition( to_sample );
    if( mTrst != NULL )
        mTrst->AdvanceToAbsPosition( to_sample );
}

void JtagAnalyzer::AdvanceTck( Frame& frm, JtagShiftedData& shifted_data )
{
    if( mTrst != NULL && mTrst->WouldAdvancingToAbsPositionCauseTransition( mTck->GetSampleOfNextEdge() ) )
    {
        mTrst->AdvanceToNextEdge();

        // close the frame and add it
        CloseFrameV2( frm, shifted_data, mTrst->GetSampleNumber() );

        // reset the TAP state
        mTAPCtrl.SetState( TestLogicReset );

        // prepare the reset frame
        frm.mStartingSampleInclusive = mTrst->GetSampleNumber() + 1;
        frm.mType = mTAPCtrl.GetCurrState();

        // find the rising edge of TRST
        mTrst->AdvanceToNextEdge();

        // bring TCK here too
        mTck->AdvanceToAbsPosition( mTrst->GetSampleNumber() );
    }
    else
    {
        mTck->AdvanceToNextEdge();
    }
}

void JtagAnalyzer::Setup()
{
    // get the channel data pointers
    mTms = GetAnalyzerChannelData( mSettings.mTmsChannel );
    mTck = GetAnalyzerChannelData( mSettings.mTckChannel );
    if( mSettings.mTdiChannel != UNDEFINED_CHANNEL )
        mTdi = GetAnalyzerChannelData( mSettings.mTdiChannel );
    else
        mTdi = NULL;

    if( mSettings.mTdoChannel != UNDEFINED_CHANNEL )
        mTdo = GetAnalyzerChannelData( mSettings.mTdoChannel );
    else
        mTdo = NULL;

    if( mSettings.mTrstChannel != UNDEFINED_CHANNEL )
        mTrst = GetAnalyzerChannelData( mSettings.mTrstChannel );
    else
        mTrst = NULL;
}

std::vector<U8> BitsToBytes( std::vector<U8>& shifted_data )
{
    std::vector<U8> byteArray;

    // get the numerical value from the bits
    std::vector<U8>::const_iterator bsi( shifted_data.begin() );

    U8 val;
    size_t bits_remaining = shifted_data.size();

    // make an array of 8 bit values
    // e.g. for 10 bits, byteArray[0] would contain the first 2 bits
    // and byteArray[1] would contain the next 8 bits.
    for( ; bsi != shifted_data.end(); )
    {
        for( val = 0; bsi != shifted_data.end(); )
        {
            val = ( val << 1 ) | ( *bsi == BIT_HIGH ? 1 : 0 );

            --bits_remaining;
            ++bsi;

            if( ( bits_remaining % 8 ) == 0 )
            {
                // We've reached a byte boundary
                break;
            }
        }

        byteArray.emplace_back( val );
    }

    return byteArray;
}

void JtagAnalyzer::CloseFrameV2( Frame& frm, JtagShiftedData& shifted_data, U64 ending_sample_number )
{
    JtagShiftedData corrected_shifted_data;

    CloseFrame( frm, shifted_data, ending_sample_number, &corrected_shifted_data );

    FrameV2 frame_v2;

    size_t max_bit_count = 0;

    if( corrected_shifted_data.mTdiBits.size() > 0 )
    {
        std::vector<U8> data = BitsToBytes( corrected_shifted_data.mTdiBits );

        frame_v2.AddByteArray( "TDI", &data[ 0 ], data.size() );

        if( max_bit_count < corrected_shifted_data.mTdiBits.size() )
        {
            max_bit_count = corrected_shifted_data.mTdiBits.size();
        }
    }

    if( corrected_shifted_data.mTdoBits.size() > 0 )
    {
        std::vector<U8> data = BitsToBytes( corrected_shifted_data.mTdoBits );

        frame_v2.AddByteArray( "TDO", &data[ 0 ], data.size() );

        if( max_bit_count < corrected_shifted_data.mTdoBits.size() )
        {
            max_bit_count = corrected_shifted_data.mTdoBits.size();
        }
    }

    frame_v2.AddInteger( "BitCount", max_bit_count );

    const char* type = JtagAnalyzerResults::GetStateDescShort( mTAPCtrl.GetCurrState() );

    mResults->AddFrameV2( frame_v2, type, frm.mStartingSampleInclusive, frm.mEndingSampleInclusive );
}

void JtagAnalyzer::CloseFrame( Frame& frm, JtagShiftedData& shifted_data, U64 ending_sample_number,
                               JtagShiftedData* corrected_shifted_data )
{
    // save the TDI/TDO values in the frame
    if( frm.mType == ShiftIR || frm.mType == ShiftDR )
    {
        // mind the bit order
        if( ( frm.mType == ShiftIR && mSettings.mInstructRegBitOrder == LSB_First ) ||
            ( frm.mType == ShiftDR && mSettings.mDataRegBitOrder == LSB_First ) )
        {
            std::reverse( shifted_data.mTdiBits.begin(), shifted_data.mTdiBits.end() );
            std::reverse( shifted_data.mTdoBits.begin(), shifted_data.mTdoBits.end() );
        }

        mResults->AddShiftedData( shifted_data );

        if( corrected_shifted_data != NULL )
        {
            corrected_shifted_data->mTdiBits = shifted_data.mTdiBits;
            corrected_shifted_data->mTdoBits = shifted_data.mTdoBits;
        }

        shifted_data.mTdiBits.clear();
        shifted_data.mTdoBits.clear();
    }

    frm.mEndingSampleInclusive = ending_sample_number;
    mResults->AddFrame( frm );
}

void JtagAnalyzer::WorkerThread()
{
    Setup();

    // make sure that we enter the loop on TRST high (inactive)
    if( mTrst != NULL && mTrst->GetBitState() == BIT_LOW )
    {
        // since we started on a active TRST we'll
        // ignore the initial state from the settings
        mTAPCtrl.SetState( TestLogicReset );

        // advance to the rising edge of TRST
        mTrst->AdvanceToNextEdge();
        SyncToSample( mTrst->GetSampleNumber() );
    }
    else
    {
        mTAPCtrl.SetState( mSettings.mTAPInitialState );
    }

    Frame frm;
    frm.mStartingSampleInclusive = mTck->GetSampleNumber();
    frm.mType = mTAPCtrl.GetCurrState();
    frm.mFlags = 0;
    frm.mData1 = 0;
    frm.mData2 = 0;

    JtagShiftedData shifted_data;

    for( ;; )
    {
        // advance TCK to the rising edge
        AdvanceTck( frm, shifted_data );
        if( mTck->GetBitState() == BIT_LOW )
            AdvanceTck( frm, shifted_data );

        // advance all other channels here too
        SyncToSample( mTck->GetSampleNumber() );

        // mark the rising edge of TCK
        mResults->AddMarker( mTck->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings.mTckChannel );

        // save TDI and TDO states markers and data
        if( mTAPCtrl.GetCurrState() == ShiftIR || mTAPCtrl.GetCurrState() == ShiftDR )
        {
            size_t bitCount = 0;

            if( mTdi != NULL )
            {
                mResults->AddMarker( mTdi->GetSampleNumber(),
                                     ( mTdi->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero ),
                                     mSettings.mTdiChannel );
                shifted_data.mTdiBits.push_back( mTdi->GetBitState() );
                bitCount = shifted_data.mTdiBits.size();
            }

            if( mTdo != NULL )
            {
                mResults->AddMarker( mTdo->GetSampleNumber(),
                                     ( mTdo->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero ),
                                     mSettings.mTdoChannel );
                shifted_data.mTdoBits.push_back( mTdo->GetBitState() );
                bitCount = shifted_data.mTdoBits.size();
            }

            if( ( mSettings.mShiftDRBitsPerDataUnit != 0 ) && ( bitCount >= mSettings.mShiftDRBitsPerDataUnit ) )
            {
                CloseFrameV2( frm, shifted_data, mTck->GetSampleNumber() );

                // prepare the next frame
                frm.mStartingSampleInclusive = mTck->GetSampleNumber() + 1;
                frm.mType = mTAPCtrl.GetCurrState();

                shifted_data.mStartSampleIndex = frm.mStartingSampleInclusive;

                mResults->CommitResults();
            }
        }

        // send TMS state to the TAP state machine - returns true if state machine has changed
        if( mTAPCtrl.AdvanceState( mTms->GetBitState() ) )
        {
            mResults->AddMarker( mTms->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mTmsChannel );

            CloseFrameV2( frm, shifted_data, mTck->GetSampleNumber() );

            // prepare the next frame
            frm.mStartingSampleInclusive = mTck->GetSampleNumber() + 1;
            frm.mType = mTAPCtrl.GetCurrState();

            shifted_data.mStartSampleIndex = frm.mStartingSampleInclusive;

            mResults->CommitResults();
        }

        // update progress bar
        ReportProgress( mTck->GetSampleNumber() );
    }
}

bool JtagAnalyzer::NeedsRerun()
{
    return false;
}

void JtagAnalyzer::SetupResults()
{
    mResults.reset( new JtagAnalyzerResults( this, &mSettings ) );
    SetAnalyzerResults( mResults.get() );

    // set which channels will carry bubbles
    mResults->AddChannelBubblesWillAppearOn( mSettings.mTmsChannel );
    if( mSettings.mTdiChannel != UNDEFINED_CHANNEL )
        mResults->AddChannelBubblesWillAppearOn( mSettings.mTdiChannel );
    if( mSettings.mTdoChannel != UNDEFINED_CHANNEL )
        mResults->AddChannelBubblesWillAppearOn( mSettings.mTdoChannel );
}

U32 JtagAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                          SimulationChannelDescriptor** simulation_channels )
{
    if( !mSimulationInitilized )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 JtagAnalyzer::GetMinimumSampleRateHz()
{
    // whatever
    return 9600;
}

const char* JtagAnalyzer::GetAnalyzerName() const
{
    return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
    return "JTAG";
}

Analyzer* CreateAnalyzer()
{
    return new JtagAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
