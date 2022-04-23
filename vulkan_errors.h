#ifndef VULKAN_ERRORS_H_
#define VULKAN_ERRORS_H_

#include <string_view>

#include <vulkan/vulkan.hpp>

// Terminates the program if `result` is not eSuccess.
//
// `method_name` is included in a diagnostic error if the program is terminated.
// For best results, it should be a Vulkan function or a vulkan.hpp method.
void VulkanCheckResult(std::string_view method_name, vk::Result result);

#endif  // VULKAN_ERRORS_H_

