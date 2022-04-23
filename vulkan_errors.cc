#include "vulkan_errors.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

void VulkanCheckResult(std::string_view method_name, vk::Result result) {
  if (result == vk::Result::eSuccess)
    return;

  std::cerr << method_name << "() failed: " << vk::to_string(result);
  std::abort();
}
