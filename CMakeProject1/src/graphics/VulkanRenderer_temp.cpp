
bool VulkanRenderer::createTextureImage(const std::string& path) {
    // Find the assets directory
    std::string assetsDir = findAssetsDirectory();
    std::string fullPath = assetsDir + "/" + path;
    
    // Check if file exists
    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "Texture file not found: " << fullPath << std::endl;
        return false;
    }
    
    // Load image using stb_image
    int width, height, channels;
    stbi_uc* pixels = stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        std::cerr << "Failed to load texture image: " << fullPath << std::endl;
        return false;
    }
    
    VkDeviceSize imageSize = width * height * 4; // 4 bytes per pixel (RGBA)
    
    // Create a new texture
    Texture newTexture{};
    newTexture.width = width;
    newTexture.height = height;
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                stagingBuffer, stagingBufferMemory);
    
    // Copy pixel data to staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Free the pixel data
    stbi_image_free(pixels);
    
    // Create image
    createImage(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 
               VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.image, newTexture.memory);
    
    // Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy buffer to image
    copyBufferToImage(stagingBuffer, newTexture.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    
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
    
    // Update descriptor sets for the new texture
    createTextureDescriptorSets();
    
    return true;
}
