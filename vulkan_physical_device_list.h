#ifndef VULKAN_PHYSICAL_DEVICE_LIST_H_
#define VULKAN_PHYSICAL_DEVICE_LIST_H_

#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan_device.h"
#include "vulkan_physical_device.h"

class VulkanConfig;
class VulkanPresentationSurface;

class VulkanPhysicalDeviceList {
 public:
  explicit VulkanPhysicalDeviceList(vk::Instance instance);
  VulkanPhysicalDeviceList(const VulkanPhysicalDeviceList&) = delete;
  VulkanPhysicalDeviceList& operator=(const VulkanPhysicalDeviceList&) = delete;
  ~VulkanPhysicalDeviceList();

  void Print() const;

  // Finds a suitable physical device and creates a logical device on it.
  VulkanDevice CreateLogicalDevice(const VulkanConfig& vulkan_config,
                                   const VulkanPresentationSurface& surface);

 private:
  std::vector<VulkanPhysicalDevice> devices_;
};

#endif // VULKAN_PHYSICAL_DEVICE_LIST_H_
