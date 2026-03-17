#include "core/string/string.h"

// ============================================================================
// NUMBER CONVERSION IMPLEMENTATIONS
// ============================================================================

USIZE StringUtils::FloatToStr(double value, Span<CHAR> buffer, UINT8 precision) noexcept
{
	if (buffer.Size() < 2)
		return 0;
	if (precision > 15)
		precision = 15;

	USIZE pos = 0;

	// Handle negative
	if (value < 0.0)
	{
		if (pos < buffer.Size() - 1)
			buffer[pos++] = '-';
		value = -value;
	}

	// Rounding: add 0.5 / 10^precision
	if (precision > 0)
	{
		double scale = 1.0;
		for (UINT8 p = 0; p < precision; p++)
			scale = scale * 10.0;
		value = value + 5.0 / (scale * 10.0);
	}
	else
	{
		value = value + 0.5;
	}

	// Integer part
	UINT64 intPart = (UINT64)value;
	double fracPart = value - (double)intPart;

	CHAR intBuf[24];
	USIZE intLen = UIntToStr(intPart, Span<CHAR>(intBuf));
	for (USIZE i = 0; i < intLen && pos < buffer.Size() - 1; i++)
		buffer[pos++] = intBuf[i];

	// Fractional part
	if (precision > 0 && pos < buffer.Size() - 1)
	{
		buffer[pos++] = '.';

		for (UINT8 p = 0; p < precision && pos < buffer.Size() - 1; p++)
		{
			fracPart = fracPart * 10.0;
			INT32 digit = (INT32)fracPart;
			if (digit < 0)
				digit = 0;
			if (digit > 9)
				digit = 9;
			buffer[pos++] = '0' + digit;
			fracPart = fracPart - (double)digit;
		}

		// Trim trailing zeros
		while (pos > 2 && buffer[pos - 1] == '0' && buffer[pos - 2] != '.')
			pos--;
	}

	buffer[pos] = '\0';
	return pos;
}

Result<double, Error> StringUtils::StrToFloat(Span<const CHAR> str) noexcept
{
	if (str.Size() == 0)
	{
		return Result<double, Error>::Err(Error::String_ParseFloatFailed);
	}

	// Validate that the string contains at least one digit
	BOOL hasDigit = false;
	for (USIZE i = 0; i < str.Size(); i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			hasDigit = true;
			break;
		}
	}

	if (!hasDigit)
	{
		return Result<double, Error>::Err(Error::String_ParseFloatFailed);
	}

	USIZE i = 0;
	double sign = 1.0;
	double result = 0.0;
	double frac = 0.0;
	double base = 1.0;

	// sign
	if (str[i] == '-')
	{
		sign = -1.0;
		i++;
	}
	else if (str[i] == '+')
	{
		i++;
	}

	// integer part
	while (i < str.Size() && str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10.0 + (double)(str[i] - '0');
		i++;
	}

	// fractional part
	if (i < str.Size() && str[i] == '.')
	{
		i++;
		while (i < str.Size() && str[i] >= '0' && str[i] <= '9')
		{
			frac = frac * 10.0 + (double)(str[i] - '0');
			base = base * 10.0;
			i++;
		}
	}

	double parsed = sign * (result + frac / base);
	return Result<double, Error>::Ok(parsed);
}

// ============================================================================
// UTF CONVERSION IMPLEMENTATIONS
// ============================================================================

// Converts a UTF-8 string to wide string (UTF-16 on Windows, UCS-4 on Linux)
// Returns the number of wide characters written (excluding null terminator)
USIZE StringUtils::Utf8ToWide(Span<const CHAR> utf8, Span<WCHAR> wide)
{
	if (utf8.Size() == 0 || wide.Size() < 3)
	{
		if (wide.Size() > 0)
			wide[0] = L'\0';
		return 0;
	}

	USIZE wideLen = 0;
	USIZE i = 0;

	while (i < utf8.Size() && utf8[i] != '\0' && wideLen + 2 < wide.Size())
	{
		UINT32 ch;
		UINT8 byte = (UINT8)utf8[i++];

		if (byte < 0x80)
		{
			ch = byte;
		}
		else if ((byte & 0xE0) == 0xC0)
		{
			ch = (byte & 0x1F) << 6;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F);
		}
		else if ((byte & 0xF0) == 0xE0)
		{
			ch = (byte & 0x0F) << 12;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F) << 6;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F);
		}
		else if ((byte & 0xF8) == 0xF0)
		{
			ch = (byte & 0x07) << 18;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F) << 12;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F) << 6;
			if (i < utf8.Size() && utf8[i] != '\0')
				ch |= (utf8[i++] & 0x3F);

			if (ch >= 0x10000)
			{
				if constexpr (sizeof(WCHAR) >= 4)
				{
					// UCS-4: store full codepoint directly
					wide[wideLen++] = (WCHAR)ch;
				}
				else
				{
					// UTF-16: encode as surrogate pair
					ch -= 0x10000;
					wide[wideLen++] = (WCHAR)(0xD800 + (ch >> 10));
					wide[wideLen++] = (WCHAR)(0xDC00 + (ch & 0x3FF));
				}
				continue;
			}
		}
		else
		{
			continue; // Invalid UTF-8 byte, skip
		}

		wide[wideLen++] = (WCHAR)ch;
	}

	wide[wideLen] = L'\0';
	return wideLen;
}
