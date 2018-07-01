#ifndef JTAG_SIMULTAION_DATA_GENERATOR_H
#define JTAG_SIMULTAION_DATA_GENERATOR_H

#include <AnalyzerHelpers.h>

class JtagAnalyzerSettings;

class JtagSimulationDataGenerator
{
public:
	JtagSimulationDataGenerator();
	~JtagSimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, JtagAnalyzerSettings* settings);
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	JtagAnalyzerSettings*	mSettings;
	U32						mSimulationSampleRateHz;

	enum { SPACE_TCK = 12 };

protected:

	ClockGenerator mClockGenerator;

	void CreateJtagTransaction();

	SimulationChannelDescriptorGroup	mJtagSimulationChannels;

	SimulationChannelDescriptor*	mTms;
	SimulationChannelDescriptor*	mTck;
	SimulationChannelDescriptor*	mTdi;
	SimulationChannelDescriptor*	mTdo;
	SimulationChannelDescriptor*	mTrst;
};

#endif	// JTAG_SIMULTAION_DATA_GENERATOR_H
