#include <pae/pae.h>

void pae_printf(const char* format, ...) {
	if (g_debug) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}

BOOL pae_find_game() {
	char path[MAX_PATH];
	DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);

	if (len == 0) {
		pae_printf("[pae_find_game] Could not get the executable path\n");
		return FALSE;
	}

	char* exe_name = strrchr(path, '\\');
	exe_name = exe_name ? exe_name + 1 : path;

	if (_stricmp(exe_name, "hoi4.exe") == 0) {
		pae_printf("[pae_find_game] Game is \"HOI4\"\n");
	}
	else if (_stricmp(exe_name, "stellaris.exe") == 0) {
		pae_printf("[pae_find_game] Game is \"Stellaris\"\n");
	}
	else if (_stricmp(exe_name, "eu4.exe") == 0) {
		pae_printf("[pae_find_game] Game is \"EU4\"\n");
	}
	else {
		pae_printf("[pae_find_game] The game \"%s\" is not supported\n", exe_name);
		return FALSE;
	}

	return TRUE;
}

static void PAE_MEM_SEARCH_FN(pae_find_checksum_str)(array* found_addresses, UINT_PTR base_addr, BYTE* buffer, SIZE_T bytes_read, PVOID args) {
	const SIZE_T checksum_size = 32;

	if (bytes_read < checksum_size) {
		return;
	}

	for (SIZE_T i = 0; i <= bytes_read - checksum_size; i++) {
		BOOL found = TRUE;

		for (SIZE_T j = 0; j < checksum_size; j++) {
			BYTE c = buffer[i + j];

			if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
				found = FALSE;
				break;
			}
		}

		if (!found) {
			continue;
		}

		if (i > 0 && buffer[i - 1] != ' ') {
			continue;
		}

		if (i + checksum_size < bytes_read && buffer[i + checksum_size] != ' ') {
			continue;
		}

		if (i + checksum_size + 1 < bytes_read && buffer[i + checksum_size + 1] != '(') {
			continue;
		}

		array_push_back(found_addresses, (array_t)((UINT_PTR)base_addr + i));
	}
}

char* pae_find_checksum_str(HANDLE h_process) {
	static char* checksum[33];

	BOOL successful = TRUE;
	array found_addresses = memory_search(h_process, 0, PAE_MEM_SEARCH_FN(pae_find_checksum_str), NULL, PAE_MEM_READONLY);

	if (found_addresses.size == 0) {
		pae_printf("[pae_find_checksum_str] Could not find a valid checksum string\n");
		successful = FALSE;
	}
	else if (found_addresses.size > 1) {
		pae_printf("[pae_find_checksum_str] Found more that one checksum string\n");

		for (int i = 0; i < found_addresses.size; i++) {
			pae_printf("[pae_find_checksum_str] \"%.35s\" at 0x%p\n", (char*)((UINT_PTR)found_addresses.data[i] - 1), (PVOID)found_addresses.data[i]);
		}

		successful = FALSE;
	}
	else {
		pae_printf("[pae_find_checksum_str] Found checksum string \"%.35s\" at 0x%p\n", (char*)((UINT_PTR)found_addresses.data[0] - 1), (PVOID)found_addresses.data[0]);
		memcpy_s(checksum, 33, found_addresses.data[0], 32);
		checksum[32] = '\0';
	}

	array_free(&found_addresses);

	return successful ? checksum : NULL;
}

static void PAE_MEM_SEARCH_FN(pae_find_checksum_addr)(array* found_addresses, UINT_PTR base_addr, BYTE* buffer, SIZE_T bytes_read, PVOID args) {
	const char* checksum = (const char*)args;
	const SIZE_T checksum_size = 32;

	if (bytes_read < checksum_size) {
		return;
	}

	for (SIZE_T i = 0; i <= bytes_read - checksum_size; i++) {
		if (memcmp(buffer + i, checksum, checksum_size) != 0) {
			continue;
		}

		if (i > 0 && buffer[i - 1] == ' ') {
			continue;
		}
			
		if (i + checksum_size < bytes_read && buffer[i + checksum_size] == ' ') {
			continue;
		}

		array_push_back(found_addresses, (array_t)(base_addr + i));
	}
}

UINT_PTR pae_find_checksum_addr(HANDLE h_process, const char* checksum) {
	UINT_PTR checksum_addr = NULL;
	
	array found_addresses = memory_search(h_process, 0, PAE_MEM_SEARCH_FN(pae_find_checksum_addr), checksum, PAE_MEM_READONLY);

	if (found_addresses.size == 0) {
		pae_printf("[pae_find_checksum_addr] Could not find a valid checksum address\n");
	}
	else if (found_addresses.size > 1) {
		pae_printf("[pae_find_checksum_addr] Found more that one checksum address\n");

		for (int i = 0; i < found_addresses.size; i++) {
			pae_printf("[pae_find_checksum_addr] 0x%p\n", (PVOID)found_addresses.data[i]);
		}
	}
	else {
		pae_printf("[pae_find_checksum_addr] Found checksum address at 0x%p\n", (PVOID)found_addresses.data[0]);
		checksum_addr = (UINT_PTR)found_addresses.data[0];
	}

	return checksum_addr;
}

static void PAE_MEM_SEARCH_FN(pae_find_lea_addr_refs)(array* found_addresses, UINT_PTR base_addr, BYTE* buffer, SIZE_T bytes_read, PVOID args) {
	UINT_PTR checksum_addr = *(UINT_PTR*)args;

	for (SIZE_T i = 0; i + 7 <= bytes_read; i++) {
		if (buffer[i] != 0x48 || buffer[i + 1] != 0x8D || buffer[i + 2] != 0x0D) {
			continue;
		}

		INT32 displacement;

		memcpy(&displacement, buffer + i + 3, sizeof(displacement));

		UINT_PTR instruction_addr = base_addr + i;
		UINT_PTR target_addr = instruction_addr + 7 + displacement;

		if (target_addr == checksum_addr) {
			array_push_back(found_addresses, (array_t)instruction_addr);
		}
	}
}

array pae_find_lea_addr_refs(HANDLE h_process, UINT_PTR checksum_addr) {
	return memory_search(h_process, 0, PAE_MEM_SEARCH_FN(pae_find_lea_addr_refs), &checksum_addr, PAE_MEM_READABLE | PAE_MEM_WRITABLE | PAE_MEM_EXECUTABLE);
}

static UINT_PTR pae_patch_is_loc_valid(HANDLE h_process, UINT_PTR* addr) {
	BOOL valid = FALSE;

	if (*addr == NULL) {
		return NULL;
	}

	SIZE_T offset = 0;

	for (offset = 0; offset < 32; offset++) {
		BYTE buffer[2];
		SIZE_T bytes_read = 0;

		if (!ReadProcessMemory(h_process, *addr + offset, buffer, 2, &bytes_read) || bytes_read != 2) {
			continue;
		}

		if (buffer[0] == 0x0F && buffer[1] == 0x94) {
			valid = TRUE;
			break;
		}
	}

	if (valid) {
		*addr += offset;
	}

	return valid;
}

BOOL pae_patch(HANDLE h_process, UINT_PTR checksum_addr) {
	array found_addresses = pae_find_lea_addr_refs(h_process, checksum_addr);

	pae_printf("[pae_patch] Found %d potential LEA %s:\n", found_addresses.size, found_addresses.size > 1 ? "addresses" : "address");

	for (int i = 0; i < found_addresses.size; i++) {
		pae_printf("[pae_patch]     0x%p\n", (PVOID)found_addresses.data[i]);
	}

	array instruction_addresses;
	array_init(&instruction_addresses);

	for (int i = 0; i < found_addresses.size; i++) {
		UINT_PTR lea_addr = (UINT_PTR)found_addresses.data[i];

		if (pae_patch_is_loc_valid(h_process, &lea_addr)) {
			pae_printf("[pae_patch] Found instruction address (%d) at 0x%p\n", instruction_addresses.size, (PVOID)lea_addr);
			array_push_back(&instruction_addresses, (array_t)lea_addr);
		}
	}

	array_free(&found_addresses);

	if (instruction_addresses.size == 0) {
		pae_printf("[pae_patch] Could not find any instruction addresses\n");
		return FALSE;
	}

	for (int i = 0; i < instruction_addresses.size; i++) {
		UINT_PTR instruction_addr = (UINT_PTR)instruction_addresses.data[i];
		
		BOOL byte_written;
		BYTE reg, new_reg;

		if (!memory_read_byte(h_process, instruction_addr + 2, &reg)) {
			pae_printf("[pae_patch] Failed to read at insruction address (%d) at 0x%p\n", i, (PVOID)instruction_addr);
			return FALSE;
		}

		new_reg = 0xB0 + (reg & 0x07);

		pae_printf("[pae_patch] Patching (%d) [0F 94 %02X -> %02X 01 90]\n", i, reg, new_reg);

		byte_written =
			memory_write_byte(h_process, instruction_addr, new_reg) &&
			memory_write_byte(h_process, instruction_addr + 1, 0x01) &&
			memory_write_byte(h_process, instruction_addr + 2, 0x90);

		if (!byte_written) {
			pae_printf("[pae_patch] Failed to patch (%d)\n", i);
			return FALSE;
		}
	}

	return TRUE;
}