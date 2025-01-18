#include "window/window.hpp"
#include "game/game.h"

#include <iostream>
#include <thread>
#include <Windows.h>

int main() {
	//ShowWindow(GetConsoleWindow(), SW_HIDE);

	std::cout << "[>>] Starting Setup\n";
	Overlay overlay;
	overlay.SetupOverlay("AC Overlay");
	AssaultCube::Setup();

	while (overlay.shouldRun) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		
		HWND gameWindow = FindWindow(nullptr, "AssaultCube");
		if (GetForegroundWindow() == gameWindow) {
			overlay.StartRender();
			AssaultCube::RunESP(gameWindow);
			overlay.EndRender();
		} else {
			overlay.StartRender();
			overlay.EndRender();
		}
	}
}