// core data types and error handling

#pragma once

#include <string_view>
#include <vector>
#include <cstdint>

namespace memfog {
  // configuration for memory fog protection
  struct config {
    std::uint32_t view_count = 3;       // number of huge views to create
    std::uint32_t max_size_shift = 44;  // max view size as power of 2 (44 = 16TB)
    std::uint32_t min_size_shift = 39;  // min view size as power of 2 (39 = 512GB)
    std::uint32_t ram_multiplier = 256; // max total reservation = RAM * this
  };

  enum class error {
    api_load_failed,
    transaction_create_failed,
    file_create_failed,
    file_write_failed,
    section_create_failed,
    all_mappings_failed
  };

  [[nodiscard]] constexpr std::string_view to_string( error e ) noexcept {
    switch ( e ) {
      case error::api_load_failed:
        return "failed to load NT API functions";
      case error::transaction_create_failed:
        return "NtCreateTransaction failed";
      case error::file_create_failed:
        return "CreateFileTransactedA failed";
      case error::file_write_failed:
        return "WriteFile to transacted file failed";
      case error::section_create_failed:
        return "NtCreateSection failed";
      case error::all_mappings_failed:
        return "all NtMapViewOfSection attempts failed";
    }

    return "unknown error";
  }

  // info about a single mapped view
  struct mapping_info {
    void* base_address = nullptr;
    std::size_t view_size = 0;
  };

  // result of protect() containing all created mappings
  struct protection_result {
    std::vector<mapping_info> mappings;
    std::size_t total_reserved_bytes = 0;
  };
} // namespace memfog
