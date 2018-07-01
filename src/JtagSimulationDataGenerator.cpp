#include "JtagSimulationDataGenerator.h"
#include "JtagAnalyzerSettings.h"

#include "JtagTypes.h"

JtagSimulationDataGenerator::JtagSimulationDataGenerator()
{}

JtagSimulationDataGenerator::~JtagSimulationDataGenerator()
{}

void JtagSimulationDataGenerator::Initialize(U32 simulation_sample_rate, JtagAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

	mTms = mJtagSimulationChannels.Add(settings->mTmsChannel, mSimulationSampleRateHz, BIT_HIGH);
	mTck = mJtagSimulationChannels.Add(settings->mTckChannel, mSimulationSampleRateHz, BIT_LOW);

	if (settings->mTdiChannel != UNDEFINED_CHANNEL)
		mTdi = mJtagSimulationChannels.Add(settings->mTdiChannel, mSimulationSampleRateHz, BIT_LOW);
	else
		mTdi = NULL;

	if (settings->mTdoChannel != UNDEFINED_CHANNEL)
		mTdo = mJtagSimulationChannels.Add(settings->mTdoChannel, mSimulationSampleRateHz, BIT_LOW);
	else
		mTdo = NULL;

	if (settings->mTrstChannel != UNDEFINED_CHANNEL)
		mTrst = mJtagSimulationChannels.Add(settings->mTrstChannel, mSimulationSampleRateHz, BIT_HIGH);
	else
		mTrst = NULL;

	mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(10));	// insert 10 bit-periods of idle
	mJtagSimulationChannels.AdvanceAll( simulation_sample_rate * 0.0003); //advance by 0.3ms
}

U32 JtagSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mTck->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
		CreateJtagTransaction();

	*simulation_channels = mJtagSimulationChannels.GetArray();

	return mJtagSimulationChannels.GetCount();
}

void JtagSimulationDataGenerator::CreateJtagTransaction()
{
	// This array represents the number of clocks for each TMS state.
	// The first element is the number of clocks of high on TMS.
	// If it's negative it means we also have to clock data on TDI/TDO
	// 0 is end of array
	const int tms_state_clocks[] =
		{ 5,  1,  2,  1, -8,  1,  4,  1, -8,  3,  1,  -80,  1,  4,  1,  -18,  2,  100,
		  2, 2, -170, 1, 1, 1, -256, 5, 0};

	U8 tdi_data = 0x80;		// shift data
	U8 tdo_data = 0x0A;
	U8 data_bits = 0;

	// set TMS, TCK and TRST to their initial states
	mTms->TransitionIfNeeded(BIT_HIGH);
	mTck->TransitionIfNeeded(BIT_LOW);
	if (mTrst != NULL)
		mTrst->TransitionIfNeeded(BIT_HIGH);

	mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_TCK));

	int arr_cnt;
	int num_clocks, clock_cnt, total_clocks = 0;
	BitState tms_state = BIT_LOW;
	for (arr_cnt = 0; tms_state_clocks[arr_cnt] != 0; ++arr_cnt)
	{
		if (tms_state_clocks[arr_cnt] < 0)			// shift data state?
		{
			num_clocks = -tms_state_clocks[arr_cnt];

			// we're shifting data; switch to low
			tms_state = BIT_LOW;

		} else {
			num_clocks = tms_state_clocks[arr_cnt];

			// change TMS state since we're not shifting data
			tms_state = Invert(tms_state);
		}

		mTms->TransitionIfNeeded(tms_state);

		for (clock_cnt = 0; clock_cnt < num_clocks; clock_cnt++)
		{
			mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_TCK));

			mTck->Transition();		// TCK goes high

			mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_TCK));

			mTck->Transition();		// TCK goes low

			// are we shifting data?
			if (tms_state_clocks[arr_cnt] < 0)
			{
				// add data on TDI/TDO
				if (mTdi != NULL)
					mTdi->TransitionIfNeeded((tdi_data & (0x80 >> data_bits)) ? BIT_HIGH : BIT_LOW);
				if (mTdo != NULL)
					mTdo->TransitionIfNeeded((tdo_data & (0x80 >> data_bits)) ? BIT_HIGH : BIT_LOW);

				data_bits = (data_bits + 1) & 7;
				if (data_bits == 0)
				{
					++tdi_data;
					++tdo_data;
				}
			}

			++total_clocks;
		}

		// simulate an active TRST if we have one selected
		if (mTrst != NULL  &&  total_clocks > 290)
			mTrst->TransitionIfNeeded(BIT_LOW);
	}

	// make a gap
	mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_TCK * 10));
}
