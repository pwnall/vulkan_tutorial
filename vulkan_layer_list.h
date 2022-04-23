#ifndef VULKAN_LAYER_LIST_H_
#define VULKAN_LAYER_LIST_H_

#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

class VulkanLayerList {
 public:
  // Creates a list of all supported instance-level layers.
  VulkanLayerList();

  // Creates a list of all device-level layers supported by a physical device.
  //
  // Vulkan 1.3 removes device-specific layers. When run under a conforming
  // implementation, this constructor will return the layers enabled for the
  // instance that produced the VkPhysicalDevice.
  explicit VulkanLayerList(vk::PhysicalDevice physical_device);

  VulkanLayerList(const VulkanLayerList&) = delete;
  VulkanLayerList& operator=(const VulkanLayerList&) = delete;
  ~VulkanLayerList();

  [[nodiscard]] bool Contains(std::string_view layer_name) const;
  void Print() const;

 private:
  const std::vector<vk::LayerProperties> layers_;
};

#endif  // VULKAN_LAYER_LIST_H_
