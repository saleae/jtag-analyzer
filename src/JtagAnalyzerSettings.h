#ifndef JTAG_ANALYZER_SETTINGS_H
#define JTAG_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

#include "JtagTypes.h"

enum BitOrder
{
	MSB_First,
	LSB_First,
};

class JtagAnalyzerSettings : public AnalyzerSettings
{
public:
	JtagAnalyzerSettings();
	virtual ~JtagAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	virtual void LoadSettings(const char* settings);
	virtual const char* SaveSettings();

	void UpdateInterfacesFromSettings();

	Channel		mTmsChannel;
	Channel		mTckChannel;
	Channel		mTdiChannel;
	Channel		mTdoChannel;
	Channel		mTrstChannel;

	JtagTAPState	mTAPInitialState;

	BitOrder		mInstructRegBitOrder;
	BitOrder		mDataRegBitOrder;

	bool			mShowBitCount;

protected:
	AnalyzerSettingInterfaceChannel		mTmsChannelInterface;
	AnalyzerSettingInterfaceChannel		mTckChannelInterface;
	AnalyzerSettingInterfaceChannel		mTdiChannelInterface;
	AnalyzerSettingInterfaceChannel		mTdoChannelInterface;
	AnalyzerSettingInterfaceChannel		mTrstChannelInterface;

	AnalyzerSettingInterfaceNumberList	mTAPInitialStateInterface;

	AnalyzerSettingInterfaceNumberList	mInstructRegBitOrderInterface;
	AnalyzerSettingInterfaceNumberList	mDataRegBitOrderInterface;

	AnalyzerSettingInterfaceBool		mShowBitCountInterface;
};

#endif	// JTAG_ANALYZER_SETTINGS_H
