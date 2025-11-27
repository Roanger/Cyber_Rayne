#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <filesystem>

// Vertex structure for our sprites
struct Vertex {
    float position[2];  // x, y
    float texCoord[2];  // u, v
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDescription;
    }
    
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        
        // Position attribute
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);
        
        // Texture coordinate attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);
        
        return attributeDescriptions;
    }
};

#include <windows.h>
#include <string>
#include <set>
#include <algorithm>
#include <stb_image.h>

#include <optional>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>

struct UniformBufferObject {
    float model[16];
    float view[16];
    float proj[16];
};

// Structure to hold sprite transform data
struct SpriteTransform {
    float x;
    float y;
    float width;
    float height;
    int textureIndex;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    bool initialize(uint32_t width, uint32_t height, const std::string& title);
    void cleanup();
    void render();
    bool isRunning() const { return m_running; }
    void renderSprite(float x, float y, float width, float height);
    
    // New methods for texture management
    int loadTexture(const std::string& path);
    void setCurrentTexture(int textureIndex);
    void renderSpriteWithTexture(float x, float y, float width, float height, int textureIndex);
    // Convenience: render using pixel coordinates (top-left in pixels)
    void renderSpritePixelsWithTexture(int leftPx, int topPx, int widthPx, int heightPx, int textureIndex);
    // Accessor for current swapchain extent (used for pixel -> NDC conversions)
    VkExtent2D getSwapchainExtent() const { return m_swapChainExtent; }
    // Accessor for assets base directory detected at init
    const std::string& getAssetsBasePath() const { return m_assetsBasePath; }

private:
    const int MAX_FRAMES_IN_FLIGHT = 2;
    size_t m_currentFrame = 0;

    // Window variables
    HWND m_window;
    HINSTANCE m_hInstance;
    std::string m_windowTitle;
    uint32_t m_windowWidth;
    uint32_t m_windowHeight;
    bool m_running;

    // Vulkan variables
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    uint32_t m_graphicsQueueFamilyIndex;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    VkRenderPass m_renderPass;
    std::vector<VkFramebuffer> m_framebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    // Synchronization objects
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    std::vector<VkSemaphore> m_imageAvailableSemaphoresPerImage;
    std::vector<VkSemaphore> m_renderFinishedSemaphoresPerImage;

    // Texture image and memory
    // Multiple texture support
    struct Texture {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        int width;
        int height;
    };
    
    std::vector<Texture> m_textures;
    VkSampler m_textureSampler;
    int m_currentTextureIndex = 0; // Default texture index
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<VkDescriptorSet> m_textureDescriptorSets; // Separate descriptor sets for each texture
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
  // Remember where assets were found so others can reference
  std::string m_assetsBasePath;
    int m_spritesToRender;
    static const int MAX_SPRITES = 100; // Maximum number of sprites per frame
    std::vector<SpriteTransform> m_spriteTransforms; // Store transform data for each sprite

    // Private methods
    bool createWindow();
    bool createInstance();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSurface();
    bool createSwapChain();
    bool createImageViews();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    bool createTextureImage(const std::string& path);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkImageView createImageView(VkImage image, VkFormat format);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createTextureDescriptorSets();
    void createUniformBuffers();
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void updateUniformBuffer(uint32_t currentImage);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    bool createGraphicsPipeline();
    bool drawFrame();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void renderColoredRect(VkCommandBuffer commandBuffer, float x, float y, float width, float height, float r, float g, float b);

    // Helper methods
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device);
    uint32_t findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // Debug messenger helpers
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    // Static window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
