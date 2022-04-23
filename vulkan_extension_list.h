#ifndef VULKAN_EXTENSION_LIST_H_
#define VULKAN_EXTENSION_LIST_H_

#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

class VulkanExtensionList {
 public:
  // Creates a list of all supported instance-level extensions.
  VulkanExtensionList();

  // Creates a list of all device-level extensions supported by a physical device.
  explicit VulkanExtensionList(vk::PhysicalDevice physical_device);

  VulkanExtensionList(const VulkanExtensionList&) = delete;
  VulkanExtensionList& operator=(const VulkanExtensionList&) = delete;
  ~VulkanExtensionList();

  [[nodiscard]] bool Contains(std::string_view extension_name) const;
  void Print() const;

 private:
  const std::vector<vk::ExtensionProperties> extensions_;
};

#endif  // VULKAN_EXTENSION_LIST_H_
