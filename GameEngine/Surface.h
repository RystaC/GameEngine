#pragma once

#include "common.h"

namespace vkw {
	class Surface {
		VkSurfaceKHR surface_;
		std::shared_ptr<Instance_> instance_;

	public:
		Surface(std::shared_ptr<Instance_> instance, SDL_Window& window) : instance_(instance) {
			VK_CHECK(SDL_Vulkan_CreateSurface(&window, instance->get(), &surface_));
		}

		~Surface() noexcept { vkDestroySurfaceKHR(instance_->get(), surface_, nullptr); }

		const auto& get() const noexcept { return surface_; }
	};
}