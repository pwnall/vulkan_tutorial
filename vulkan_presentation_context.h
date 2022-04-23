#ifndef VULKAN_PRESENTATION_CONTEXT_H_
#define VULKAN_PRESENTATION_CONTEXT_H_

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

// Abstract representation for a windowing system drawing surface.
class VulkanPresentationSurface {
 public:
  // The state is hidden to avoid introducing dependencies in the header.
  struct State;

  // Use VulkanPresentationContext.CreateSurface() instead of calling this directly.
  explicit VulkanPresentationSurface(std::unique_ptr<State> state);

  // Moving supported so instances can be returned.
  VulkanPresentationSurface(const VulkanPresentationSurface&) = delete;
  VulkanPresentationSurface(VulkanPresentationSurface&&) noexcept;
  VulkanPresentationSurface& operator=(const VulkanPresentationSurface&) = delete;
  VulkanPresentationSurface& operator=(VulkanPresentationSurface&&) noexcept;

  ~VulkanPresentationSurface();

  // The surface's dimensions, in pixels.
  vk::Extent2D Size() const;

  vk::SurfaceKHR VulkanHandle() const;

  void MainLoop();

 private:
  std::unique_ptr<State> state_;
};

// Bridge between the windowing system and Vulkan.
//
// Instances must outlive all created VulkanPresentationSurface instances.
class VulkanPresentationContext {
 public:
  VulkanPresentationContext();
  VulkanPresentationContext(const VulkanPresentationContext&) = delete;
  VulkanPresentationContext& operator=(const VulkanPresentationContext&) = delete;
  ~VulkanPresentationContext();

  // vkCreateInstance()-friendly list of Vulkan extensions used by this class.
  [[nodiscard]] const std::vector<const char*>& RequiredVulkanInstanceExtensions() const {
    return required_instance_extensions_;
  }

  // vkCreateDevice()-friendly list of Vulkan extensions used by this class.
  [[nodiscard]] const std::vector<const char*>& RequiredVulkanDeviceExtensions() const {
    return required_device_extensions_;
  }

  // Creates a Vulkan surface and its backing window.
  //
  // The returned VulkanPresentationSurface must be destroyed before this instance goes out of
  // scope.
  [[nodiscard]] VulkanPresentationSurface CreateSurface(vk::Instance instance, int width, int height);

 private:
  const std::vector<const char*> required_instance_extensions_;
  const std::vector<const char*> required_device_extensions_;
};

#endif  // VULKAN_PRESENTATION_CONTEXT_H_
