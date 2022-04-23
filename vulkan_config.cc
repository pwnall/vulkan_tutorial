#include "vulkan_config.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <vulkan/vulkan.hpp>

#include "vulkan_extension_list.h"
#include "vulkan_layer_list.h"
#include "vulkan_presentation_context.h"

namespace {

bool WantVulkanValidation() {
#if defined(NDEBUG)
    return false;
#else
    return true;
#endif  // defined(NDEBUG)
}

[[nodiscard]] std::vector<const char*> RequiredVulkanLayers(bool want_validation) {
  std::vector<const char*> required_layers;

  if (want_validation) {
    static constexpr char kValidationLayerName[] = "VK_LAYER_KHRONOS_validation";
    VulkanLayerList layers;
    if (!layers.Contains(kValidationLayerName)) {
      std::cerr << "Validation layer required but not available" << std::endl;
      std::abort();
    }
    required_layers.push_back(kValidationLayerName);
  }

  return required_layers;
}


[[nodiscard]] std::vector<const char*> RequiredVulkanInstanceExtensions(
    const VulkanPresentationContext& presentation_context, bool want_validation) {
  std::vector<const char*> required_extensions = presentation_context.RequiredVulkanInstanceExtensions();

  if (want_validation) {
    static constexpr char kDebugUtilsExtensionName[] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VulkanExtensionList extension_list;
    if (!extension_list.Contains(kDebugUtilsExtensionName)) {
      std::cerr << "Validation layer required but debugging extension not available" << std::endl;
      std::abort();
    }
    required_extensions.push_back(kDebugUtilsExtensionName);
  }

  // MoltenVK exposes devices with the VK_KHR_portability_subset extension.
  // Enumerating them requires the VK_KHR_portability_enumeration extension.
  static constexpr char kPortabilityEnumerationExtensionName[] = "VK_KHR_portability_enumeration";
  required_extensions.push_back(kPortabilityEnumerationExtensionName);

  return required_extensions;
}

[[nodiscard]] vk::PhysicalDeviceFeatures RequiredDeviceFeatures() {
  vk::PhysicalDeviceFeatures required_features;
  required_features.geometryShader = true;

  return required_features;
}

}  // namespace

VulkanConfig::VulkanConfig(const VulkanPresentationContext& presentation_context)
    : want_validation_(WantVulkanValidation()),
      required_layers_(RequiredVulkanLayers(want_validation_)),
      required_instance_extensions_(RequiredVulkanInstanceExtensions(presentation_context, want_validation_)),
      required_device_extensions_(presentation_context.RequiredVulkanDeviceExtensions()),
      required_features_(RequiredDeviceFeatures()) {
}

VulkanConfig::~VulkanConfig() = default;
