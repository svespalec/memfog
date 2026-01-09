#include <memfog/memfog.hxx>

int main() {
  auto result = memfog::protect();

  if ( !result ) {
    std::println( "error: {}", memfog::to_string( result.error() ) );
    return 1;
  }

  std::println( "reserved: {} GB", result->total_reserved_bytes >> 30 );
  std::println( "press enter to exit..." );
  
  return std::getchar();
}
