#include "vulkan_layer_list.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan_errors.h"

namespace {

[[nodiscard]] std::vector<vk::LayerProperties> ListVulkanInstanceLayers() {
  vk::ResultValue<std::vector<vk::LayerProperties>> enumerate_result =
      vk::enumerateInstanceLayerProperties();
  VulkanCheckResult("vkEnumerateInstanceLayerProperties", enumerate_result.result);

  return std::move(enumerate_result.value);
}

[[nodiscard]] std::vector<vk::LayerProperties> ListVulkanDeviceLayers(
    vk::PhysicalDevice physical_device) {
  vk::ResultValue<std::vector<vk::LayerProperties>> enumerate_result =
      physical_device.enumerateDeviceLayerProperties();
  VulkanCheckResult("vkEnumerateDeviceLayerProperties", enumerate_result.result);

  return std::move(enumerate_result.value);
}

}  // namespace

VulkanLayerList::VulkanLayerList() : layers_(ListVulkanInstanceLayers()) {}

VulkanLayerList::VulkanLayerList(vk::PhysicalDevice physical_device)
    : layers_(ListVulkanDeviceLayers(physical_device)) {}

VulkanLayerList::~VulkanLayerList() = default;

[[nodiscard]] bool VulkanLayerList::Contains(std::string_view layer_name) const {
  return std::any_of(
      layers_.begin(), layers_.end(), [&layer_name](const vk::LayerProperties& layer_properties) {
        return layer_name.compare(std::string_view(layer_properties.layerName)) == 0;
      });
}

void VulkanLayerList::Print() const {
  std::cout << layers_.size() << " supported layers:\n";
  for (const auto& layer : layers_)
    std::cout << "  " << layer.layerName << " version: " << layer.specVersion << "\n";
  std::cout << "\n";
}
