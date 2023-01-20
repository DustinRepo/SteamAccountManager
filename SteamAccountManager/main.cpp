#include <Windows.h>
#include <thread>//sleep
#include <conio.h>//_getch()

#include "util/SteamAccount.h"
#include "util/SteamUtil.h"
#include "Window.h"

#define WAITFORUSER printf_s("Press a key..."); _getch()

//Decided to remake an old project I had that no longer worked after the recent
//Steam login window change. I also decided to try 'snake case' rather than 'hungarian notation'
//for naming variables, functions, etc so if there is a variable or function in hungarian notation, that's why

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE previous, PSTR args, INT show_window) {
	c_steam_account_manager.load_accounts();

	if (!Window::init_window(instance)) {
		printf_s("ERORR: Could not init window!");
		return 0;
	}

	while (!Window::request_closed) {
		Window::render_window();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	Window::close_window();
	return 1;
}