#include <AnalyzerHelpers.h>

#include "JtagAnalyzerSettings.h"
#include "JtagAnalyzerResults.h"
#include "JtagTypes.h"

JtagAnalyzerSettings::JtagAnalyzerSettings()
:	mTmsChannel(UNDEFINED_CHANNEL),
	mTckChannel(UNDEFINED_CHANNEL),
	mTdiChannel(UNDEFINED_CHANNEL),
	mTdoChannel(UNDEFINED_CHANNEL),
	mTrstChannel(UNDEFINED_CHANNEL),
	mTAPInitialState(RunTestIdle),
	mInstructRegBitOrder(LSB_First),
	mDataRegBitOrder(LSB_First),
	mShowBitCount( false )
{
	// init the interfaces
	mTmsChannelInterface.SetTitleAndTooltip("TMS", "JTAG Test mode select");
	mTmsChannelInterface.SetChannel(mTmsChannel);

	mTckChannelInterface.SetTitleAndTooltip("TCK", "JTAG Test clock");
	mTckChannelInterface.SetChannel(mTckChannel);

	mTdiChannelInterface.SetTitleAndTooltip("TDI", "JTAG Test data input");
	mTdiChannelInterface.SetChannel(mTdiChannel);
	mTdiChannelInterface.SetSelectionOfNoneIsAllowed(true);

	mTdoChannelInterface.SetTitleAndTooltip("TDO", "JTAG Test data output");
	mTdoChannelInterface.SetChannel(mTdoChannel);
	mTdoChannelInterface.SetSelectionOfNoneIsAllowed(true);

	mTrstChannelInterface.SetTitleAndTooltip("TRST", "JTAG Test reset");
	mTrstChannelInterface.SetChannel(mTrstChannel);
	mTrstChannelInterface.SetSelectionOfNoneIsAllowed(true);

	mTAPInitialStateInterface.SetTitleAndTooltip("TAP initial state", "JTAG TAP controller initial state");
	int state_cnt;
	for (state_cnt = 0; state_cnt < NUM_TAP_STATES; ++state_cnt)
		mTAPInitialStateInterface.AddNumber(state_cnt, JtagAnalyzerResults::GetStateDescLong(JtagTAPState(state_cnt)), JtagAnalyzerResults::GetStateDescLong(JtagTAPState(state_cnt)));

	mTAPInitialStateInterface.SetNumber(mTAPInitialState);

	mInstructRegBitOrderInterface.SetTitleAndTooltip("Shift-IR bit order", "Instruction register shift bit order");
	mInstructRegBitOrderInterface.AddNumber(MSB_First, "Most significant bit first", "Instruction register shift MOST significant bit first");
	mInstructRegBitOrderInterface.AddNumber(LSB_First, "Least significant bit first", "Instruction register shift LEAST significant bit first");
	mInstructRegBitOrderInterface.SetNumber(mInstructRegBitOrder);

	mDataRegBitOrderInterface.SetTitleAndTooltip("Shift-DR bit order", "Data register shift bit order");
	mDataRegBitOrderInterface.AddNumber(MSB_First, "Most significant bit first", "Data register shift MOST significant bit first");
	mDataRegBitOrderInterface.AddNumber(LSB_First, "Least significant bit first", "Data register shift LEAST significant bit first");
	mDataRegBitOrderInterface.SetNumber(mDataRegBitOrder);

	mShowBitCountInterface.SetTitleAndTooltip( "Show TDI/TDO bit counts", "Used to count bits sent during Shift state" );
	mShowBitCountInterface.SetValue( mShowBitCount );

	// add the interfaces
	AddInterface(&mTmsChannelInterface);
	AddInterface(&mTckChannelInterface);
	AddInterface(&mTdiChannelInterface);
	AddInterface(&mTdoChannelInterface);
	AddInterface(&mTrstChannelInterface);

	AddInterface(&mTAPInitialStateInterface);
	AddInterface(&mInstructRegBitOrderInterface);
	AddInterface(&mDataRegBitOrderInterface);

	AddInterface( &mShowBitCountInterface );

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "csv", "csv");
	AddExportExtension(0, "text", "txt");

	ClearChannels();

	AddChannel(mTmsChannel,	"TMS",	false);
	AddChannel(mTckChannel,	"TCK",	false);
	AddChannel(mTdiChannel,	"TDI",	false);
	AddChannel(mTdoChannel,	"TDO",	false);
	AddChannel(mTrstChannel,"TRST",	false);
}

JtagAnalyzerSettings::~JtagAnalyzerSettings()
{}

bool JtagAnalyzerSettings::SetSettingsFromInterfaces()
{
	const int NUM_CHANNELS = 5;

	Channel	all_channels[NUM_CHANNELS] = {	mTmsChannelInterface.GetChannel(),
											mTckChannelInterface.GetChannel(),
											mTdiChannelInterface.GetChannel(),
											mTdoChannelInterface.GetChannel(),
											mTrstChannelInterface.GetChannel()};

	if (all_channels[0] == UNDEFINED_CHANNEL  ||  all_channels[1] == UNDEFINED_CHANNEL)
	{
		SetErrorText("Please select inputs for TMS and TCK channels.");
		return false;
	}

	if (AnalyzerHelpers::DoChannelsOverlap(all_channels, 5))
	{
		SetErrorText("Please select different channels for each input.");
		return false;
	}

	mTmsChannel = all_channels[0];
	mTckChannel = all_channels[1];
	mTdiChannel = all_channels[2];
	mTdoChannel = all_channels[3];
	mTrstChannel = all_channels[4];

	ClearChannels();

	AddChannel(mTmsChannel,	"TMS",	true);
	AddChannel(mTckChannel,	"TCK",	true);
	AddChannel(mTdiChannel,	"TDI",	mTdiChannel != UNDEFINED_CHANNEL);
	AddChannel(mTdoChannel,	"TDO",	mTdoChannel != UNDEFINED_CHANNEL);
	AddChannel(mTrstChannel,"TRST",	mTrstChannel != UNDEFINED_CHANNEL);

	// the TAP initial state
	int cast2Int = int(mTAPInitialStateInterface.GetNumber());
	mTAPInitialState = JtagTAPState(cast2Int);

	// the shift bit orders
	cast2Int = int(mInstructRegBitOrderInterface.GetNumber());
	mInstructRegBitOrder = BitOrder(cast2Int);

	cast2Int = int(mDataRegBitOrderInterface.GetNumber());
	mDataRegBitOrder = BitOrder(cast2Int);

	mShowBitCount = mShowBitCountInterface.GetValue();

	return true;
}

void JtagAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mTmsChannelInterface.SetChannel(mTmsChannel);
	mTckChannelInterface.SetChannel(mTckChannel);
	mTdiChannelInterface.SetChannel(mTdiChannel);
	mTdoChannelInterface.SetChannel(mTdoChannel);
	mTrstChannelInterface.SetChannel(mTrstChannel);

	mTAPInitialStateInterface.SetNumber(mTAPInitialState);
	mInstructRegBitOrderInterface.SetNumber(mInstructRegBitOrder);
	mDataRegBitOrderInterface.SetNumber(mDataRegBitOrder);
	mShowBitCountInterface.SetValue( mShowBitCount );
}

void JtagAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mTmsChannel;
	text_archive >> mTckChannel;
	text_archive >> mTdiChannel;
	text_archive >> mTdoChannel;
	text_archive >> mTrstChannel;

	int ival;
	text_archive >> ival;
	if (ival >= 0  &&  ival < NUM_TAP_STATES)
		mTAPInitialState = JtagTAPState(ival);

	text_archive >> ival;
	if (ival == MSB_First  ||  ival == LSB_First)
		mInstructRegBitOrder = BitOrder(ival);

	text_archive >> ival;
	if (ival == MSB_First  ||  ival == LSB_First)
		mDataRegBitOrder = BitOrder(ival);

	text_archive >> mShowBitCount; //added after 1.2.3. Defaults false on failure to load.



	ClearChannels();

	AddChannel(mTmsChannel,	"TMS",	true);
	AddChannel(mTckChannel,	"TCK",	true);
	AddChannel(mTdiChannel,	"TDI",	mTdiChannel != UNDEFINED_CHANNEL);
	AddChannel(mTdoChannel,	"TDO",	mTdoChannel != UNDEFINED_CHANNEL);
	AddChannel(mTrstChannel,"TRST",	mTrstChannel != UNDEFINED_CHANNEL);

	UpdateInterfacesFromSettings();
}

const char* JtagAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mTmsChannel;
	text_archive << mTckChannel;
	text_archive << mTdiChannel;
	text_archive << mTdoChannel;
	text_archive << mTrstChannel;
	text_archive << int(mTAPInitialState);
	text_archive << int(mInstructRegBitOrder);
	text_archive << int(mDataRegBitOrder);

	text_archive << mShowBitCount; //added after 1.2.3

	return SetReturnString(text_archive.GetString());
}
