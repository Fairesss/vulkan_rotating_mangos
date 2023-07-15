//
// Created by fair on 01/07/23.
//

#ifndef FAIR_ENGINE_APP_H
#define FAIR_ENGINE_APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <fstream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>


const int MAX_FRAME_IN_FLIGHT = 2;

VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreteInfo,
        VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback
);

void DestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT callback,
        const VkAllocationCallbacks* pAllocator
);

struct QueueFamilyIndices {
    uint32_t graphicalFamily = -1;
    uint32_t presentFamily = -1;
    bool is_complete() { return graphicalFamily >= 0 && presentFamily >= 0; }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilitiesExt;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentMode;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3  color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class App {
    void init_window();
    void init_vulkan();
    void main_loop();
    void cleanup();

    // VULKAN INSTANCE
    GLFWwindow* window;
    VkInstance instance;

    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
    void create_instance();

    // devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    bool is_device_suitable(VkPhysicalDevice device);
    bool check_device_extension_support(VkPhysicalDevice device);

    void pickPhysicalDevice();
    uint32_t rate_device_suitability(VkPhysicalDevice device);
    void create_logical_device();

    void print_instance_device(std::vector<VkPhysicalDevice> devices);
    // Queue Families
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void print_device_queue_family(std::vector<VkQueueFamilyProperties> properties);

    // Surface
    VkSurfaceKHR surfaceKhr;
    void crete_surface();

    // debug / validations
    VkDebugUtilsMessengerEXT callbacks;
    void print_instance_extensions();
    bool check_validation_layers_support();

    // swapchain
    VkSwapchainKHR swapchainKhr;
    std::vector<VkImage> swapchainImages;
    SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device);
    VkSurfaceFormatKHR choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilitiesKhr);
    void create_swapchain();
    void setup_debug_callback();

    // Image View
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    void create_image_view();


    // pipeline
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    void create_graphics_pipeline();
    void create_render_pass();

    // buffers
    std::vector<VkFramebuffer> swapchainFrameBuffers;
    void create_frame_buffers();

    // command pool
    VkCommandPool commandPool;
    void create_command_pool();

    // command buffer
    std::vector<VkCommandBuffer> commandBuffer;
    void create_command_buffer();
    void record_command_buffer(VkCommandBuffer vkCommandBuffer, uint32_t imageIndex);

    // shaders
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    // sync objects
    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderFinishedSemaphore;
    std::vector<VkFence> inFlightFence;
    void create_sync_objects();

    //drawing
    void drawFrame();

    // recreate swapchain
    void cleanup_swapchain();
    void recreate_swapchain();

    // vertex buffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkMemoryRequirements memoryRequirements;

    const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f, .5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, .5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

            {{-0.5f, -0.5f, -.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f, -.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, -.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    };
    const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4
    };

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory);
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void create_vertex_buffer();
    void create_indices_buffer();
    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // uniform buffer
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBufferMemory;
    std::vector<void*> uniformBufferMapped;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    void create_descriptor_set_layout();
    void create_uniform_buffer();
    void create_descriptor_pool();
    void create_descriptor_set();
    void update_uniform_buffer(uint32_t currentImage);

    // texture image
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory);

    void create_texture_image_view();
    void create_texture_image();
    VkImageView create_image_views(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkCommandBuffer begin_single_time_command();
    void end_single_time_command(VkCommandBuffer vkCommandBuffer);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout odlLayout, VkImageLayout newLayout);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // texture sampler
    VkSampler textureSampler;
    void create_texture_sampler();

    // depth buffering
    VkImage  depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    void create_depth_resources();
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling imageTiling, VkFormatFeatureFlags featureFlags);
    VkFormat find_depth_format();
    constexpr bool has_stencil_component(VkFormat format);

    // resizes handle
    bool frameBufferResized = false;
    static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void *pUserData
    );

public:

    void run();
};


#endif //FAIR_ENGINE_APP_H
