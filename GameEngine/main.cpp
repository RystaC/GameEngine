#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <shaderc/shaderc.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "GraphicsEngine.h"
#include "GLSLCompiler.h"
#include "PMXLoader.h"

// fuck Windows
#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
	UINT cp = GetConsoleOutputCP();
	SetConsoleOutputCP(65001);
#endif

	//PMXLoader loader{};
	//loader.load("miku.pmx");
	//return 0;

	GLSLCompiler compiler{};

	compiler.compile("basic.vert.glsl");
	compiler.compile("basic.frag.glsl");

	constexpr std::int32_t windowWidth = 640;
	constexpr std::int32_t windowHeight = 480;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		SDL_Log("failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	auto window = SDL_CreateWindow("Game Engine", 100, 100, windowWidth, windowHeight, SDL_WINDOW_VULKAN);

	if (!window) {
		SDL_Log("failed to create window: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	auto graphicsEngine = GraphicsEngine();
	graphicsEngine.initialize(window, "Game Engine", VK_MAKE_API_VERSION(0, 0, 1, 0), { windowWidth, windowHeight });

	bool isRunning = true;

	while (isRunning) {
		SDL_Event event{};

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				isRunning = false;
				break;
			default:
				break;
			}
		}

		const std::uint8_t* keyState = SDL_GetKeyboardState(nullptr);
		if (keyState[SDL_SCANCODE_ESCAPE]) {
			isRunning = false;
		}

		graphicsEngine.draw();
	}

	SDL_Quit();

#ifdef _WIN32
	SetConsoleOutputCP(cp);
#endif

	return 0;
}