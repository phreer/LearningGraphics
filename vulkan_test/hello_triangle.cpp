#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <stdlib.h>
#include <optional>
#include <set>

#include <sys/types.h>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


const char kAppName[] = "HelloTriangleApp";
const char kVertShaderPath[] = "vert.spv";
const char kFragShaderPath[] = "frag.spv";

std::vector<char> ReadFile(const std::string &path) {
  std::ifstream ifs(path, std::ios::ate | std::ios::binary);
  if (!ifs.is_open()) {
    std::cout << "Error: failed to open file: " << path << std::endl;
    exit(EXIT_FAILURE);
  }
  size_t file_size = ifs.tellg();
  std::vector<char> buffer(file_size);
  ifs.seekg(0);
  ifs.read(buffer.data(), file_size);
  ifs.close();

  return buffer;
}

const std::vector<const char*> kValidationLayers = {
  "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> kDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
    const bool kEnableValidationLayers = false;
#else
    const bool kEnableValidationLayers = true;
#endif


struct QueueFamliesIndices {
  std::optional<uint32_t> graphics_index;
  std::optional<uint32_t> presenting_index;
  
  bool IsComplete() {
    return graphics_index.has_value() && presenting_index.has_value();
  }
  QueueFamliesIndices(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    FindQueueFamilyIndices(device, surface);
  }

  void FindQueueFamilyIndices(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    if (count == 0) {
      std::cout << "Error: no queue family found\n";
      exit(EXIT_FAILURE);
    }
    std::vector<VkQueueFamilyProperties> properties_list(count);
    std::vector<bool> support_graphics(count);
    std::vector<bool> support_presenting(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties_list.data());
    size_t i = 0;
    for (auto& p: properties_list) {
      if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        support_graphics[i] = true;
      }
      VkBool32 support;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &support);
      if (support) {
        support_presenting[i] = true;
      }
      ++i;
    }
    for (i = 0; i < count; ++i) {
      if (support_graphics[i] && support_presenting[i]) {
        graphics_index = presenting_index = i;
        break;
      }
      if (support_graphics[i] && !graphics_index.has_value()) {
        graphics_index = i;
      }
      if (support_presenting[i] && !presenting_index.has_value()) {
        presenting_index = i;
      }
    }
  } 
};


struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;

  SwapChainSupportDetails(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    QuerySwapChainSupportDetails(device, surface);
  }

  void QuerySwapChainSupportDetails(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    auto res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to get physical device capabilities, error code: " << res << "\n";
      exit(EXIT_FAILURE);
    }
    
    uint32_t format_count = 0;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to get physical device surface formats, error code: " << res << "\n";
      exit(EXIT_FAILURE);
    }
    if (format_count != 0) {
      formats.resize(format_count);
      res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());
      if (res != VK_SUCCESS) {
        std::cerr << "Error: failed to get physical device surface formats, error code: " << res << "\n";
        exit(EXIT_FAILURE);
      }
    }

    uint32_t present_modes_count = 0;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to get physical device present modes, error code: " << res << "\n";
      exit(EXIT_FAILURE);
    }
    if (present_modes_count != 0) {
      present_modes.resize(present_modes_count);
      res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, present_modes.data());
      if (res != VK_SUCCESS) {
        std::cerr << "Failed to get physical device present modes, error code: " << res << ".\n";
        exit(EXIT_FAILURE);
      }
    }
  }
  
  bool IsComplete() const {
    return !formats.empty() && !present_modes.empty();
  }
  VkSurfaceFormatKHR PickFormat() {
    assert(IsComplete());
    for (auto format: formats) {
      if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        std::cout << "Info: format VK_FORMAT_R8G8B8A8_SRGB and color space VK_COLOR_SPACE_SRGB_NONLINEAR_KHR found\n";
        return format;
      }
    }
    return formats[0];
  }

  VkPresentModeKHR PickPresentMode() {
    assert(IsComplete());
    for (auto mode: present_modes) {
      if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        std::cout << "Info: VK_PRESENT_MODE_MAILBOX_KHR present mode found\n";
        return mode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D PickExtent2D(GLFWwindow *window) {
    assert(IsComplete());
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actual_extent = {
      .width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      .height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    return actual_extent;
  }
};


class PhysicalDeviceSelector {
public:
  static bool HasDesiredQueues(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    QueueFamliesIndices indices(device, surface);
    return indices.IsComplete();
  }

  static bool IsDeviceExtensionSupported(VkPhysicalDevice &device) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available_extensions.data());
    std::set<std::string> required_extensions(kDeviceExtensions.cbegin(), kDeviceExtensions.cend());
    for (auto &available_extension: available_extensions) {
      required_extensions.erase(available_extension.extensionName);
    }
    return required_extensions.empty();
  }

  static bool IsSwapChainCompatible(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    SwapChainSupportDetails details(device, surface);
    return details.IsComplete();
  }

  static bool IsPhysicalDeviceSuitable(VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
    if (!HasDesiredQueues(device, surface)) {
      return false;
    }
    if (!IsDeviceExtensionSupported(device)) {
      return false;
    }
    if (!IsSwapChainCompatible(device, surface)) {
      return false;
    }
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(device, &prop);
    VkPhysicalDeviceFeatures feature;
    vkGetPhysicalDeviceFeatures(device, &feature);
    
    return feature.geometryShader;
  }

};


class HelloTriangleApp {
 public:
  void Run() {
    Init();
    MainLoop();
    CleanUp();
  }
 private:
  void _InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, kAppName, nullptr, nullptr);
    if (!window_) {
      std::cerr << "Failed to create window.\n";
      exit(EXIT_FAILURE);
    }
  }
  

  void _InitVulkan() {
    _CreateVulkanInstance();
    _SetupDebugMessager();
    _CreateSurface();

    _PickPhysicalDevice();
    _CreateLogicalDevice();
    _CreateSwapChain();
    _CreateImageViews();
    _CreateRenderPass();
    _CreatePipelines();
    _CreateFramebuffers();
    _CreateCommandPool();
    _CreateCommandBuffer();
    
    _CreateSyncObjects();
  }

  void Init() {
    _InitWindow();
    _InitVulkan();
  }
  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      DrawFrame();
    }
    vkDeviceWaitIdle(device_);
  }
  

  void DrawFrame() {
    vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &in_flight_fence_);
    
    uint32_t image_index;
    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_semaphore_, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffer_, 0);
    _RecordCommandBUffer(command_buffer_, image_index);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphore_};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    VkSemaphore signal_semaphores[] = {render_finished_semaphore_};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    // The last parameter references an optional fence that will be signaled
    // when the command buffers finish execution. This allows us to know when
    // it is safe for the command buffer to be reused.
    auto res = vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fence_);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to queue submit, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.waitSemaphoreCount = 1;
    VkSwapchainKHR swapchains[] = {swapchain_};
    present_info.pSwapchains = swapchains;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    vkQueuePresentKHR(presenting_queue_, &present_info);
  }

  void _RecordCommandBUffer(VkCommandBuffer &command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;
    auto res = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to begin commnad buffer, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    
    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass_;
    render_pass_begin_info.framebuffer = swapchain_framebuffers_[image_index];
    render_pass_begin_info.renderArea.extent = swapchain_extent_;
    render_pass_begin_info.renderArea.offset = {0, 0};
    VkClearValue clear_color = {{{0.f, 0.f, 0.f, 1.f}}};
    render_pass_begin_info.pClearValues = &clear_color;
    render_pass_begin_info.clearValueCount = 1;
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
    
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent_;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    
   vkCmdEndRenderPass(command_buffer);

   res = vkEndCommandBuffer(command_buffer);
   if (res != VK_SUCCESS) {
    std::cerr << "Error: failed to end command buffer, code: " << res << std::endl;
    exit(EXIT_FAILURE);
   }
   
  }

  void CleanUp() {
    vkDestroyFence(device_, in_flight_fence_, nullptr);
    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    for (auto &framebuffer: swapchain_framebuffers_) {
      vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    for (auto &image_view: swapchain_image_views_) {
      vkDestroyImageView(device_, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    if (kEnableValidationLayers) {
      __DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    }
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);
    glfwTerminate();
  }
  
  // Initialize `this->instance_`;
  void _CreateVulkanInstance() {
    VkApplicationInfo info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = kAppName,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3
    };
    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &info;
    instance_info.enabledLayerCount = 0;

    auto extension_names = _GetRequiredExtensions();
    instance_info.enabledExtensionCount = extension_names.size();
    instance_info.ppEnabledExtensionNames = extension_names.data();

    
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (kEnableValidationLayers) {
      if (!CheckValidationLayperSupport()) {
        exit(EXIT_FAILURE);
      }
      __PopulateDebugMessengerCreateInfo(debug_create_info);
      instance_info.enabledLayerCount = kValidationLayers.size();
      instance_info.ppEnabledLayerNames = kValidationLayers.data();
      instance_info.pNext = &debug_create_info;
    }
    VkResult res = vkCreateInstance(&instance_info, nullptr, &instance_);
    if (res) {
      std::cerr << "Failed to create vulkan instance.\n";
      std::cerr << "Error code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  
  
  void _CreateSurface() {
    auto res = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    if (res != VK_SUCCESS) {
      std::cerr << "Failed to create window surface, error code: " << res << ".\n";
      exit(EXIT_FAILURE);
    }
  }
  

  void _PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0) {
      std::cerr << "Error: no physical device found\n" << std::endl;
      exit(EXIT_FAILURE);
    }

    std::cout << device_count << " devices found\n";    
    std::vector<VkPhysicalDevice> physical_devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());
    std::vector<VkPhysicalDevice> candidates;
    std::vector<uint32_t> scores;

    for (auto& pd: physical_devices) __PrintDevicePropertiesAndFeatures(pd);
    for (auto& pd: physical_devices) {
      if (PhysicalDeviceSelector::IsPhysicalDeviceSuitable(pd, surface_)) {
        candidates.emplace_back(pd);
        scores.emplace_back(__ComputePhysicalDeviceScore(pd));
      }
    }
    
    if (candidates.empty()) {
      std::cerr << "Error: no candidate physical device found\n";
      exit(EXIT_FAILURE);
    }
    auto max_iter = std::max_element(scores.cbegin(), scores.cend());
    auto index = max_iter - scores.cbegin();

    physical_device_ = candidates[index];
    if (physical_device_ == VK_NULL_HANDLE) {
      std::cerr << "Error: no suitable device found\n";
      exit(EXIT_FAILURE);
    }
    
    std::cout << "Info: selected physical device: " << __GetPhysicalDeviceName(physical_device_) << std::endl;
    
  }

  std::string __GetPhysicalDeviceName(VkPhysicalDevice &device) {
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(device, &prop);
    return prop.deviceName;
  }

  uint32_t __ComputePhysicalDeviceScore(VkPhysicalDevice &device) {
    uint32_t score = 0;
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(device, &prop);
    VkPhysicalDeviceFeatures feature;
    vkGetPhysicalDeviceFeatures(device, &feature);
    if (feature.geometryShader) score += 100;
    if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 100;
    else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 50;

    return score;
  }
  
  void __PrintDevicePropertiesAndFeatures(VkPhysicalDevice &device) {
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(device, &prop);
    VkPhysicalDeviceFeatures feature;
    vkGetPhysicalDeviceFeatures(device, &feature);
    std::cout << "Device name: " << prop.deviceName << std::endl;
    std::cout << "\tDevice type: " << prop.deviceType << std::endl;
    std::cout << "\tDevice ID: " << prop.deviceID << std::endl;
    std::cout << "\tVender ID: " << prop.vendorID << std::endl;
    std::cout << "\tMax image dimension 2D: " << prop.limits.maxImageDimension2D << std::endl;
    std::cout << "\tMax image dimension 3D: " << prop.limits.maxImageDimension3D << std::endl;
    std::cout << "\tGeometry shader: " << feature.geometryShader << std::endl;
    std::cout << "\tTessellation shader: " << feature.tessellationShader << std::endl;
  }

  
  void _CreateLogicalDevice() {
    QueueFamliesIndices indices(physical_device_, surface_);
    std::set<uint32_t> queue_indices_set{indices.graphics_index.value(), indices.presenting_index.value()};
    std::vector<VkDeviceQueueCreateInfo> queue_create_info_list{};
    float queue_priority = 1.f;
    for (auto& index: queue_indices_set) {
      VkDeviceQueueCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueCount = 1;
      info.queueFamilyIndex = index;
      info.pQueuePriorities = &queue_priority;
      queue_create_info_list.push_back(info);
    }
    
    VkPhysicalDeviceFeatures features{};
    
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_info_list.data();
    create_info.queueCreateInfoCount = queue_create_info_list.size();
    create_info.pEnabledFeatures = &features;
    create_info.ppEnabledExtensionNames = kDeviceExtensions.data();
    create_info.enabledExtensionCount = kDeviceExtensions.size();

    if (kEnableValidationLayers) {
      create_info.ppEnabledLayerNames = kValidationLayers.data();
      create_info.enabledLayerCount = kValidationLayers.size();
    } else {
      create_info.enabledLayerCount = 0;
    }
    
    auto res = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to create logical device, code : " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    vkGetDeviceQueue(device_, indices.graphics_index.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.presenting_index.value(), 0, &presenting_queue_);
  }


  void _CreateSwapChain() {
    SwapChainSupportDetails details(physical_device_, surface_);
    auto &capabilities = details.capabilities;
    VkSurfaceFormatKHR format = details.PickFormat();
    VkPresentModeKHR mode = details.PickPresentMode();
    VkExtent2D extent = details.PickExtent2D(window_);
    // A value of 0 of maxImageCount means there is no limit on image count.
    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
      image_count = capabilities.maxImageCount;
    }
    
    QueueFamliesIndices indices(physical_device_, surface_);
    assert(indices.IsComplete());
    uint32_t queue_indices[] = {indices.graphics_index.value(), indices.presenting_index.value()};

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    // The imageArrayLayers specifies the amount of layers each image consists
    // of. This is always 1 unless you are developing a stereoscopic
    // 3D application.
    create_info.imageArrayLayers = 1;
    // The imageUsage bit field specifies what kind of operations we'll use the
    // images in the swap chain for. In this tutorial we're going to render
    // directly to them, which means that they're used as color attachment.
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (indices.graphics_index.value() != indices.presenting_index.value()) {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.pQueueFamilyIndices = queue_indices;
      create_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(queue_indices[0]);
    } else {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.pQueueFamilyIndices = nullptr;
      create_info.queueFamilyIndexCount = 0;
    }
    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    auto res = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
    if (res) {
      std::cerr << "Error: failed to create swap chain, code " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Info: swap chain created.\n";
    
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());
    swapchain_extent_ = extent;
    swapchain_image_format_ = format.format;
  }


  void _CreateImageViews() {
    swapchain_image_views_.resize(swapchain_images_.size());
    for (uint32_t i = 0; i < swapchain_image_views_.size(); ++i) {
      VkImageViewCreateInfo create_info{};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = swapchain_images_[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = swapchain_image_format_;
      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      // The subresourceRange field describes what the image's purpose is and
      // which part of the image should be accessed. Our images will be used as
      // color targets without any mipmapping levels or multiple layers.
      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;
      auto res = vkCreateImageView(device_, &create_info, nullptr, &swapchain_image_views_[i]);
      if (res != VK_SUCCESS) {
        std::cout << "Error: failed to create image view " << i << ", code: " << res << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  
  void _CreatePipelines() {
    auto vert_shader_code = ReadFile(kVertShaderPath);
    auto frag_shader_code = ReadFile(kFragShaderPath);
    
    VkShaderModule vert_shader_module = __CreateShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = __CreateShaderModule(frag_shader_code);
    
    // Create shader stages
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Entry point of the shader
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
    
    // Create dynamic states
    std::vector<VkDynamicState> states = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(states.size());
    dynamic_state_info.pDynamicStates = states.data();
    
    // Vertex Input format
    // describes the format of the vertex data that will be passed to the
    // vertex shader. It describes this in roughly two ways:
    // - Bindings: spacing between data and whether the data is per-vertex or 
    //   per-instance (see instancing)
    // - Attribute descriptions: type of the attributes passed to the vertex
    //   shader, which binding to load them from and at which offset.
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_info.vertexBindingDescriptionCount = 0;

    // Input assembly information
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissors
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent_;
    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
    rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.lineWidth = 1.f;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    rasterization_state_info.depthBiasConstantFactor = 0.f;
    rasterization_state_info.depthBiasClamp = 0.f;
    rasterization_state_info.depthBiasSlopeFactor = 0.f;

    // Multisampling for anti-aliasing
    // We don't use multi-sampling currently.
    VkPipelineMultisampleStateCreateInfo multisample_state_info{};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.colorWriteMask = \
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    
    // Pipeline layout
    // Access to descriptor sets from a pipeline is accomplished through a
    // pipeline layout. 
    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    auto res = vkCreatePipelineLayout(device_, &layout_create_info, nullptr, &pipeline_layout_);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to create pipeline layout, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }

    VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
    graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.pStages = shader_stages;
    graphics_pipeline_info.stageCount = 2;
    graphics_pipeline_info.pVertexInputState = &vertex_input_state_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterization_state_info;
    graphics_pipeline_info.pMultisampleState = &multisample_state_info;
    graphics_pipeline_info.pDepthStencilState = nullptr;
    graphics_pipeline_info.pColorBlendState = &color_blend_state_info;
    graphics_pipeline_info.pDynamicState = &dynamic_state_info;
    graphics_pipeline_info.layout = pipeline_layout_;
    graphics_pipeline_info.renderPass = render_pass_;
    graphics_pipeline_info.subpass = 0;
    res = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &graphics_pipeline_);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to create graphics pipeline, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Info: graphics pipeline created\n";

    // Clean up
    vkDestroyShaderModule(device_, vert_shader_module, nullptr);
    vkDestroyShaderModule(device_, frag_shader_module, nullptr);
  }


  void _CreateRenderPass() {
    // Introduction In order to help optimize deferred shading on tile-based
    // renderers, Vulkan splits the rendering operations of a render pass into
    // subpasses. All subpasses in a render pass share the same resolution and
    // tile arrangement, and as a result, they can access the results of
    // previous subpass.

    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // The loadOp and storeOp determine what to do with the data in the
    // attachment before rendering and after rendering. We have the following
    // choices for loadOp:
    // - VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the
    //   attachment
    // - VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the
    //   start
    // - VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined;
    //   we don't care about them
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // In our case we're going to use the clear operation to clear the
    // framebuffer to black before drawing a new frame. There are only two
    // possibilities for the storeOp:
    // - VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in
    //   memory and can be read later
    // - VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will
    //   be undefined after the rendering operation
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // The initialLayout specifies which layout the image will have before the
    // render pass begins. The finalLayout specifies the layout to
    // automatically transition to when the render pass finishes.
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    VkSubpassDependency dependency{};
    // If we wanted to depend on a subpass that's part of a previous render
    // pass, we could just pass in VK_SUBPASS_EXTERNAL here instead.
    // In this case, that would mean "wait for all of the subpasses within all
    // of the render passes before this one".
    // Reference: https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    // srcStageMask is a bitmask of all of the Vulkan "stages" (basically,
    // steps of the rendering process) we're asking Vulkan to finish executing
    // within srcSubpass before we move on to dstSubpass.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    // dstStageMask is a bitmask of all of the Vulkan stages in dstSubpass
    // that we're not allowed to execute until after the stages in srcStageMask
    // have completed within srcSubpass.
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.subpassCount = 1;
    render_pass_info.pDependencies = &dependency;
    render_pass_info.dependencyCount = 1;

    auto res = vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to create render pass, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  
  void _CreateFramebuffers() {
    swapchain_framebuffers_.resize(swapchain_image_views_.size());
    for (size_t i = 0; i < swapchain_image_views_.size(); ++i) {
      VkImageView attachments[] = {
        swapchain_image_views_[i],
      };

      VkFramebufferCreateInfo framebuffer_info{};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass_;
      framebuffer_info.attachmentCount = 1;
      framebuffer_info.pAttachments = attachments;
      framebuffer_info.width = swapchain_extent_.width;
      framebuffer_info.height = swapchain_extent_.height;
      framebuffer_info.layers = 1;

      auto res = vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &swapchain_framebuffers_[i]);
      if (res != VK_SUCCESS) {
        std::cerr << "Error: failed to create framebuffer, code: " << res << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  
  void _CreateCommandPool() {
    QueueFamliesIndices queue_family_indices = QueueFamliesIndices(physical_device_, surface_);

    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_indices.graphics_index.value();
    auto res = vkCreateCommandPool(device_, &command_pool_info, nullptr, &command_pool_);
    if (res != VK_SUCCESS) {
      std::cout << "Error: failed to create command pool, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    
  }


  void _CreateCommandBuffer() {
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.commandPool = command_pool_;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;
    auto res = vkAllocateCommandBuffers(device_, &command_buffer_alloc_info, &command_buffer_);
    if (res != VK_SUCCESS) {
      std::cout << "Error: failed to create command buffer, code: " << res << std::endl;
      exit(EXIT_FAILURE); 
    }
  }
  

  void _CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphore_) != VK_SUCCESS ||
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fence_)) {
      std::cerr << "Error: failed to create synchronization objects\n";
      exit(EXIT_FAILURE);
    }
  }

  bool CheckValidationLayperSupport() {
    uint32_t available_layers_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);
    std::vector<VkLayerProperties> available_layers(available_layers_count);
    vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data());

    std::cout << "Available layers: " << std::endl;
    for (auto &layer: available_layers) {
      std::cout << "\t" << layer.layerName << std::endl;
      std::cout << "\tDescription: " << layer.description << std::endl;
    }
    
    for (auto &layer_to_check: kValidationLayers) {
      bool found = false;
      for (auto &layer: available_layers) {
        if (strcmp(layer_to_check, layer.layerName) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::cout << "Validation layer " << layer_to_check << " not available.\n";
        return false;
      }
    }
    
    return true;
  }
  
  std::vector<const char *> _GetRequiredExtensions() {

    // Since Vulkan is platform agnostic, it needs extension to interface with Window system, which has been provided
    // by GLFW.
    uint32_t glfw_extension_count = 0;
    const char** glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::cout << "GLFW extensions: \n";
    for (size_t i = 0; i < glfw_extension_count; ++i) {
      std::cout << "\t" << glfw_extension_names[i] << std::endl;
    }

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extension_properties_list(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_properties_list.data());
    std::cout << "Available extensions: \n";
    for (auto &ep: extension_properties_list) {
      std::cout << "\t" << ep.extensionName << " (specVersion: " << ep.specVersion << ")\n";
    }
    
    for (size_t i = 0; i < glfw_extension_count; ++i) {
      bool found = false;
      for (auto& extension: extension_properties_list) {
        if (strcmp(glfw_extension_names[i], extension.extensionName) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::cout << "Extension " << glfw_extension_names[i] << " not available.\n";
      }
    }

    // The returned pointer always exists?
    std::vector<const char *> result(glfw_extension_names, glfw_extension_names + glfw_extension_count);
    
    if (kEnableValidationLayers) {
      result.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return result;
  }
  
  static VKAPI_ATTR VkBool32 VKAPI_CALL __DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
      VkDebugUtilsMessageTypeFlagsEXT message_type,
      const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
      void *user_data) {
    std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;
    
    return VK_FALSE;
  }

  // Proxy functions to create debug messenger, since the functions are loaded dynamically.
  VkResult __CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }
  
  void __DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
    }
  }

  void __PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = __DebugCallback;
    create_info.pUserData = nullptr;
  }

  VkShaderModule __CreateShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shader_module;
    auto res = vkCreateShaderModule(device_, &create_info, nullptr, &shader_module);
    if (res != VK_SUCCESS) {
      std::cerr << "Error: failed to create shader module, code: " << res << std::endl;
      exit(EXIT_FAILURE);
    }
    return shader_module;
  }

  void _SetupDebugMessager() {
    if (!kEnableValidationLayers) return;
      VkDebugUtilsMessengerCreateInfoEXT create_info{};
      __PopulateDebugMessengerCreateInfo(create_info);
      
      auto res = __CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_); 
      if (res) {
        std::cerr << "Failed to create debug messenger.\n";
        exit(EXIT_FAILURE);
      }
  }
  

  static const uint32_t kWindowWidth = 800;
  static const uint32_t kWindowHeight = 600;

  GLFWwindow* window_;
  VkInstance instance_;
  VkDebugUtilsMessengerEXT debug_messenger_;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_;
  VkQueue graphics_queue_;
  VkQueue presenting_queue_;
  VkSurfaceKHR surface_;

  VkSwapchainKHR swapchain_;
  VkFormat swapchain_image_format_;
  VkExtent2D swapchain_extent_;
  VkPipelineLayout pipeline_layout_;
  VkRenderPass render_pass_;
  VkPipeline graphics_pipeline_;
  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;
  
  VkSemaphore image_available_semaphore_;
  VkSemaphore render_finished_semaphore_;
  VkFence in_flight_fence_;

  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
  std::vector<VkFramebuffer> swapchain_framebuffers_;
};


int main() {
  HelloTriangleApp app;
  try {
    app.Run();
  } catch (std::exception &e) {
    std::cerr << "Error occurred.\n";
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Exit gracefully.\n";
  return EXIT_SUCCESS;
}