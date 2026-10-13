// XBXPS3 core <-> shell interface.
//
// The emulator core (RPCS3 + core/src) is compiled as ordinary desktop C++ and never sees
// C++/WinRT headers. The UWP shell (app/src) never sees RPCS3 headers. This plain C ABI is
// the only contract between the two halves, so either side can be rebuilt independently.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Services the shell provides to the core. Every callback may be called from any thread.
typedef struct xbx_host_api
{
	void* ctx;

	// Current output size in pixels and refresh rate of the TV.
	void (*get_output_mode)(void* ctx, int* width, int* height, double* refresh_hz);

	// Leave the app (Exit from Big Picture Mode, or the core asked to quit).
	void (*request_exit)(void* ctx);

	// Show a fatal error to the user, then terminate. Must not return.
	void (*fatal_error)(void* ctx, const char* utf8_text);

	// Keep the TV awake while a game runs (DisplayRequest).
	void (*set_display_sleep)(void* ctx, int enabled);

	// Debugger output (OutputDebugStringA). Optional.
	void (*debug_print)(void* ctx, const char* utf8_text);

	// vk_icdGetInstanceProcAddr exported by the packaged Dozen driver (vulkan_dzn.dll),
	// or null when the driver could not be loaded (the core then falls back to the Null renderer).
	void* vk_icd_get_instance_proc_addr;
} xbx_host_api;

typedef struct xbx_core_paths
{
	const char* install_dir;     // Package install folder (read-only), UTF-8, trailing '/'
	const char* local_state_dir; // ApplicationData LocalFolder, UTF-8, trailing '/'
	const char* usb_root;        // "E:/XBXPS3/" if that folder exists on a USB drive, else null
} xbx_core_paths;

// Initializes logging, paths, Xbox default settings and the emulator. Returns 0 on success.
int xbx_core_init(const xbx_host_api* host, const xbx_core_paths* paths);

// Boots the console UI (RPCS3 Big Picture Mode), then imports anything waiting in the Install folders.
void xbx_core_start(void);

// Process lifetime events from CoreApplication.Suspending / Resuming.
void xbx_core_suspend(void);
void xbx_core_resume(void);

// Opens the PS button home menu (View + Menu held on the Xbox pad).
void xbx_core_open_home_menu(void);

// Stops emulation and flushes logs and settings.
void xbx_core_shutdown(void);

// Tests address-space reservation, JIT page protections, CPU features and D3D12 caps on the
// running console and writes the results to report_path (header text is written first).
// Safe to call before xbx_core_init.
void xbx_core_run_probe(const char* report_path_utf8, const char* header_utf8);

#ifdef __cplusplus
}
#endif
