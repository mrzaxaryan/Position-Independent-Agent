/**
 * @file runtime.h
 * @brief Runtime Abstraction Layer
 *
 * @details Aggregate header for the RUNTIME layer. Includes CORE + PLATFORM
 * (via platform.h) plus all runtime-level features: cryptography, networking,
 * and TLS 1.3.
 *
 * @defgroup runtime Runtime Abstraction Layer
 * @{
 */

#pragma once

#include "platform/platform.h"

/// @name Containers
/// @{
#include "lib/vector.h"
/// @}

/// @name Cryptography
/// @{
#include "lib/crypto/sha2.h"
#include "lib/crypto/ecc.h"
#include "lib/crypto/chacha20.h"
#include "lib/crypto/chacha20_encoder.h"
/// @}

/// @name Networking
/// @{
#include "lib/network/dns/dns_client.h"
#include "lib/network/http/http_client.h"
#include "lib/network/websocket/websocket_client.h"
/// @}

/// @name Image
/// @{
#include "lib/image/jpeg_encoder.h"
#include "lib/image/image_processor.h"
/// @}

/// @name TLS 1.3
/// @{
#include "lib/network/tls/tls_client.h"
#include "lib/network/tls/tls_buffer.h"
#include "lib/network/tls/tls_cipher.h"
#include "lib/network/tls/tls_hash.h"
#include "lib/network/tls/tls_hkdf.h"
/// @}

/** @} */ // end of runtime group
