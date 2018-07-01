#include <iostream>
#include <fstream>
#include <algorithm>

#include <AnalyzerHelpers.h>

#include "JtagAnalyzerResults.h"
#include "JtagAnalyzer.h"
#include "JtagAnalyzerSettings.h"

const char* TAPStateDescLong[] = 
{
	"Test-Logic-Reset",
	"Run-Test/Idle",

	"Select-DR-Scan",
	"Capture-DR",
	"Shift-DR",
	"Exit1-DR",
	"Pause-DR",
	"Exit2-DR",
	"Update-DR",

	"Select-IR-Scan",
	"Capture-IR",
	"Shift-IR",
	"Exit1-IR",
	"Pause-IR",
	"Exit2-IR",
	"Update-IR"
};

const char* TAPStateDescShort[] = 
{
	"TstLogRst",
	"RunTstIdl",

	"SelDRScn",
	"CapDR",
	"ShDR",
	"Ex1DR",
	"PsDR",
	"Ex2DR",
	"UpdDR",

	"SelIRScn",
	"CapIR",
	"ShIR",
	"Ex1IR",
	"PsIR",
	"Ex2IR",
	"UpdIR"
};

JtagAnalyzerResults::JtagAnalyzerResults(JtagAnalyzer* analyzer, JtagAnalyzerSettings* settings)
:	mSettings(settings),
	mAnalyzer(analyzer)
{}

JtagAnalyzerResults::~JtagAnalyzerResults()
{}

void JtagAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame f = GetFrame(frame_index);

	if (channel == mSettings->mTmsChannel)
	{
		// add the TAP state descriptions to the TMS channel
		AddResultString(GetStateDescLong((JtagTAPState) f.mType));
		AddResultString(GetStateDescShort((JtagTAPState) f.mType));

	} else if (channel == mSettings->mTdiChannel  ||  channel == mSettings->mTdoChannel) {

		// find this frame's TDI/TDO data
		JtagShiftedData sd;
		sd.mStartSampleIndex = f.mStartingSampleInclusive;
		std::set<JtagShiftedData>::iterator sdi(mShiftedData.find(sd));

		// found?
		if( sdi != mShiftedData.end() )
		{
			std::string tdi_tdo_result_string = ( channel == mSettings->mTdiChannel ? sdi->GetTDIString( display_base, JtagShiftedData::TdiTdoStringFormat::Ellipsis256 ).c_str() : sdi->GetTDOString( display_base, JtagShiftedData::TdiTdoStringFormat::Ellipsis256 ).c_str() );

			if( mSettings->mShowBitCount )
			{
				std::string bit_count_string = ( channel == mSettings->mTdiChannel ? sdi->GetTDILengthString() : sdi->GetTDOLengthString() );
				tdi_tdo_result_string = tdi_tdo_result_string + " " + bit_count_string;
			}

			AddResultString( tdi_tdo_result_string.c_str() );

			int max_lengths[4] = { 5, 10, 15, 25 };

			for( int i = 0; i < 4; ++i )
			{
				if( tdi_tdo_result_string.length() > max_lengths[i] )
				{
					std::string short_str = tdi_tdo_result_string.substr( 0, max_lengths[i]-3 );
					short_str += "...";
					AddResultString( short_str.c_str() );
				}
			}
		}
			
	}

   // char number_str[128];
   // AnalyzerHelpers::GetNumberString( f.mData1, display_base, 8, number_str, 128 );
   // AddResultString( number_str );
}

void JtagAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	if( mSettings->mShowBitCount )
		file_stream << "Time [s];TAP state;TDI;TDO;TDIBitCount;TDOBitCount" << std::endl;
	else
		file_stream << "Time [s];TAP state;TDI;TDO" << std::endl;

	Frame frm;
	JtagShiftedData sd;
	std::set<JtagShiftedData>::iterator sdi;
	char time_str[128];
	std::string tdi_str, tdo_str, tdi_count_str, tdo_count_str;
	const U64 num_frames = GetNumFrames();
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		frm = GetFrame(fcnt);

		// make the time string
		AnalyzerHelpers::GetTimeString(frm.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

		// get the TAP state
		JtagTAPState tap_state = (JtagTAPState) frm.mType;

		// make the TDI/TDO data if we're in a shift state
		tdi_str.clear();
		tdo_str.clear();
		tdi_count_str.clear();
		tdo_count_str.clear();
		if (tap_state == ShiftIR  ||  tap_state == ShiftDR)
		{
			// find TDI/TDO data
			sd.mStartSampleIndex = frm.mStartingSampleInclusive;
			sdi = mShiftedData.find(sd);

			// found?
			if (sdi != mShiftedData.end())
			{
				tdi_str = sdi->GetTDIString(display_base, JtagShiftedData::TdiTdoStringFormat::Break64 );
				tdo_str = sdi->GetTDOString(display_base, JtagShiftedData::TdiTdoStringFormat::Break64 );

				if( mSettings->mShowBitCount )
				{
					tdi_count_str = sdi->GetTDILengthString( false );
					tdo_count_str = sdi->GetTDOLengthString( false );
				}

			}
		}

		// output
		if( mSettings->mShowBitCount )
			file_stream << time_str << ";" << GetStateDescLong( tap_state ) << ";" << tdi_str << ";" << tdo_str << ";" << tdi_count_str << ";" << tdo_count_str << std::endl;
		else
			file_stream << time_str << ";" << GetStateDescLong(tap_state) << ";" << tdi_str << ";" << tdo_str << std::endl;

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void JtagAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
    ClearTabularText();

    Frame f = GetFrame(frame_index);

    std::vector< std::string > result_strings;

    ClearTabularText();

    bool tms_used = true;
    bool tdi_used = true;
    bool tdo_used = true;

    if( mSettings->mTmsChannel == UNDEFINED_CHANNEL )
        tms_used = false;

    if( mSettings->mTdiChannel == UNDEFINED_CHANNEL )
        tdi_used = false;

    if( mSettings->mTdoChannel == UNDEFINED_CHANNEL )
        tdo_used = false;

    if( tms_used = true )
    {
        // add the TAP state descriptions to the TMS channel

        result_strings.push_back( GetStateDescLong((JtagTAPState) f.mType) );
        //result_strings.push_back( GetStateDescShort((JtagTAPState) f.mType) );
    }
    if (  tdi_used == true ||  tdo_used == true ) {

        // find this frame's TDI/TDO data
        JtagShiftedData sd;
        sd.mStartSampleIndex = f.mStartingSampleInclusive;
        std::set<JtagShiftedData>::iterator sdi(mShiftedData.find(sd));

        // found?
        if (sdi != mShiftedData.end())
        {
			if( tdi_used == true )
			{
				std::string tdi_str = sdi->GetTDIString( display_base, JtagShiftedData::TdiTdoStringFormat::Ellipsis256 );

				if( mSettings->mShowBitCount )
					tdi_str += " " + sdi->GetTDILengthString();

				result_strings.push_back( tdi_str.c_str() );
			}

			if( tdo_used == true )
			{
				std::string tdo_str = sdi->GetTDOString( display_base, JtagShiftedData::TdiTdoStringFormat::Ellipsis256 );

				if( mSettings->mShowBitCount )
					tdo_str += " " + sdi->GetTDOLengthString();

				result_strings.push_back( tdo_str.c_str() );
			}
        }
    }

    for( int i = 0; i < result_strings.size(); i++ )
    {
        AddTabularText( result_strings[i].c_str());
    }

}

void JtagAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void JtagAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void JtagAnalyzerResults::AddShiftedData(const JtagShiftedData& shifted_data)
{
	mShiftedData.insert(shifted_data);
}

const char* JtagAnalyzerResults::GetStateDescLong(const JtagTAPState mCurrTAPState)
{
	if (mCurrTAPState > UpdateIR)
		return "<undefined>";

	return TAPStateDescLong[mCurrTAPState];
}

const char* JtagAnalyzerResults::GetStateDescShort(const JtagTAPState mCurrTAPState)
{
	if (mCurrTAPState > UpdateIR)
		return "<undef>";

	return TAPStateDescShort[mCurrTAPState];
}
