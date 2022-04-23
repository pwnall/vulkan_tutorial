#include <cstdint>
#include <iostream>
#include <tuple>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

int main() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "VulkanWindow", /*monitor=*/nullptr,
                                        /*share=*/nullptr);

  uint32_t extension_count = 0;
  std::ignore = vkEnumerateInstanceExtensionProperties(
      /*pLayerName=*/nullptr, &extension_count, /*pProperties=*/nullptr);

  std::cout << extension_count << " extensions supported\n";

  glm::mat4 matrix;
  glm::vec4 vector;
  [[maybe_unused]] auto test = matrix * vector;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
