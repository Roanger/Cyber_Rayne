#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

int main() {
    std::cout << "Minimal Vulkan Test" << std::endl;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Check VULKAN_SDK env var
    char* sdkPath = nullptr;
    size_t len = 0;
    if (_dupenv_s(&sdkPath, &len, "VULKAN_SDK") == 0 && sdkPath != nullptr) {
        std::cout << "VULKAN_SDK: " << sdkPath << std::endl;
        free(sdkPath);
    } else {
        std::cout << "VULKAN_SDK environment variable not set!" << std::endl;
    }

    // Enumerate layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available Layers (" << layerCount << "):" << std::endl;
    for (const auto& layer : availableLayers) {
        std::cout << "\t" << layer.layerName << std::endl;
    }

    // Enumerate extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "Available Extensions (" << extensionCount << "):" << std::endl;
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.enabledExtensionCount = 0;

    VkInstance instance;
    std::cout << "Calling vkCreateInstance..." << std::endl;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result == VK_SUCCESS) {
        std::cout << "vkCreateInstance succeeded!" << std::endl;
        vkDestroyInstance(instance, nullptr);
        return 0;
    } else {
        std::cerr << "vkCreateInstance failed with error code: " << result << std::endl;
        return 1;
    }
}
