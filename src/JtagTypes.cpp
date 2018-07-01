#include <AnalyzerHelpers.h>

#include <algorithm>

#include "JtagTypes.h"

struct JtagTAPStateChange
{
	JtagTAPState	tms_low_change;
	JtagTAPState	tms_high_change;
};

// This maps each TAP state into it's next state depending on the TMS line
const JtagTAPStateChange tap_state_change_map[] = 
{
	{RunTestIdle, TestLogicReset},		// TestLogicReset
	{RunTestIdle, SelectDRScan},		// RunTestIdle

	{CaptureDR, SelectIRScan},			// SelectDRScan
	{ShiftDR, Exit1DR},					// CaptureDR
	{ShiftDR, Exit1DR},					// ShiftDR
	{PauseDR, UpdateDR},				// Exit1DR
	{PauseDR, Exit2DR},					// PauseDR
	{ShiftDR, UpdateDR},				// Exit2DR
	{RunTestIdle, SelectDRScan},		// UpdateDR

	{CaptureIR, TestLogicReset},		// SelectIRScan
	{ShiftIR, Exit1IR},					// CaptureIR
	{ShiftIR, Exit1IR},					// ShiftIR
	{PauseIR, UpdateIR},				// Exit1IR
	{PauseIR, Exit2IR},					// PauseIR
	{ShiftIR, UpdateIR},				// Exit2IR
	{RunTestIdle, SelectDRScan},		// UpdateIR
};

JtagTAP_Controller::JtagTAP_Controller()
	: mCurrTAPState(RunTestIdle)
{}

bool JtagTAP_Controller::AdvanceState(BitState tms_state)
{
	JtagTAPState new_state;

	if (tms_state == BIT_HIGH)
		new_state = tap_state_change_map[mCurrTAPState].tms_high_change;
	else
		new_state = tap_state_change_map[mCurrTAPState].tms_low_change;

	bool ret_val = new_state != mCurrTAPState;

	mCurrTAPState = new_state;

	return ret_val;
}

std::string JtagShiftedData::GetDecimalString(const std::vector<U8>& bits)
{
	std::string ret_val("0");
	std::vector<U8>::const_iterator bi(bits.begin());
	int carry, digit;
	while (bi != bits.end())
	{
		carry = *bi ? 1 : 0;		

		// multiply ret_val by 2 and add the carry bit
		std::string::reverse_iterator ai(ret_val.rbegin());
		while (ai != ret_val.rend())
		{
			digit = (*ai - '0') * 2 + carry;
			*ai = (digit % 10) + '0';
			carry = digit / 10;

			++ai;
		}

		if (carry > 0)
			ret_val = char(carry + '0') + ret_val;

		++bi;
	}

	return ret_val;
}

std::string JtagShiftedData::GetASCIIString(const std::vector<U8>& bits)
{
	std::string ret_val;

	// Check if the value of the number represented by bits is less than 0x100.
	// If it is, we can make an ASCII out of it, otherwise use GetDecimalString()
	std::vector<U8>::const_iterator srch_hi(std::find(bits.begin(), bits.end(), BIT_HIGH));
	if (bits.end() - srch_hi <= 8)
	{
		// Get the numerical value from the bits.
		U64 val;

		// Only get the 8 least significant bits
		std::vector<U8>::const_iterator bsi(bits.end() - 8);
		for (val = 0; bsi != bits.end(); ++bsi)
			val = (val << 1) | (*bsi == BIT_HIGH ? 1 : 0);

		// make a string out of that value
		char number_str[32];
		AnalyzerHelpers::GetNumberString(val, ASCII, 8, number_str, sizeof(number_str));

		ret_val = number_str;
	} else {
		ret_val = '\'' + GetDecimalString(bits) + '\'';
	}

	return ret_val;
}

std::string JtagShiftedData::GetHexOrBinaryString(const std::vector<U8>& bits, DisplayBase display_base)
{
	std::string ret_val;
	std::vector<U8>::const_iterator bsi(bits.begin());

	U64 val;
	size_t remain_bits = bits.size(), chunk_bits, bit_cnt;
	char number_str[128];
	while (bsi != bits.end())
	{
		chunk_bits = remain_bits % 64;
		if (chunk_bits == 0)
			chunk_bits = 64;

		// make a 64 bit value
		for (bit_cnt = chunk_bits, val = 0;
				bsi != bits.end()  &&  bit_cnt > 0;
				++bsi, --bit_cnt)
		{
			val = (val << 1) | (*bsi == BIT_HIGH ? 1 : 0);
		}

		// make a string out of that value
		AnalyzerHelpers::GetNumberString(val, display_base, (U32) chunk_bits, number_str, sizeof(number_str));

		// concat the 64bit chunks but chop off all but the first 0x or 0b
		if (ret_val.empty())
			ret_val = number_str;
		else
			ret_val += (number_str + 2);

		remain_bits -= chunk_bits;
	}

	return ret_val;
}

std::string JtagShiftedData::GetStringFromBitStates(const std::vector<U8>& bits, DisplayBase display_base, TdiTdoStringFormat format )
{
	if ( format == TdiTdoStringFormat::Ellipsis64 && bits.size() > 64 )
	{
		std::vector<U8> subset( bits.end() - 64, bits.end() );
		return GetStringFromBitStates( subset, display_base, format ) + "...";
	}

	if ( format == TdiTdoStringFormat::Ellipsis256 && bits.size() > 256 )
	{
		std::vector<U8> subset( bits.end() - 256, bits.end() );
		return GetStringFromBitStates( subset, display_base, format ) + "...";
	}

	if ( ( format == TdiTdoStringFormat::Break64 && bits.size() > 64 ) ||
		( format == TdiTdoStringFormat::Break256 && bits.size() > 256 ) )
	{
		//lets break the result into N shorter strings of length range_size.
		//data is transmitted LSB first, as a signle, huge word. lets write out the data with the lower word first.
		S32 range_size = ( format == TdiTdoStringFormat::Break64 ) ? 64 : 256;

		S32 range_end = bits.size();
		std::string result = "";
		while ( range_end > 0 )
		{
			S32 range_start = std::max( range_end - range_size, 0 );
			std::vector<U8> subset( bits.begin() + range_start, bits.begin() + range_end );
			range_end -= range_size;

			result += "[" + GetStringFromBitStates( subset, display_base, format ) + "]";
			if ( range_end > 0 )
				result += ", ";
		}

		return result;
	}

	std::string ret_val;

	if (bits.size() > 64)
	{
		if (display_base == Hexadecimal  ||  display_base == Binary)
			ret_val = GetHexOrBinaryString(bits, display_base);
		else if (display_base == Decimal)
			ret_val = GetDecimalString(bits);
		else if (display_base == ASCII)
			ret_val = GetASCIIString(bits);
		else if (display_base == AsciiHex)
			ret_val = GetASCIIString(bits) + " (" + GetHexOrBinaryString(bits, Hexadecimal) + ')';

	} else {

		// get the numerical value from the bits
		std::vector<U8>::const_iterator bsi(bits.begin());

		U64 val;

		// make a 64 bit value
		for (val = 0; bsi != bits.end(); ++bsi)
			val = (val << 1) | (*bsi == BIT_HIGH ? 1 : 0);

		// make a string out of that value
		char number_str[128];

		AnalyzerHelpers::GetNumberString(val, display_base, (U32) bits.size(), number_str, sizeof(number_str));

		ret_val = number_str;
	}

	return ret_val;
}
