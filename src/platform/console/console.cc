#include "platform/console/console.h"

#if !defined(PLATFORM_UEFI)

#include "core/encoding/utf16.h"

UINT32 Console::Write(Span<const WCHAR> text)
{
	constexpr USIZE BUFFER_SIZE = 256;
	CHAR utf8[BUFFER_SIZE];
	UINT32 totalWritten = 0;

	USIZE inputIndex = 0;
	USIZE bufIndex = 0;

	while (inputIndex < text.Size())
	{
		CHAR bytes[4];
		USIZE n = UTF16::CodepointToUTF8(text, inputIndex, Span<CHAR>(bytes, sizeof(bytes)));
		if (n == 0)
		{
			inputIndex++;
			continue;
		}

		if (bufIndex + n > BUFFER_SIZE)
		{
			totalWritten += Write(Span<const CHAR>(utf8, bufIndex));
			bufIndex = 0;
		}

		for (USIZE j = 0; j < n; j++)
			utf8[bufIndex++] = bytes[j];
	}

	if (bufIndex > 0)
		totalWritten += Write(Span<const CHAR>(utf8, bufIndex));

	return totalWritten;
}

#endif
