/**
 * @file sha2.h
 * @brief FIPS 180-2 SHA-256/384 and HMAC Implementation
 *
 * @details PIC-safe cryptographic hash functions for position-independent code.
 * Constants are embedded in code to avoid .rdata section dependencies.
 *
 * Supported algorithms:
 * - SHA-256: 256-bit hash with 64 rounds
 * - SHA-384: 384-bit hash with 80 rounds
 * - HMAC-SHA256: Keyed-hash message authentication code using SHA-256
 * - HMAC-SHA384: Keyed-hash message authentication code using SHA-384
 *
 * @par Example Usage:
 * @code
 * // One-shot hash
 * UINT8 digest[SHA256_DIGEST_SIZE];
 * SHA256::Hash(message, Span<UINT8, SHA256_DIGEST_SIZE>(digest));
 *
 * // Incremental hash
 * SHA256 ctx;
 * ctx.Update(part1);
 * ctx.Update(part2);
 * ctx.Final(Span<UINT8, SHA256_DIGEST_SIZE>(digest));
 *
 * // HMAC
 * UINT8 mac[SHA256_DIGEST_SIZE];
 * HMAC_SHA256::Compute(key, message, Span<UCHAR>(mac));
 * @endcode
 *
 * @note All constants are embedded in code to avoid .rdata dependencies.
 *
 * @see RFC 6234 — US Secure Hash Algorithms (SHA and SHA-based HMAC and HKDF)
 *      https://datatracker.ietf.org/doc/html/rfc6234
 * @see RFC 2104 — HMAC: Keyed-Hashing for Message Authentication
 *      https://datatracker.ietf.org/doc/html/rfc2104
 *
 * @ingroup crypt
 *
 * @defgroup sha2 SHA-2 Hash Functions
 * @ingroup crypt
 * @{
 */

#pragma once

#include "core/core.h"
#include "core/math/bitops.h"

/** @brief SHA-256 digest size in bytes (256 bits) */
constexpr USIZE SHA256_DIGEST_SIZE = 256 / 8;

/** @brief SHA-384 digest size in bytes (384 bits) */
constexpr USIZE SHA384_DIGEST_SIZE = 384 / 8;

/** @brief SHA-256 block size in bytes (512 bits) */
constexpr USIZE SHA256_BLOCK_SIZE = 512 / 8;

/** @brief SHA-384 block size in bytes (1024 bits) */
constexpr USIZE SHA384_BLOCK_SIZE = 1024 / 8;

struct SHA256Traits;
struct SHA384Traits;
template<typename Traits> class SHABase;
template<typename SHAType, typename Traits> class HMACBase;

/**
 * @struct SHA256Traits
 * @brief Type traits for SHA-256 algorithm
 *
 * @details Defines constants and helper functions specific to SHA-256:
 * - 32-bit word size
 * - 64-byte (512-bit) block size
 * - 64 rounds of compression
 * - 8 output words (256 bits)
 */
struct SHA256Traits
{
	using Word = UINT32;                              /**< @brief Word type: 32-bit unsigned integer */
	static constexpr USIZE BlockSize     = SHA256_BLOCK_SIZE;  /**< @brief Block size in bytes */
	static constexpr USIZE DigestSize    = SHA256_DIGEST_SIZE; /**< @brief Digest size in bytes */
	static constexpr USIZE RoundCount    = 64;        /**< @brief Number of compression rounds */
	static constexpr USIZE OutputWords   = 8;         /**< @brief Number of output words */
	static constexpr USIZE BlockShift    = 6;         /**< @brief Log2 of block size */
	static constexpr USIZE WordShift     = 2;         /**< @brief Log2 of word size */
	static constexpr USIZE PaddingOffset = 9;         /**< @brief Padding overhead in bytes */

	/**
	 * @brief Fills initial hash values (H0) for SHA-256
	 * @param out Output array for 8 initial hash values
	 */
	static VOID FillH0(Word (&out)[8]);

	/**
	 * @brief Fills round constants (K) for SHA-256
	 * @param out Output array for 64 round constants
	 */
	static VOID FillK(Word (&out)[RoundCount]);

	/**
	 * @brief Packs 4 bytes into a 32-bit word (big-endian)
	 * @param str Input byte span (exactly sizeof(Word) bytes)
	 * @param x Output word reference
	 */
	static constexpr FORCE_INLINE VOID Pack(Span<const UINT8, sizeof(Word)> str, Word &x);

	/**
	 * @brief Unpacks a 32-bit word into 4 bytes (big-endian)
	 * @param x Input word
	 * @param str Output byte span (exactly sizeof(Word) bytes)
	 */
	static constexpr FORCE_INLINE VOID Unpack(Word x, Span<UINT8, sizeof(Word)> str);

	/** @brief SHA-256 Sigma0 function: ROTR(x,2) XOR ROTR(x,13) XOR ROTR(x,22) */
	static constexpr FORCE_INLINE Word F1(Word x) { return BitOps::Rotr32(x, 2) ^ BitOps::Rotr32(x, 13) ^ BitOps::Rotr32(x, 22); }

	/** @brief SHA-256 Sigma1 function: ROTR(x,6) XOR ROTR(x,11) XOR ROTR(x,25) */
	static constexpr FORCE_INLINE Word F2(Word x) { return BitOps::Rotr32(x, 6) ^ BitOps::Rotr32(x, 11) ^ BitOps::Rotr32(x, 25); }

	/** @brief SHA-256 sigma0 function: ROTR(x,7) XOR ROTR(x,18) XOR SHR(x,3) */
	static constexpr FORCE_INLINE Word F3(Word x) { return BitOps::Rotr32(x, 7) ^ BitOps::Rotr32(x, 18) ^ (x >> 3); }

	/** @brief SHA-256 sigma1 function: ROTR(x,17) XOR ROTR(x,19) XOR SHR(x,10) */
	static constexpr FORCE_INLINE Word F4(Word x) { return BitOps::Rotr32(x, 17) ^ BitOps::Rotr32(x, 19) ^ (x >> 10); }
};

/**
 * @struct SHA384Traits
 * @brief Type traits for SHA-384 algorithm
 *
 * @details Defines constants and helper functions specific to SHA-384:
 * - 64-bit word size
 * - 128-byte (1024-bit) block size
 * - 80 rounds of compression
 * - 6 output words (384 bits, truncated from 512)
 */
struct SHA384Traits
{
	using Word = UINT64;                              /**< @brief Word type: 64-bit unsigned integer */
	static constexpr USIZE BlockSize     = SHA384_BLOCK_SIZE;  /**< @brief Block size in bytes */
	static constexpr USIZE DigestSize    = SHA384_DIGEST_SIZE; /**< @brief Digest size in bytes */
	static constexpr USIZE RoundCount    = 80;        /**< @brief Number of compression rounds */
	static constexpr USIZE OutputWords   = 6;         /**< @brief Number of output words (truncated) */
	static constexpr USIZE BlockShift    = 7;         /**< @brief Log2 of block size */
	static constexpr USIZE WordShift     = 3;         /**< @brief Log2 of word size */
	static constexpr USIZE PaddingOffset = 17;        /**< @brief Padding overhead in bytes */

	/**
	 * @brief Fills initial hash values (H0) for SHA-384
	 * @param out Output array for 8 initial hash values
	 */
	static VOID FillH0(Word (&out)[8]);

	/**
	 * @brief Fills round constants (K) for SHA-384
	 * @param out Output array for 80 round constants
	 */
	static VOID FillK(Word (&out)[RoundCount]);

	/**
	 * @brief Packs 8 bytes into a 64-bit word (big-endian)
	 * @param str Input byte span (exactly sizeof(Word) bytes)
	 * @param x Output word reference
	 */
	static constexpr FORCE_INLINE VOID Pack(Span<const UINT8, sizeof(Word)> str, Word &x);

	/**
	 * @brief Unpacks a 64-bit word into 8 bytes (big-endian)
	 * @param x Input word
	 * @param str Output byte span (exactly sizeof(Word) bytes)
	 */
	static constexpr FORCE_INLINE VOID Unpack(Word x, Span<UINT8, sizeof(Word)> str);

	/** @brief SHA-384 Sigma0 function: ROTR(x,28) XOR ROTR(x,34) XOR ROTR(x,39) */
	static constexpr FORCE_INLINE Word F1(Word x) { return BitOps::Rotr64(x, 28) ^ BitOps::Rotr64(x, 34) ^ BitOps::Rotr64(x, 39); }

	/** @brief SHA-384 Sigma1 function: ROTR(x,14) XOR ROTR(x,18) XOR ROTR(x,41) */
	static constexpr FORCE_INLINE Word F2(Word x) { return BitOps::Rotr64(x, 14) ^ BitOps::Rotr64(x, 18) ^ BitOps::Rotr64(x, 41); }

	/** @brief SHA-384 sigma0 function: ROTR(x,1) XOR ROTR(x,8) XOR SHR(x,7) */
	static constexpr FORCE_INLINE Word F3(Word x) { return BitOps::Rotr64(x, 1) ^ BitOps::Rotr64(x, 8) ^ (x >> 7); }

	/** @brief SHA-384 sigma1 function: ROTR(x,19) XOR ROTR(x,61) XOR SHR(x,6) */
	static constexpr FORCE_INLINE Word F4(Word x) { return BitOps::Rotr64(x, 19) ^ BitOps::Rotr64(x, 61) ^ (x >> 6); }
};

/**
 * @class SHABase
 * @brief Template base class for SHA-2 family hash functions
 *
 * @tparam Traits Type traits defining algorithm parameters (SHA256Traits or SHA384Traits)
 *
 * @details Implements the Merkle-Damgard construction for SHA-2 family hashes.
 * Supports both incremental hashing (Update/Final) and one-shot hashing (Hash).
 *
 * @par Example Usage:
 * @code
 * // One-shot hash
 * UINT8 digest[SHA256_DIGEST_SIZE];
 * SHA256::Hash(data, Span<UINT8, SHA256_DIGEST_SIZE>(digest));
 *
 * // Incremental hash
 * SHA256 ctx;
 * ctx.Update(chunk1);
 * ctx.Update(chunk2);
 * ctx.Final(Span<UINT8, SHA256_DIGEST_SIZE>(digest));
 * @endcode
 */
template<typename Traits>
class SHABase
{
public:
	using Word = typename Traits::Word;  /**< @brief Word type from traits */

	// Non-copyable, non-movable: prevents duplication of sensitive hash state
	SHABase(const SHABase &) = delete;
	SHABase &operator=(const SHABase &) = delete;
	SHABase(SHABase &&) = delete;
	SHABase &operator=(SHABase &&) = delete;

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

private:
	UINT64 totLen;                         /**< @brief Total message length processed */
	UINT64 len;                            /**< @brief Current block buffer length */
	UINT8 block[2 * Traits::BlockSize];    /**< @brief Message block buffer */
	Word h[8];                             /**< @brief Hash state (intermediate hash values) */

	/**
	 * @brief Copies hash state from another context
	 * @param src Source context to copy from
	 *
	 * @details Formal mechanism for duplicating SHA state, used by HMAC
	 * to snapshot contexts after key setup for efficient reinit.
	 */
	VOID CopyStateFrom(const SHABase &src);

	template<typename, typename> friend class HMACBase;
	friend class TlsHash;

public:
	/**
	 * @brief Default constructor - initializes hash state
	 * @details Sets initial hash values (H0) from traits.
	 */
	SHABase();

	/**
	 * @brief Destructor - securely clears hash state
	 */
	~SHABase();

	/**
	 * @brief Updates hash with additional message data
	 * @param message Span of message data bytes
	 *
	 * @details Can be called multiple times to hash large messages incrementally.
	 *
	 * @see RFC 6234 Section 6.2 — SHA-256 (message block processing)
	 *      https://datatracker.ietf.org/doc/html/rfc6234#section-6.2
	 */
	VOID Update(Span<const UINT8> message);

	/**
	 * @brief Finalizes hash computation and outputs digest
	 * @param digest Output span for hash digest (must be Traits::DigestSize bytes)
	 *
	 * @details Applies padding, processes final block, and outputs the digest.
	 * @note After calling Final(), the context should not be reused without re-initialization.
	 *
	 * @see RFC 6234 Section 4.1 — SHA-256 Padding
	 *      https://datatracker.ietf.org/doc/html/rfc6234#section-4.1
	 */
	VOID Final(Span<UINT8, Traits::DigestSize> digest);

	/**
	 * @brief Computes hash of a complete message in one call
	 * @param message Span of message data bytes
	 * @param digest Output span for hash digest (must be Traits::DigestSize bytes)
	 *
	 * @details Convenience method for hashing complete messages.
	 *
	 * @see RFC 6234 Section 6.2 — SHA-256 (complete hash computation)
	 *      https://datatracker.ietf.org/doc/html/rfc6234#section-6.2
	 */
	static VOID Hash(Span<const UINT8> message, Span<UINT8, Traits::DigestSize> digest);

	/**
	 * @brief Processes message blocks through the compression function
	 * @param ctx SHA context
	 * @param message Span of message bytes (must be a multiple of BlockSize)
	 * @param k Pre-filled round constants array
	 *
	 * @details Internal function that applies the SHA compression function.
	 * The caller is responsible for filling K via Traits::FillK() and passing
	 * it in, so that multi-block calls share a single K-table copy.
	 */
	static NOINLINE VOID Transform(SHABase &ctx, Span<const UINT8> message, const Word (&k)[Traits::RoundCount]);
};

/** @brief SHA-256 hash algorithm (256-bit output) */
using SHA256 = SHABase<SHA256Traits>;

/** @brief SHA-384 hash algorithm (384-bit output) */
using SHA384 = SHABase<SHA384Traits>;

/**
 * @class HMACBase
 * @brief Template base class for HMAC (Hash-based Message Authentication Code)
 *
 * @tparam SHAType Underlying SHA type (SHA256 or SHA384)
 * @tparam Traits Type traits for the SHA algorithm
 *
 * @details Implements HMAC as defined in RFC 2104. HMAC provides message
 * authentication using a secret key combined with a hash function.
 *
 * HMAC(K, m) = H((K' XOR opad) || H((K' XOR ipad) || m))
 *
 * Where:
 * - K' is the key padded/hashed to block size
 * - ipad is 0x36 repeated for block size
 * - opad is 0x5c repeated for block size
 *
 * @par Example Usage:
 * @code
 * // One-shot HMAC
 * UINT8 mac[SHA256_DIGEST_SIZE];
 * HMAC_SHA256::Compute(key, message, Span<UCHAR>(mac));
 *
 * // Incremental HMAC
 * HMAC_SHA256 hmac;
 * hmac.Init(key);
 * hmac.Update(chunk1);
 * hmac.Update(chunk2);
 * hmac.Final(Span<UCHAR>(mac));
 * @endcode
 */
template<typename SHAType, typename Traits>
class HMACBase
{
public:
	HMACBase() = default;

	// Non-copyable, non-movable: prevents duplication of sensitive key material
	HMACBase(const HMACBase &) = delete;
	HMACBase &operator=(const HMACBase &) = delete;
	HMACBase(HMACBase &&) = delete;
	HMACBase &operator=(HMACBase &&) = delete;

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

private:
	SHAType ctxInside;                        /**< @brief Inner hash context */
	SHAType ctxOutside;                       /**< @brief Outer hash context */
	SHAType ctxInsideReinit;                  /**< @brief Saved inner context for reinit */
	SHAType ctxOutsideReinit;                 /**< @brief Saved outer context for reinit */
	UCHAR blockIpad[Traits::BlockSize];       /**< @brief Inner padding block (K XOR ipad) */
	UCHAR blockOpad[Traits::BlockSize];       /**< @brief Outer padding block (K XOR opad) */

public:
	/**
	 * @brief Destructor - securely clears key material
	 */
	~HMACBase();

	/**
	 * @brief Initializes HMAC with a secret key
	 * @param key Span of secret key bytes
	 *
	 * @details If key is longer than block size, it is hashed first.
	 * Key is padded with zeros to block size, then XORed with ipad/opad.
	 *
	 * @see RFC 2104 Section 2 — Definition of HMAC
	 *      https://datatracker.ietf.org/doc/html/rfc2104#section-2
	 */
	VOID Init(Span<const UCHAR> key);

	/**
	 * @brief Reinitializes HMAC for computing another MAC with same key
	 *
	 * @details Restores the context to state immediately after Init().
	 * More efficient than calling Init() again with the same key.
	 */
	VOID Reinit();

	/**
	 * @brief Updates HMAC with additional message data
	 * @param message Span of message data bytes
	 */
	VOID Update(Span<const UCHAR> message);

	/**
	 * @brief Finalizes HMAC computation and outputs MAC
	 * @param mac Output span for MAC (can be less than digest size for truncation)
	 */
	VOID Final(Span<UCHAR> mac);

	/**
	 * @brief Computes HMAC of a complete message in one call
	 * @param key Span of secret key bytes
	 * @param message Span of message data bytes
	 * @param mac Output span for MAC
	 *
	 * @details Convenience method for computing HMAC of complete messages.
	 *
	 * @see RFC 2104 Section 2 — Definition of HMAC
	 *      https://datatracker.ietf.org/doc/html/rfc2104#section-2
	 */
	static VOID Compute(Span<const UCHAR> key, Span<const UCHAR> message, Span<UCHAR> mac);
};

/** @brief HMAC-SHA256 message authentication code */
using HMAC_SHA256 = HMACBase<SHA256, SHA256Traits>;

/** @brief HMAC-SHA384 message authentication code */
using HMAC_SHA384 = HMACBase<SHA384, SHA384Traits>;

/** @} */ // end of sha2 group
