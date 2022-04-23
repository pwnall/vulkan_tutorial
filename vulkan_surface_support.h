#ifndef VULKAN_SURFACE_SUPPORT_H_
#define VULKAN_SURFACE_SUPPORT_H_

#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

class VulkanPhysicalDevice;

// Information about a physical device's usability on a surface.
//
// This instance can be discarded after a VulkanDevice is created.
class VulkanSurfaceSupport {
 public:
  struct Queues {
    uint32_t graphics_queue_family_index;
    uint32_t presentation_queue_family_index;
  };

  explicit VulkanSurfaceSupport(const VulkanPhysicalDevice& physical_device, vk::SurfaceKHR surface);

  VulkanSurfaceSupport(const VulkanSurfaceSupport&) = delete;
  VulkanSurfaceSupport& operator=(const VulkanSurfaceSupport&) = delete;
  ~VulkanSurfaceSupport();

  [[nodiscard]] bool IsAcceptable() const;

  // Must only be called if IsAcceptable() returns true.
  [[nodiscard]] vk::SurfaceTransformFlagBitsKHR CurrentTransform() const;
  [[nodiscard]] vk::SurfaceFormatKHR BestFormat() const;
  [[nodiscard]] vk::PresentModeKHR BestMode() const;
  [[nodiscard]] vk::Extent2D BestExtentFor(vk::Extent2D surface_size) const;
  [[nodiscard]] int BestImageCount() const;
  [[nodiscard]] Queues QueueFamilyIndexes() const;

#if !defined(NDEBUG)
  vk::PhysicalDevice PhysicalDeviceVulkanHandle() const {
    assert(physical_device_handle_);
    return physical_device_handle_;
  }

  vk::SurfaceKHR SurfaceVulkanHandle() const {
    assert(surface_handle_);
    return surface_handle_;
  }
#endif  // !defined(NDEBUG)

 private:
  const vk::SurfaceCapabilitiesKHR capabilities_;
  const std::vector<vk::SurfaceFormatKHR> formats_;
  const std::vector<vk::PresentModeKHR> modes_;

#if !defined(NDEBUG)
  const vk::PhysicalDevice physical_device_handle_;
  const vk::SurfaceKHR surface_handle_;
#endif  // !defined(NDEBUG)

  const std::set<uint32_t> graphics_queue_family_indexes_;

  // The list of the device's queue families that can present on the surface.
  const std::set<uint32_t> presentation_queue_family_indexes_;
};

#endif  // VULKAN_SURFACE_SUPPORT_H_
