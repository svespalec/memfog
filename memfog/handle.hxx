#pragma once

#include <Windows.h>
#include <utility>

namespace memfog {
// move-only RAII wrapper for Windows HANDLEs
class unique_handle {
public:
  unique_handle() = default;

  explicit unique_handle( HANDLE h ) noexcept : m_handle( h ) {}

  ~unique_handle() {
    close();
  }

  // non-copyable
  unique_handle( const unique_handle& ) = delete;
  unique_handle& operator= ( const unique_handle& ) = delete;

  // movable
  unique_handle( unique_handle&& other ) noexcept : m_handle( std::exchange( other.m_handle, nullptr ) ) {}

  unique_handle& operator= ( unique_handle&& other ) noexcept {
    if ( this != &other ) {
      close();
      m_handle = std::exchange( other.m_handle, nullptr );
    }

    return *this;
  }

  [[nodiscard]] HANDLE get() const noexcept {
    return m_handle;
  }

  [[nodiscard]] HANDLE* addressof() noexcept {
    return &m_handle;
  }

  [[nodiscard]] bool valid() const noexcept {
    return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
  }

private:
  void close() noexcept {
    if ( valid() ) {
      CloseHandle( m_handle );
      m_handle = nullptr;
    }
  }

  HANDLE m_handle = nullptr;
};
} // namespace memfog
