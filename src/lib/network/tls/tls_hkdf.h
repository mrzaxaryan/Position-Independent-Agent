#pragma once

#include "core/core.h"

// HKDF (HMAC-based Key Derivation Function) for TLS 1.3
class TlsHKDF
{
private:
	static INT32 Label(Span<const CHAR> label, Span<const UCHAR> data, PUCHAR hkdflabel, UINT16 length);

public:
	static VOID Extract(Span<UCHAR> output, Span<const UCHAR> salt, Span<const UCHAR> ikm);
	static VOID Expand(Span<UCHAR> output, Span<const UCHAR> secret, Span<const UCHAR> info);
	static VOID ExpandLabel(Span<UCHAR> output, Span<const UCHAR> secret, Span<const CHAR> label, Span<const UCHAR> data);
};