#include "vulkan_physical_device.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "vulkan_config.h"
#include "vulkan_extension_list.h"
#include "vulkan_layer_list.h"

namespace {

[[nodiscard]] std::set<uint32_t> GetGraphicsQueueFamilyIndexes(
    const std::vector<vk::QueueFamilyProperties>& queue_families) {
  std::set<uint32_t> graphics_queue_family_indexes;
  for (size_t queue_family_index = 0; queue_family_index < queue_families.size();
       ++queue_family_index) {
    if (queue_families[queue_family_index].queueFlags & vk::QueueFlagBits::eGraphics)
      graphics_queue_family_indexes.insert(queue_family_index);
  }
  return graphics_queue_family_indexes;
}

}  // namespace

VulkanPhysicalDevice::VulkanPhysicalDevice(vk::PhysicalDevice physical_device_handle)
    : physical_device_(physical_device_handle),
      properties_(physical_device_.getProperties()),
      features_(physical_device_.getFeatures()),
      memory_properties_(physical_device_.getMemoryProperties()),
      queue_families_(physical_device_.getQueueFamilyProperties()),
      graphics_queue_family_indices_(GetGraphicsQueueFamilyIndexes(queue_families_)) {
  assert(physical_device_handle);
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&&) noexcept = default;
VulkanPhysicalDevice& VulkanPhysicalDevice::operator=(VulkanPhysicalDevice&&) noexcept = default;

VulkanPhysicalDevice::~VulkanPhysicalDevice() = default;

void VulkanPhysicalDevice::Print() const {
  std::cout << "  " << properties_.deviceName  << " id: " << properties_.deviceID
            << " type: " << vk::to_string(properties_.deviceType) << " API: "
            << properties_.apiVersion << "\n";
}

bool VulkanPhysicalDevice::HasRequiredFeatures() const {
  return features_.tessellationShader == VK_TRUE;
}

bool VulkanPhysicalDevice::HasLayers(const std::vector<const char*>& layer_names) const {
  VulkanLayerList device_layers(physical_device_);

  for (const char* layer_name : layer_names) {
    if (!device_layers.Contains(layer_name))
      return false;
  }
  return true;
}

bool VulkanPhysicalDevice::HasExtension(std::string_view extension_name) const {
  // TODO(costan): Cache VulkanExtensionList.
  VulkanExtensionList device_extensions(physical_device_);

  return device_extensions.Contains(extension_name);
}

bool VulkanPhysicalDevice::HasExtensions(const std::vector<const char*>& extension_names) const {
  VulkanExtensionList device_extensions(physical_device_);

  for (const char* extension_name : extension_names) {
    if (!device_extensions.Contains(extension_name))
      return false;
  }
  return true;
}
