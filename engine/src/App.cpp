#include <cstring>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../headers/App.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define SHADER_PATH "../engine/shader/"

void App::run() {
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
}

void App::init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); Can be removed
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
}

void App::init_vulkan() {
    create_instance();
    crete_surface();
    setup_debug_callback();
    pickPhysicalDevice();
    create_logical_device();
    create_swapchain();
    create_image_view();
    create_render_pass();
    create_command_pool();
    create_command_buffer();

    create_depth_resources();
    create_frame_buffers();

    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();

    create_uniform_buffer();
    create_descriptor_pool();
    create_descriptor_set_layout();
    create_descriptor_set();

    create_graphics_pipeline();
    create_vertex_buffer();
    create_indices_buffer();
    create_sync_objects();
}

void App::main_loop() {
    int frames = 0;
    double t, t0, fps;
    std::stringstream title;
//    char title_string[100];

    t0 = glfwGetTime();

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
        t = glfwGetTime();
        if((t - t0) > 1.0 || frames == 0)
        {
            fps = (double)frames / (t - t0);
            title << "FPS: " << fps;
//            sprintf(title_string, "FPS: %.1f", fps);
            glfwSetWindowTitle(window, title.str().c_str());
            title.str(std::string());
            t0 = t;
            frames = 0;
        }
        frames ++;
    }

    vkDeviceWaitIdle(device);
}

void App::cleanup() {
    for(size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++){
        vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(device, inFlightFence[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    cleanup_swapchain();
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBufferMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDevice(device, nullptr);
    DestroyDebugUtilsMessengerEXT(instance, callbacks, nullptr);
    vkDestroySurfaceKHR(instance, surfaceKhr, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::create_instance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    print_instance_extensions();
    if (!check_validation_layers_support()) {
        throw std::runtime_error("no validation layers supported");
    }


    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Instance failed");
    }


}

void App::print_instance_extensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "Available Extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << ":";
        std::cout << "\t" << extension.specVersion << "\n";
    }
}

bool App::check_validation_layers_support() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto& layerName : validationLayers) {
        std::cout << "available layers:\n";
        for (const auto& layerProperties : availableLayers) {
            std::cout << "\t" << layerProperties.layerName << "\n";
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                return true;
            }
        }
    }
    return false;
}

VkBool32
App::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbakData, void *pUserData) {

    std::cerr << "VALIDATION LAYERS: " << messageSeverity << "\n\t" << pCallbakData->pMessage << std::endl;
    return 0;
}

void App::setup_debug_callback() {
    VkDebugUtilsMessengerCreateInfoEXT createInfoExt = {};
    createInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfoExt.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    createInfoExt.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    createInfoExt.pfnUserCallback = debugCallback;
    createInfoExt.pUserData = nullptr;

    if (CreateDebugUtilsMessengerEXT(instance, &createInfoExt, nullptr, &callbacks) != VK_SUCCESS) {
        throw std::runtime_error("debug callback creation failed");
    }

}

void App::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("no device found!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    print_instance_device(devices);

    std::multimap<uint32_t , VkPhysicalDevice> candidates;
    for (const auto& device : devices) {
        uint32_t score = rate_device_suitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("no device suitable");
    }
}

void App::print_instance_device(std::vector<VkPhysicalDevice> devices) {
    VkPhysicalDeviceProperties deviceProperties;
    std::cout << "Physical Devices:\n";
    for (const auto& device : devices) {
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "\t" << deviceProperties.deviceName << "\n";
        std::cout << "\t" << deviceProperties.deviceType << "\n";
        std::cout << "\tversion: " << deviceProperties.apiVersion << "\n";
    }
}

bool App::is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);
    bool swapchainAdequate = false;

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    SwapChainSupportDetails swapChainSupportDetails =
            query_swapchain_support(device);
    swapchainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentMode.empty();

    return indices.is_complete() && check_device_extension_support(device) && swapchainAdequate && deviceFeatures.samplerAnisotropy;
}

uint32_t App::rate_device_suitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if(!is_device_suitable(device)) return 0;

    uint32_t score = 0;
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;
    auto queueFamily = find_queue_families(device);

    if ((!deviceFeatures.geometryShader) || !(queueFamily.is_complete() || (!deviceFeatures.samplerAnisotropy))) {
        return 0;
    }

    return score;
}

QueueFamilyIndices App::find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // print_device_queue_family(queueFamilies);

    int i = 0;
    for(const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;

        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surfaceKhr, &presentSupport);

        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicalFamily = i;
        }

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.is_complete()) break;
        i++;
    }

    return indices;
}

void App::print_device_queue_family(std::vector<VkQueueFamilyProperties> properties) {
    std::cout << "Queue Families:" <<"\n";
    for (const auto& property : properties) {
        std::cout << "\t" << property.queueCount << "\n";
    }
}

void App::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(physicalDevice);
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicalFamily, indices.presentFamily};

    for(auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicalFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device, indices.graphicalFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

void App::crete_surface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

bool App::check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    std::cout << "Device Extensions:\n";
    for (const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << "\n";
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails App::query_swapchain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surfaceKhr, &details.capabilitiesExt);
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceKhr, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceKhr, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceKhr, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentMode.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceKhr, &presentModeCount, details.presentMode.data());
    }

    return details;
}

VkSurfaceFormatKHR App::choose_swapchain_format(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR App::choose_present_mode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilitiesKhr) {
    if (capabilitiesKhr.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return capabilitiesKhr.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
            static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilitiesKhr.minImageExtent.width,
                                    capabilitiesKhr.maxImageExtent.width);

    actualExtent.height = std::clamp(actualExtent.height, capabilitiesKhr.minImageExtent.height,
                                    capabilitiesKhr.maxImageExtent.height);

    return actualExtent;
}

void App::create_swapchain() {
    SwapChainSupportDetails swapChainSupportDetails = query_swapchain_support(physicalDevice);

    VkSurfaceFormatKHR surfaceFormatKhr = choose_swapchain_format(swapChainSupportDetails.formats);
    VkPresentModeKHR presentModeKhr = choose_present_mode(swapChainSupportDetails.presentMode);
    VkExtent2D extent2D = choose_swapchain_extent(swapChainSupportDetails.capabilitiesExt);
    QueueFamilyIndices indices = find_queue_families(physicalDevice);
    uint32_t queueFamilyIndices[] = {
            indices.graphicalFamily, indices.presentFamily
    };

    uint32_t imageCount = swapChainSupportDetails.capabilitiesExt.minImageCount + 1;

    if (swapChainSupportDetails.capabilitiesExt.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilitiesExt.maxImageCount) {
        imageCount = swapChainSupportDetails.capabilitiesExt.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfoKhr = {};
    createInfoKhr.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfoKhr.surface = surfaceKhr;
    createInfoKhr.minImageCount = imageCount;
    createInfoKhr.imageFormat = surfaceFormatKhr.format;
    createInfoKhr.imageColorSpace = surfaceFormatKhr.colorSpace;
    createInfoKhr.imageExtent = extent2D;
    createInfoKhr.imageArrayLayers = 1;
    createInfoKhr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (indices.graphicalFamily != indices.presentFamily) {
        createInfoKhr.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfoKhr.queueFamilyIndexCount = 2;
        createInfoKhr.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfoKhr.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfoKhr.preTransform = swapChainSupportDetails.capabilitiesExt.currentTransform;
    createInfoKhr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfoKhr.presentMode = presentModeKhr;
    createInfoKhr.clipped = VK_TRUE;
    createInfoKhr.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfoKhr, nullptr, &swapchainKhr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain");
    }

    swapchainImageFormat = surfaceFormatKhr.format;
    swapchainExtent = extent2D;
    vkGetSwapchainImagesKHR(device, swapchainKhr, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchainKhr, &imageCount, swapchainImages.data());
}

void App::create_image_view() {
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        swapchainImageViews[i] = create_image_views(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

std::vector<char> App::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file");
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void App::create_graphics_pipeline() {
    auto vertexShaderCode = readFile(SHADER_PATH "../shader/shader.vert.spv");
    auto fragShaderCode = readFile(SHADER_PATH "../shader/shader.frag.spv");

    VkShaderModule vertexShaderModule = create_shader_module(vertexShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);


    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageCreateInfo.module = vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.module = fragShaderModule;
    fragShaderStageCreateInfo.pName = "main";

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    auto bindingDescriptions = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescriptions;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainExtent.width;
    viewport.height = (float) swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
    depthStencilStateCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo = {};
    blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateCreateInfo.logicOpEnable = VK_FALSE;
    blendStateCreateInfo.attachmentCount = 1;
    blendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
    VkPipelineShaderStageCreateInfo shaderInfos[] = {
            vertexShaderStageCreateInfo, fragShaderStageCreateInfo
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderInfos;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &blendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

VkShaderModule App::create_shader_module(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }


    return shaderModule;
}

void App::create_render_pass() {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = swapchainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = find_depth_format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;


    std::array<VkAttachmentDescription , 2> attachments = {
            attachmentDescription, depthAttachment
    };


    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

void App::create_frame_buffers() {


    swapchainFrameBuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        std::array<VkImageView, 2> attachments = {
                swapchainImageViews[i],
                depthImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = attachments.size();
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = swapchainExtent.width;
        framebufferCreateInfo.height = swapchainExtent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapchainFrameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}

void App::create_command_pool() {
    QueueFamilyIndices indices = find_queue_families(physicalDevice);

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = indices.graphicalFamily;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }
}

void App::create_command_buffer() {
    commandBuffer.resize(MAX_FRAME_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = commandBuffer.size();

    if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffer.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}

void App::record_command_buffer(VkCommandBuffer vkCommandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkClearValue clearValue = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = swapchainFrameBuffers[imageIndex];
    renderPassBeginInfo.renderArea = {{0, 0}, swapchainExtent};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();
        if (vkBeginCommandBuffer(vkCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }
        vkCmdBeginRenderPass(vkCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport{
                0.0f, 0.0f,
                static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height),
                0.0f, 1.0f
        };
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);
        VkRect2D scissor{
                {0, 0}, swapchainExtent
        };

        vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(vkCommandBuffer, 
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 
                                0,
                                1,
                                &descriptorSets[imageIndex],
                                0,
                                nullptr);
        
        vkCmdDrawIndexed(vkCommandBuffer, indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(vkCommandBuffer);

        if (vkEndCommandBuffer(vkCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
}

void App::create_sync_objects() {
    imageAvailableSemaphore.resize(MAX_FRAME_IN_FLIGHT);
    renderFinishedSemaphore.resize(MAX_FRAME_IN_FLIGHT);
    inFlightFence.resize(MAX_FRAME_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,  &renderFinishedSemaphore[i]) != VK_SUCCESS
            || vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to crate semaphores");
        }
    }


}

void App::drawFrame() {

    uint32_t currentFrame = 0;
    vkWaitForFences(device, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(device, swapchainKhr, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

    update_uniform_buffer(imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    vkResetFences(device, 1, &inFlightFence[currentFrame]);
    vkResetCommandBuffer(commandBuffer[currentFrame], 0);
    record_command_buffer(commandBuffer[currentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {
            imageAvailableSemaphore[currentFrame]
    };
    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSemaphore  signalSemaphores[] = {
            renderFinishedSemaphore[currentFrame]
    };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command");
    }


    VkSwapchainKHR vkSwapchains[] = {swapchainKhr};
    VkPresentInfoKHR presentInfoKhr = {};
    presentInfoKhr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfoKhr.waitSemaphoreCount = 1;
    presentInfoKhr.pWaitSemaphores = signalSemaphores;
    presentInfoKhr.swapchainCount = 1;
    presentInfoKhr.pSwapchains = vkSwapchains;
    presentInfoKhr.pImageIndices = &imageIndex;
    presentInfoKhr.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfoKhr);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
        frameBufferResized = false;
        recreate_swapchain();
    } else if(result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain image!");
    }
    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void App::recreate_swapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while(width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);

    cleanup_swapchain();

    create_swapchain();
    create_image_view();
    create_depth_resources();
    create_frame_buffers();
}

void App::cleanup_swapchain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto & swapchainFrameBuffer : swapchainFrameBuffers) {
        vkDestroyFramebuffer(device, swapchainFrameBuffer, nullptr);
    }

    for (auto& imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchainKhr, nullptr);
}

void App::frameBufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app =
            reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

void App::create_vertex_buffer() {
    VkDeviceSize deviceSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    create_buffer(deviceSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, deviceSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) deviceSize);
    vkUnmapMemory(device, stagingBufferMemory);

    create_buffer(deviceSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer,
                  vertexBufferMemory);

    copy_buffer(stagingBuffer, vertexBuffer, deviceSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

}

uint32_t App::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)
           && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to create suitable memory type");
}

void App::create_buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                        VkBuffer &buffer, VkDeviceMemory &deviceMemory) {
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, propertyFlags);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, deviceMemory, 0);
}

void App::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer  vkCommandBuffer = begin_single_time_command();

    VkBufferCopy bufferCopy = {};
    bufferCopy.size = size;

    vkCmdCopyBuffer(vkCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

    end_single_time_command(vkCommandBuffer);
}

void App::create_indices_buffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stageingBufferMemory;
    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stageingBufferMemory);

    void* data;
    vkMapMemory(device, stageingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stageingBufferMemory);

    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT
                  | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
                  indexBuffer, indexBufferMemory);

    copy_buffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stageingBufferMemory, nullptr);

}

void App::create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    std::array<VkDescriptorSetLayoutBinding, 2> binding = {
            layoutBinding, samplerLayoutBinding
    };
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = binding.size();
    layoutCreateInfo.pBindings = binding.data();
    if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void App::create_uniform_buffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAME_IN_FLIGHT);
    uniformBufferMemory.resize(MAX_FRAME_IN_FLIGHT);
    uniformBufferMapped.resize(MAX_FRAME_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; ++i) {
        create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                      | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                      uniformBuffers[i], uniformBufferMemory[i]);

        vkMapMemory(device, uniformBufferMemory[i], 0, bufferSize, 0, &uniformBufferMapped[i]);
    }
}

void App::update_uniform_buffer(uint32_t currentImage) {
    static auto startTime =
            std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {};
    ubo.model =
            glm::rotate(
                    glm::mat4(1.0f),
                    time * glm::radians(90.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f)
            );

    ubo.view = glm::lookAt(
            glm::vec3(1.5f, 1.5f, 1.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.proj = glm::perspective(
            glm::radians(45.0f),
            (float) swapchainExtent.width / (float) swapchainExtent.height,
            0.1f,
            10.0f
    );
    ubo.proj[1][1] *= -1;
    memcpy(uniformBufferMapped[currentImage], &ubo, sizeof(ubo));
}

void App::create_descriptor_pool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAME_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAME_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.poolSizeCount = poolSizes.size();
    poolCreateInfo.pPoolSizes = poolSizes.data();
    poolCreateInfo.maxSets = MAX_FRAME_IN_FLIGHT;
    if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void App::create_descriptor_set() {
    std::vector<VkDescriptorSetLayout> layout(MAX_FRAME_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr, //pNext;
        descriptorPool,
        MAX_FRAME_IN_FLIGHT,
        layout.data()
    };

    descriptorSets.resize(MAX_FRAME_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; ++i) {

        VkDescriptorBufferInfo bufferInfo = {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

}

void App::create_texture_image() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../textures/mango.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    create_image(texWidth, texHeight,
                 VK_FORMAT_R8G8B8A8_SRGB,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT
                 | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 textureImage, textureImageMemory);

    transition_image_layout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_buffer_to_image(stagingBuffer,
                         textureImage,
                         texWidth,
                         texHeight);

    transition_image_layout(textureImage,
                            VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void App::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                       VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &imageMemory) {

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1
    };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkCommandBuffer App::begin_single_time_command() {
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer vkCommandBuffer;
    vkAllocateCommandBuffers(device, &allocateInfo, &vkCommandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);

    return vkCommandBuffer;
}

void App::end_single_time_command(VkCommandBuffer vkCommandBuffer) {
    vkEndCommandBuffer(vkCommandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &vkCommandBuffer);
}

void App::transition_image_layout(VkImage image, VkFormat format, VkImageLayout odlLayout, VkImageLayout newLayout) {
    VkCommandBuffer vkCommandBuffer = begin_single_time_command();

    VkImageMemoryBarrier vkImageMemoryBarrier = {};
    vkImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkImageMemoryBarrier.oldLayout = odlLayout;
    vkImageMemoryBarrier.newLayout = newLayout;
    vkImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrier.image = image;
    VkImageAspectFlags aspectFlags;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (has_stencil_component(format)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkImageMemoryBarrier.subresourceRange = {
            aspectFlags,
            0, 1, 0, 1
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (odlLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        vkImageMemoryBarrier.srcAccessMask = 0;
        vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (odlLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(vkCommandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &vkImageMemoryBarrier);

    end_single_time_command(vkCommandBuffer);
}

void App::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer vkCommandBuffer = begin_single_time_command();

    VkBufferImageCopy bufferImageCopy = {};
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;

    bufferImageCopy.imageSubresource = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 0, 1
    };
    bufferImageCopy.imageOffset = {0, 0, 0};
    bufferImageCopy.imageExtent = {
            width, height, 1
    };

    vkCmdCopyBufferToImage(vkCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);



    end_single_time_command(vkCommandBuffer);
}

void App::create_texture_image_view() {
    textureImageView = create_image_views(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView App::create_image_views(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo  viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange = {
            aspectFlags,
            0, 1, 0, 1
    };

    VkImageView imageView;
    if (vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view");
    }

    return imageView;
}

void App::create_texture_sampler() {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void App::create_depth_resources() {
    VkFormat depthFormat = find_depth_format();

    create_image(swapchainExtent.width, swapchainExtent.height,
                 depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

    depthImageView = create_image_views(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

//    transition_image_layout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

VkFormat App::find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling imageTiling,
                                    VkFormatFeatureFlags featureFlags) {

    for (auto format: candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        if (imageTiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags) {
            return format;
        } else if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags){
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat App::find_depth_format() {
    return find_supported_format(

            {VK_FORMAT_D32_SFLOAT,
             VK_FORMAT_D32_SFLOAT_S8_UINT,
             VK_FORMAT_D24_UNORM_S8_UINT},

             VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

constexpr bool App::has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        std::cerr << "error extension not present"<<std::endl;
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    } else {
        throw std::runtime_error("vkDestroyDebugUtilsMessengerEXT not found!");
    }
}
