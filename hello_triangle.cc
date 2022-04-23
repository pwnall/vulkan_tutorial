#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "vulkan_config.h"
#include "vulkan_device.h"
#include "vulkan_errors.h"
#include "vulkan_extension_list.h"
#include "vulkan_layer_list.h"
#include "vulkan_physical_device_list.h"
#include "vulkan_presentation_context.h"

namespace {

constexpr int kWindowWidth = 800;
constexpr int kwindowHeight = 600;

// Dispatches messages from the Vulkan validation layer to an application.
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallbackThunk(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* message_data,
    void *user_data);

class HelloTriangleApplication {
 public:
  HelloTriangleApplication()
    : presentation_context_(), vulkan_config_(presentation_context_) {}

  HelloTriangleApplication(const HelloTriangleApplication&) = delete;
  HelloTriangleApplication& operator=(const HelloTriangleApplication&) = delete;

  ~HelloTriangleApplication() = default;

  void Run() {
    InitVulkan();

    surface_->MainLoop();

    TeardownVulkan();
  }

  void OnVulkanDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                            VkDebugUtilsMessageTypeFlagsEXT message_type,
                            const VkDebugUtilsMessengerCallbackDataEXT* message_data) {
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
        message_type != VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
      std::cerr << "Vulkan validation message: " << message_data->pMessage << std::endl;

      if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::abort();
    }
  }

 private:
  void InitVulkan() {
    VulkanLayerList layers;
    layers.Print();

    VulkanExtensionList extensions;
    extensions.Print();

    CreateVulkanInstance();
    SetupVulkanDebugMessenger();
    surface_ = presentation_context_.CreateSurface(instance_.get(), kWindowWidth, kwindowHeight);
    SelectPhysicalDevice();
  }

  void TeardownVulkan() {
    device_.reset();
    surface_.reset();
    TeardownVulkanDebugMessenger();
    instance_.reset();
  }

  void CreateVulkanInstance() {
    vk::ApplicationInfo application_info;
    application_info
        .setPApplicationName("Hello Triangle")
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
        .setPEngineName("No engine")
        .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
        .setApiVersion(VK_API_VERSION_1_1);

    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
        create_info_chain;
    create_info_chain.get<vk::InstanceCreateInfo>()
        .setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
        .setPApplicationInfo(&application_info)
        .setPEnabledLayerNames(vulkan_config_.RequiredLayers())
        .setPEnabledExtensionNames(vulkan_config_.RequiredInstanceExtensions());

    // This mildly duplicates SetupVulkanValidationCallback().
    if (vulkan_config_.WantValidation()) {
      create_info_chain.get<vk::DebugUtilsMessengerCreateInfoEXT>()
          .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
          .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
          .setPfnUserCallback(&VulkanDebugCallbackThunk)
          .setPUserData(static_cast<void*>(this));
    } else {
      create_info_chain.unlink<vk::DebugUtilsMessengerCreateInfoEXT>();
    }

    vk::ResultValue<vk::UniqueInstance> create_result =
        vk::createInstanceUnique(create_info_chain.get());

    VulkanCheckResult("vkCreateInstance", create_result.result);
    instance_ = std::move(create_result.value);
  }

  void SetupVulkanDebugMessenger() {
    assert(instance_);

    if (!vulkan_config_.WantValidation())
      return;

    // vkCreateDebugUtilsMessengerEXT() isn't available for static linking.
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_.get(), "vkCreateDebugUtilsMessengerEXT"));
    if (!vkCreateDebugUtilsMessengerEXT) {
      std::cerr << "Failed to dynamically locate vkCreateDebugUtilsMessengerEXT()" << std::endl;
      std::abort();
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = &VulkanDebugCallbackThunk,
      .pUserData = static_cast<void*>(this),
    };
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        instance_.get(), &create_info, /*pAllocator=*/nullptr, &debug_messenger_);
    if (result != VK_SUCCESS) {
      std::cerr << "vkCreateDebugUtilsMessengerEXT() failed" << std::endl;
      std::abort();
    }
  }

  void TeardownVulkanDebugMessenger() {
    assert(instance_);

    assert(vulkan_config_.WantValidation() == (debug_messenger_ != VK_NULL_HANDLE));
    if (debug_messenger_ == VK_NULL_HANDLE)
      return;

    // vkDestroyDebugUtilsMessengerEXT() isn't available for static linking.
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_.get(), "vkDestroyDebugUtilsMessengerEXT"));
    if (!vkDestroyDebugUtilsMessengerEXT) {
      std::cerr << "Failed to dynamically locate vkDestroyDebugUtilsMessengerEXT()" << std::endl;
      std::abort();
    }
    vkDestroyDebugUtilsMessengerEXT(instance_.get(), debug_messenger_, /*pAllocator=*/nullptr);
  }

  void SelectPhysicalDevice() {
    assert(instance_);

    VulkanPhysicalDeviceList devices(instance_.get());
    devices.Print();

    device_ = devices.CreateLogicalDevice(vulkan_config_, *surface_);
  }

  VulkanPresentationContext presentation_context_;
  VulkanConfig vulkan_config_;
  vk::UniqueInstance instance_;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  std::optional<VulkanPresentationSurface> surface_;
  std::optional<VulkanDevice> device_;
};

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallbackThunk(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* message_data,
    void *user_data) {
  assert(user_data);
  HelloTriangleApplication* app = static_cast<HelloTriangleApplication*>(user_data);

  app->OnVulkanDebugMessage(message_severity, message_type, message_data);
  return VK_FALSE;
}


}  // namespace

int main() {
  HelloTriangleApplication app;

  app.Run();
  return 0;
}
