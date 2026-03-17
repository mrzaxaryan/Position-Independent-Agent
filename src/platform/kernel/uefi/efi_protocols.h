/**
 * @file efi_protocols.h
 * @brief Common EFI protocol definitions for console I/O.
 *
 * @details Defines EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL for text-mode console output (OutputString,
 *          ClearScreen, SetAttribute, SetCursorPosition, EnableCursor) and
 *          EFI_SIMPLE_TEXT_INPUT_PROTOCOL for keyboard input (ReadKeyStroke, WaitForKey).
 *          Includes text color attribute constants and the SIMPLE_TEXT_OUTPUT_MODE structure.
 *
 * @see UEFI Specification 2.10 — Section 12.3, Simple Text Input Protocol
 * @see UEFI Specification 2.10 — Section 12.4, Simple Text Output Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// Simple Text Output Protocol
// =============================================================================

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_TEXT_RESET)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	BOOL ExtendedVerification);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_STRING)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	CHAR16 *String);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_TEST_STRING)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	CHAR16 *String);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_QUERY_MODE)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	USIZE ModeNumber,
	USIZE *Columns,
	USIZE *Rows);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_MODE)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	USIZE ModeNumber);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	USIZE Attribute);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	USIZE Column,
	USIZE Row);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	BOOL Visible);

typedef struct
{
	INT32 MaxMode;
	INT32 Mode;
	INT32 Attribute;
	INT32 CursorColumn;
	INT32 CursorRow;
	BOOL CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	EFI_TEXT_RESET Reset;
	EFI_TEXT_STRING OutputString;
	EFI_TEXT_TEST_STRING TestString;
	EFI_TEXT_QUERY_MODE QueryMode;
	EFI_TEXT_SET_MODE SetMode;
	EFI_TEXT_SET_ATTRIBUTE SetAttribute;
	EFI_TEXT_CLEAR_SCREEN ClearScreen;
	EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
	EFI_TEXT_ENABLE_CURSOR EnableCursor;
	SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Text colors
#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN 0x03
#define EFI_RED 0x04
#define EFI_MAGENTA 0x05
#define EFI_BROWN 0x06
#define EFI_LIGHTGRAY 0x07
#define EFI_DARKGRAY 0x08
#define EFI_LIGHTBLUE 0x09
#define EFI_LIGHTGREEN 0x0A
#define EFI_LIGHTCYAN 0x0B
#define EFI_LIGHTRED 0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW 0x0E
#define EFI_WHITE 0x0F

// =============================================================================
// Simple Text Input Protocol (minimal, for completeness)
// =============================================================================

typedef struct
{
	UINT16 ScanCode;
	CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_INPUT_RESET)(
	struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
	BOOL ExtendedVerification);

typedef EFI_STATUS(EFIAPI *EFI_INPUT_READ_KEY)(
	struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
	EFI_INPUT_KEY *Key);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
	EFI_INPUT_RESET Reset;
	EFI_INPUT_READ_KEY ReadKeyStroke;
	EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
