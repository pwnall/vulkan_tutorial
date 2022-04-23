#include "vulkan_device.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "vulkan_config.h"
#include "vulkan_errors.h"
#include "vulkan_presentation_context.h"
#include "vulkan_physical_device.h"
#include "vulkan_surface_support.h"

namespace {

[[nodiscard]] vk::UniqueDevice CreateDevice(
    const VulkanConfig& vulkan_config,
    const VulkanSurfaceSupport& surface_support,
    VulkanPhysicalDevice& physical_device) {
  assert(physical_device.VulkanHandle() == surface_support.PhysicalDeviceVulkanHandle());
  assert(surface_support.IsAcceptable());

  vk::PhysicalDeviceFeatures required_features{};
  required_features.tessellationShader = true;

  VulkanSurfaceSupport::Queues queues = surface_support.QueueFamilyIndexes();

  std::set<uint32_t> family_indexes = {
      queues.graphics_queue_family_index,
      queues.presentation_queue_family_index,
  };

  static constexpr std::array<float, 1> kQueuePriorities = {1.0};

  std::vector<vk::DeviceQueueCreateInfo> queue_create_info;
  for (uint32_t family_index : family_indexes) {
    queue_create_info.emplace_back(vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex(family_index)
        .setQueuePriorities(kQueuePriorities));
  }

  const std::vector<const char*>& required_layers = vulkan_config.RequiredLayers();
  assert(physical_device.HasLayers(required_layers));

  std::vector<const char*> required_extensions = vulkan_config.RequiredDeviceExtensions();
  assert(physical_device.HasExtensions(required_extensions));

  // MoltenVK devices must have the VK_KHR_portability_subset extension enabled.
  // This serves as an acknowledgment that we're using a driver that's not
  // fully Vulkan-compliant.
  static constexpr char kPortabilityExtensionName[] = "VK_KHR_portability_subset";
  if (physical_device.HasExtension({kPortabilityExtensionName}))
    required_extensions.push_back(kPortabilityExtensionName);

  vk::DeviceCreateInfo device_create_info;
  device_create_info.setQueueCreateInfos(queue_create_info);
  device_create_info.setPEnabledLayerNames(required_layers);
  device_create_info.setPEnabledExtensionNames(required_extensions);
  device_create_info.setPEnabledFeatures(&required_features);

  vk::ResultValue<vk::UniqueDevice> device =
      physical_device.VulkanHandle().createDeviceUnique(device_create_info);
  VulkanCheckResult("vkCreateDevice", device.result);
  return std::move(device.value);
}

[[nodiscard]] vk::UniqueSwapchainKHR CreateSwapChain(
    const VulkanSurfaceSupport& surface_support,
    const VulkanPresentationSurface& surface,
    vk::Device logical_device) {
  assert(surface.VulkanHandle() == surface_support.SurfaceVulkanHandle());
  assert(surface_support.IsAcceptable());

  VulkanSurfaceSupport::Queues queues = surface_support.QueueFamilyIndexes();
  bool is_unified_queue =
      (queues.graphics_queue_family_index == queues.presentation_queue_family_index);
  const std::array<uint32_t, 2> queue_family_indexes = {
    queues.graphics_queue_family_index, queues.presentation_queue_family_index
  };

  vk::SurfaceFormatKHR surface_format = surface_support.BestFormat();
  vk::Extent2D image_extent = surface_support.BestExtentFor(surface.Size());
  vk::SwapchainCreateInfoKHR create_info;
  create_info
      .setMinImageCount(static_cast<uint32_t>(surface_support.BestImageCount()))
      .setSurface(surface.VulkanHandle())
      .setImageFormat(surface_format.format)
      .setImageColorSpace(surface_format.colorSpace)
      .setImageExtent(image_extent)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(surface_support.CurrentTransform())
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(surface_support.BestMode())
      .setClipped(true)
      .setOldSwapchain(nullptr);  // TODO(pwnall): Change when recreating.

  if (is_unified_queue) {
    create_info.setImageSharingMode(vk::SharingMode::eExclusive).setQueueFamilyIndices({});
  } else {
    create_info
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndices(queue_family_indexes);

  }

  vk::ResultValue<vk::UniqueSwapchainKHR> create_result =
      logical_device.createSwapchainKHRUnique(create_info);
  VulkanCheckResult("vkCreateSwapchainKHR", create_result.result);
  return std::move(create_result.value);
}

[[nodiscard]] vk::Queue GetGraphicsQueue(const VulkanSurfaceSupport& surface_support,
                                         vk::Device logical_device) {
  assert(logical_device);

  uint32_t family_index = surface_support.QueueFamilyIndexes().graphics_queue_family_index;

  vk::Queue queue = logical_device.getQueue(family_index, /*queueIndex=*/0);
  assert(queue);

  return queue;
}

[[nodiscard]] vk::Queue GetPresentationQueue(const VulkanSurfaceSupport& surface_support,
                                             vk::Device logical_device) {
  assert(logical_device);

  uint32_t family_index = surface_support.QueueFamilyIndexes().presentation_queue_family_index;

  vk::Queue queue = logical_device.getQueue(family_index, /*queueIndex=*/0);
  assert(queue);

  return queue;
}

[[nodiscard]] std::vector<vk::Image> GetSwapChainImages(
    vk::Device logical_device, vk::SwapchainKHR swap_chain) {
  assert(logical_device);
  assert(swap_chain);

  vk::ResultValue<std::vector<vk::Image>> get_images_result =
      logical_device.getSwapchainImagesKHR(swap_chain);
  VulkanCheckResult("vkGetSwapchainImagesKHR", get_images_result.result);

  return std::move(get_images_result.value);
}

[[nodiscard]] vk::UniqueImageView CreateImageView(
    vk::Format image_format, vk::Device logical_device, vk::Image image) {
  vk::ImageViewCreateInfo create_info;
  create_info
    .setImage(image)
    .setViewType(vk::ImageViewType::e2D)
    .setFormat(image_format)
    .setComponents(vk::ComponentMapping(
        vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity))
    .setSubresourceRange(vk::ImageSubresourceRange()
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1));

  vk::ResultValue<vk::UniqueImageView> create_result =
      logical_device.createImageViewUnique(create_info);
  VulkanCheckResult("vkCreateImageView", create_result.result);

  return std::move(create_result.value);
}

[[nodiscard]] std::vector<vk::UniqueImageView> CreateImageViews(
    vk::Format image_format, vk::Device logical_device, const std::vector<vk::Image>& images) {
  std::vector<vk::UniqueImageView> image_views;
  image_views.reserve(images.size());

  for (vk::Image image : images)
    image_views.push_back(CreateImageView(image_format, logical_device, image));
  return image_views;
}

}  // namespace


VulkanDevice::VulkanDevice(
    const VulkanConfig& vulkan_config, const VulkanSurfaceSupport& surface_support,
    const VulkanPresentationSurface& surface, VulkanPhysicalDevice& physical_device)
    : device_(CreateDevice(vulkan_config, surface_support, physical_device)),
      swap_chain_(CreateSwapChain(surface_support, surface, device_.get())),
      swap_chain_format_(surface_support.BestFormat()),
      graphics_queue_(GetGraphicsQueue(surface_support, device_.get())),
      presentation_queue_(GetPresentationQueue(surface_support, device_.get())),
      swap_chain_images_(GetSwapChainImages(device_.get(), swap_chain_.get())),
      swap_chain_image_views_(CreateImageViews(
          swap_chain_format_.format, device_.get(), swap_chain_images_)) {
}

VulkanDevice::VulkanDevice(VulkanDevice&& rhs) noexcept = default;
VulkanDevice& VulkanDevice::operator=(VulkanDevice&& rhs) noexcept = default;

VulkanDevice::~VulkanDevice() {
  // This class supports move construction and assignment.
  if (device_) {
    // Ensure there are no in-progress commands when the device is destroyed.
    std::ignore = device_->waitIdle();
  }
}
