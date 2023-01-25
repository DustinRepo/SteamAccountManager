#include "Window.h"
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <thread>
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "util/SteamAccount.h"
#include "util/SteamUtil.h"
#include "util/Util.h"
#include "resource1.h"

#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")

#define DEFAULT_WIDTH 500
#define DEFAULT_HEIGHT 200
#define WINDOW_NAME "Steam Acccount Manager"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool add_account_page = false;
std::string steam_path = "";

bool Window::init_window(HINSTANCE instance) {
	//Set up WNDCLASS
	wnd_class.style = CS_CLASSDC;
	wnd_class.lpfnWndProc = wnd_proc;
	wnd_class.hInstance = instance;
	wnd_class.lpszClassName = WINDOW_NAME " Class";
	wnd_class.hIcon = static_cast<HICON>(LoadImage(instance,
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON,
		128, 128,    // or whatever size icon you want to load
		LR_DEFAULTCOLOR));
	RegisterClassA(&wnd_class);
	window_width = DEFAULT_WIDTH;
	window_height = DEFAULT_HEIGHT;

	//Create window
	hwnd = CreateWindowExA(0, wnd_class.lpszClassName, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, DEFAULT_WIDTH, DEFAULT_HEIGHT, nullptr, nullptr, wnd_class.hInstance, nullptr);
	if (!hwnd || hwnd == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	//Setup DX11
	if (!create_device_d3d(hwnd)) {
		cleanup_device_d3d();
		UnregisterClassA(wnd_class.lpszClassName, wnd_class.hInstance);
		return FALSE;
	}

	//Make the window visible
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	//Init ImGui
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	set_style();
	//disable ini file
	ImGui::GetIO().IniFilename = NULL;

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(d3d11_device, d3d11_device_ctx);

	//Remove rounding from the style
	auto& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	return TRUE;
}

void Window::close_window() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	cleanup_device_d3d();
	DestroyWindow(hwnd);
	UnregisterClassA(wnd_class.lpszClassName, wnd_class.hInstance);
}

void Window::render_window() {
	//Check if window has requested to be closed
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT) {
			request_closed = TRUE;
			return;
		}
	}

	//Set new frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Set ImGui window size to window size and pos to 0 since we use just a single window for the application
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ window_width, window_height });

	ImGui::Begin("SAM", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	if (add_account_page)
		render_add_account();
	else
		render_main();

	ImGui::End();

	//Finish frame
	ImGui::Render();
	constexpr float color[4]{ 1.f, 1.f, 1.f, 1.f };
	d3d11_device_ctx->OMSetRenderTargets(1, &render_target_view, nullptr);
	d3d11_device_ctx->ClearRenderTargetView(render_target_view, color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	swap_chain->Present(1, 0);//VSync on
	//swap_chain->Present(0, 0);//VSync off
}

void center_text(const char* text) {
	float win_width = ImGui::GetWindowSize().x;
	float text_width = ImGui::CalcTextSize(text).x;

	// calculate the indentation that centers the text on one line, relative
	// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
	float text_indentation = (win_width - text_width) * 0.5f;

	// if text is too long to be drawn on one line, `text_indentation` can
	// become too small or even negative, so we check a minimum indentation
	float min_indentation = 20.0f;
	if (text_indentation <= min_indentation) {
		text_indentation = min_indentation;
	}

	ImGui::SameLine(text_indentation);
	ImGui::PushTextWrapPos(win_width - text_indentation);
	ImGui::TextWrapped(text);
	ImGui::PopTextWrapPos();
}

void Window::render_main() {
	static int current_account = -1;
	//Make sure we have the steam path
	if (strlen(steam_path.c_str()) == 0) {
		ImGui::Text("Steam exe not found. Please launch Steam atleast once.");
		int steam_pid = SteamUtil::find_steam_pid();
		if (steam_pid != -1) {
			steam_path = SteamUtil::get_steam_exe_path(steam_pid);
		}
	}
	//Set up columns to make the elements show next to each other
	ImGui::Columns(2);
	//Listbox on the left that contains the accounts
	ImGui::ListBox("##", &current_account, [](void* data, int idx, const char** out_text) {
		*out_text = c_steam_account_manager.steam_accounts.at(idx).username.c_str();
		return true;
	}, nullptr, c_steam_account_manager.steam_accounts.size(), 6);
	if (current_account >= c_steam_account_manager.steam_accounts.size())
		current_account = -1;
	//Change column and create buttons for managing accounts
	ImGui::NextColumn();
	if (ImGui::Button("Export List")) {
		std::string file_path = Util::open_file_dialog();
		if (strlen(file_path.c_str()) > 0) {
			c_steam_account_manager.export_accounts(file_path);
		}
	}
	if (ImGui::Button("Import List")) {
		std::string file_path = Util::open_file_dialog();
		if (strlen(file_path.c_str()) > 0) {
			c_steam_account_manager.import_accounts(file_path);
			c_steam_account_manager.save_accounts();
		}
	}
	if (ImGui::Button("Add Account")) {
		add_account_page = true;
	}
	if (current_account != -1) {
		if (ImGui::Button("Remove Account")) {
			c_steam_account_manager.delete_account(current_account);
			c_steam_account_manager.save_accounts();
			current_account = 0;
		}
	}
	//Set columns back to 1 to reset then make the large login button
	ImGui::Columns(1);
	if (ImGui::Button("Login", { window_width - 16, 30 })) {
		CSteamAccount c_steam_account = c_steam_account_manager.steam_accounts.at(current_account);
		//Kill steam if open
		int steam_pid = SteamUtil::find_steam_pid();
		if (steam_pid != -1) {
			SteamUtil::kill_steam_process(steam_pid);
			while (steam_pid = SteamUtil::find_steam_pid() != -1) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		//Open steam with the -login command line
		SteamUtil::start_steam(steam_path.c_str(), c_steam_account);
	}
}

void Window::render_add_account() {
	if (ImGui::Button("Back")) {
		add_account_page = false;
	}
	center_text("Add Account");
	//using a dummy is required if doing center_text after a center_text
	//because of the weird setup that is done with it's spacing
	ImGui::Dummy(ImVec2{ 0, 15 });
	static char username[256]{};
	static char password[512]{};
	center_text("Username");
	ImGui::PushItemWidth(window_width - 16);
	ImGui::InputText("##Username", username, IM_ARRAYSIZE(username));
	ImGui::Dummy(ImVec2{ 0, 15 });
	center_text("Password");
	ImGui::InputText("##Password", password, IM_ARRAYSIZE(password));
	ImGui::PopItemWidth();

	if (ImGui::Button("Add Account", { window_width - 16 * 2, 30 })) {
		//XOR password, Add account to list, save, then clear the username and password field
		CSteamAccount c_steam_account(username, Util::xor_string(password, XOR_KEY));
		c_steam_account_manager.add_account(c_steam_account);
		c_steam_account_manager.save_accounts();
		add_account_page = false;
		ZeroMemory(username, 256);
		ZeroMemory(password, 512);
	}
}

bool Window::create_device_d3d(HWND hwnd) {
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferDesc.Width = 0;
	swap_chain_desc.BufferDesc.Height = 0;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 30;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = hwnd;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT create_device_flags = 0;
	D3D_FEATURE_LEVEL feature_level;
	const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &d3d11_device, &feature_level, &d3d11_device_ctx);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &d3d11_device, &feature_level, &d3d11_device_ctx);
	if (res != S_OK)
		return false;

	create_render_target();
	return true;
}

void Window::cleanup_device_d3d() {
	cleanup_render_target();
	if (swap_chain) {
		swap_chain->Release();
		swap_chain = NULL;
	}
	if (d3d11_device_ctx) {
		d3d11_device_ctx->Release();
		d3d11_device_ctx = NULL;
	}
	if (d3d11_device) {
		d3d11_device->Release();
		d3d11_device = NULL;
	}
}

void Window::create_render_target() {
	ID3D11Texture2D* back_buffer;
	swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	if (!back_buffer)
		return;
	d3d11_device->CreateRenderTargetView(back_buffer, NULL, &render_target_view);
	back_buffer->Release();
}

void Window::cleanup_render_target() {
	if (render_target_view) {
		render_target_view->Release();
		render_target_view = NULL;
	}
}

LRESULT CALLBACK Window::wnd_proc(HWND hWindow, UINT nMessage, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWindow, nMessage, wParam, lParam))
		return true;

	switch (nMessage) {
	case WM_SIZE:// Handle resize
		if (d3d11_device != NULL && wParam != SIZE_MINIMIZED) {
			cleanup_render_target();
			swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			window_width = (UINT)LOWORD(lParam);
			window_height = (UINT)HIWORD(lParam);
			create_render_target();
		}
		return 0;
	case WM_SYSCOMMAND:// Disable ALT application menu
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:// Close
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWindow, nMessage, wParam, lParam);
}

void Window::set_style() {
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.37f, 0.14f, 0.14f, 0.67f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.39f, 0.20f, 0.20f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.56f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.17f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.33f, 0.35f, 0.36f, 0.53f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.76f, 0.28f, 0.44f, 0.67f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
	colors[ImGuiCol_Separator] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.07f, 0.07f, 0.51f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.86f, 0.23f, 0.43f, 0.67f);
	colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 0.57f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.13f, 0.13f, 0.74f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}