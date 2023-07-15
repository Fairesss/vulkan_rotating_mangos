//
// Created by fair on 12/07/23.
//

#ifndef FAIR_ENGINE_SWAPCHAIN_H
#define FAIR_ENGINE_SWAPCHAIN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <limits>
#include <functional>


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilitiesKhr;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentMode;
};

class SwapChain {
    VkSwapchainKHR swapchainKhr;
    std::vector<VkImage> images;

public:


    static VkSurfaceFormatKHR choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D choose_swapchain_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilitiesKhr) {
        if (capabilitiesKhr.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilitiesKhr.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width), static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
                actualExtent.width,
                capabilitiesKhr.minImageExtent.width,
                capabilitiesKhr.maxImageExtent.width);
        actualExtent.height = std::clamp(
                actualExtent.height,
                capabilitiesKhr.minImageExtent.height,
                capabilitiesKhr.maxImageExtent.height
                );

        return actualExtent;
    }

    static SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surfaceKhr) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surfaceKhr, &details.capabilitiesKhr);
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

    static void create_swapchain(GLFWwindow* window,VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceKhr) {
        SwapChainSupportDetails swapChainSupportDetails = SwapChain::query_swapchain_support(physicalDevice, surfaceKhr);
        VkSurfaceFormatKHR surfaceFormatKhr = choose_swapchain_format(swapChainSupportDetails.formats);
        VkPresentModeKHR presentModeKhr = choose_present_mode(swapChainSupportDetails.presentMode);
        VkExtent2D extent2D = choose_swapchain_extent(window, swapChainSupportDetails.capabilitiesKhr);

    }
};


#endif //FAIR_ENGINE_SWAPCHAIN_H
