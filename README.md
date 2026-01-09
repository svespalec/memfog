# memfog

an anti memory scan technique for windows based on research by [secret.club](https://secret.club/2021/05/23/big-memory.html).

## improvements over original
- uses transacted files (no disk artifacts)
- stacks multiple mappings for multiplied effect
- clean c++23 header-only library
- configurable parameters

## limitations
- does NOT stop debuggers
- does NOT stop static analysis
- only effective against memory scanning tools

## how it works

memory scanners query each region of a process's address space using
`NtQueryVirtualMemory`. for normal regions, this returns instantly. but
when the kernel encounters a massive reserved section view (terabytes),
it must walk billions of page table entries to answer the query. this
takes minutes per region.

```
normal 1MB region:    ~256 PTEs checked    -> microseconds
reserved 12TB view:   ~3 billion PTEs      -> 3-4 minutes
```

the technique:

1. `NtCreateTransaction` - sandbox for file ops, never committed
2. `CreateFileTransactedA` - ghost file, invisible to filesystem
3. `NtCreateSection` - section backed by the ghost file
4. `NtMapViewOfSection` with `MEM_RESERVE` - reserve terabytes of VA space
5. repeat step 4 - stack multiple regions, multiply scan time

scanners can't skip these regions because they appear legitimate until
the slow query completes.

## usage

```cpp
#include <memfog/memfog.hxx>

int main() {
  auto result = memfog::protect();

  if ( !result ) {
    // handle error: result.error()
    return 1;
  }

  // result->total_reserved_bytes contains total VA reserved
  // result->mappings contains info about each view
}
```

customize with `memfog::config`:

```cpp
memfog::config cfg {
  .view_count = 5,        // number of views to create (default: 3)
  .max_size_shift = 44,   // max view size 2^44 = 16TB (default)
  .min_size_shift = 39,   // min view size 2^39 = 512GB (default)
  .ram_multiplier = 256   // max reservation = RAM * 256 (default)
};

auto result = memfog::protect( cfg );
```

## known behavior

process termination takes longer than usual (1-5 minutes) because
windows must clean up the large va reservations. this is unavoidable
without architectural changes.

for applications requiring fast shutdown, consider a sacrificial child process pattern.
