#ifndef PAE_MEMORY_H
#define PAE_MEMORY_H

#include <windows.h>
#include <psapi.h>

#include <pae/array.h>

#define PAE_MEM_READABLE (1u << 0)
#define PAE_MEM_WRITABLE (1u << 1)
#define PAE_MEM_EXECUTABLE (1u << 2)
#define PAE_MEM_READONLY (1u << 3)

#define PAE_MEM_SEARCH_FN(name) (name##_search_fn)

typedef void (*search_fn_t)(array*, UINT_PTR, BYTE*, SIZE_T, PVOID);

BOOL memory_read_byte(HANDLE h_process, UINT_PTR address, BYTE* dst);
BOOL memory_write_byte(HANDLE h_process, UINT_PTR address, BYTE value);
BOOL memory_is_region_valid(DWORD protect, UINT32 flags);
array memory_search(HANDLE h_process, UINT_PTR start_addr, search_fn_t validation_fn, PVOID search_fn_args, UINT32 flags);
array memory_find_bytes(HANDLE h_process, UINT_PTR start_addr, const UINT16* target_bytes, SIZE_T target_bytes_size, UINT32 flags);

#endif