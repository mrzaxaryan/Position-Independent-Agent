#include "platform/fs/posix/posix_path.h"
#include "platform/fs/path.h"
#include "core/encoding/utf16.h"

NOINLINE USIZE NormalizePathToUtf8(PCWCHAR path, Span<CHAR> utf8Out)
{
	if (utf8Out.Size() == 0)
		return 0;

	WCHAR normalizedPath[1024];
	USIZE pathLen = Path::NormalizePath(path, Span<WCHAR>(normalizedPath));
	USIZE utf8Len = UTF16::ToUTF8(Span<const WCHAR>(normalizedPath, pathLen),
								   Span<CHAR>(utf8Out.Data(), utf8Out.Size() - 1));
	utf8Out[utf8Len] = '\0';
	return utf8Len;
}
