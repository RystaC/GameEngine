#pragma once

#include "common.h"

namespace vkw {
	class DeviceQueue {
		VkQueue queue_;
		std::uint32_t familyIndex_, queueIndex_;

	public:
		DeviceQueue(std::shared_ptr<Device_> device, std::uint32_t familyIndex, std::uint32_t queueIndex) noexcept :
			familyIndex_(familyIndex), queueIndex_(queueIndex)
		{
			vkGetDeviceQueue(device->get(), familyIndex_, queueIndex_, &queue_);
		}

		const auto& get() const noexcept { return queue_; }
		const auto familyIndex() const noexcept { return familyIndex_; }
		const auto queueIndex() const noexcept { return queueIndex_; }
	};
}
