#include "vulkan_extension_list.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan_errors.h"

namespace {

[[nodiscard]] std::vector<vk::ExtensionProperties> ListVulkanInstanceExtensions() {
  vk::ResultValue<std::vector<vk::ExtensionProperties>> enumerate_result =
      vk::enumerateInstanceExtensionProperties();
  VulkanCheckResult("vkEnumerateInstanceExtensionProperties", enumerate_result.result);

  return std::move(enumerate_result.value);
}

[[nodiscard]] std::vector<vk::ExtensionProperties> ListVulkanDeviceExtensions(
    vk::PhysicalDevice physical_device) {
  assert(physical_device);

  vk::ResultValue<std::vector<vk::ExtensionProperties>> enumerate_result =
      physical_device.enumerateDeviceExtensionProperties();
  VulkanCheckResult("vkEnumerateDeviceExtensionProperties", enumerate_result.result);

  return std::move(enumerate_result.value);
}

}  // namespace

VulkanExtensionList::VulkanExtensionList() : extensions_(ListVulkanInstanceExtensions()) {}

VulkanExtensionList::VulkanExtensionList(vk::PhysicalDevice physical_device)
    : extensions_(ListVulkanDeviceExtensions(physical_device)) {}

VulkanExtensionList::~VulkanExtensionList() = default;

[[nodiscard]] bool VulkanExtensionList::Contains(std::string_view extension_name) const {
  return std::any_of(extensions_.begin(), extensions_.end(),
                     [extension_name](const vk::ExtensionProperties& extension) {
    return extension_name.compare(std::string_view(extension.extensionName)) == 0;
  });
}

void VulkanExtensionList::Print() const {
  std::cout << extensions_.size() << " supported extensions:\n";
  for (const auto& extension : extensions_)
    std::cout << "  " << extension.extensionName << " version: " << extension.specVersion << "\n";
  std::cout << "\n";
}
