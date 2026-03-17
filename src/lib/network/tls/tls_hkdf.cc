#include "lib/network/tls/tls_hkdf.h"
#include "core/binary/binary_writer.h"
#include "platform/console/logger.h"
#include "core/memory/memory.h"
#include "lib/crypto/sha2.h"
#include "lib/network/tls/tls_buffer.h"

/// @brief Create an HKDF label according to TLS 1.3 specification
/// @param label Span containing the label to use in the HKDF label
/// @param data Span containing the data to include in the HKDF label
/// @param hkdflabel The buffer to store the created HKDF label
/// @param length Length of the output keying material (OKM) that will be derived using this label
/// @return The total length of the created HKDF label
INT32 TlsHKDF::Label(Span<const CHAR> label, Span<const UCHAR> data, PUCHAR hkdflabel, UINT16 length)
{
	const CHAR prefix[] = "tls13 ";
	UCHAR labelLen = (UCHAR)label.Size();
	UCHAR dataLen = (UCHAR)data.Size();
	INT32 prefixLen = sizeof(prefix) - 1;

	LOG_DEBUG("Creating HKDF label with label_len: %d, data_len: %d, length: %d", labelLen, dataLen, length);

	BinaryWriter writer(Span<UINT8>(hkdflabel, 512));

	writer.WriteU16BE(length);
	writer.WriteU8((UINT8)(prefixLen + labelLen));
	writer.WriteBytes(Span<const UINT8>((const UINT8 *)(const CHAR *)prefix, prefixLen));
	writer.WriteBytes(Span<const UINT8>((const UINT8 *)label.Data(), label.Size()));
	writer.WriteU8(dataLen);
	if (dataLen)
	{
		LOG_DEBUG("Copying data to HKDF label, data_len: %d", dataLen);
		writer.WriteBytes(Span<const UINT8>(data.Data(), dataLen));
	}

	LOG_DEBUG("HKDF label created with total length: %d bytes", (INT32)writer.GetOffset());
	return (INT32)writer.GetOffset();
}

/// @brief Extract the HKDF keying material using the given salt and input keying material (IKM)
/// @param output Span to store the extracted keying material
/// @param salt Span containing the salt value
/// @param ikm Span containing the input keying material
/// @return void
VOID TlsHKDF::Extract(Span<UCHAR> output, Span<const UCHAR> salt, Span<const UCHAR> ikm)
{
	HMAC_SHA256 hmac;
	hmac.Init(salt);

	LOG_DEBUG("Extracting HKDF with output length: %d, salt length: %d, ikm length: %d", (UINT32)output.Size(), (UINT32)salt.Size(), (UINT32)ikm.Size());
	hmac.Update(ikm);
	hmac.Final(output);
}

/// @brief Expand the HKDF keying material using the given secret, info, and output length
/// @param output Span to store the expanded keying material
/// @param secret Span containing the secret value
/// @param info Span containing the info value
/// @return void
VOID TlsHKDF::Expand(Span<UCHAR> output, Span<const UCHAR> secret, Span<const UCHAR> info)
{
	UCHAR digestOut[SHA256_DIGEST_SIZE];
	UINT32 idx = 0;
	UCHAR i2 = 0;
	UINT32 outlen = (UINT32)output.Size();

	LOG_DEBUG("Expanding HKDF with output length: %d, secret length: %d, info length: %d", outlen, (UINT32)secret.Size(), (UINT32)info.Size());
	constexpr UINT32 hashLen = SHA256_DIGEST_SIZE;
	while (outlen)
	{
		HMAC_SHA256 hmac;
		hmac.Init(secret);
		if (i2)
		{
			LOG_DEBUG("Using previous digest for HKDF expansion, i2: %d", i2);
			hmac.Update(Span<const UCHAR>(digestOut, hashLen));
		}

		if (info.Data() && info.Size())
		{
			LOG_DEBUG("Updating HMAC with info, info length: %d", (UINT32)info.Size());
			hmac.Update(info);
		}
		i2++;
		hmac.Update(Span<const UCHAR>(&i2, 1));
		hmac.Final(Span<UCHAR>(digestOut, hashLen));

		UINT32 copylen = outlen;
		if (copylen > hashLen)
		{
			LOG_DEBUG("Copying %d bytes from digestOut to output", hashLen);
			copylen = (UINT32)hashLen;
		}

		for (UINT32 i = 0; i < copylen; i++)
		{
			output[idx++] = digestOut[i];
			outlen--;
		}

		if (!outlen)
		{
			LOG_DEBUG("Finished HKDF expansion, no more output needed");
			break;
		}
	}
}

/// @brief Expand the HKDF keying material using a label according to TLS 1.3 specification
/// @param output Span to store the expanded keying material
/// @param secret Span containing the secret value
/// @param label Span containing the label to use in the HKDF label
/// @param data Span containing the data to include in the HKDF label
/// @return void 
VOID TlsHKDF::ExpandLabel(Span<UCHAR> output, Span<const UCHAR> secret, Span<const CHAR> label, Span<const UCHAR> data)
{
	UCHAR hkdfLabel[512];
	UINT32 outlen = (UINT32)output.Size();
	INT32 len = TlsHKDF::Label(label, data, hkdfLabel, outlen);
	LOG_DEBUG("Expanding HKDF label with output length: %d, secret length: %d, label length: %d, data length: %d", outlen, (UINT32)secret.Size(), (UINT32)label.Size(), (UINT32)data.Size());
	TlsHKDF::Expand(output, secret, Span<const UCHAR>(hkdfLabel, len));
}
