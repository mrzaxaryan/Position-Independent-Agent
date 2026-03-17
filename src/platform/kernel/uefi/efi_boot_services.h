/**
 * @file efi_boot_services.h
 * @brief EFI Boot Services function table.
 *
 * @details Defines the EFI_BOOT_SERVICES table and all associated function pointer typedefs.
 *          Boot Services provide memory allocation (AllocatePages, AllocatePool, FreePages, FreePool),
 *          event and timer management, protocol handler services (HandleProtocol, OpenProtocol,
 *          LocateProtocol, LocateHandleBuffer), image loading/starting, and miscellaneous utilities
 *          (Stall, SetWatchdogTimer, CopyMem, SetMem). Boot Services are available only before
 *          ExitBootServices() is called.
 *
 * @see UEFI Specification 2.10 — Section 4.4, EFI Boot Services Table
 * @see UEFI Specification 2.10 — Section 7.1, Event, Timer, and Task Priority Services
 * @see UEFI Specification 2.10 — Section 7.2, Memory Allocation Services
 * @see UEFI Specification 2.10 — Section 7.3, Protocol Handler Services
 * @see UEFI Specification 2.10 — Section 7.4, Image Services
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// Forward declarations
struct EFI_BOOT_SERVICES;

// =============================================================================
// Boot Services Function Types
// =============================================================================

// Task Priority Level
typedef EFI_TPL(EFIAPI *EFI_RAISE_TPL)(EFI_TPL NewTpl);
typedef VOID(EFIAPI *EFI_RESTORE_TPL)(EFI_TPL OldTpl);

// Memory Services
typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_PAGES)(
	EFI_ALLOCATE_TYPE Type,
	EFI_MEMORY_TYPE MemoryType,
	USIZE Pages,
	EFI_PHYSICAL_ADDRESS *Memory);

typedef EFI_STATUS(EFIAPI *EFI_FREE_PAGES)(
	EFI_PHYSICAL_ADDRESS Memory,
	USIZE Pages);

typedef EFI_STATUS(EFIAPI *EFI_GET_MEMORY_MAP)(
	USIZE *MemoryMapSize,
	PVOID MemoryMap,
	USIZE *MapKey,
	USIZE *DescriptorSize,
	UINT32 *DescriptorVersion);

typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_POOL)(
	EFI_MEMORY_TYPE PoolType,
	USIZE Size,
	PVOID *Buffer);

typedef EFI_STATUS(EFIAPI *EFI_FREE_POOL)(PVOID Buffer);

// Event Services
typedef VOID(EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT Event, PVOID Context);

typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT)(
	UINT32 Type,
	EFI_TPL NotifyTpl,
	EFI_EVENT_NOTIFY NotifyFunction,
	PVOID NotifyContext,
	EFI_EVENT *Event);

typedef EFI_STATUS(EFIAPI *EFI_SET_TIMER)(
	EFI_EVENT Event,
	UINT32 Type,
	UINT64 TriggerTime);

typedef EFI_STATUS(EFIAPI *EFI_WAIT_FOR_EVENT)(
	USIZE NumberOfEvents,
	EFI_EVENT *Event,
	USIZE *Index);

typedef EFI_STATUS(EFIAPI *EFI_SIGNAL_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS(EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS(EFIAPI *EFI_CHECK_EVENT)(EFI_EVENT Event);

// Protocol Handler Services
typedef EFI_STATUS(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(
	EFI_HANDLE *Handle,
	EFI_GUID *Protocol,
	UINT32 InterfaceType,
	PVOID Interface);

typedef EFI_STATUS(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	PVOID OldInterface,
	PVOID NewInterface);

typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	PVOID Interface);

typedef EFI_STATUS(EFIAPI *EFI_HANDLE_PROTOCOL)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	PVOID *Interface);

typedef EFI_STATUS(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
	EFI_GUID *Protocol,
	EFI_EVENT Event,
	PVOID *Registration);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE)(
	UINT32 SearchType,
	EFI_GUID *Protocol,
	PVOID SearchKey,
	USIZE *BufferSize,
	EFI_HANDLE *Buffer);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_DEVICE_PATH)(
	EFI_GUID *Protocol,
	PVOID *DevicePath,
	EFI_HANDLE *Device);

typedef EFI_STATUS(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
	EFI_GUID *Guid,
	PVOID Table);

// Image Services
typedef EFI_STATUS(EFIAPI *EFI_IMAGE_LOAD)(
	BOOL BootPolicy,
	EFI_HANDLE ParentImageHandle,
	PVOID DevicePath,
	PVOID SourceBuffer,
	USIZE SourceSize,
	EFI_HANDLE *ImageHandle);

typedef EFI_STATUS(EFIAPI *EFI_IMAGE_START)(
	EFI_HANDLE ImageHandle,
	USIZE *ExitDataSize,
	CHAR16 **ExitData);

typedef EFI_STATUS(EFIAPI *EFI_EXIT)(
	EFI_HANDLE ImageHandle,
	EFI_STATUS ExitStatus,
	USIZE ExitDataSize,
	CHAR16 *ExitData);

typedef EFI_STATUS(EFIAPI *EFI_IMAGE_UNLOAD)(EFI_HANDLE ImageHandle);

typedef EFI_STATUS(EFIAPI *EFI_EXIT_BOOT_SERVICES)(
	EFI_HANDLE ImageHandle,
	USIZE MapKey);

// Miscellaneous Services
typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(UINT64 *Count);
typedef EFI_STATUS(EFIAPI *EFI_STALL)(USIZE Microseconds);

typedef EFI_STATUS(EFIAPI *EFI_SET_WATCHDOG_TIMER)(
	USIZE Timeout,
	UINT64 WatchdogCode,
	USIZE DataSize,
	CHAR16 *WatchdogData);

// Protocol Services (added in later UEFI versions)
typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	PVOID *Interface,
	EFI_HANDLE AgentHandle,
	EFI_HANDLE ControllerHandle,
	UINT32 Attributes);

typedef EFI_STATUS(EFIAPI *EFI_CLOSE_PROTOCOL)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	EFI_HANDLE AgentHandle,
	EFI_HANDLE ControllerHandle);

typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
	EFI_HANDLE Handle,
	EFI_GUID *Protocol,
	PVOID *EntryBuffer,
	USIZE *EntryCount);

typedef EFI_STATUS(EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
	EFI_HANDLE Handle,
	EFI_GUID ***ProtocolBuffer,
	USIZE *ProtocolBufferCount);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
	UINT32 SearchType,
	EFI_GUID *Protocol,
	PVOID SearchKey,
	USIZE *NoHandles,
	EFI_HANDLE **Buffer);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_PROTOCOL)(
	EFI_GUID *Protocol,
	PVOID Registration,
	PVOID *Interface);

// Memory copy/set
typedef VOID(EFIAPI *EFI_COPY_MEM)(PVOID Destination, PVOID Source, USIZE Length);
typedef VOID(EFIAPI *EFI_SET_MEM)(PVOID Buffer, USIZE Size, UINT8 Value);

// =============================================================================
// Boot Services Table
// =============================================================================

typedef struct EFI_BOOT_SERVICES {
	EFI_TABLE_HEADER Hdr;

	// Task Priority Services
	EFI_RAISE_TPL RaiseTPL;
	EFI_RESTORE_TPL RestoreTPL;

	// Memory Services
	EFI_ALLOCATE_PAGES AllocatePages;
	EFI_FREE_PAGES FreePages;
	EFI_GET_MEMORY_MAP GetMemoryMap;
	EFI_ALLOCATE_POOL AllocatePool;
	EFI_FREE_POOL FreePool;

	// Event & Timer Services
	EFI_CREATE_EVENT CreateEvent;
	EFI_SET_TIMER SetTimer;
	EFI_WAIT_FOR_EVENT WaitForEvent;
	EFI_SIGNAL_EVENT SignalEvent;
	EFI_CLOSE_EVENT CloseEvent;
	EFI_CHECK_EVENT CheckEvent;

	// Protocol Handler Services
	EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface;
	EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface;
	EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface;
	EFI_HANDLE_PROTOCOL HandleProtocol;
	PVOID Reserved;
	EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
	EFI_LOCATE_HANDLE LocateHandle;
	EFI_LOCATE_DEVICE_PATH LocateDevicePath;
	EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;

	// Image Services
	EFI_IMAGE_LOAD LoadImage;
	EFI_IMAGE_START StartImage;
	EFI_EXIT Exit;
	EFI_IMAGE_UNLOAD UnloadImage;
	EFI_EXIT_BOOT_SERVICES ExitBootServices;

	// Miscellaneous Services
	EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount;
	EFI_STALL Stall;
	EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;

	// DriverSupport Services (UEFI 1.1+)
	PVOID ConnectController;
	PVOID DisconnectController;

	// Open and Close Protocol Services (UEFI 1.1+)
	EFI_OPEN_PROTOCOL OpenProtocol;
	EFI_CLOSE_PROTOCOL CloseProtocol;
	EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation;

	// Library Services (UEFI 1.1+)
	EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle;
	EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
	EFI_LOCATE_PROTOCOL LocateProtocol;
	PVOID InstallMultipleProtocolInterfaces;
	PVOID UninstallMultipleProtocolInterfaces;

	// CRC Services (UEFI 1.1+)
	PVOID CalculateCrc32;

	// Miscellaneous Services (UEFI 1.1+)
	EFI_COPY_MEM CopyMem;
	EFI_SET_MEM SetMem;

	// Extended Event Services (UEFI 2.0+)
	PVOID CreateEventEx;
} EFI_BOOT_SERVICES;

// Open Protocol Attributes
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL 0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER 0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE 0x00000020
