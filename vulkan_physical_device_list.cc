#include "vulkan_physical_device_list.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan_config.h"
#include "vulkan_device.h"
#include "vulkan_errors.h"
#include "vulkan_presentation_context.h"
#include "vulkan_surface_support.h"

namespace {

[[nodiscard]] std::vector<VulkanPhysicalDevice> CreateVulkanPhysicalDevices(vk::Instance instance) {
  assert(instance);

  vk::ResultValue<std::vector<vk::PhysicalDevice>> enumerate_result =
      instance.enumeratePhysicalDevices();
  VulkanCheckResult("vkEnumeratePhysicalDevices", enumerate_result.result);

  std::vector<VulkanPhysicalDevice> devices;
  devices.reserve(enumerate_result.value.size());

  for (vk::PhysicalDevice device_handle : enumerate_result.value)
    devices.emplace_back(device_handle);
  return devices;
}

}  // namespace

VulkanPhysicalDeviceList::VulkanPhysicalDeviceList(vk::Instance instance) :
  devices_(CreateVulkanPhysicalDevices(instance)) {}

VulkanPhysicalDeviceList::~VulkanPhysicalDeviceList() = default;

void VulkanPhysicalDeviceList::Print() const {
  std::cout << devices_.size() << " physical devices:\n";
  for (const VulkanPhysicalDevice& device : devices_)
    device.Print();
  std::cout << "\n";
}


VulkanDevice VulkanPhysicalDeviceList::CreateLogicalDevice(
    const VulkanConfig& vulkan_config, const VulkanPresentationSurface& surface) {
  const std::vector<const char*>& required_layers = vulkan_config.RequiredLayers();
  const std::vector<const char*>& required_extensions = vulkan_config.RequiredDeviceExtensions();

  for (VulkanPhysicalDevice& physical_device : devices_) {
    if (!physical_device.HasRequiredFeatures())
      continue;
    if (!physical_device.HasLayers(required_layers))
      continue;
    if (!physical_device.HasExtensions(required_extensions))
      continue;
    if (physical_device.GraphicsQueueFamilyIndices().empty())
      continue;

    VulkanSurfaceSupport surface_support(physical_device, surface.VulkanHandle());
    if (!surface_support.IsAcceptable())
      continue;

    return VulkanDevice(vulkan_config, surface_support, surface, physical_device);
  }

  std::cerr << "No suitable Vulkan device attached" << std::endl;
  std::abort();
}
