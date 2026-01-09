// ntapi types, constants, and runtime function loading

#pragma once

#include <Windows.h>
#include <winternl.h>
#include <cstdint>

namespace memfog::nt {
enum class section_inherit : std::uint32_t {
  view_share = 1,
  view_unmap = 2
};

// undocumented allocation flag that allows the view size to exceed
// the section size. this is the core of the technique because it lets us
// reserve terabytes of VA space from a tiny file-backed section.
inline constexpr std::uint32_t mem_reserve_flag = 0x2000;
inline constexpr std::uint32_t transaction_all_access = 0x12003F;

// nt function signatures
using fn_nt_create_transaction = NTSTATUS( NTAPI* )( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, LPGUID, HANDLE, ULONG, ULONG, ULONG, PLARGE_INTEGER, PUNICODE_STRING );
using fn_nt_create_section = NTSTATUS( NTAPI* )( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PLARGE_INTEGER, ULONG, ULONG, HANDLE );
using fn_nt_map_view_of_section = NTSTATUS( NTAPI* )( HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T, PLARGE_INTEGER, PSIZE_T, section_inherit, ULONG, ULONG );
using fn_create_file_transacted_a = HANDLE( WINAPI* )( LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE, HANDLE, PUSHORT, PVOID );

// runtime resolved ntapi functions
struct api {
  fn_nt_create_transaction NtCreateTransaction = nullptr;
  fn_nt_create_section NtCreateSection = nullptr;
  fn_nt_map_view_of_section NtMapViewOfSection = nullptr;
  fn_create_file_transacted_a CreateFileTransactedA = nullptr;

  [[nodiscard]] bool load() noexcept {
    const auto ntdll = GetModuleHandleA( "ntdll.dll" );
    const auto kernel32 = GetModuleHandleA( "kernel32.dll" );

    if ( !ntdll || !kernel32 )
      return false;

    NtCreateTransaction = reinterpret_cast<fn_nt_create_transaction>( GetProcAddress( ntdll, "NtCreateTransaction" ) );
    NtCreateSection = reinterpret_cast<fn_nt_create_section>( GetProcAddress( ntdll, "NtCreateSection" ) );
    NtMapViewOfSection = reinterpret_cast<fn_nt_map_view_of_section>( GetProcAddress( ntdll, "NtMapViewOfSection" ) );
    CreateFileTransactedA = reinterpret_cast<fn_create_file_transacted_a>( GetProcAddress( kernel32, "CreateFileTransactedA" ) );

    return NtCreateTransaction && NtCreateSection && NtMapViewOfSection && CreateFileTransactedA;
  }
};
} // namespace memfog::nt
