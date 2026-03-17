/**
 * @file efi_simple_network_protocol.h
 * @brief EFI Simple Network Protocol for low-level NIC control.
 *
 * @details Defines the EFI_SIMPLE_NETWORK_PROTOCOL for basic network interface card management,
 *          including Start, Stop, Initialize, Shutdown, Reset, Transmit, and Receive operations.
 *          Also provides ReceiveFilters, StationAddress, Statistics, MCastIpToMac, NvData, and
 *          GetStatus functions. Includes EFI_SIMPLE_NETWORK_MODE for querying NIC capabilities
 *          (MAC address, media status, packet sizes) and EFI_SIMPLE_NETWORK_STATE enumeration.
 *
 * @see UEFI Specification 2.10 — Section 24.1, Simple Network Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// Simple Network Protocol GUID
// =============================================================================

// {A19832B9-AC25-11D3-9A2D-0090273FC14D}
#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID                                                           \
	{                                                                                              \
		0xA19832B9, 0xAC25, 0x11D3, { 0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }             \
	}

// =============================================================================
// Simple Network State
// =============================================================================

typedef enum
{
	EfiSimpleNetworkStopped,
	EfiSimpleNetworkStarted,
	EfiSimpleNetworkInitialized,
	EfiSimpleNetworkMaxState
} EFI_SIMPLE_NETWORK_STATE;

// =============================================================================
// Simple Network Mode
// =============================================================================

typedef struct
{
	UINT32 State;
	UINT32 HwAddressSize;
	UINT32 MediaHeaderSize;
	UINT32 MaxPacketSize;
	UINT32 NvRamSize;
	UINT32 NvRamAccessSize;
	UINT32 ReceiveFilterMask;
	UINT32 ReceiveFilterSetting;
	UINT32 MaxMCastFilterCount;
	UINT32 MCastFilterCount;
	UINT8 MCastFilter[16][32];
	UINT8 CurrentAddress[32];
	UINT8 BroadcastAddress[32];
	UINT8 PermanentAddress[32];
	UINT8 IfType;
	BOOL MacAddressChangeable;
	BOOL MultipleTxSupported;
	BOOL MediaPresentSupported;
	BOOL MediaPresent;
} EFI_SIMPLE_NETWORK_MODE;

// =============================================================================
// Simple Network Protocol
// =============================================================================

typedef struct EFI_SIMPLE_NETWORK_PROTOCOL EFI_SIMPLE_NETWORK_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_START)(EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_STOP)(EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_INITIALIZE)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	USIZE ExtraRxBufferSize,
	USIZE ExtraTxBufferSize);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_RESET)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	BOOL ExtendedVerification);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_SHUTDOWN)(EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_RECEIVE_FILTERS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	UINT32 Enable,
	UINT32 Disable,
	BOOL ResetMCastFilter,
	USIZE MCastFilterCnt,
	PVOID MCastFilter);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_STATION_ADDRESS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	BOOL Reset,
	PVOID New);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_STATISTICS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	BOOL Reset,
	USIZE *StatisticsSize,
	PVOID StatisticsTable);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	BOOL IPv6,
	PVOID IP,
	PVOID MAC);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_NVDATA)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	BOOL ReadWrite,
	USIZE Offset,
	USIZE BufferSize,
	PVOID Buffer);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_GET_STATUS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	UINT32 *InterruptStatus,
	PVOID *TxBuf);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_TRANSMIT)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	USIZE HeaderSize,
	USIZE BufferSize,
	PVOID Buffer,
	PVOID SrcAddr,
	PVOID DestAddr,
	UINT16 *Protocol);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_NETWORK_RECEIVE)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	USIZE *HeaderSize,
	USIZE *BufferSize,
	PVOID Buffer,
	PVOID SrcAddr,
	PVOID DestAddr,
	UINT16 *Protocol);

struct EFI_SIMPLE_NETWORK_PROTOCOL
{
	UINT64 Revision;
	EFI_SIMPLE_NETWORK_START Start;
	EFI_SIMPLE_NETWORK_STOP Stop;
	EFI_SIMPLE_NETWORK_INITIALIZE Initialize;
	EFI_SIMPLE_NETWORK_RESET Reset;
	EFI_SIMPLE_NETWORK_SHUTDOWN Shutdown;
	EFI_SIMPLE_NETWORK_RECEIVE_FILTERS ReceiveFilters;
	EFI_SIMPLE_NETWORK_STATION_ADDRESS StationAddress;
	EFI_SIMPLE_NETWORK_STATISTICS Statistics;
	EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC MCastIpToMac;
	EFI_SIMPLE_NETWORK_NVDATA NvData;
	EFI_SIMPLE_NETWORK_GET_STATUS GetStatus;
	EFI_SIMPLE_NETWORK_TRANSMIT Transmit;
	EFI_SIMPLE_NETWORK_RECEIVE Receive;
	EFI_EVENT WaitForPacket;
	EFI_SIMPLE_NETWORK_MODE *Mode;
};
