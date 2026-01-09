# memfog

anti-memory-scanning technique for windows.

based on research by [secret.club](https://secret.club/2021/05/23/big-memory.html).

## improvements over original
- uses transacted files (no disk artifacts)
- stacks multiple mappings for multiplied effect
- clean c++23 single-header library
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
normal region (1mb committed):
    kernel checks ~256 page table entries
    query returns in microseconds

reserved section view (12tb):
    kernel walks ~3 billion page table entries
    query returns in 3-4 minutes
```

the technique:

1. create a transaction (`NtCreateTransaction`)
   - acts as a sandbox for file operations
   - never committed, so no disk artifacts

2. create a file inside the transaction (`CreateFileTransactedA`)
   - file exists only in transaction context
   - invisible to filesystem, leaves no trace

3. create a section backed by that file (`NtCreateSection`)
   - section object now exists in kernel
   - requires non-empty file backing

4. map a view with the `MEM_RESERVE` flag (`NtMapViewOfSection`)
   - normally view_size must be <= section_size
   - with `0x2000` flag: view_size can be terabytes larger
   - kernel reserves virtual address space without committing pages

5. repeat step 4 at different base addresses
   - stack multiple huge regions
   - each one multiplies the scan time

scanners can't skip these regions because they don't know it's a trap
until after the slow query completes. and skipping large regions would
miss legitimate threats hiding in large mappings (databases, games, etc).

## known behavior

process termination takes longer than usual (1-5 minutes) because
windows must clean up the large va reservations. this is unavoidable
without architectural changes.

for applications requiring fast shutdown, consider a sacrificial child process pattern.
