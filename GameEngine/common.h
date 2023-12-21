#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace vkw {
	inline std::uint32_t size_cast(std::size_t size) { return static_cast<std::uint32_t>(size); }

	class Instance {
		VkInstance instance_;
		VkAllocationCallbacks* allocator_;

	public:
		Instance(VkInstance instance, VkAllocationCallbacks* allocator) noexcept : instance_(instance), allocator_(allocator) {}
		~Instance() noexcept {
			vkDestroyInstance(instance_, allocator_);
		}

		const VkInstance& get() const noexcept { return instance_; }
	};

	class Device {
		VkDevice device_;
		VkAllocationCallbacks* allocator_;

	public:
		Device(VkDevice device, VkAllocationCallbacks* allocator) noexcept : device_(device), allocator_(allocator) {}
		~Device() noexcept {
			vkDestroyDevice(device_, allocator_);
		}

		const VkDevice& get() const noexcept { return device_; }
	};

}