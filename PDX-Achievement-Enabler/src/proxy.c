#include <pae/proxy.h>

void* g_handle_main_thread = NULL;
void* g_hmodule_winmm = NULL;

void proxy_cleanup() {
	if (g_hmodule_winmm) {
		FreeLibrary(g_hmodule_winmm);
		g_hmodule_winmm = NULL;
	}
}