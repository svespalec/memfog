#pragma once

#include "nt.hxx"
#include "types.hxx"

#include <Windows.h>
#include <expected>
#include <string>
#include <cstdint>
#include <print>

namespace memfog::detail {
// dummy file content written to the transacted file
inline constexpr std::uint8_t file_content[] = { 'm', 'e', 'm', 'f', 'o', 'g' };

// generates a unique temp filename based on tick count
[[nodiscard]] inline std::string generate_temp_filename() {
  const auto ticks = GetTickCount64();

  char buffer[ 32 ];
  snprintf( buffer, sizeof( buffer ), "~mfg%llx.tmp", ticks );

  return buffer;
}

// returns total physical RAM in bytes, or 16GB as fallback
[[nodiscard]] inline std::size_t get_physical_memory_bytes() noexcept {
  MEMORYSTATUSEX mem_info{ .dwLength = sizeof(mem_info) };

  if ( !GlobalMemoryStatusEx( &mem_info ) )
    return 16ULL * 1024 * 1024 * 1024;

  return static_cast<std::size_t>( mem_info.ullTotalPhys );
}

// calculates max safe reservation based on physical RAM
[[nodiscard]] inline std::size_t calculate_safe_max_reservation( std::uint32_t multiplier ) noexcept {
  return get_physical_memory_bytes() * multiplier;
}

// attempts to map a section view starting at max_shift bits (e.g. 44 = 16TB)
// and works down to min_shift until one succeeds. the kernel will reject
// sizes that exceed available VA space, so we try progressively smaller.
[[nodiscard]] inline std::expected<mapping_info, NTSTATUS>
  create_mapping( const nt::api& api, HANDLE section_handle, std::uint32_t max_shift, std::uint32_t min_shift, void* hint_address = nullptr ) {
  for ( auto shift = max_shift; shift >= min_shift; --shift ) {
    void* base = hint_address;
    SIZE_T view_size = static_cast<SIZE_T>( 1ULL << shift );

    const auto status = api.NtMapViewOfSection(
      section_handle,                  // section to map
      GetCurrentProcess(),             // target process
      &base,                           // base address (in/out)
      0,                               // zero bits (address constraint)
      0x1000,                          // commit size (1 page)
      nullptr,                         // section offset
      &view_size,                      // view size (in/out)
      nt::section_inherit::view_unmap, // child process inheritance
      nt::mem_reserve_flag,            // MEM_RESERVE key to the technique
      PAGE_READWRITE                   // page protection
    );

    if ( NT_SUCCESS( status ) ) {
      return mapping_info { .base_address = base, .view_size = view_size };
    }
  }

  return std::unexpected( static_cast<NTSTATUS>( 0xC0000001L ) );
}
} // namespace memfog::detail
