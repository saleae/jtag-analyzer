#ifndef JTAG_ANALYZER_RESULTS_H
#define JTAG_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

#include <set>

#include "JtagTypes.h"

class JtagAnalyzer;
class JtagAnalyzerSettings;

class JtagAnalyzerResults: public AnalyzerResults
{
public:
	JtagAnalyzerResults(JtagAnalyzer* analyzer, JtagAnalyzerSettings* settings);
	virtual ~JtagAnalyzerResults();

	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

	void AddShiftedData(const JtagShiftedData& shifted_data);

	// returns the TAP state description
	static const char* GetStateDescLong(const JtagTAPState mCurrTAPState);
	static const char* GetStateDescShort(const JtagTAPState mCurrTAPState);

protected:	// functions

protected:	// vars

	JtagAnalyzerSettings*	mSettings;
	JtagAnalyzer*			mAnalyzer;

	// TDI/TDO bits
	std::set<JtagShiftedData>		mShiftedData;
};

#endif	// JTAG_ANALYZER_RESULTS_H
