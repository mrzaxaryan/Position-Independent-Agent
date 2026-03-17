/**
 * @file efi_graphics_output_protocol.h
 * @brief EFI Graphics Output Protocol (GOP) definitions.
 *
 * @details Defines the EFI_GRAPHICS_OUTPUT_PROTOCOL interface for querying
 *          display modes and performing framebuffer block transfers. Includes
 *          pixel format enumeration, mode information structure, BLT operation
 *          types, and the BLT pixel structure.
 *
 * @see UEFI Specification 2.10 — Section 12.9, Graphics Output Protocol
 *      https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#efi-graphics-output-protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// Graphics Output Protocol GUID
// =============================================================================

/// {9042A9DE-23DC-4A38-96FB-7ADED080516A}
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID                                                          \
	{                                                                                              \
		0x9042A9DE, 0x23DC, 0x4A38, { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A }            \
	}

// =============================================================================
// Pixel Format
// =============================================================================

/// @brief Pixel format enumeration for GOP framebuffer
/// @see UEFI Specification 2.10 — Section 12.9, EFI_GRAPHICS_PIXEL_FORMAT
typedef enum
{
	PixelRedGreenBlueReserved8BitPerColor, ///< RGBX byte order (R at offset 0)
	PixelBlueGreenRedReserved8BitPerColor, ///< BGRX byte order (B at offset 0)
	PixelBitMask,                          ///< Custom layout defined by PixelInformation
	PixelBltOnly,                          ///< Framebuffer not directly accessible; use Blt()
	PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

// =============================================================================
// Pixel Bitmask
// =============================================================================

/// @brief Per-channel bitmask for PixelBitMask format
typedef struct
{
	UINT32 RedMask;
	UINT32 GreenMask;
	UINT32 BlueMask;
	UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

// =============================================================================
// Mode Information
// =============================================================================

/// @brief Describes a single GOP video mode
/// @see UEFI Specification 2.10 — Section 12.9, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
typedef struct
{
	UINT32 Version;                              ///< Structure version (0 for UEFI 2.1+)
	UINT32 HorizontalResolution;                 ///< Horizontal resolution in pixels
	UINT32 VerticalResolution;                    ///< Vertical resolution in pixels
	EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;        ///< Pixel format or PixelBltOnly
	EFI_PIXEL_BITMASK PixelInformation;           ///< Bitmasks (valid when PixelFormat == PixelBitMask)
	UINT32 PixelsPerScanLine;                     ///< Pixels per scanline (>= HorizontalResolution)
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

// =============================================================================
// BLT Operations
// =============================================================================

/// @brief Block transfer operation types
/// @see UEFI Specification 2.10 — Section 12.9, EFI_GRAPHICS_OUTPUT_BLT_OPERATION
typedef enum
{
	EfiBltVideoFill,          ///< Fill rectangle with a single pixel color
	EfiBltVideoToBltBuffer,   ///< Copy from video to BLT buffer (screen capture)
	EfiBltBufferToVideo,      ///< Copy from BLT buffer to video (draw)
	EfiBltVideoToVideo,       ///< Copy within the video framebuffer
	EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

/// @brief Pixel structure for BLT operations (always BGRX byte order)
/// @see UEFI Specification 2.10 — Section 12.9, EFI_GRAPHICS_OUTPUT_BLT_PIXEL
typedef struct
{
	UINT8 Blue;
	UINT8 Green;
	UINT8 Red;
	UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

// =============================================================================
// Protocol Mode Structure
// =============================================================================

/// @brief Current mode state reported by the GOP protocol
typedef struct
{
	UINT32 MaxMode;                                    ///< Number of supported modes (0-based)
	UINT32 Mode;                                       ///< Current mode number
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;         ///< Pointer to current mode info
	USIZE SizeOfInfo;                                  ///< Size of the Info structure
	EFI_PHYSICAL_ADDRESS FrameBufferBase;               ///< Physical address of the framebuffer
	USIZE FrameBufferSize;                             ///< Size of the framebuffer in bytes
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

// =============================================================================
// Protocol Function Types
// =============================================================================

struct EFI_GRAPHICS_OUTPUT_PROTOCOL;

/// @brief Query information about a specific video mode
typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
	struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
	UINT32 ModeNumber,
	USIZE *SizeOfInfo,
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);

/// @brief Set the video mode
typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
	struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
	UINT32 ModeNumber);

/// @brief Perform a block transfer (BLT) operation
typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
	struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
	EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
	USIZE SourceX,
	USIZE SourceY,
	USIZE DestinationX,
	USIZE DestinationY,
	USIZE Width,
	USIZE Height,
	USIZE Delta);

// =============================================================================
// Graphics Output Protocol
// =============================================================================

/// @brief EFI Graphics Output Protocol interface
/// @see UEFI Specification 2.10 — Section 12.9
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL
{
	EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT Blt;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;
