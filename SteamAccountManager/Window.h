#pragma once
#include <Windows.h>
#include <d3d11.h>

namespace Window {
	inline WNDCLASS wnd_class{};
	inline ID3D11Device* d3d11_device = nullptr;
	inline ID3D11DeviceContext* d3d11_device_ctx = nullptr;
	inline IDXGISwapChain* swap_chain = nullptr;
	inline ID3D11RenderTargetView* render_target_view = nullptr;
	inline HWND hwnd;

	inline bool request_closed;
	inline float window_width, window_height;

	inline LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
	bool init_window(HINSTANCE instance);
	void render_window();
	void render_main();
	void render_add_account();
	void close_window();
	void cleanup_render_target();
	void create_render_target();
	void cleanup_device_d3d();
	bool create_device_d3d(HWND hwnd);
	void set_style();
}