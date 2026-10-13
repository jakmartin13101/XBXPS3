#pragma once

#include "Emu/RSX/GSFrameBase.h"

// The game "window" on Xbox is the app's CoreWindow, owned by the shell. Dozen's UWP
// WSI builds a composition swapchain and hands it to the shell through
// mesa_uwp_set_swapchain_attach_callback, so the HWND below is only a token that
// keeps RPCS3's Win32 surface path happy. Dozen ignores it under _XBOX_UWP.
class xbx_gs_frame final : public GSFrameBase
{
public:
	xbx_gs_frame() = default;
	~xbx_gs_frame() override = default;

	void close() override {}
	void reset() override {}
	bool shown() override { return true; }
	void hide() override {}
	void show() override {}
	void toggle_fullscreen() override {} // Always full screen

	// OpenGL contexts do not exist on Xbox
	void delete_context(draw_context_t) override {}
	draw_context_t make_context() override { return nullptr; }
	void set_current(draw_context_t) override {}

	void flip(draw_context_t, bool) override {}

	int client_width() override;
	int client_height() override;
	f64 client_display_rate() override;
	bool has_alpha() override { return false; }

	display_handle_t handle() const override;

	bool can_consume_frame() const override { return false; }
	void present_frame(std::vector<u8>&&, u32, u32, u32, bool) const override {}
	void take_screenshot(std::vector<u8>&& sshot_data, u32 sshot_width, u32 sshot_height, bool is_bgra) override;

	void update_title(double) override {}
};
