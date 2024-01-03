#pragma once

#include "common.h"

namespace vkw {
	class PhysicalDevice {
		VkPhysicalDevice physicalDevice_;

	public:
		const auto& get() const noexcept { return physicalDevice_; }
	};
}