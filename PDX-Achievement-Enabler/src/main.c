#include <pae/pae.h>

BOOL g_debug = FALSE;

static void console_wait_to_close() {
    pae_printf("\nPress END to close\n");
    
    while (TRUE) {
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        Sleep(100);
    }

    fclose(stdout);
    fclose(stderr);

    FreeConsole();
}

static DWORD WINAPI main_thread(LPVOID lpParam) {
    g_debug = strstr(GetCommandLineA(), "-debug") != NULL;

    FILE* fp;
    DWORD exit_code = 0;

    if (g_debug) {
        if (AllocConsole()) {
            SetConsoleTitleA("Paradox Achievement Enabler v1.0 - Debug Console");
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        }
        else {
            g_debug = FALSE;
        }
    }

    pae_printf("== Paradox Achievement Enabler ==\n\n");

    HANDLE h_process = GetCurrentProcess();

    if (!h_process) {
        pae_printf("[main_thread] Could not get a valid process handle\n");
        exit_code = 1;
        goto LBL_EXIT;
    }

    if (!pae_find_game()) {
        exit_code = 1;
        goto LBL_EXIT;
    }

    char* checksum = pae_find_checksum_str(h_process);

    if (!checksum) {
        exit_code = 1;
        goto LBL_EXIT;
    }

    UINT_PTR checksum_addr = pae_find_checksum_addr(h_process, checksum);

    if (!checksum_addr) {
        exit_code = 1;
        goto LBL_EXIT;
    }

    if (!pae_patch(h_process, checksum_addr)) {
        MessageBoxA(NULL, "Failed to patch the game, achievements are not enabled! Download/build the latest version!", "Paradox Achievement Enabler v1.0", MB_OK | MB_ICONERROR);
    }

LBL_EXIT:
    if (g_debug) {
        console_wait_to_close();
    }

    return exit_code;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);

        /*
        The thread is suspended here and started by one of the exported functions of winmm.dll.
        Stellaris v4.x returns 0xC0000409 (STATUS_FAIL_FAST_EXCEPTION) when the game is launched
        from Steam through the Paradox Launcher. From my analysis, I suspect this is related to
        newer Stellaris versions using a newer version of the Visual C++ runtime (UCRT), which the
        game is statically linked to. It seems that this runtime may not handle a new thread being
        created and executed from DllMain, possibly as some kind of security mechanism against DLL
        proxying.

        The strange thing is that the error only occurs when the game is launched from Steam and
        then through the launcher. It does not occur when launching stellaris.exe directly, or when
        opening the Paradox Launcher without Steam and then launching the game. This makes me think
        that launching through Steam may load additional DLLs or set certain process/security flags
        that cause the game to behave differently, possibly forcing it to use or initialise its
        statically linked UCRT in a different way?

        Either way, this method seems to work, however janky it is; in the words of Todd Howard,
        it just works.
        */
        g_handle_main_thread = CreateThread(NULL, 0, main_thread, NULL, CREATE_SUSPENDED, NULL);

        if (!g_handle_main_thread) {
            proxy_cleanup();
            return FALSE;
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH) {
        proxy_cleanup();
    }

    return TRUE;
}