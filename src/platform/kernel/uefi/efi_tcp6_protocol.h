/**
 * @file efi_tcp6_protocol.h
 * @brief EFI TCP6 Protocol for IPv6 TCP networking.
 *
 * @details Defines the EFI_TCP6_PROTOCOL interface and all supporting types for TCP/IPv6
 *          communication in UEFI. Includes configuration structures (EFI_TCP6_CONFIG_DATA,
 *          EFI_TCP6_ACCESS_POINT, EFI_TCP6_OPTION), connection state enumeration,
 *          asynchronous I/O tokens (connection, listen, transmit, receive, close), fragment
 *          data structures, and the protocol function table (GetModeData, Configure, Connect,
 *          Accept, Transmit, Receive, Close, Cancel, Poll). Also defines protocol and service
 *          binding GUIDs and the EFI_IPv6_ADDRESS type.
 *
 * @see UEFI Specification 2.10 — Section 28.2, EFI TCP6 Service Binding Protocol
 * @see UEFI Specification 2.10 — Section 28.2.2, EFI TCP6 Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// TCP6 Protocol GUIDs
// =============================================================================

// {EC20EB79-6C1A-4664-9A0D-D2E4CC16D664}
#define EFI_TCP6_SERVICE_BINDING_PROTOCOL_GUID                                                     \
	{                                                                                              \
		0xEC20EB79, 0x6C1A, 0x4664, { 0x9A, 0x0D, 0xD2, 0xE4, 0xCC, 0x16, 0xD6, 0x64 }             \
	}

// {46E44855-BD60-4AB7-AB0D-A6790824A3F0}
#define EFI_TCP6_PROTOCOL_GUID                                                                     \
	{                                                                                              \
		0x46E44855, 0xBD60, 0x4AB7, { 0xAB, 0x0D, 0xA6, 0x79, 0x08, 0x24, 0xA3, 0xF0 }             \
	}

// =============================================================================
// IPv6 Address Type
// =============================================================================

typedef struct
{
	UINT8 Addr[16];
} EFI_IPv6_ADDRESS;

// =============================================================================
// TCP6 Access Point
// =============================================================================

typedef struct
{
	EFI_IPv6_ADDRESS StationAddress;
	UINT16 StationPort;
	EFI_IPv6_ADDRESS RemoteAddress;
	UINT16 RemotePort;
	BOOL ActiveFlag;
} EFI_TCP6_ACCESS_POINT;

// =============================================================================
// TCP6 Option
// =============================================================================

typedef struct
{
	UINT32 ReceiveBufferSize;
	UINT32 SendBufferSize;
	UINT32 MaxSynBackLog;
	UINT32 ConnectionTimeout;
	UINT32 DataRetries;
	UINT32 FinTimeout;
	UINT32 TimeWaitTimeout;
	UINT32 KeepAliveProbes;
	UINT32 KeepAliveTime;
	UINT32 KeepAliveInterval;
	BOOL EnableNagle;
	BOOL EnableTimeStamp;
	BOOL EnableWindowScaling;
	BOOL EnableSelectiveAck;
	BOOL EnablePathMtuDiscovery;
} EFI_TCP6_OPTION;

// =============================================================================
// TCP6 Configuration Data
// =============================================================================

typedef struct
{
	UINT8 TrafficClass;
	UINT8 HopLimit;
	EFI_TCP6_ACCESS_POINT AccessPoint;
	EFI_TCP6_OPTION *ControlOption;
} EFI_TCP6_CONFIG_DATA;

// =============================================================================
// TCP6 Connection State
// =============================================================================

typedef enum
{
	Tcp6StateClosed = 0,
	Tcp6StateListen = 1,
	Tcp6StateSynSent = 2,
	Tcp6StateSynReceived = 3,
	Tcp6StateEstablished = 4,
	Tcp6StateFinWait1 = 5,
	Tcp6StateFinWait2 = 6,
	Tcp6StateClosing = 7,
	Tcp6StateTimeWait = 8,
	Tcp6StateCloseWait = 9,
	Tcp6StateLastAck = 10
} EFI_TCP6_CONNECTION_STATE;

// =============================================================================
// TCP6 Completion Token
// =============================================================================

typedef struct
{
	EFI_EVENT Event;
	EFI_STATUS Status;
} EFI_TCP6_COMPLETION_TOKEN;

// =============================================================================
// TCP6 Connection Token
// =============================================================================

typedef struct
{
	EFI_TCP6_COMPLETION_TOKEN CompletionToken;
} EFI_TCP6_CONNECTION_TOKEN;

// =============================================================================
// TCP6 Listen Token
// =============================================================================

typedef struct
{
	EFI_TCP6_COMPLETION_TOKEN CompletionToken;
	EFI_HANDLE NewChildHandle;
} EFI_TCP6_LISTEN_TOKEN;

// =============================================================================
// TCP6 Fragment Data
// =============================================================================

typedef struct
{
	UINT32 FragmentLength;
	PVOID FragmentBuffer;
} EFI_TCP6_FRAGMENT_DATA;

// =============================================================================
// TCP6 Receive Data
// =============================================================================

typedef struct
{
	BOOL UrgentFlag;
	UINT32 DataLength;
	UINT32 FragmentCount;
	EFI_TCP6_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP6_RECEIVE_DATA;

// =============================================================================
// TCP6 Transmit Data
// =============================================================================

typedef struct
{
	BOOL Push;
	BOOL Urgent;
	UINT32 DataLength;
	UINT32 FragmentCount;
	EFI_TCP6_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP6_TRANSMIT_DATA;

// =============================================================================
// TCP6 I/O Token
// =============================================================================

typedef struct
{
	EFI_TCP6_COMPLETION_TOKEN CompletionToken;
	union
	{
		EFI_TCP6_RECEIVE_DATA *RxData;
		EFI_TCP6_TRANSMIT_DATA *TxData;
	} Packet;
} EFI_TCP6_IO_TOKEN;

// =============================================================================
// TCP6 Close Token
// =============================================================================

typedef struct
{
	EFI_TCP6_COMPLETION_TOKEN CompletionToken;
	BOOL AbortOnClose;
} EFI_TCP6_CLOSE_TOKEN;

// =============================================================================
// TCP6 Protocol
// =============================================================================

typedef struct EFI_TCP6_PROTOCOL EFI_TCP6_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_TCP6_GET_MODE_DATA)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_CONNECTION_STATE *Tcp6State,
	EFI_TCP6_CONFIG_DATA *Tcp6ConfigData,
	PVOID Ip6ModeData,
	PVOID MnpConfigData,
	PVOID SnpModeData);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_CONFIGURE)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_CONFIG_DATA *TcpConfigData);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_CONNECT)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_CONNECTION_TOKEN *ConnectionToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_ACCEPT)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_LISTEN_TOKEN *ListenToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_TRANSMIT)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_IO_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_RECEIVE)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_IO_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_CLOSE)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_CLOSE_TOKEN *CloseToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_CANCEL)(
	EFI_TCP6_PROTOCOL *This,
	EFI_TCP6_COMPLETION_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP6_POLL)(EFI_TCP6_PROTOCOL *This);

struct EFI_TCP6_PROTOCOL
{
	EFI_TCP6_GET_MODE_DATA GetModeData;
	EFI_TCP6_CONFIGURE Configure;
	EFI_TCP6_CONNECT Connect;
	EFI_TCP6_ACCEPT Accept;
	EFI_TCP6_TRANSMIT Transmit;
	EFI_TCP6_RECEIVE Receive;
	EFI_TCP6_CLOSE Close;
	EFI_TCP6_CANCEL Cancel;
	EFI_TCP6_POLL Poll;
};
