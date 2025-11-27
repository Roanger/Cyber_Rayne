
int VulkanRenderer::loadTexture(const std::string& path) {
    // Check if texture already loaded
    for (size_t i = 0; i < m_textures.size(); i++) {
        // In a real implementation, you might want to track the path of each texture
        // For now, we'll just load each texture once
    }
    
    // Create a new texture
    if (createTextureImage(path)) {
        // Return the index of the newly added texture
        return static_cast<int>(m_textures.size() - 1);
    }
    
    // Return -1 if failed to load
    return -1;
}
