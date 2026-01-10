#define STB_IMAGE_IMPLEMENTATION
#include <array>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <optional>
#include <cstdint> // Necessary for UINT32_MAX
#include <filesystem>
#ifndef _WIN32
 #define GLFW_INCLUDE_VULKAN
 #include <GLFW/glfw3.h>
#endif
#include "../../include/VulkanRenderer.h"
#include <limits>

// Required extensions
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int m_maxFramesInFlight = 2;

// Helper function to find the assets directory
std::string findAssetsDirectory() {
    // Check common asset directory locations
    std::vector<std::string> possiblePaths = {
        "assets",
        "CyberRayne/assets",
        "CMakeProject1/assets",
        "../assets",
        "../CyberRayne/assets",
        "../../assets",
        "../../../assets"
    };
    
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            std::cout << "Found assets directory at: " << path << std::endl;
            return path;
        }
    }
    
    // If we can't find the assets directory, return the default path
    std::cout << "Warning: Could not find assets directory, using default path" << std::endl;
    return "assets";
}

// Helper function to find the shaders directory (expects compiled SPIR-V)
std::string findShadersDirectory() {
    std::vector<std::string> possiblePaths = {
        "shaders",                // when CWD is build dir and shaders are copied there
        "build/shaders",          // when CWD is repo root and shaders are copied next to executable
        "../build/shaders",       // when CWD is CyberRayne/ (source dir)
        "CyberRayne/shaders",     // when CWD is repo root and using source shaders
        "../shaders",             // CWD build/ -> project/shaders
        "CMakeProject1/shaders",  // CWD repo root
        "../CMakeProject1/shaders",
        "../../CMakeProject1/shaders"
    };
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            // Check for required files
            if (std::filesystem::exists(path + std::string("/vert.spv")) &&
                std::filesystem::exists(path + std::string("/frag.spv"))) {
                std::cout << "Found shaders directory at: " << path << std::endl;
                return path;
            }
        }
    }
    std::cout << "Warning: Could not find shaders directory with SPIR-V. Falling back to 'shaders' relative path." << std::endl;
    return "shaders";
}

// Convenience: pixel-based rendering helper. Positions use top-left pixel origin.
void VulkanRenderer::renderSpritePixelsWithTexture(int leftPx, int topPx, int widthPx, int heightPx, int textureIndex) {
    if (m_swapChainExtent.width == 0 || m_swapChainExtent.height == 0) {
        return;
    }
    // Convert pixel size to NDC scale. Our unit quad spans 1.0 in model space, so scale 2*px/extent.
    float ndcWidth  = 2.0f * static_cast<float>(widthPx)  / static_cast<float>(m_swapChainExtent.width);
    float ndcHeight = 2.0f * static_cast<float>(heightPx) / static_cast<float>(m_swapChainExtent.height);

    // Convert pixel top-left to NDC center position
    float centerXpx = static_cast<float>(leftPx) + static_cast<float>(widthPx) * 0.5f;
    float centerYpx = static_cast<float>(topPx)  + static_cast<float>(heightPx) * 0.5f;
    float ndcX = -1.0f + 2.0f * (centerXpx / static_cast<float>(m_swapChainExtent.width));
    float ndcY = -1.0f + 2.0f * (centerYpx / static_cast<float>(m_swapChainExtent.height));

    renderSpriteWithTexture(ndcX, ndcY, ndcWidth, ndcHeight, textureIndex);
}

// Debug messenger helpers
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        std::cerr << "Failed to set up debug messenger!" << std::endl;
    } else {
        std::cout << "Vulkan debug messenger created." << std::endl;
    }
}

VulkanRenderer::VulkanRenderer()
    : m_window(nullptr),
#ifdef _WIN32
      m_hInstance(GetModuleHandle(nullptr)),
#endif
      m_instance(VK_NULL_HANDLE),
      m_debugMessenger(VK_NULL_HANDLE),
      m_physicalDevice(VK_NULL_HANDLE),
      m_device(VK_NULL_HANDLE),
      m_graphicsQueue(VK_NULL_HANDLE),
      m_graphicsQueueFamilyIndex(UINT32_MAX),
      m_surface(VK_NULL_HANDLE),
      m_swapChain(VK_NULL_HANDLE),
      m_renderPass(VK_NULL_HANDLE),
      m_commandPool(VK_NULL_HANDLE),
      m_currentFrame(0),
      m_windowWidth(800),
      m_windowHeight(600),
      m_running(false),
      m_spritesToRender(0) {
    // Initialize vectors
    m_swapChainImages.resize(0);
    m_swapChainImageViews.resize(0);
    m_framebuffers.resize(0);
    m_commandBuffers.resize(0);
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(0);
    m_spriteTransforms.resize(MAX_SPRITES); // Initialize sprite transforms vector
}

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(uint32_t width, uint32_t height, const std::string& title) {
    std::cout << "Initializing Vulkan renderer..." << std::endl;
    
    try {
        this->m_windowWidth = width;
        this->m_windowHeight = height;
        this->m_windowTitle = title;
        this->m_running = true;

        std::cout << "Creating window..." << std::endl;
        if (!this->createWindow()) {
            std::cerr << "Failed to create window!" << std::endl;
            return false;
        }
        std::cout << "Window created successfully." << std::endl;

        if (!this->createInstance()) {
            std::cerr << "Failed to create Vulkan instance!" << std::endl;
            return false;
        }
        std::cout << "Vulkan instance created successfully." << std::endl;

        if (!this->createSurface()) {
            std::cerr << "Failed to create window surface!" << std::endl;
            return false;
        }
        std::cout << "Window surface created successfully." << std::endl;

        if (!this->pickPhysicalDevice()) {
            std::cerr << "Failed to pick suitable physical device!" << std::endl;
            return false;
        }
        std::cout << "Physical device picked successfully." << std::endl;

        if (!this->createLogicalDevice()) {
            std::cerr << "Failed to create logical device!" << std::endl;
            return false;
        }
        std::cout << "Logical device created successfully." << std::endl;

        if (!this->createSwapChain()) {
            std::cerr << "Failed to create swap chain!" << std::endl;
            return false;
        }
        std::cout << "Swap chain created successfully." << std::endl;

        if (!this->createImageViews()) {
            std::cerr << "Failed to create image views!" << std::endl;
            return false;
        }
        std::cout << "Image views created successfully." << std::endl;

        if (!this->createRenderPass()) {
            std::cerr << "Failed to create render pass!" << std::endl;
            return false;
        }
        std::cout << "Render pass created successfully." << std::endl;

        // Create descriptor set layout (used by the graphics pipeline)
        createDescriptorSetLayout();

        // Create command pool early because texture uploads use single-time command buffers
        if (!this->createCommandPool()) {
            std::cerr << "Failed to create command pool!" << std::endl;
            return false;
        }
        std::cout << "Command pool created successfully." << std::endl;

        // Create uniform buffers before allocating/writing descriptor sets
        createUniformBuffers();

        // Create texture sampler and load an initial texture before descriptor writes
        createTextureSampler();

        // Create a default fallback texture (1x1 white)
        createDefaultTexture();

        // Find the assets directory
        std::string assetsDir = findAssetsDirectory();
        m_assetsBasePath = assetsDir;
        
        // Load a texture (e.g., mage)
        std::string texturePath = assetsDir + "/mage.png";
        if (!createTextureImage(texturePath)) {
            std::cerr << "Failed to load mage texture from: " << texturePath << std::endl;
            std::cerr << "Continuing with default texture." << std::endl;
        }

        // Now that we have uniform buffers and a texture, create pool and write descriptor sets
        createDescriptorPool();
        createDescriptorSets();

        // Create graphics pipeline after descriptor set layout is ready
        if (!this->createGraphicsPipeline()) {
            std::cerr << "Failed to create graphics pipeline!" << std::endl;
            return false;
        }
        std::cout << "Graphics pipeline created successfully." << std::endl;

        // Create vertex and index buffers for rendering
        this->createVertexBuffer();
        this->createIndexBuffer();

        if (!this->createFramebuffers()) {
            std::cerr << "Failed to create framebuffers!" << std::endl;
            return false;
        }
        std::cout << "Framebuffers created successfully." << std::endl;

        if (!this->createCommandBuffers()) {
            std::cerr << "Failed to create command buffers!" << std::endl;
            return false;
        }
        std::cout << "Command buffers created successfully." << std::endl;

        if (!this->createSyncObjects()) {
            std::cerr << "Failed to create synchronization objects!" << std::endl;
            return false;
        }
        std::cout << "Synchronization objects created successfully." << std::endl;

        std::cout << "Vulkan renderer initialized successfully." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during Vulkan initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown exception during Vulkan initialization!" << std::endl;
        return false;
    }
}

void VulkanRenderer::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        // Wait for device to finish before cleanup
        vkDeviceWaitIdle(m_device);
    }

    // Cleanup in reverse order of creation
    // Cleanup synchronization objects
    if (m_device != VK_NULL_HANDLE) {
        const size_t frameCount = std::min<size_t>(MAX_FRAMES_IN_FLIGHT, std::min(m_renderFinishedSemaphores.size(), std::min(m_imageAvailableSemaphores.size(), m_inFlightFences.size())));
        for (size_t i = 0; i < frameCount; i++) {
            if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            if (m_inFlightFences[i] != VK_NULL_HANDLE) vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }
        m_renderFinishedSemaphores.clear();
        m_imageAvailableSemaphores.clear();
        m_inFlightFences.clear();
    }
    
    // Cleanup per-swapchain-image semaphores
    if (m_device != VK_NULL_HANDLE) {
        const size_t perImageCount = std::min(m_swapChainImages.size(), std::min(m_renderFinishedSemaphoresPerImage.size(), m_imageAvailableSemaphoresPerImage.size()));
        for (size_t i = 0; i < perImageCount; i++) {
            if (m_renderFinishedSemaphoresPerImage[i] != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_renderFinishedSemaphoresPerImage[i], nullptr);
            if (m_imageAvailableSemaphoresPerImage[i] != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_imageAvailableSemaphoresPerImage[i], nullptr);
        }
        m_renderFinishedSemaphoresPerImage.clear();
        m_imageAvailableSemaphoresPerImage.clear();
    }

    // Cleanup command buffers
    if (m_device != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE && !m_commandBuffers.empty()) {
        vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
    }

    // Cleanup command pool
    if (m_device != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    // Cleanup framebuffers
    if (m_device != VK_NULL_HANDLE) {
        for (auto framebuffer : m_framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();

    // Cleanup render pass
    if (m_device != VK_NULL_HANDLE && m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    // Cleanup image views
    if (m_device != VK_NULL_HANDLE) {
        for (auto imageView : m_swapChainImageViews) {
            if (imageView != VK_NULL_HANDLE) vkDestroyImageView(m_device, imageView, nullptr);
        }
    }
    m_swapChainImageViews.clear();

    // Cleanup swap chain
    if (m_device != VK_NULL_HANDLE && m_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }

    // Cleanup uniform buffers
    if (m_device != VK_NULL_HANDLE) {
        const size_t ubCount = std::min(m_uniformBuffers.size(), m_uniformBuffersMemory.size());
        for (size_t i = 0; i < ubCount; i++) {
            if (m_uniformBuffers[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
            if (m_uniformBuffersMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
        }
    }
    m_uniformBuffers.clear();
    m_uniformBuffersMemory.clear();
    
    // Cleanup descriptor pool
    // Cleanup graphics pipeline
    if (m_device != VK_NULL_HANDLE && m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    
    // Cleanup pipeline layout
    if (m_device != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    // Cleanup descriptor pool
    if (m_device != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    
    // Cleanup descriptor set layout
    if (m_device != VK_NULL_HANDLE && m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    // Cleanup index buffer
    if (m_device != VK_NULL_HANDLE && m_indexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    if (m_device != VK_NULL_HANDLE && m_indexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
    m_indexBufferMemory = VK_NULL_HANDLE;
    
    // Cleanup vertex buffer
    if (m_device != VK_NULL_HANDLE && m_vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    if (m_device != VK_NULL_HANDLE && m_vertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexBufferMemory = VK_NULL_HANDLE;
    
    // Cleanup texture sampler
    if (m_device != VK_NULL_HANDLE && m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }
    
    // Cleanup all textures
    if (m_device != VK_NULL_HANDLE) {
        for (const auto& texture : m_textures) {
            if (texture.view != VK_NULL_HANDLE) vkDestroyImageView(m_device, texture.view, nullptr);
            if (texture.image != VK_NULL_HANDLE) vkDestroyImage(m_device, texture.image, nullptr);
            if (texture.memory != VK_NULL_HANDLE) vkFreeMemory(m_device, texture.memory, nullptr);
        }
    }
    m_textures.clear();

    // Cleanup logical device
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    // Cleanup surface
    if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    // Destroy debug messenger if enabled
    if (enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }

    // Cleanup instance
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    // Destroy the window
    if (m_window) {
#ifndef _WIN32
        glfwDestroyWindow(m_window);
        glfwTerminate();
#else
        DestroyWindow(m_window);
#endif
    }

    m_running = false;
}

void VulkanRenderer::render() {
#ifndef _WIN32
    glfwPollEvents();
    if (m_window && glfwWindowShouldClose(m_window)) {
        m_running = false;
    }
#else
    // Handle window messages
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    drawFrame();
}

bool VulkanRenderer::drawFrame() {
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    // Use the per-frame semaphore for acquisition since we don't know the image index yet
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    // Add logging to trace imageIndex and vector sizes
    std::cout << "imageIndex: " << imageIndex << std::endl;
    std::cout << "m_imagesInFlight size: " << m_imagesInFlight.size() << std::endl;
    std::cout << "m_commandBuffers size: " << m_commandBuffers.size() << std::endl;

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swap chain is out of date, recreate it
        return true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire swap chain image!" << std::endl;
        return false;
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    vkResetCommandBuffer(m_commandBuffers[imageIndex], 0);
    try {
        recordCommandBuffer(m_commandBuffers[imageIndex], imageIndex);
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to record command buffer: " << e.what() << std::endl;
        return false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Use per-frame semaphore for wait (acquisition) but per-image semaphore for signal
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphoresPerImage[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult submitResult = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    if (submitResult != VK_SUCCESS) {
        std::cerr << "Failed to submit draw command buffer! Error code: " << submitResult << std::endl;
        return false;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swap chain is out of date or suboptimal, recreate it
        return true;
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present swap chain image!" << std::endl;
        return false;
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

bool VulkanRenderer::createWindow() {
    std::cout << "Creating window..." << std::endl;

#ifndef _WIN32
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(
        static_cast<int>(m_windowWidth),
        static_cast<int>(m_windowHeight),
        m_windowTitle.c_str(),
        nullptr,
        nullptr
    );

    if (!m_window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return false;
    }

    std::cout << "Window created successfully." << std::endl;
    return true;
#else
    // Register window class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "VulkanWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wc)) {
        std::cerr << "Failed to register window class!" << std::endl;
        return false;
    }

    // Calculate window size
    RECT windowRect = { 0, 0, static_cast<LONG>(m_windowWidth), static_cast<LONG>(m_windowHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create window
    m_window = CreateWindowEx(
        0,
        "VulkanWindowClass",
        m_windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (!m_window) {
        std::cerr << "Failed to create window!" << std::endl;
        return false;
    }

    // Show window
    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);

    std::cout << "Window created successfully." << std::endl;
    return true;
#endif
}

bool VulkanRenderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        std::cerr << "Validation layers requested, but not available!" << std::endl;
        return false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "FF9 Style JRPG";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        // Populate debug messenger create info and chain it to pNext
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // std::cout << "Calling vkCreateInstance..." << std::endl;
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
        return false;
    }

    std::cout << "Vulkan instance created successfully." << std::endl;
    if (enableValidationLayers) {
        setupDebugMessenger();
    }
    return true;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Use the first suitable device
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        return false;
    }

    std::cout << "Physical device picked successfully." << std::endl;
    return true;
}

uint32_t VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }

    return graphicsFamily;
}

bool VulkanRenderer::createLogicalDevice() {
    m_graphicsQueueFamilyIndex = findQueueFamilies(m_physicalDevice);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device!" << std::endl;
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

    std::cout << "Logical device created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createSurface() {
#ifndef _WIN32
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        std::cerr << "Failed to create window surface!" << std::endl;
        return false;
    }

    std::cout << "Window surface created successfully." << std::endl;
    return true;
#else
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = m_window;
    createInfo.hinstance = m_hInstance;

    if (vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS) {
        std::cerr << "Failed to create window surface!" << std::endl;
        return false;
    }

    std::cout << "Window surface created successfully." << std::endl;
    return true;
#endif
}

bool VulkanRenderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { m_graphicsQueueFamilyIndex };

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        std::cerr << "Failed to create swap chain!" << std::endl;
        return false;
    }

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    // Resize the images in flight vector
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

    std::cout << "Swap chain created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create image views!" << std::endl;
            return false;
        }
    }

    std::cout << "Image views created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        std::cerr << "Failed to create render pass!" << std::endl;
        return false;
    }

    std::cout << "Render pass created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    m_framebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer!" << std::endl;
            return false;
        }
    }

    std::cout << "Framebuffers created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create command pool!" << std::endl;
        return false;
    }

    std::cout << "Command pool created successfully." << std::endl;
    return true;
}

bool VulkanRenderer::createCommandBuffers() {
    m_commandBuffers.resize(m_framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        return false;
    }

    std::cout << "Command buffers created successfully." << std::endl;
    return true;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    // Add logging to trace framebuffer access
    std::cout << "Accessing framebuffer with imageIndex: " << imageIndex << std::endl;
    std::cout << "m_framebuffers size: " << m_framebuffers.size() << std::endl;
    renderPassInfo.framebuffer = m_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    VkClearValue clearColor = {{{1.0f, 0.0f, 0.0f, 1.0f}}};
    // Reduced logging - only log once per session
    static bool loggedClearColor = false;
    if (!loggedClearColor) {
        std::cout << "[DEBUG] Setting clear color to blue (R=0, G=0, B=1, A=1)" << std::endl;
        loggedClearColor = true;
    }
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainExtent.width;
    viewport.height = (float)m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // Bind index buffer
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // Only log sprite count if it's excessive (debugging)
    if (m_spritesToRender > 10) {
        std::cout << "[DEBUG] Processing " << m_spritesToRender << " sprites in command buffer" << std::endl;
    }

    // DEBUG: Log swapchain extent
    static bool loggedExtent = false;
    if (!loggedExtent) {
        std::cout << "[DEBUG] SwapChain Extent: " << m_swapChainExtent.width << "x" << m_swapChainExtent.height << std::endl;
        loggedExtent = true;
    }

    // Draw indexed - one draw call for each sprite using push constants
    int lastTextureIndex = -1;
    for (int i = 0; i < m_spritesToRender; i++) {
        SpriteTransform& transform = m_spriteTransforms[i];
        
        // Bind the appropriate texture descriptor set if texture changed
        int texIndex = transform.textureIndex;
        if (texIndex >= 0 && texIndex < static_cast<int>(m_textureDescriptorSets.size())) {
            if (texIndex != lastTextureIndex) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_textureDescriptorSets[texIndex], 0, nullptr);
                lastTextureIndex = texIndex;
            }
        } else {
            // Fall back to default descriptor set
            if (lastTextureIndex != 0) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
                lastTextureIndex = 0;
            }
        }
        
        // Push constants: x, y, width, height
        float pushData[4] = { transform.x, transform.y, transform.width, transform.height };
        vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushData), pushData);
        
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
    }

    // Reset sprite counter for next frame
    m_spritesToRender = 0;

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// This is a placeholder for actual sprite rendering
// In a real implementation, we would use vertex buffers, shaders, and textures
void VulkanRenderer::renderColoredRect(VkCommandBuffer commandBuffer, float x, float y, float width, float height, float r, float g, float b) {
    // For now, we're just drawing a solid color to the entire screen
    // In the future, we'll implement proper sprite rendering with vertex buffers and shaders
    // This would involve:
    // 1. Creating vertex buffers for the rectangle
    // 2. Setting up a graphics pipeline with shaders
    // 3. Binding the vertex buffer and pipeline
    // 4. Drawing the rectangle with vkCmdDraw
    // For now, we'll just change the clear color based on the position
    // This is just for demonstration purposes
    VkClearValue clearColor = { {{r, g, b, 1.0f}} };
    // Note: We can't actually change the clear color here as it's set in the render pass
    // This is just to show where actual rendering code would go
}

void VulkanRenderer::renderSpriteWithTexture(float x, float y, float width, float height, int textureIndex) {
    // Store the sprite transform data for later use in recordCommandBuffer
    if (m_spritesToRender < MAX_SPRITES) {
        SpriteTransform& transform = m_spriteTransforms[m_spritesToRender];
        transform.x = x;
        transform.y = y;
        transform.width = width;
        transform.height = height;
        transform.textureIndex = textureIndex;

        // Increment sprite counter
        m_spritesToRender++;
    } else {
        // Only warn once per frame to avoid spam
        static bool warnedThisFrame = false;
        if (!warnedThisFrame) {
            std::cerr << "Warning: Maximum number of sprites per frame exceeded!" << std::endl;
            warnedThisFrame = true;
        }
    }
}

void VulkanRenderer::renderSprite(float x, float y, float width, float height) {
    // Store the sprite transform data for later use in recordCommandBuffer
    if (m_spritesToRender < MAX_SPRITES) {
        SpriteTransform& transform = m_spriteTransforms[m_spritesToRender];
        transform.x = x;
        transform.y = y;
        transform.width = width;
        transform.height = height;
        
        // Increment sprite counter
        m_spritesToRender++;
    } else {
        std::cerr << "Warning: Maximum number of sprites per frame exceeded!" << std::endl;
    }
}

void VulkanRenderer::setCurrentTexture(int textureIndex) {
    if (textureIndex >= 0 && textureIndex < static_cast<int>(m_textures.size())) {
        m_currentTextureIndex = textureIndex;
    } else {
        std::cerr << "Invalid texture index: " << textureIndex << std::endl;
    }
}

int VulkanRenderer::loadTexture(const std::string& path) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cerr << "Failed to load texture image: " << path << std::endl;
        return -1; // Return -1 to indicate failure
    }

    std::cout << "Loaded texture: " << path << " (" << texWidth << "x" << texHeight << ")" << std::endl;
    
    // Create a staging buffer for the pixel data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy pixel data to the staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Create a new texture entry
    Texture newTexture{};
    newTexture.width = texWidth;
    newTexture.height = texHeight;
    
    // Create a Vulkan image for the texture
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.image, newTexture.memory);
    
    // Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy pixel data from staging buffer to texture image
    copyBufferToImage(stagingBuffer, newTexture.image, 
                     static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    
    // Transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Create image view for the texture
    newTexture.view = createImageView(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
    
    // Free staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    
    // Add the texture to our collection and return its index
    m_textures.push_back(newTexture);
    
    // Free the pixel data
    stbi_image_free(pixels);
    
    // Update descriptor sets for the new texture if they have been allocated already
    if (!m_descriptorSets.empty()) {
        createTextureDescriptorSets();
    }
    
    return static_cast<int>(m_textures.size() - 1);
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void VulkanRenderer::createVertexBuffer() {
    // Define vertices for a simple quad (two triangles)
    float vertices[] = {
        // Position    // Texture coordinates (flipped V so textures aren't upside down)
        -0.5f, -0.5f, 0.0f, 0.0f,  // Bottom left
         0.5f, -0.5f, 1.0f, 0.0f,  // Bottom right
         0.5f,  0.5f, 1.0f, 1.0f,  // Top right
        -0.5f,  0.5f, 0.0f, 1.0f   // Top left
    };
    
    VkDeviceSize bufferSize = sizeof(vertices);
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy vertex data to staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Create vertex buffer
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);
    
    // Copy staging buffer to vertex buffer
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
    
    // Cleanup staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createIndexBuffer() {
    // Define indices for a simple quad (two triangles)
    uint16_t indices[] = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };
    
    VkDeviceSize bufferSize = sizeof(indices);
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy index data to staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Create index buffer
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);
    
    // Copy staging buffer to index buffer
    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    
    // Cleanup staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createDescriptorSetLayout() {
    std::cout << "Creating descriptor set layout..." << std::endl;
    // Create descriptor set layout binding for uniform buffer
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
    
    // Create descriptor set layout binding for texture sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    
    // Create array of layout bindings
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    
    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    std::cout << "Descriptor set layout created." << std::endl;
}

void VulkanRenderer::createDescriptorPool() {
    std::cout << "Creating descriptor pool..." << std::endl;
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + 100); // Extra space for texture descriptor sets

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + 100); // Extra space for texture descriptor sets

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    std::cout << "Descriptor pool created." << std::endl;
}

void VulkanRenderer::createDescriptorSets() {
    std::cout << "Allocating and writing descriptor sets..." << std::endl;
    // Create descriptor sets for each frame
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    
    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Configure uniform buffer descriptor
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        // Configure image descriptor with the first texture as default
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (!m_textures.empty()) {
            imageInfo.imageView = m_textures[0].view;
        } else {
            imageInfo.imageView = VK_NULL_HANDLE; // Fallback
        }
        imageInfo.sampler = m_textureSampler;
        
        // Configure descriptor writes
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        
        // Update descriptor sets
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    std::cout << "Descriptor sets created and updated for " << MAX_FRAMES_IN_FLIGHT << " frames." << std::endl;
}

void VulkanRenderer::createTextureDescriptorSets() {
    // Do nothing if descriptor sets are not allocated yet
    if (m_descriptorSets.empty()) {
        return;
    }

    // Wait for device to be idle before recreating descriptor sets
    // This ensures no command buffers are still using the old descriptor sets
    vkDeviceWaitIdle(m_device);

    // Only create descriptor sets for textures that don't have them yet
    size_t existingSets = m_textureDescriptorSets.size();
    size_t newTextures = m_textures.size() - existingSets;

    if (newTextures == 0) {
        return; // No new textures to create descriptor sets for
    }

    // Resize to accommodate new textures
    m_textureDescriptorSets.resize(m_textures.size());

    std::vector<VkDescriptorSetLayout> layouts(newTextures, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(newTextures);
    allocInfo.pSetLayouts = layouts.data();

    // Allocate only the new descriptor sets
    std::vector<VkDescriptorSet> newSets(newTextures);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, newSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate texture descriptor sets!");
    }

    // Copy the new sets into the main vector
    for (size_t i = 0; i < newTextures; ++i) {
        m_textureDescriptorSets[existingSets + i] = newSets[i];
    }

    // Update each new texture descriptor set with both UBO and texture bindings
    for (size_t i = existingSets; i < m_textures.size(); ++i) {
        // Configure uniform buffer descriptor (same for all, using frame 0 as reference)
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[0];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textures[i].view;
        imageInfo.sampler = m_textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        
        // UBO binding
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_textureDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        
        // Texture binding
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_textureDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    std::cout << "Created " << newTextures << " new texture descriptor sets (total: " << m_textures.size() << ")." << std::endl;
    for (size_t i = existingSets; i < m_textureDescriptorSets.size(); ++i) {
        std::cout << "  Texture descriptor set " << i << ": " << m_textureDescriptorSets[i] << std::endl;
    }
}

std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
    std::cout << "Reading file: " << filename << std::endl;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        throw std::runtime_error("failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::cout << "File size: " << fileSize << " bytes" << std::endl;
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    std::cout << "File read successfully: " << filename << " (" << fileSize << " bytes)" << std::endl;
    return buffer;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    
    // Vulkan requires the bytecode to be stored in a uint32_t array
    // Since std::vector<char> ensures that the bytes are contiguous, we can cast the pointer
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    return shaderModule;
}

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    // For now, we'll just initialize the uniform buffer with identity matrices
    // In a real implementation, we would calculate the model, view, and projection matrices
    UniformBufferObject ubo{};
    
    // Initialize model matrix as identity
    for (int i = 0; i < 16; i++) {
        ubo.model[i] = (i % 5 == 0) ? 1.0f : 0.0f;  // Identity matrix
    }
    
    // Initialize view matrix as identity
    for (int i = 0; i < 16; i++) {
        ubo.view[i] = (i % 5 == 0) ? 1.0f : 0.0f;  // Identity matrix
    }
    
    // Initialize projection matrix as identity
    for (int i = 0; i < 16; i++) {
        ubo.proj[i] = (i % 5 == 0) ? 1.0f : 0.0f;  // Identity matrix
    }
    
    void* data;
    vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    
    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

bool VulkanRenderer::createGraphicsPipeline() {
    std::cout << "Creating graphics pipeline..." << std::endl;
    
    // Get current working directory
#ifdef _WIN32
    char cwd[1024];
    if (GetCurrentDirectoryA(1024, cwd)) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
#else
    std::cout << "Current working directory: " << std::filesystem::current_path().string() << std::endl;
#endif
    
    // Read shader code
    std::cout << "Reading shader code..." << std::endl;
    std::string shadersDir = findShadersDirectory();
    auto vertShaderCode = readFile(shadersDir + "/vert.spv");
    if (vertShaderCode.empty()) {
        std::cerr << "Vertex shader is empty!" << std::endl;
        return false;
    }
    auto fragShaderCode = readFile(shadersDir + "/frag.spv");
    if (fragShaderCode.empty()) {
        std::cerr << "Fragment shader is empty!" << std::endl;
        return false;
    }
    
    // Create shader modules
    std::cout << "Creating shader modules..." << std::endl;
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    std::cout << "Shader modules created successfully." << std::endl;
    
    // Add logging to trace the creation of the graphics pipeline
    std::cout << "Graphics pipeline creation:" << std::endl;
    std::cout << "  vertShaderModule: " << vertShaderModule << std::endl;
    std::cout << "  fragShaderModule: " << fragShaderModule << std::endl;
    
    // Create shader stage info
    std::cout << "Creating shader stage info..." << std::endl;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input
    std::cout << "Setting up vertex input..." << std::endl;
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    std::cout << "Setting up input assembly..." << std::endl;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    std::cout << "Setting up viewport and scissor..." << std::endl;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterization
    std::cout << "Setting up rasterization..." << std::endl;
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    std::cout << "Setting up multisampling..." << std::endl;
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    std::cout << "Setting up color blending..." << std::endl;
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic states
    std::cout << "Setting up dynamic states..." << std::endl;
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Pipeline layout
    std::cout << "Creating pipeline layout..." << std::endl;
    
    // Push constant range for per-sprite transform (x, y, width, height)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4; // x, y, width, height
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Graphics pipeline
    std::cout << "Creating graphics pipeline..." << std::endl;
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Cleanup shader modules
    std::cout << "Cleaning up shader modules..." << std::endl;
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    
    std::cout << "Graphics pipeline created successfully." << std::endl;
    std::cout << "Vulkan initialization completed successfully!" << std::endl;
    return true;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // We need a graphics queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    bool queueFamilyFound = false;
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyFound = true;
            break;
        }
    }

    if (!queueFamilyFound) {
        return false;
    }

    // We need to check if the device supports the swap chain
    if (!checkDeviceExtensionSupport(device)) {
        return false;
    }

    // We need to check if the device has a suitable surface format and present mode
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        return false;
    }

    return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { m_windowWidth, m_windowHeight };

        actualExtent.width = std::max<uint32_t>(capabilities.minImageExtent.width, std::min<uint32_t>(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max<uint32_t>(capabilities.minImageExtent.height, std::min<uint32_t>(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createSyncObjects() {
    // Resize the synchronization object vectors
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);
    
    // Create semaphores per swapchain image (new approach to fix validation errors)
    m_imageAvailableSemaphoresPerImage.resize(m_swapChainImages.size());
    m_renderFinishedSemaphoresPerImage.resize(m_swapChainImages.size());
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait
    
    // Create per-frame semaphores (for backward compatibility)
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            
            std::cerr << "Failed to create synchronization objects for frame " << i << std::endl;
            return false;
        }
    }
    
    // Create per-swapchain-image semaphores (to fix validation errors)
    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphoresPerImage[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphoresPerImage[i]) != VK_SUCCESS) {
            
            std::cerr << "Failed to create synchronization objects for swapchain image " << i << std::endl;
            return false;
        }
    }
    
    std::cout << "Created " << MAX_FRAMES_IN_FLIGHT << " sets of frame synchronization objects and "
              << m_swapChainImages.size() << " sets of per-image synchronization objects successfully." << std::endl;

    // Add logging to trace the creation of sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::cout << "Created sync objects for frame " << i << ":" << std::endl;
        std::cout << "  m_imageAvailableSemaphores[" << i << "]: " << m_imageAvailableSemaphores[i] << std::endl;
        std::cout << "  m_renderFinishedSemaphores[" << i << "]: " << m_renderFinishedSemaphores[i] << std::endl;
        std::cout << "  m_inFlightFences[" << i << "]: " << m_inFlightFences[i] << std::endl;
    }

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        std::cout << "Created sync objects for swapchain image " << i << ":" << std::endl;
        std::cout << "  m_imageAvailableSemaphoresPerImage[" << i << "]: " << m_imageAvailableSemaphoresPerImage[i] << std::endl;
        std::cout << "  m_renderFinishedSemaphoresPerImage[" << i << "]: " << m_renderFinishedSemaphoresPerImage[i] << std::endl;
    }

    return true;
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
    std::vector<const char*> extensions;

#ifndef _WIN32
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions && glfwExtensionCount > 0) {
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            extensions.push_back(glfwExtensions[i]);
        }
    }
#else
    // For Windows, we need the surface extension and Win32 surface extension
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

#ifdef _WIN32
LRESULT CALLBACK VulkanRenderer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    VulkanRenderer* renderer = nullptr;

    if (uMsg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        renderer = static_cast<VulkanRenderer*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(renderer));
    } else {
        renderer = reinterpret_cast<VulkanRenderer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (renderer) {
        switch (uMsg) {
            case WM_DESTROY:
                renderer->m_running = false;
                PostQuitMessage(0);
                return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

bool VulkanRenderer::createTextureImage(const std::string& path) {
    std::cout << "Attempting to load texture: " << path << std::endl;

    // Load image using stb_image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cerr << "Failed to load texture image: " << path << std::endl;
        std::cerr << "  STB error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "Successfully loaded texture: " << path << " (" << texWidth << "x" << texHeight << ", " << texChannels << " channels)" << std::endl;
    
    // Create a staging buffer for the pixel data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy pixel data to the staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Create a new texture entry
    Texture newTexture{};
    newTexture.width = texWidth;
    newTexture.height = texHeight;
    
    // Create a Vulkan image for the texture
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.image, newTexture.memory);
    
    // Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy pixel data from staging buffer to texture image
    copyBufferToImage(stagingBuffer, newTexture.image, 
                     static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    
    // Transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Create image view for the texture
    newTexture.view = createImageView(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
    
    // Free staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    
    // Add the texture to our collection and return its index
    m_textures.push_back(newTexture);
    
    // Free the pixel data
    stbi_image_free(pixels);
    
    // Update descriptor sets for the new texture
    createTextureDescriptorSets();
    
    return true;
}

void VulkanRenderer::createDefaultTexture() {
    std::cout << "Creating default white texture..." << std::endl;
    
    // 1x1 white pixel
    uint32_t texWidth = 1;
    uint32_t texHeight = 1;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    unsigned char pixels[] = { 255, 255, 255, 255 };
    
    // Create a staging buffer for the pixel data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy pixel data to the staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Create a new texture entry
    Texture newTexture{};
    newTexture.width = texWidth;
    newTexture.height = texHeight;
    
    // Create a Vulkan image for the texture
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.image, newTexture.memory);
    
    // Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy pixel data from staging buffer to texture image
    copyBufferToImage(stagingBuffer, newTexture.image, 
                     static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    
    // Transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Create image view for the texture
    newTexture.view = createImageView(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
    
    // Free staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    
    // Add the texture to our collection
    m_textures.push_back(newTexture);
    
    std::cout << "Default white texture created at index " << (m_textures.size() - 1) << std::endl;
}
