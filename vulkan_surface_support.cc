#include "vulkan_surface_support.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "vulkan_errors.h"
#include "vulkan_physical_device.h"

namespace {

[[nodiscard]] vk::SurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilities(
    vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
  assert(physical_device);
  assert(surface);

  vk::ResultValue<vk::SurfaceCapabilitiesKHR> capabilities =
      physical_device.getSurfaceCapabilitiesKHR(surface);
  VulkanCheckResult("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", capabilities.result);
  return capabilities.value;
}

[[nodiscard]] std::vector<vk::SurfaceFormatKHR> GetPhysicalDeviceSurfaceFormats(
    vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
  assert(physical_device);
  assert(surface);

  vk::ResultValue<std::vector<vk::SurfaceFormatKHR>> formats =
      physical_device.getSurfaceFormatsKHR(surface);
  VulkanCheckResult("vkGetPhysicalDeviceSurfaceFormatsKHR", formats.result);
  return std::move(formats.value);
}

[[nodiscard]] std::vector<vk::PresentModeKHR> GetPhysicalDeviceSurfacePresentModes(
    const vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
  assert(surface);

  vk::ResultValue<std::vector<vk::PresentModeKHR>> modes =
      physical_device.getSurfacePresentModesKHR(surface);
  VulkanCheckResult("vkGetPhysicalDeviceSurfacePresentModesKHR", modes.result);
  return std::move(modes.value);
}

std::set<uint32_t> GetPresentationQueueFamilyIndexes(
    const VulkanPhysicalDevice& physical_device, vk::SurfaceKHR surface) {
  std::set<uint32_t> presentation_queue_family_indexes;

  vk::PhysicalDevice physical_device_handle = physical_device.VulkanHandle();
  size_t queue_family_count = physical_device.QueueFamilyCount();
  for (size_t queue_family_index = 0; queue_family_index < queue_family_count;
       ++queue_family_index) {
    vk::ResultValue<vk::Bool32> can_present_on_surface =
        physical_device_handle.getSurfaceSupportKHR(
            static_cast<uint32_t>(queue_family_index), surface);
    VulkanCheckResult("vkGetPhysicalDeviceSurfaceSupportKHR", can_present_on_surface.result);
    if (can_present_on_surface.value)
      presentation_queue_family_indexes.insert(queue_family_index);
  }
  return presentation_queue_family_indexes;
}

}  // namespace

VulkanSurfaceSupport::VulkanSurfaceSupport(const VulkanPhysicalDevice& physical_device,
                                           vk::SurfaceKHR surface)
    : capabilities_(GetPhysicalDeviceSurfaceCapabilities(physical_device.VulkanHandle(), surface)),
      formats_(GetPhysicalDeviceSurfaceFormats(physical_device.VulkanHandle(), surface)),
      modes_(GetPhysicalDeviceSurfacePresentModes(physical_device.VulkanHandle(), surface)),
#if !defined(NDEBUG)
      physical_device_handle_(physical_device.VulkanHandle()),
      surface_handle_(surface),
#endif  // !defined(NDEBUG)
      graphics_queue_family_indexes_(physical_device.GraphicsQueueFamilyIndices()),
      presentation_queue_family_indexes_(
          GetPresentationQueueFamilyIndexes(physical_device, surface)) {
  assert(physical_device.VulkanHandle());
  assert(surface);
  assert(!physical_device.GraphicsQueueFamilyIndices().empty());
}

VulkanSurfaceSupport::~VulkanSurfaceSupport() = default;

bool VulkanSurfaceSupport::IsAcceptable() const {
  return !formats_.empty() && !modes_.empty() && !graphics_queue_family_indexes_.empty() &&
         !presentation_queue_family_indexes_.empty();
}

vk::SurfaceTransformFlagBitsKHR VulkanSurfaceSupport::CurrentTransform() const {
  assert(IsAcceptable());
  return capabilities_.currentTransform;
}

vk::SurfaceFormatKHR VulkanSurfaceSupport::BestFormat() const {
  assert(IsAcceptable());
  assert(!formats_.empty());

  auto it = std::find_if(formats_.begin(), formats_.end(), [](vk::SurfaceFormatKHR format) {
    if (format.format != vk::Format::eB8G8R8A8Srgb)
      return false;
    if (format.colorSpace != vk::ColorSpaceKHR::eSrgbNonlinear)
      return false;
    return true;
  });
  if (it != formats_.end())
    return *it;

  return formats_[0];
}

vk::PresentModeKHR VulkanSurfaceSupport::BestMode() const {
  assert(IsAcceptable());
  assert(!modes_.empty());

  auto it = std::find_if(modes_.begin(), modes_.end(), [](vk::PresentModeKHR mode) {
    return mode == vk::PresentModeKHR::eMailbox;
  });
  if (it != modes_.end())
    return *it;

  // The Vulkan spec requires VK_PRESENT_MODE_FIFO_KHR support.
  assert(std::count(modes_.begin(), modes_.end(), vk::PresentModeKHR::eFifo) == 1);

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSurfaceSupport::BestExtentFor(vk::Extent2D surface_size) const {
  assert(IsAcceptable());

  uint32_t extent_width = std::clamp(surface_size.width, capabilities_.minImageExtent.width,
                                     capabilities_.maxImageExtent.width);
  uint32_t extent_height = std::clamp(surface_size.height, capabilities_.minImageExtent.height,
                                      capabilities_.maxImageExtent.height);
  return vk::Extent2D(extent_width, extent_height);
}

int VulkanSurfaceSupport::BestImageCount() const {
  assert(IsAcceptable());

  // One slack image reduces the risk of being blocked on driver ops.
  uint32_t image_count = capabilities_.minImageCount + 1;

  if (image_count < capabilities_.maxImageCount && capabilities_.maxImageCount != 0)
    image_count = capabilities_.maxImageCount;

  return static_cast<int>(image_count);
}

VulkanSurfaceSupport::Queues VulkanSurfaceSupport::QueueFamilyIndexes() const {
  assert(IsAcceptable());
  assert(!graphics_queue_family_indexes_.empty());
  assert(!presentation_queue_family_indexes_.empty());

  // Prefer to use the same queue family for graphics and presentation commands.
  //
  // This avoids having to share images across queues.
  for (uint32_t graphics_queue_family_index : graphics_queue_family_indexes_) {
    if (presentation_queue_family_indexes_.count(graphics_queue_family_index)) {
      return {
        .graphics_queue_family_index = graphics_queue_family_index,
        .presentation_queue_family_index = graphics_queue_family_index,
      };
    }
  }

  // No queue family supports both graphics commands and presentation commands
  // for the given device. Fall back to the first queue family in each category.
  return {
    .graphics_queue_family_index = *graphics_queue_family_indexes_.begin(),
    .presentation_queue_family_index = *graphics_queue_family_indexes_.begin(),
  };
}
