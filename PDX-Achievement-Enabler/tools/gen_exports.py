class Function:
    def __init__(self, return_type, call_type, name, args):
        self.return_type = return_type
        self.call_type = call_type
        self.name = name
        self.args = args

def parse_functions(path):
    functions = []
    signitures = open(path, "r").read().split("\n")
    
    for s in signitures:
        args_index = s.find("(")
        
        return_type, call_type, name = s[:args_index-1].split(" ")
        args = s[args_index+1:-1].split(",")

        if args == ['']:
            args = []
        
        functions.append(Function(return_type, call_type, name, args))
        
    return functions

def write_header(file):
    file.write('// Auto-generated, see "tools/gen_exports.py"\n\n')
    file.write('#include <pae/proxy.h>\n\n')

def write_export_table(file, functions):
    file.write('// Export Table\n');
    
    for f in functions:
        file.write(f'#pragma comment(linker, "/EXPORT:{f.name}=PAE_{f.name}")\n')
        
    file.write('\n')

def write_type_definitions(file, functions):
    file.write('// Type Definitions\n');
    
    file.write('typedef void(WINAPI* LPTASKCALLBACK)(DWORD_PTR);\n');
    file.write('typedef const MMCKINFO CMMCKINFO;\n');
    
    for f in functions:
        type_def = f'typedef {f.return_type}({f.call_type}* PFN_{f.name})('
        
        for i, a in enumerate(f.args):
            arg_type = a.split(" ")[0]
            type_def += arg_type
            
            if i < len(f.args) - 1:
                type_def += ', '
        
        type_def += ');\n'
        
        file.write(type_def)
    
    file.write('\n')

def write_resume_main_thread(file):
    file.write("""// Safely resume the suspended main thread, see \"main.c\" for explanation
static inline void resume_main_thread() {
    void* h = InterlockedExchangePointer((void* volatile*)&g_handle_main_thread, NULL);

    if (h != NULL) {
        ResumeThread(h);
    }
}\n\n""")

def write_load_winmm_dll(file):
    file.write("""// Safely lazy load winmm.dll in exported functions as it may deadlock in DllMain
static void load_winmm_dll() {
	enum {
		DLL_STATE_NOT_LOADED,
		DLL_STATE_RESOLVING,
		DLL_STATE_LOADED
	};
	
	static volatile long dll_state = DLL_STATE_NOT_LOADED;

	long current_state = InterlockedCompareExchange(&dll_state, DLL_STATE_RESOLVING, DLL_STATE_NOT_LOADED);

	if (current_state == DLL_STATE_LOADED) {
		return;
	}

	if (current_state == DLL_STATE_NOT_LOADED) {
		char path[MAX_PATH];

		if (GetSystemDirectoryA(path, MAX_PATH) == 0) {
			InterlockedExchange(&dll_state, DLL_STATE_NOT_LOADED);
			return;
		}

		if (strcat_s(path, sizeof(path), "\\\\winmm.dll") != 0) {
			InterlockedExchange(&dll_state, DLL_STATE_NOT_LOADED);
			return;
		}

		void* h = (void*)LoadLibraryA(path);

		if (h == NULL) {
			InterlockedExchange(&dll_state, DLL_STATE_NOT_LOADED);
			return;
		}

		g_hmodule_winmm = h;

		InterlockedExchange(&dll_state, DLL_STATE_LOADED);

		return;
	}

	while (InterlockedCompareExchange(&dll_state, DLL_STATE_NOT_LOADED, DLL_STATE_NOT_LOADED) == DLL_STATE_RESOLVING);
}\n\n""")

def write_func_definitions(file, functions):
    file.write('// Function Definitions\n')
    
    for f in functions:
        func_def = f'{f.return_type} {f.call_type} PAE_{f.name}('
        
        for i, a in enumerate(f.args):
            func_def += a
            
            if i < len(f.args) - 1:
                func_def += ', '
        
        func_def += ') {\n'
        func_def += f'\tstatic PFN_{f.name} func = NULL;\n\n'
        func_def += '\tload_winmm_dll();\n'
        func_def += '\tresume_main_thread();\n\n'
        func_def += '\tif (!func) {\n'
        func_def += f'\t\tfunc = (PFN_{f.name})GetProcAddress(g_hmodule_winmm, "{f.name}");\n\t}}\n\n'
        func_def += f'\treturn func('
        
        for i, a in enumerate(f.args):
            func_def += a.split(" ")[1]
            
            if i < len(f.args) - 1:
                func_def += ', '
        
        func_def += ');\n}\n\n'
        
        file.write(func_def)

if __name__ == "__main__":
    functions = parse_functions("winmm_signitures.txt")
    export_file = open("exports.c", "w")
    
    write_header(export_file)
    write_export_table(export_file, functions)
    write_type_definitions(export_file, functions)
    write_resume_main_thread(export_file)
    write_load_winmm_dll(export_file)
    write_func_definitions(export_file, functions)
    
    export_file.close()