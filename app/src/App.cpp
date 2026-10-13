// XBXPS3 UWP shell: a CoreWindow app (no XAML) that hosts the RPCS3 core.
//
// The shell owns everything that needs WinRT: the window and its composition visual,
// TV output mode, app lifecycle, the B/Back button, the controller chord for the PS
// home menu, and loading the Dozen Vulkan driver from the package. The console UI itself
// is RPCS3's Big Picture Mode, rendered by the emulator through Vulkan -> Dozen -> D3D12.
#include "pch.h"

#include "xbx_core.h"

using namespace winrt;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Gaming::Input;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Core;

namespace
{
	// Exports of the Mesa-UWP Dozen build (src/util/uwp_util.h in rodrigoandrigo/Mesa-UWP)
	using pfn_uwp_set_window_reference = void (*)(void* window, int width, int height);
	using pfn_mesa_uwp_swapchain_attach_callback = long(__cdecl*)(void* opaque, void* swapchain);
	using pfn_mesa_uwp_set_swapchain_attach_callback = void (*)(pfn_mesa_uwp_swapchain_attach_callback callback, void* opaque);
	using pfn_mesa_uwp_set_log_callback = void (*)(void (*callback)(void* opaque, const char* message), void* opaque);
	using pfn_vk_icdNegotiateLoaderICDInterfaceVersion = int(__stdcall*)(uint32_t* version);

	std::string to_utf8_dir(const hstring& path)
	{
		std::string s = to_string(path);
		for (char& c : s)
		{
			if (c == '\\')
				c = '/';
		}
		if (!s.empty() && s.back() != '/')
			s += '/';
		return s;
	}

	// Largest swapchain each console can create (XBSX2 found that a 4K swapchain crashes on Series S)
	void max_output_for_console(int& width, int& height)
	{
		width = 3840;
		height = 2160;

		GAMING_DEVICE_MODEL_INFORMATION info{};
		if (SUCCEEDED(GetGamingDeviceModelInformation(&info)) && info.vendorId == GAMING_DEVICE_VENDOR_ID_MICROSOFT)
		{
			switch (info.deviceId)
			{
			case GAMING_DEVICE_DEVICE_ID_XBOX_SERIES_S:
				width = 2560;
				height = 1440;
				break;
			case GAMING_DEVICE_DEVICE_ID_XBOX_ONE:
			case GAMING_DEVICE_DEVICE_ID_XBOX_ONE_S:
				width = 1920;
				height = 1080;
				break;
			default:
				break;
			}
		}
	}

	const char* console_name()
	{
		GAMING_DEVICE_MODEL_INFORMATION info{};
		if (FAILED(GetGamingDeviceModelInformation(&info)) || info.vendorId != GAMING_DEVICE_VENDOR_ID_MICROSOFT)
			return "not an Xbox";

		switch (info.deviceId)
		{
		case GAMING_DEVICE_DEVICE_ID_XBOX_SERIES_X: return "Xbox Series X";
		case GAMING_DEVICE_DEVICE_ID_XBOX_SERIES_X_DEVKIT: return "Xbox Series X devkit";
		case GAMING_DEVICE_DEVICE_ID_XBOX_SERIES_S: return "Xbox Series S";
		case GAMING_DEVICE_DEVICE_ID_XBOX_ONE_X: return "Xbox One X";
		case GAMING_DEVICE_DEVICE_ID_XBOX_ONE_S: return "Xbox One S";
		case GAMING_DEVICE_DEVICE_ID_XBOX_ONE: return "Xbox One";
		default: return "unknown Xbox";
		}
	}
}

struct App : implements<App, IFrameworkViewSource, IFrameworkView>
{
	CoreWindow m_window{ nullptr };
	CoreDispatcher m_dispatcher{ nullptr };

	// Composition tree: the Dozen swapchain is shown through a sprite visual filling the window
	Compositor m_compositor{ nullptr };
	CompositionTarget m_target{ nullptr };
	SpriteVisual m_sprite{ nullptr };
	ICompositionSurface m_swapchain_surface{ nullptr };

	winrt::Windows::System::Display::DisplayRequest m_display_request{ nullptr };
	bool m_display_request_active = false;

	std::atomic<bool> m_running{ true };
	bool m_core_started = false;

	int m_out_width = 1920;
	int m_out_height = 1080;
	double m_refresh = 60.0;

	HMODULE m_dozen = nullptr;
	void* m_vk_gipa = nullptr;

	bool m_chord_down = false;

	IFrameworkView CreateView()
	{
		return *this;
	}

	void Initialize(CoreApplicationView const& view)
	{
		view.Activated([](CoreApplicationView const&, IActivatedEventArgs const&)
		{
			CoreWindow::GetForCurrentThread().Activate();
		});

		CoreApplication::Suspending([this](IInspectable const&, SuspendingEventArgs const& args)
		{
			// Get the emulator frozen before the OS suspends the process
			const SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();
			if (m_core_started)
			{
				xbx_core_suspend();
			}
			deferral.Complete();
		});

		CoreApplication::Resuming([this](IInspectable const&, IInspectable const&)
		{
			if (m_core_started)
			{
				xbx_core_resume();
			}
		});
	}

	void SetWindow(CoreWindow const& window)
	{
		m_window = window;
		m_dispatcher = window.Dispatcher();

		m_compositor = Compositor();
		m_target = m_compositor.CreateTargetForCurrentView();

		ContainerVisual root = m_compositor.CreateContainerVisual();
		root.RelativeSizeAdjustment({ 1.0f, 1.0f });

		m_sprite = m_compositor.CreateSpriteVisual();
		m_sprite.RelativeSizeAdjustment({ 1.0f, 1.0f });
		m_sprite.Brush(m_compositor.CreateColorBrush({ 0xFF, 0, 0, 0 }));
		root.Children().InsertAtTop(m_sprite);

		m_target.Root(root);
	}

	void Load(hstring const&)
	{
	}

	void Uninitialize()
	{
	}

	void Run()
	{
		m_window.Activate();

		// B is "back" to the system. Swallow it so the console never navigates away from the app;
		// RPCS3 reads B (Circle) from the controller itself.
		SystemNavigationManager::GetForCurrentView().BackRequested([](IInspectable const&, BackRequestedEventArgs const& args)
		{
			args.Handled(true);
		});

		m_display_request = winrt::Windows::System::Display::DisplayRequest();

		query_output_mode();
		load_dozen();

		const std::string local_state = to_utf8_dir(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path());
		const std::string install_dir = to_utf8_dir(Package::Current().InstalledLocation().Path());
		const std::string usb_root = find_usb_root();

		run_probe(local_state);

		xbx_host_api host{};
		host.ctx = this;
		host.get_output_mode = &App::host_get_output_mode;
		host.request_exit = &App::host_request_exit;
		host.fatal_error = &App::host_fatal_error;
		host.set_display_sleep = &App::host_set_display_sleep;
		host.debug_print = [](void*, const char* text) { OutputDebugStringA(text); };
		host.vk_icd_get_instance_proc_addr = m_vk_gipa;

		xbx_core_paths paths{};
		paths.install_dir = install_dir.c_str();
		paths.local_state_dir = local_state.c_str();
		paths.usb_root = usb_root.empty() ? nullptr : usb_root.c_str();

		if (xbx_core_init(&host, &paths) != 0)
		{
			show_message_and_wait(L"XBXPS3 could not start the emulator core. See LocalState/log/RPCS3.log.");
			CoreApplication::Exit();
			return;
		}

		m_core_started = true;
		warn_if_not_game_mode();
		xbx_core_start();

		while (m_running)
		{
			m_dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			poll_home_menu_chord();
			std::this_thread::sleep_for(std::chrono::milliseconds(4));
		}

		xbx_core_shutdown();
		CoreApplication::Exit();
	}

	// --- Output ---------------------------------------------------------------------------

	void query_output_mode()
	{
		int max_w, max_h;
		max_output_for_console(max_w, max_h);

		if (const HdmiDisplayInformation hdi = HdmiDisplayInformation::GetForCurrentView())
		{
			const HdmiDisplayMode mode = hdi.GetCurrentDisplayMode();
			m_out_width = static_cast<int>(mode.ResolutionWidthInRawPixels());
			m_out_height = static_cast<int>(mode.ResolutionHeightInRawPixels());
			m_refresh = mode.RefreshRate();
		}

		m_out_width = std::min(m_out_width > 0 ? m_out_width : 1920, max_w);
		m_out_height = std::min(m_out_height > 0 ? m_out_height : 1080, max_h);

		if (m_refresh <= 0.0)
			m_refresh = 60.0;
	}

	// --- Vulkan driver --------------------------------------------------------------------

	void load_dozen()
	{
		m_dozen = LoadPackagedLibrary(L"vulkan_dzn.dll", 0);
		if (!m_dozen)
		{
			OutputDebugStringA("XBXPS3: vulkan_dzn.dll is missing from the package\n");
			return;
		}

		const auto set_window = reinterpret_cast<pfn_uwp_set_window_reference>(GetProcAddress(m_dozen, "uwp_set_window_reference"));
		const auto set_attach = reinterpret_cast<pfn_mesa_uwp_set_swapchain_attach_callback>(GetProcAddress(m_dozen, "mesa_uwp_set_swapchain_attach_callback"));
		const auto set_log = reinterpret_cast<pfn_mesa_uwp_set_log_callback>(GetProcAddress(m_dozen, "mesa_uwp_set_log_callback"));
		const auto negotiate = reinterpret_cast<pfn_vk_icdNegotiateLoaderICDInterfaceVersion>(GetProcAddress(m_dozen, "vk_icdNegotiateLoaderICDInterfaceVersion"));
		m_vk_gipa = reinterpret_cast<void*>(GetProcAddress(m_dozen, "vk_icdGetInstanceProcAddr"));

		if (!set_window || !set_attach || !m_vk_gipa)
		{
			OutputDebugStringA("XBXPS3: vulkan_dzn.dll is not a Mesa-UWP build (uwp_* exports missing)\n");
			m_vk_gipa = nullptr;
			return;
		}

		if (negotiate)
		{
			uint32_t version = 5;
			negotiate(&version);
		}

		if (set_log)
		{
			set_log([](void*, const char* message) { OutputDebugStringA(message); }, nullptr);
		}

		set_window(winrt::get_abi(m_window), m_out_width, m_out_height);
		set_attach(&App::attach_swapchain, this);
	}

	// Called by Dozen on the RSX thread after vkCreateSwapchainKHR made a composition swapchain
	static long __cdecl attach_swapchain(void* opaque, void* swapchain)
	{
		App* app = static_cast<App*>(opaque);
		::IUnknown* unknown = static_cast<::IUnknown*>(swapchain);

		if (app->m_dispatcher.HasThreadAccess())
		{
			return app->attach_on_ui_thread(unknown);
		}

		com_ptr<::IUnknown> keep;
		keep.copy_from(unknown);

		auto promise = std::make_shared<std::promise<HRESULT>>();
		std::future<HRESULT> result = promise->get_future();

		app->m_dispatcher.RunAsync(CoreDispatcherPriority::High, [app, keep, promise]()
		{
			promise->set_value(app->attach_on_ui_thread(keep.get()));
		});

		return result.get();
	}

	HRESULT attach_on_ui_thread(::IUnknown* swapchain)
	{
		try
		{
			const auto interop = m_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();

			com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> abi_surface;
			check_hresult(interop->CreateCompositionSurfaceForSwapChain(swapchain, abi_surface.put()));

			ICompositionSurface surface{ nullptr };
			winrt::copy_from_abi(surface, abi_surface.get());

			CompositionSurfaceBrush brush = m_compositor.CreateSurfaceBrush(surface);
			brush.Stretch(CompositionStretch::Uniform);
			m_sprite.Brush(brush);
			m_swapchain_surface = surface;
			return S_OK;
		}
		catch (const hresult_error& e)
		{
			return e.code();
		}
	}

	// --- Files ----------------------------------------------------------------------------

	// External drives get letters from E: up. The folder must exist and grant
	// "ALL APPLICATION PACKAGES" access (docs/DEPLOYING.md).
	static std::string find_usb_root()
	{
		for (wchar_t letter = L'E'; letter <= L'K'; letter++)
		{
			const std::wstring root = std::wstring(1, letter) + L":\\XBXPS3\\";
			std::error_code ec;

			if (std::filesystem::is_directory(root, ec))
			{
				return std::string(1, static_cast<char>(letter)) + ":/XBXPS3/";
			}
		}

		return {};
	}

	void run_probe(const std::string& local_state)
	{
		const auto limit = winrt::Windows::System::MemoryManager::AppMemoryUsageLimit();

		std::string header = "XBXPS3 hardware probe\r\n";
		header += "Console: ";
		header += console_name();
		header += "\r\nDevice family: " + to_string(winrt::Windows::System::Profile::AnalyticsInfo::VersionInfo().DeviceFamily());
		header += "\r\nApp memory limit: " + std::to_string(limit >> 20) + " MiB" + (limit < (3ull << 30) ? " (running as App: set XBXPS3 to Game in Dev Home)" : " (Game mode)");
		header += "\r\nOutput: " + std::to_string(m_out_width) + "x" + std::to_string(m_out_height) + " @ " + std::to_string(m_refresh) + " Hz";
		header += "\r\nDozen driver: ";
		header += m_vk_gipa ? "loaded" : "NOT loaded";
		header += "\r\n\r\n";

		xbx_core_run_probe((local_state + "xbxps3_probe.txt").c_str(), header.c_str());
	}

	void warn_if_not_game_mode()
	{
		// An "App" gets ~1 GiB and 45% of the GPU; RPCS3 needs the ~5 GiB "Game" budget
		if (winrt::Windows::System::MemoryManager::AppMemoryUsageLimit() < (3ull << 30))
		{
			winrt::Windows::UI::Popups::MessageDialog dialog(
				L"XBXPS3 is running as an App, which only gets about 1 GB of memory.\n\n"
				L"Open Dev Home, select XBXPS3, choose View details, and set App type to Game. Then restart XBXPS3.",
				L"Set XBXPS3 to Game mode");
			dialog.ShowAsync();
		}
	}

	void show_message_and_wait(const wchar_t* text)
	{
		winrt::Windows::UI::Popups::MessageDialog dialog(text, L"XBXPS3");
		auto op = dialog.ShowAsync();

		while (op.Status() == AsyncStatus::Started)
		{
			m_dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	// --- Input ----------------------------------------------------------------------------

	// The Xbox button belongs to the console, so View + Menu together act as the PS button
	void poll_home_menu_chord()
	{
		bool down = false;

		for (const Gamepad& pad : Gamepad::Gamepads())
		{
			const GamepadButtons buttons = pad.GetCurrentReading().Buttons;
			if ((buttons & GamepadButtons::View) == GamepadButtons::View && (buttons & GamepadButtons::Menu) == GamepadButtons::Menu)
			{
				down = true;
				break;
			}
		}

		if (down && !m_chord_down && m_core_started)
		{
			xbx_core_open_home_menu();
		}

		m_chord_down = down;
	}

	// --- Host callbacks for the core --------------------------------------------------------

	static void host_get_output_mode(void* ctx, int* width, int* height, double* refresh)
	{
		const App* app = static_cast<const App*>(ctx);
		*width = app->m_out_width;
		*height = app->m_out_height;
		*refresh = app->m_refresh;
	}

	static void host_request_exit(void* ctx)
	{
		static_cast<App*>(ctx)->m_running = false;
	}

	static void host_set_display_sleep(void* ctx, int enabled)
	{
		App* app = static_cast<App*>(ctx);
		app->m_dispatcher.RunAsync(CoreDispatcherPriority::Normal, [app, enabled]()
		{
			if (!enabled && !app->m_display_request_active)
			{
				app->m_display_request.RequestActive();
				app->m_display_request_active = true;
			}
			else if (enabled && app->m_display_request_active)
			{
				app->m_display_request.RequestRelease();
				app->m_display_request_active = false;
			}
		});
	}

	[[noreturn]] static void host_fatal_error(void* ctx, const char* text)
	{
		App* app = static_cast<App*>(ctx);
		OutputDebugStringA(text);

		const hstring message = to_hstring(text);

		if (app->m_dispatcher.HasThreadAccess())
		{
			app->show_message_and_wait(message.c_str());
			std::_Exit(1);
		}

		std::promise<void> shown;

		app->m_dispatcher.RunAsync(CoreDispatcherPriority::High, [message, &shown]()
		{
			winrt::Windows::UI::Popups::MessageDialog dialog(message, L"XBXPS3 has stopped");
			dialog.ShowAsync().Completed([&shown](auto&&, auto&&) { shown.set_value(); });
		});

		shown.get_future().wait_for(std::chrono::minutes(5));
		std::_Exit(1);
	}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	winrt::init_apartment();
	CoreApplication::Run(make<App>());
	winrt::uninit_apartment();
	return 0;
}
