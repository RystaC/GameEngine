#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#define VK_THROW(error) throw std::runtime_error(std::format("an error occurs in function {}, erroe code is {}.", __func__, error));
#define VK_CHECK(x) if((x) != VK_SUCCESS) { VK_THROW(x); }

namespace vkw {

	inline std::uint32_t size_cast(std::size_t size) { return static_cast<std::uint32_t>(size); }

	// object wrappers for RAII
	template<typename T, void Destructor(T, const VkAllocationCallbacks*)>
	class Factory {
		T object_;
		VkAllocationCallbacks* allocator_;

	public:
		using type = T;

		Factory(T object, const VkAllocationCallbacks* allocator) noexcept : object_(object), allocator_(allocator) {}
		~Factory() noexcept { Destructor(object_, allocator_); }

		const auto& get() const noexcept { return object_; }
	};

	using Instance_ = Factory<VkInstance, vkDestroyInstance>;
	using Device_ = Factory<VkDevice, vkDestroyDevice>;

	template<typename Factory, typename T, void Destructor(Factory::type, T, const VkAllocationCallbacks*)>
	class Object {
		std::shared_ptr<Factory> factory_;
		T object_;
		VkAllocationCallbacks* allocator_;

	public:
		using type = T;

		Object(std::shared_ptr<Factory> factory, T object, const VkAllocationCallbacks* allocator) noexcept : factory_(factory), object_(object), allocator_(allocator) {}
		~Object() noexcept { Destructor(factory_->get(), object_, allocator_); }

		const auto& get() const noexcept { return object_; }
	};
}