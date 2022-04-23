#include "vulkan_presentation_context.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

// Vulkan must be included before GLFW to get Vulkan-specific functionality.
#include <GLFW/glfw3.h>

namespace {

[[nodiscard]] std::vector<const char*> GlfwRequiredVulkanExtensions() {
  glfwInit();

  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  if (!glfw_extensions) {
    std::cerr << "GLFW did not find a working Vulkan implementation" << std::endl;
    std::abort();
  }

  return std::vector<const char*>(glfw_extensions, glfw_extensions + glfw_extension_count);
}

[[nodiscard]] std::vector<const char*> KhrSwapchainExtensionList() {
  static constexpr char kKhrSwapchainExtensionName[] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  return {kKhrSwapchainExtensionName};
}

}  // namespace

struct VulkanPresentationSurface::State {
  GLFWwindow* window = nullptr;
  vk::UniqueSurfaceKHR surface;
};

VulkanPresentationSurface::VulkanPresentationSurface(std::unique_ptr<State> state)
    : state_(std::move(state)) {
  assert(state_ != nullptr);
}

VulkanPresentationSurface::VulkanPresentationSurface(VulkanPresentationSurface&&) noexcept
    = default;
VulkanPresentationSurface& VulkanPresentationSurface::operator=(VulkanPresentationSurface&&)
    noexcept = default;

VulkanPresentationSurface::~VulkanPresentationSurface() {
  if (!state_)
    return;

  assert(state_->surface);

  // Must be called before glfwDestroyWindow(). Necessary because there's no
  // smart handle for GLFWwindow.
  state_->surface.reset();

  assert(state_->window != nullptr);
  glfwDestroyWindow(state_->window);
}

vk::Extent2D VulkanPresentationSurface::Size() const {
  assert(state_);

  int width = 0, height = 0;
  glfwGetFramebufferSize(state_->window, &width, &height);

  return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

vk::SurfaceKHR VulkanPresentationSurface::VulkanHandle() const {
  assert(state_);
  assert(state_->surface);

  return state_->surface.get();
}

void VulkanPresentationSurface::MainLoop() {
  assert(state_ != nullptr);
  assert(state_->window != nullptr);

  while (!glfwWindowShouldClose(state_->window))
    glfwPollEvents();
}

VulkanPresentationContext::VulkanPresentationContext()
    : required_instance_extensions_(GlfwRequiredVulkanExtensions()),
      required_device_extensions_(KhrSwapchainExtensionList()) {}

VulkanPresentationContext::~VulkanPresentationContext() {
  glfwTerminate();
}

VulkanPresentationSurface VulkanPresentationContext::CreateSurface(vk::Instance instance,
                                                                   int width, int height) {
  assert(instance);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan window", /*monitor=*/nullptr,
                                         /*share=*/nullptr);
  if (window == nullptr) {
    std::cerr << "glfwCreateWindow() failed\n";
    std::abort();
  }

  VkSurfaceKHR raw_surface = VK_NULL_HANDLE;
  VkResult result = glfwCreateWindowSurface(instance, window, /*allocator=*/nullptr, &raw_surface);
  if (result != VK_SUCCESS) {
    std::cerr << "glfwCreateWindowSurface() failed\n";
    std::abort();
  }
  assert(raw_surface != VK_NULL_HANDLE);

  auto state = std::make_unique<VulkanPresentationSurface::State>(VulkanPresentationSurface::State{
      .window = window, .surface = vk::UniqueSurfaceKHR(raw_surface, instance) });
  return VulkanPresentationSurface(std::move(state));
}
