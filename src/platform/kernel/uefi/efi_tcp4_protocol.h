/**
 * @file efi_tcp4_protocol.h
 * @brief EFI TCP4 Protocol for IPv4 TCP networking.
 *
 * @details Defines the EFI_TCP4_PROTOCOL interface and all supporting types for TCP/IPv4
 *          communication in UEFI. Includes configuration structures (EFI_TCP4_CONFIG_DATA,
 *          EFI_TCP4_ACCESS_POINT, EFI_TCP4_OPTION), connection state enumeration,
 *          asynchronous I/O tokens (connection, listen, transmit, receive, close), fragment
 *          data structures, and the protocol function table (GetModeData, Configure, Routes,
 *          Connect, Accept, Transmit, Receive, Close, Cancel, Poll). Also defines protocol
 *          and service binding GUIDs, and event/timer type constants.
 *
 * @see UEFI Specification 2.10 — Section 28.1, EFI TCP4 Service Binding Protocol
 * @see UEFI Specification 2.10 — Section 28.1.2, EFI TCP4 Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// TCP4 Protocol GUIDs
// =============================================================================

// {00720665-67EB-4A99-BAF7-D3C33A1C7CC9}
#define EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID                                                     \
	{                                                                                              \
		0x00720665, 0x67EB, 0x4A99, { 0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9 }             \
	}

// {65530BC7-A359-410F-B010-5AADC7EC2B62}
#define EFI_TCP4_PROTOCOL_GUID                                                                     \
	{                                                                                              \
		0x65530BC7, 0xA359, 0x410F, { 0xB0, 0x10, 0x5A, 0xAD, 0xC7, 0xEC, 0x2B, 0x62 }             \
	}

// =============================================================================
// IPv4 Address Type
// =============================================================================

typedef struct
{
	UINT8 Addr[4];
} EFI_IPv4_ADDRESS;

// =============================================================================
// TCP4 Access Point
// =============================================================================

typedef struct
{
	BOOL UseDefaultAddress;
	EFI_IPv4_ADDRESS StationAddress;
	EFI_IPv4_ADDRESS SubnetMask;
	UINT16 StationPort;
	EFI_IPv4_ADDRESS RemoteAddress;
	UINT16 RemotePort;
	BOOL ActiveFlag;
} EFI_TCP4_ACCESS_POINT;

// =============================================================================
// TCP4 Option
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
} EFI_TCP4_OPTION;

// =============================================================================
// TCP4 Configuration Data
// =============================================================================

typedef struct
{
	UINT8 TypeOfService;
	UINT8 TimeToLive;
	EFI_TCP4_ACCESS_POINT AccessPoint;
	EFI_TCP4_OPTION *ControlOption;
} EFI_TCP4_CONFIG_DATA;

// =============================================================================
// TCP4 Connection State
// =============================================================================

typedef enum
{
	Tcp4StateClosed = 0,
	Tcp4StateListen = 1,
	Tcp4StateSynSent = 2,
	Tcp4StateSynReceived = 3,
	Tcp4StateEstablished = 4,
	Tcp4StateFinWait1 = 5,
	Tcp4StateFinWait2 = 6,
	Tcp4StateClosing = 7,
	Tcp4StateTimeWait = 8,
	Tcp4StateCloseWait = 9,
	Tcp4StateLastAck = 10
} EFI_TCP4_CONNECTION_STATE;

// =============================================================================
// TCP4 Completion Token (base for all async operations)
// =============================================================================

typedef struct
{
	EFI_EVENT Event;
	EFI_STATUS Status;
} EFI_TCP4_COMPLETION_TOKEN;

// =============================================================================
// TCP4 Connection Token
// =============================================================================

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
} EFI_TCP4_CONNECTION_TOKEN;

// =============================================================================
// TCP4 Listen Token
// =============================================================================

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	EFI_HANDLE NewChildHandle;
} EFI_TCP4_LISTEN_TOKEN;

// =============================================================================
// TCP4 Fragment Data
// =============================================================================

typedef struct
{
	UINT32 FragmentLength;
	PVOID FragmentBuffer;
} EFI_TCP4_FRAGMENT_DATA;

// =============================================================================
// TCP4 Receive Data
// =============================================================================

typedef struct
{
	BOOL UrgentFlag;
	UINT32 DataLength;
	UINT32 FragmentCount;
	EFI_TCP4_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP4_RECEIVE_DATA;

// =============================================================================
// TCP4 Transmit Data
// =============================================================================

typedef struct
{
	BOOL Push;
	BOOL Urgent;
	UINT32 DataLength;
	UINT32 FragmentCount;
	EFI_TCP4_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP4_TRANSMIT_DATA;

// =============================================================================
// TCP4 I/O Token
// =============================================================================

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	union
	{
		EFI_TCP4_RECEIVE_DATA *RxData;
		EFI_TCP4_TRANSMIT_DATA *TxData;
	} Packet;
} EFI_TCP4_IO_TOKEN;

// =============================================================================
// TCP4 Close Token
// =============================================================================

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	BOOL AbortOnClose;
} EFI_TCP4_CLOSE_TOKEN;

// =============================================================================
// TCP4 Protocol
// =============================================================================

typedef struct EFI_TCP4_PROTOCOL EFI_TCP4_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_TCP4_GET_MODE_DATA)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_CONNECTION_STATE *Tcp4State,
	EFI_TCP4_CONFIG_DATA *Tcp4ConfigData,
	PVOID Ip4ModeData,
	PVOID MnpConfigData,
	PVOID SnpModeData);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_CONFIGURE)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_CONFIG_DATA *TcpConfigData);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_ROUTES)(
	EFI_TCP4_PROTOCOL *This,
	BOOL DeleteRoute,
	EFI_IPv4_ADDRESS *SubnetAddress,
	EFI_IPv4_ADDRESS *SubnetMask,
	EFI_IPv4_ADDRESS *GatewayAddress);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_CONNECT)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_CONNECTION_TOKEN *ConnectionToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_ACCEPT)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_LISTEN_TOKEN *ListenToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_TRANSMIT)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_IO_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_RECEIVE)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_IO_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_CLOSE)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_CLOSE_TOKEN *CloseToken);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_CANCEL)(
	EFI_TCP4_PROTOCOL *This,
	EFI_TCP4_COMPLETION_TOKEN *Token);

typedef EFI_STATUS(EFIAPI *EFI_TCP4_POLL)(EFI_TCP4_PROTOCOL *This);

struct EFI_TCP4_PROTOCOL
{
	EFI_TCP4_GET_MODE_DATA GetModeData;
	EFI_TCP4_CONFIGURE Configure;
	EFI_TCP4_ROUTES Routes;
	EFI_TCP4_CONNECT Connect;
	EFI_TCP4_ACCEPT Accept;
	EFI_TCP4_TRANSMIT Transmit;
	EFI_TCP4_RECEIVE Receive;
	EFI_TCP4_CLOSE Close;
	EFI_TCP4_CANCEL Cancel;
	EFI_TCP4_POLL Poll;
};

// =============================================================================
// Event Types for CreateEvent
// =============================================================================

#ifndef EVT_TIMER
#define EVT_TIMER 0x80000000
#endif
#ifndef EVT_RUNTIME
#define EVT_RUNTIME 0x40000000
#endif
#ifndef EVT_NOTIFY_WAIT
#define EVT_NOTIFY_WAIT 0x00000100
#endif
#ifndef EVT_NOTIFY_SIGNAL
#define EVT_NOTIFY_SIGNAL 0x00000200
#endif

// Timer Types for SetTimer
#ifndef TimerCancel
#define TimerCancel 0
#endif
#ifndef TimerPeriodic
#define TimerPeriodic 1
#endif
#ifndef TimerRelative
#define TimerRelative 2
#endif

// Search Types for LocateHandle/LocateHandleBuffer
#ifndef AllHandles
#define AllHandles 0
#endif
#ifndef ByRegisterNotify
#define ByRegisterNotify 1
#endif
#ifndef ByProtocol
#define ByProtocol 2
#endif
