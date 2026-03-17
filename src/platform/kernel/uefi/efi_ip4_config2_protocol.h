/**
 * @file efi_ip4_config2_protocol.h
 * @brief EFI IPv4 Configuration 2 Protocol for network configuration.
 *
 * @details Defines the EFI_IP4_CONFIG2_PROTOCOL for configuring IPv4 network parameters,
 *          supporting both static and DHCP-based address assignment. Includes data type
 *          enumerations (InterfaceInfo, Policy, ManualAddress, Gateway, DnsServer),
 *          the EFI_IP4_CONFIG2_POLICY enum for selecting static vs. DHCP configuration,
 *          EFI_IP4_CONFIG2_MANUAL_ADDRESS for static IP configuration, and
 *          EFI_IP4_CONFIG2_INTERFACE_INFO for querying interface details.
 *
 * @see UEFI Specification 2.10 — Section 28.5, EFI IPv4 Configuration II Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"
#include "platform/kernel/uefi/efi_tcp4_protocol.h"

// =============================================================================
// IP4 Config2 Protocol GUID
// =============================================================================

// {5B446ED1-E30B-4FAA-871A-3654ECA36080}
#define EFI_IP4_CONFIG2_PROTOCOL_GUID                                                              \
	{                                                                                              \
		0x5B446ED1, 0xE30B, 0x4FAA, { 0x87, 0x1A, 0x36, 0x54, 0xEC, 0xA3, 0x60, 0x80 }             \
	}

// =============================================================================
// IP4 Config2 Data Types
// =============================================================================

typedef enum
{
	Ip4Config2DataTypeInterfaceInfo,
	Ip4Config2DataTypePolicy,
	Ip4Config2DataTypeManualAddress,
	Ip4Config2DataTypeGateway,
	Ip4Config2DataTypeDnsServer,
	Ip4Config2DataTypeMaximum
} EFI_IP4_CONFIG2_DATA_TYPE;

typedef enum
{
	Ip4Config2PolicyStatic,
	Ip4Config2PolicyDhcp,
	Ip4Config2PolicyMax
} EFI_IP4_CONFIG2_POLICY;

typedef struct
{
	EFI_IPv4_ADDRESS Address;
	EFI_IPv4_ADDRESS SubnetMask;
} EFI_IP4_CONFIG2_MANUAL_ADDRESS;

typedef struct
{
	CHAR16 Name[32];
	UINT8 IfType;
	UINT32 HwAddressSize;
	UINT8 HwAddress[32];
	UINT32 RouteTableSize;
	PVOID RouteTable;
} EFI_IP4_CONFIG2_INTERFACE_INFO;

// =============================================================================
// IP4 Config2 Protocol
// =============================================================================

typedef struct EFI_IP4_CONFIG2_PROTOCOL EFI_IP4_CONFIG2_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_IP4_CONFIG2_SET_DATA)(
	EFI_IP4_CONFIG2_PROTOCOL *This,
	EFI_IP4_CONFIG2_DATA_TYPE DataType,
	USIZE DataSize,
	PVOID Data);

typedef EFI_STATUS(EFIAPI *EFI_IP4_CONFIG2_GET_DATA)(
	EFI_IP4_CONFIG2_PROTOCOL *This,
	EFI_IP4_CONFIG2_DATA_TYPE DataType,
	USIZE *DataSize,
	PVOID Data);

typedef EFI_STATUS(EFIAPI *EFI_IP4_CONFIG2_REGISTER_DATA_NOTIFY)(
	EFI_IP4_CONFIG2_PROTOCOL *This,
	EFI_IP4_CONFIG2_DATA_TYPE DataType,
	EFI_EVENT Event);

typedef EFI_STATUS(EFIAPI *EFI_IP4_CONFIG2_UNREGISTER_DATA_NOTIFY)(
	EFI_IP4_CONFIG2_PROTOCOL *This,
	EFI_IP4_CONFIG2_DATA_TYPE DataType,
	EFI_EVENT Event);

struct EFI_IP4_CONFIG2_PROTOCOL
{
	EFI_IP4_CONFIG2_SET_DATA SetData;
	EFI_IP4_CONFIG2_GET_DATA GetData;
	EFI_IP4_CONFIG2_REGISTER_DATA_NOTIFY RegisterDataNotify;
	EFI_IP4_CONFIG2_UNREGISTER_DATA_NOTIFY UnregisterDataNotify;
};
