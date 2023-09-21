#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <vector>

#include "GraphicsEngine.h"

int main(int argc, char* argv[]) {
	SDL_version version{};
	SDL_VERSION(&version);

	std::cout << (int)version.major << "." << (int)version.minor << "." << (int)version.patch << std::endl;

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

	return 0;
}