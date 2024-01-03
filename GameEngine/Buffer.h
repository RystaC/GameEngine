#pragma once

#include "common.h"

namespace vkw {
	class Buffer {
		VkBuffer buffer_;
		VkDeviceMemory memory_;
		std::shared_ptr<Device_> device_;

	public:
		Buffer(std::shared_ptr<Device_> device, std::size_t size, VkBufferUsageFlags usage) {
			VkBufferCreateInfo bufferInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			};
		}
	};
}