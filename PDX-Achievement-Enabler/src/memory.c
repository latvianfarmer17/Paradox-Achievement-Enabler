#include <pae/memory.h>

BOOL memory_read_byte(HANDLE h_process, UINT_PTR address, BYTE* dst) {
	BYTE byte;
	SIZE_T bytes_read = 0;

	if (ReadProcessMemory(h_process, address, &byte, 1, &bytes_read) && bytes_read == 1) {
		*dst = byte;
		return TRUE;
	}

	return FALSE;
}

BOOL memory_write_byte(HANDLE h_process, UINT_PTR address, BYTE value) {
	SIZE_T bytes_written;

	if (WriteProcessMemory(h_process, address, &value, sizeof(BYTE), &bytes_written) && bytes_written == 1) {
		return TRUE;
	}

	return FALSE;
}

// Check if a memory region is valid given provided flags
BOOL memory_is_region_valid(DWORD protect, UINT32 flags) {
	protect &= 0xFF;

	BOOL readable = protect == PAGE_READONLY || protect == PAGE_READWRITE || protect == PAGE_WRITECOPY || protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
	BOOL writable = protect == PAGE_READWRITE || protect == PAGE_WRITECOPY || protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
	BOOL executable = protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;

	if ((flags & PAE_MEM_READONLY) && protect == PAGE_READONLY) {
		return TRUE;
	}

	if ((flags & PAE_MEM_READABLE) && readable) {
		return TRUE;
	}

	if ((flags & PAE_MEM_WRITABLE) && writable) {
		return TRUE;
	}

	if ((flags & PAE_MEM_EXECUTABLE) && executable) {
		return TRUE;
	}

	return FALSE;
}

// Get all specific memory regions within a process
array memory_get_regions(HANDLE h_process, UINT_PTR start_addr, UINT32 flags) {
	array memory_regions;
	array_init(&memory_regions);

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	UINT_PTR addr;

	if (start_addr < (UINT_PTR)sys_info.lpMinimumApplicationAddress) {
		addr = (UINT_PTR)sys_info.lpMinimumApplicationAddress;
	}
	else {
		addr = start_addr;
	}

	while (addr < (UINT_PTR)sys_info.lpMaximumApplicationAddress) {
		MEMORY_BASIC_INFORMATION mbi;

		if (!VirtualQueryEx(h_process, addr, &mbi, sizeof(mbi))) {
			break;
		}

		BOOL valid_region = memory_is_region_valid(mbi.Protect, flags);

		if (mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_GUARD) && !(mbi.Protect & PAGE_NOACCESS) && valid_region) {
			array_push_back(&memory_regions, (array_t)mbi.BaseAddress);
			array_push_back(&memory_regions, (array_t)mbi.RegionSize);
		}

		addr += mbi.RegionSize;
	}

	return memory_regions;
}

// Search a processes memory using a custom search function
array memory_search(HANDLE h_process, UINT_PTR start_addr, search_fn_t search_fn, PVOID search_fn_args, UINT32 flags) {
	array found_addresses;
	array_init(&found_addresses);

	array memory_regions = memory_get_regions(h_process, start_addr, flags);
	SIZE_T memory_regions_size = (SIZE_T)memory_regions.size >> 1;

	for (SIZE_T i = 0; i < memory_regions_size; i++) {
		UINT_PTR base_addr = (UINT_PTR)memory_regions.data[2 * i];
		SIZE_T region_size = (SIZE_T)memory_regions.data[2 * i + 1];

		BYTE* buffer = (BYTE*)malloc(region_size * sizeof(BYTE));
		SIZE_T bytes_read = 0;

		if (buffer == NULL) {
			continue;
		}

		if (ReadProcessMemory(h_process, base_addr, buffer, region_size, &bytes_read)) {
			search_fn(&found_addresses, base_addr, buffer, bytes_read, search_fn_args);
		}
	}

	return found_addresses;
}

// Find all arrays of bytes; any target byte larger than 0xFF is ignored (wildcard)
array memory_find_bytes(HANDLE h_process, UINT_PTR start_addr, const UINT16* target_bytes, SIZE_T target_bytes_size, UINT32 flags) {
	array found_addresses;
	array_init(&found_addresses);

	if (target_bytes == NULL) {
		return found_addresses;
	}

	array memory_regions = memory_get_regions(h_process, start_addr, flags);
	SIZE_T memory_regions_size = (SIZE_T)memory_regions.size >> 1;

	for (SIZE_T i = 0; i < memory_regions_size; i++) {
		UINT_PTR base_addr = (UINT_PTR)memory_regions.data[2 * i];
		SIZE_T region_size = (SIZE_T)memory_regions.data[2 * i + 1];

		BYTE* buffer = (BYTE*)malloc(region_size * sizeof(BYTE));
		SIZE_T bytes_read = 0;

		if (buffer == NULL) {
			continue;
		}

		if (ReadProcessMemory(h_process, base_addr, buffer, region_size, &bytes_read)) {
			if (bytes_read >= target_bytes_size) {
				for (SIZE_T j = 0; j <= bytes_read - target_bytes_size; j++) {
					BOOL found = TRUE;

					for (SIZE_T k = 0; k < target_bytes_size; k++) {
						if (target_bytes[k] > 0xFF) {
							continue;
						}
						
						BYTE value = target_bytes[k] & 0xFF;

						if (buffer[j + k] != value) {
							found = FALSE;
							break;
						}
					}

					if (found) {
						array_push_back(&found_addresses, (array_t)((ULONG_PTR)base_addr + j));
					}
				}
			}
		}

		free(buffer);
	}

	array_free(&memory_regions);

	return found_addresses;
}