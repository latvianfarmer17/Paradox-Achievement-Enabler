#ifndef PAE_PROXY_H
#define PAE_PROXY_H

#include <windows.h>
#include <string.h>

extern void* g_handle_main_thread;
extern void* g_hmodule_winmm;

void proxy_cleanup();

#endif