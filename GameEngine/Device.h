#pragma once

#include "common.h"

namespace vkw {
	class Device {
		std::shared_ptr<Device_> device_;
		VkAllocationCallbacks* allocator_;

	public:
		Device(const VkPhysicalDevice& physicalDevice, const std::vector<std::uint32_t>& queueFamilyIndices, const std::vector<std::vector<float>>& queuePriorities, const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
			std::vector<VkDeviceQueueCreateInfo> queueInfos{};
			queueInfos.reserve(queueFamilyIndices.size());

			for (auto i = 0; i < queueFamilyIndices.size(); ++i) {
				queueInfos.emplace_back(
					VkDeviceQueueCreateInfo{
						.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						.queueFamilyIndex = queueFamilyIndices[i],
						.queueCount = size_cast(queuePriorities[i].size()),
						.pQueuePriorities = queuePriorities[i].data(),
					}
				);
			}

			VkDeviceCreateInfo deviceInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount = size_cast(queueInfos.size()),
				.pQueueCreateInfos = queueInfos.data(),
				.enabledLayerCount = size_cast(layers.size()),
				.ppEnabledLayerNames = layers.data(),
				.enabledExtensionCount = size_cast(extensions.size()),
				.ppEnabledExtensionNames = extensions.data(),
			};

			VkDevice device{};

			auto result = vkCreateDevice(physicalDevice, &deviceInfo, allocator_, &device);

			if (result == VK_SUCCESS) device_ = std::make_shared<Device_>(device, allocator_);
			else VK_THROW(result);
		}
	};
}
