#ifndef PAE_H
#define PAE_H

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <pae/proxy.h>
#include <pae/array.h>
#include <pae/memory.h>

extern BOOL g_debug;

void pae_printf(const char* format, ...);
BOOL pae_find_game();
char* pae_find_checksum_str(HANDLE h_process);
UINT_PTR pae_find_checksum_addr(HANDLE h_process, const char* checksum);
array pae_find_lea_addr_refs(HANDLE h_process, UINT_PTR checksum_addr);
BOOL pae_patch(HANDLE h_process, UINT_PTR checksum_addr);

#endif