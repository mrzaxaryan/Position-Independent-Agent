/**
 * @file sha2.cc
 * @brief FIPS 180-2 SHA-256/384 and HMAC implementation
 *
 * @details SHA-2 compression, padding, and HMAC construction.
 *
 * @copyright Copyright (C) 2005-2023 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved. BSD 3-clause license.
 *
 * @see RFC 6234 — US Secure Hash Algorithms (SHA and SHA-based HMAC and HKDF)
 *      https://datatracker.ietf.org/doc/html/rfc6234
 * @see RFC 2104 — HMAC: Keyed-Hashing for Message Authentication
 *      https://datatracker.ietf.org/doc/html/rfc2104
 */

#include "lib/crypto/sha2.h"
#include "core/memory/memory.h"
#include "core/types/primitives.h"
#include "platform/platform.h"

template <typename T>
static constexpr FORCE_INLINE T Ch(T x, T y, T z) { return (x & y) ^ (~x & z); }

template <typename T>
static constexpr FORCE_INLINE T Maj(T x, T y, T z) { return (x & y) ^ (x & z) ^ (y & z); }

VOID SHA256Traits::FillH0(Word (&out)[8])
{
	const UINT32 embedded[] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
	Memory::Copy(out, embedded, sizeof(out));
}

VOID SHA256Traits::FillK(Word (&out)[RoundCount])
{
	const UINT32 embedded[] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
	Memory::Copy(out, embedded, sizeof(out));
}

constexpr FORCE_INLINE VOID SHA256Traits::Pack(Span<const UINT8, sizeof(Word)> str, Word &x)
{
	x = ((Word)str[3]) | ((Word)str[2] << 8) | ((Word)str[1] << 16) | ((Word)str[0] << 24);
}

constexpr FORCE_INLINE VOID SHA256Traits::Unpack(Word x, Span<UINT8, sizeof(Word)> str)
{
	str[3] = (UINT8)(x);
	str[2] = (UINT8)(x >> 8);
	str[1] = (UINT8)(x >> 16);
	str[0] = (UINT8)(x >> 24);
}

VOID SHA384Traits::FillH0(Word (&out)[8])
{
	const UINT64 embedded[] = {
		0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
		0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
		0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
		0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL};
	Memory::Copy(out, embedded, sizeof(out));
}

VOID SHA384Traits::FillK(Word (&out)[RoundCount])
{
	const UINT64 embedded[] = {
		0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
		0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
		0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
		0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
		0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
		0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
		0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
		0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
		0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
		0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
		0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
		0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
		0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
		0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
		0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
		0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
		0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
		0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
		0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
		0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
		0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
		0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
		0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
		0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
		0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
		0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
		0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
		0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
		0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
		0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
		0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
		0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
		0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
		0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
		0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
		0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
		0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
		0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
		0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
		0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};
	Memory::Copy(out, embedded, sizeof(out));
}

constexpr FORCE_INLINE VOID SHA384Traits::Pack(Span<const UINT8, sizeof(Word)> str, Word &x)
{
	x = ((Word)str[7]) | ((Word)str[6] << 8) | ((Word)str[5] << 16) | ((Word)str[4] << 24) |
		((Word)str[3] << 32) | ((Word)str[2] << 40) | ((Word)str[1] << 48) | ((Word)str[0] << 56);
}

constexpr FORCE_INLINE VOID SHA384Traits::Unpack(Word x, Span<UINT8, sizeof(Word)> str)
{
	str[7] = (UINT8)(x);
	str[6] = (UINT8)(x >> 8);
	str[5] = (UINT8)(x >> 16);
	str[4] = (UINT8)(x >> 24);
	str[3] = (UINT8)(x >> 32);
	str[2] = (UINT8)(x >> 40);
	str[1] = (UINT8)(x >> 48);
	str[0] = (UINT8)(x >> 56);
}

template<typename Traits>
SHABase<Traits>::SHABase()
{
	Traits::FillH0(this->h);
	this->len = 0;
	this->totLen = 0;
}

template<typename Traits>
SHABase<Traits>::~SHABase()
{
	Memory::Zero(this->h, sizeof(this->h));
	Memory::Zero(this->block, sizeof(this->block));
	this->len = 0;
	this->totLen = 0;
}

template<typename Traits>
NOINLINE VOID SHABase<Traits>::Transform(SHABase &ctx, Span<const UINT8> message, const Word (&k)[Traits::RoundCount])
{
	Word w[Traits::RoundCount];
	Word wv[8];
	Word t1, t2;
	UINT64 i;
	INT32 j;

	for (i = 0; i < (message.Size() >> Traits::BlockShift); i++)
	{
		auto subBlock = message.Subspan(i << Traits::BlockShift, Traits::BlockSize);

		for (j = 0; j < 16; j++)
		{
			Traits::Pack(Span<const UINT8, sizeof(Word)>(subBlock.Data() + (j << Traits::WordShift)), w[j]);
		}

		for (j = 16; j < (INT32)Traits::RoundCount; j++)
		{
			w[j] = Traits::F4(w[j - 2]) + w[j - 7] + Traits::F3(w[j - 15]) + w[j - 16];
		}

		for (j = 0; j < 8; j++)
		{
			wv[j] = ctx.h[j];
		}

		for (j = 0; j < (INT32)Traits::RoundCount; j++)
		{
			t1 = wv[7] + Traits::F2(wv[4]) + Ch(wv[4], wv[5], wv[6]) + k[j] + w[j];
			t2 = Traits::F1(wv[0]) + Maj(wv[0], wv[1], wv[2]);
			wv[7] = wv[6];
			wv[6] = wv[5];
			wv[5] = wv[4];
			wv[4] = wv[3] + t1;
			wv[3] = wv[2];
			wv[2] = wv[1];
			wv[1] = wv[0];
			wv[0] = t1 + t2;
		}

		for (j = 0; j < 8; j++)
		{
			ctx.h[j] += wv[j];
		}
	}
}

template<typename Traits>
VOID SHABase<Traits>::Update(Span<const UINT8> message)
{
	UINT64 blockNb;
	UINT64 newLen, remLen, tmpLen;
	const UINT8 *shiftedMessage;

	tmpLen = Traits::BlockSize - this->len;
	remLen = (UINT64)message.Size() < tmpLen ? (UINT64)message.Size() : tmpLen;

	Memory::Copy(&(this->block[this->len]), message.Data(), (USIZE)remLen);

	if (this->len + (UINT64)message.Size() < Traits::BlockSize)
	{
		this->len += (UINT64)message.Size();
		return;
	}

	newLen = (UINT64)message.Size() - remLen;
	blockNb = newLen / Traits::BlockSize;

	shiftedMessage = message.Data() + remLen;

	Word k[Traits::RoundCount];
	Traits::FillK(k);

	SHABase<Traits>::Transform(*this, Span<const UINT8>(this->block, Traits::BlockSize), k);
	SHABase<Traits>::Transform(*this, Span<const UINT8>(shiftedMessage, blockNb << Traits::BlockShift), k);

	remLen = newLen % Traits::BlockSize;

	Memory::Copy(this->block, &shiftedMessage[blockNb << Traits::BlockShift], (USIZE)remLen);

	this->len = remLen;
	this->totLen += (blockNb + 1) << Traits::BlockShift;
}

template<typename Traits>
VOID SHABase<Traits>::Final(Span<UINT8, Traits::DigestSize> digest)
{
	UINT64 blockNb;
	UINT64 pmLen;
	UINT64 lenB;
	UINT64 totLen;

	INT32 i;

	blockNb = (1 + ((Traits::BlockSize - Traits::PaddingOffset) < (this->len % Traits::BlockSize)));

	totLen = this->totLen + this->len;
	this->totLen = totLen;

	lenB = totLen << 3;
	pmLen = blockNb << Traits::BlockShift;

	Memory::Set(this->block + this->len, 0, (USIZE)pmLen - (USIZE)this->len);
	this->block[this->len] = 0x80;

	// Unpack length as 64-bit big-endian at end of block
	UINT8* lenPtr = this->block + pmLen - 8;
	lenPtr[7] = (UINT8)(lenB);
	lenPtr[6] = (UINT8)(lenB >> 8);
	lenPtr[5] = (UINT8)(lenB >> 16);
	lenPtr[4] = (UINT8)(lenB >> 24);
	lenPtr[3] = (UINT8)(lenB >> 32);
	lenPtr[2] = (UINT8)(lenB >> 40);
	lenPtr[1] = (UINT8)(lenB >> 48);
	lenPtr[0] = (UINT8)(lenB >> 56);

	Word k[Traits::RoundCount];
	Traits::FillK(k);

	SHABase<Traits>::Transform(*this, Span<const UINT8>(this->block, blockNb << Traits::BlockShift), k);

	for (i = 0; i < (INT32)Traits::OutputWords; i++)
	{
		Traits::Unpack(this->h[i], Span<UINT8, sizeof(Word)>(digest.Data() + (i << Traits::WordShift)));
	}
}

template<typename Traits>
VOID SHABase<Traits>::Hash(Span<const UINT8> message, Span<UINT8, Traits::DigestSize> digest)
{
	SHABase<Traits> ctx;
	ctx.Update(message);
	ctx.Final(digest);
}

template<typename Traits>
VOID SHABase<Traits>::CopyStateFrom(const SHABase &src)
{
	this->totLen = src.totLen;
	this->len = src.len;
	Memory::Copy(this->block, src.block, sizeof(this->block));
	Memory::Copy(this->h, src.h, sizeof(this->h));
}

template class SHABase<SHA256Traits>;
template class SHABase<SHA384Traits>;

template<typename SHAType, typename Traits>
HMACBase<SHAType, Traits>::~HMACBase()
{
	Memory::Zero(blockIpad, sizeof(blockIpad));
	Memory::Zero(blockOpad, sizeof(blockOpad));
}

template<typename SHAType, typename Traits>
VOID HMACBase<SHAType, Traits>::Init(Span<const UCHAR> key)
{
	UINT32 fill;
	UINT32 num;

	const UCHAR *keyUsed;
	UCHAR keyTemp[Traits::DigestSize];
	INT32 i;

	if ((UINT32)key.Size() == Traits::BlockSize)
	{
		keyUsed = key.Data();
		num = Traits::BlockSize;
	}
	else
	{
		if ((UINT32)key.Size() > Traits::BlockSize)
		{
			num = Traits::DigestSize;
			SHAType::Hash(Span<const UINT8>(key.Data(), key.Size()), keyTemp);
			keyUsed = keyTemp;
		}
		else
		{
			keyUsed = key.Data();
			num = (UINT32)key.Size();
		}
		fill = Traits::BlockSize - num;

		Memory::Set(this->blockIpad + num, 0x36, fill);
		Memory::Set(this->blockOpad + num, 0x5c, fill);
	}

	for (i = 0; i < (INT32)num; i++)
	{
		this->blockIpad[i] = keyUsed[i] ^ 0x36;
		this->blockOpad[i] = keyUsed[i] ^ 0x5c;
	}

	this->ctxInside.Update(Span<const UINT8>(this->blockIpad, Traits::BlockSize));
	this->ctxOutside.Update(Span<const UINT8>(this->blockOpad, Traits::BlockSize));

	this->ctxInsideReinit.CopyStateFrom(this->ctxInside);
	this->ctxOutsideReinit.CopyStateFrom(this->ctxOutside);
}

template<typename SHAType, typename Traits>
VOID HMACBase<SHAType, Traits>::Reinit()
{
	this->ctxInside.CopyStateFrom(this->ctxInsideReinit);
	this->ctxOutside.CopyStateFrom(this->ctxOutsideReinit);
}

template<typename SHAType, typename Traits>
VOID HMACBase<SHAType, Traits>::Update(Span<const UCHAR> message)
{
	this->ctxInside.Update(Span<const UINT8>(message.Data(), message.Size()));
}

template<typename SHAType, typename Traits>
VOID HMACBase<SHAType, Traits>::Final(Span<UCHAR> mac)
{
	UCHAR digestInside[Traits::DigestSize];
	UCHAR macTemp[Traits::DigestSize];

	this->ctxInside.Final(digestInside);
	this->ctxOutside.Update(Span<const UINT8>(digestInside, Traits::DigestSize));
	this->ctxOutside.Final(macTemp);
	Memory::Copy(mac.Data(), macTemp, mac.Size());
	Memory::Zero(digestInside, sizeof(digestInside));
	Memory::Zero(macTemp, sizeof(macTemp));
}

template<typename SHAType, typename Traits>
VOID HMACBase<SHAType, Traits>::Compute(Span<const UCHAR> key, Span<const UCHAR> message, Span<UCHAR> mac)
{
	HMACBase<SHAType, Traits> ctx;
	ctx.Init(key);
	ctx.Update(message);
	ctx.Final(mac);
}

template class HMACBase<SHA256, SHA256Traits>;
template class HMACBase<SHA384, SHA384Traits>;
