#pragma once

#include "common.h"

namespace vkw {
	class PhysicalDevice {
		VkPhysicalDevice physicalDevice_;

		struct {
			VkPhysicalDeviceProperties device;
			VkFormatProperties format;
			VkPhysicalDeviceMemoryProperties memory;
		} properties_;

	public:
		PhysicalDevice(VkPhysicalDevice physicalDevice) noexcept;
	};
}