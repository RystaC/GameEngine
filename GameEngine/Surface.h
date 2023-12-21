#pragma once

#include "common.h"

namespace vkw {
	class Surface {
		VkSurfaceKHR surface_;
		std::shared_ptr<Instance> instance_;
		VkAllocationCallbacks* allocator_;

	public:
		Surface(VkSurfaceKHR surface, std::shared_ptr<Instance> instance, VkAllocationCallbacks* allocator = nullptr) noexcept : surface_(surface), instance_(instance), allocator_(allocator) {}
		~Surface() noexcept {
			vkDestroySurfaceKHR(instance_->get(), surface_, allocator_);
		}
	};
}