#pragma once

#include "common.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "DeviceQueue.h"

namespace vkw {
	struct SwapchainFormat {
		VkColorSpaceKHR colorSpace;
		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;
	};

	class Swapchain {
		VkSwapchainKHR swapchain_;
		std::shared_ptr<Device_> device_;

	public:
		Swapchain(std::shared_ptr<Device_> device, const PhysicalDevice& physicalDevice, const DeviceQueue& queue, const Surface& surface, const SwapchainFormat& format, const VkExtent2D size) : device_(device) {
			VkBool32 isSupported{};
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice.get(), queue.familyIndex(), surface.get(), &isSupported);
			if (!isSupported) VK_THROW(0);

			std::uint32_t formatsCount{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.get(), surface.get(), &formatsCount, nullptr);
			std::vector<VkSurfaceFormatKHR> formats(formatsCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.get(), surface.get(), &formatsCount, formats.data());

			std::uint32_t modesCount{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.get(), surface.get(), &modesCount, nullptr);
			std::vector<VkPresentModeKHR> modes{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.get(), surface.get(), &modesCount, modes.data());

			VkSwapchainCreateInfoKHR swapchainInfo{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = surface.get(),
				.minImageCount = 2,
				.imageFormat = format.surfaceFormat.format,
				.imageColorSpace = format.colorSpace,
				.imageExtent = size,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = queue.familyIndex(),
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = format.presentMode,
				.clipped = VK_TRUE,
			}
		}

		~Swapchain() noexcept { vkDestroySwapchainKHR(device_->get(), swapchain_, nullptr); }

		const auto& get() const noexcept { return swapchain_; }

		auto acquireImages() const {
			std::uint32_t count{};
			VK_CHECK(vkGetSwapchainImagesKHR(device_->get(), swapchain_, &count, nullptr));
			std::vector<VkImage> images(count);
			VK_CHECK(vkGetSwapchainImagesKHR(device_->get(), swapchain_, &count, images.data()));

			return images;
		}
	};
}