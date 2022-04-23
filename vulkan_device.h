#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <cassert>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

class VulkanConfig;
class VulkanPhysicalDevice;
class VulkanPresentationSurface;
class VulkanSurfaceSupport;

class VulkanDevice {
 public:
  // Creates a new logical device connected to the given physical device.
  explicit VulkanDevice(
      const VulkanConfig& vulkan_config, const VulkanSurfaceSupport& surface_support,
      const VulkanPresentationSurface& surface, VulkanPhysicalDevice& physical_device);

  // Moving supported so instances can be returned.
  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice(VulkanDevice &&rhs) noexcept;
  VulkanDevice& operator=(const VulkanDevice&) = delete;
  VulkanDevice& operator=(VulkanDevice &&rhs) noexcept;

  // Blocks until all the currently queued operations on the device complete.
  ~VulkanDevice();

  vk::Device VulkanHandle() const {
    assert(device_);
    return device_.get();
  }

  vk::Queue GraphicsQueue() const {
    assert(device_);
    assert(graphics_queue_);
    return graphics_queue_;
  }
  vk::Queue PresentationQueue() const {
    assert(device_);
    assert(presentation_queue_);
    return presentation_queue_;
  }

 private:
  vk::UniqueDevice device_;
  vk::UniqueSwapchainKHR swap_chain_;
  vk::SurfaceFormatKHR swap_chain_format_;
  // TODO(costan): Add vk::Extent2D swap_chain_extent_;
  vk::Queue graphics_queue_;
  vk::Queue presentation_queue_;
  std::vector<vk::Image> swap_chain_images_;
  std::vector<vk::UniqueImageView> swap_chain_image_views_;
};

#endif  // VULKAN_DEVICE_H_
