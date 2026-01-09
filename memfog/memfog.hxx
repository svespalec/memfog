// anti-memory-scanning technique that creates massive reserved section views
// to drastically slow down memory analysis tools (pe-sieve, processhacker, etc).
// based on: https://secret.club/2021/05/23/big-memory.html

#pragma once

#include "nt.hxx"
#include "types.hxx"
#include "handle.hxx"
#include "detail.hxx"

#include <expected>
#include <cstdint>

namespace memfog {
  // activates memory scanning protection by creating multiple huge reserved
  // section views. each view forces memory scanning tools to iterate through
  // billions of page table entries, causing massive slowdowns.
  [[nodiscard]] inline std::expected<protection_result, error> protect( const config& cfg = {} ) {
    // load ntapi functions at runtime
    nt::api api {};

    if ( !api.load() )
      return std::unexpected( error::api_load_failed );

    // create a transaction for the ghost file (never committed to disk)
    OBJECT_ATTRIBUTES obj_attr { .Length = sizeof(obj_attr) };
    unique_handle transaction {};

    auto status = api.NtCreateTransaction(
      transaction.addressof(),    // transaction handle (out)
      nt::transaction_all_access, // desired access
      &obj_attr,                  // object attributes
      nullptr,                    // UOW (unit of work)
      nullptr,                    // TM handle
      0,                          // create options
      0,                          // isolation level
      0,                          // isolation flags
      nullptr,                    // timeout
      nullptr                     // description
    );

    if ( !NT_SUCCESS( status ) )
      return std::unexpected( error::transaction_create_failed );

    // create a transacted temp file (exists only within the transaction)
    const auto filename = detail::generate_temp_filename();

    const auto file_handle = api.CreateFileTransactedA(
      filename.c_str(),             // filename
      GENERIC_READ | GENERIC_WRITE, // desired access
      0,                            // share mode (exclusive)
      nullptr,                      // security attributes
      CREATE_NEW,                   // creation disposition
      FILE_ATTRIBUTE_NORMAL,        // flags and attributes
      nullptr,                      // template file
      transaction.get(),            // transaction handle
      nullptr,                      // mini version
      nullptr                       // extended parameter
    );

    if ( file_handle == INVALID_HANDLE_VALUE )
      return std::unexpected( error::file_create_failed );

    unique_handle file( file_handle );

    // write minimal content to make the file valid
    DWORD written = 0;

    if ( !WriteFile( file.get(), detail::file_content, static_cast<DWORD>( sizeof( detail::file_content ) ), &written, nullptr ) )
      return std::unexpected( error::file_write_failed );

    // create a section backed by the transacted file
    unique_handle section {};

    status = api.NtCreateSection(
      section.addressof(), // section handle (out)
      SECTION_ALL_ACCESS,  // desired access
      nullptr,             // object attributes
      nullptr,             // maximum size (file size)
      PAGE_READWRITE,      // page protection
      SEC_COMMIT,          // allocation attributes
      file.get()           // file handle
    );

    if ( !NT_SUCCESS( status ) )
      return std::unexpected( error::section_create_failed );

    // map multiple huge views to flood the VA space
    protection_result result {};
    result.mappings.reserve( cfg.view_count );

    const std::size_t max_total = detail::calculate_safe_max_reservation( cfg.ram_multiplier );
    void* next_hint = nullptr;

    for ( std::uint32_t i = 0; i < cfg.view_count; ++i ) {
      if ( result.total_reserved_bytes >= max_total )
        break;

      auto mapping = detail::create_mapping( api, section.get(), cfg.max_size_shift, cfg.min_size_shift, next_hint );

      if ( mapping ) {
        result.total_reserved_bytes += mapping->view_size;
        next_hint = static_cast<std::uint8_t*>( mapping->base_address ) + mapping->view_size;
        result.mappings.push_back( *mapping );
      }
    }

    if ( result.mappings.empty() )
      return std::unexpected( error::all_mappings_failed );

    return result;
  }

  // convenience wrapper that returns total reserved bytes, or 0 on failure.
  // use protect() directly if you need error details.
  [[nodiscard]] inline std::size_t protect_simple( const config& cfg = {} ) {
    auto result = protect( cfg );
    return result ? result->total_reserved_bytes : 0;
  }
} // namespace memfog
