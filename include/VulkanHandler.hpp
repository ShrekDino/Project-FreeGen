/**
 * @file VulkanHandler.hpp
 * @brief Core Vulkan rendering engine for FreeGen.
 *
 * Manages the complete Vulkan pipeline:
 * - Instance, surface, physical/logical device setup
 * - Swapchain creation with MAILBOX present mode (fallback to FIFO)
 * - Texture upload from CPU staging buffer
 * - Compute shader dispatch for FSR 1.0 or integer upscaling
 * - Graphics pipeline for full-screen quad rendering
 * - Synchronization with semaphores and fences
 *
 * The pipeline flow is:
 *   Staging Buffer → vkCmdCopyBufferToImage → Compute Shader (FSR/Passthrough)
 *   → Graphics Pipeline (sample upscaled texture) → Present
 */

#pragma once
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "ImGuiHandler.hpp"
#include <mutex>
#include <atomic>

class VulkanHandler {
public:
    VulkanHandler(SDL_Window* window);
    ~VulkanHandler();

    void Render(ImGuiHandler* imgui = nullptr);
    void UpdateTexture(void* data, uint32_t width, uint32_t height, uint32_t stride, uint32_t format);
    
    UpscaleSettings& GetUpscaleSettings() { return m_upscaleSettings; }
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    uint32_t GetQueueFamily() const { return m_graphicsQueueFamily; }

private:
    SDL_Window* m_window;
    VkInstance m_instance;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    uint32_t m_graphicsQueueFamily = 0;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImageView> m_swapchainImageViews;

    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;
    VkFence m_inFlightFence;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t m_currentFrame = 0;

    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;
    VkFormat m_textureFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;

    UpscaleSettings m_upscaleSettings;
    UpscaleSettings::Mode m_currentUpscaleMode = UpscaleSettings::Off;

    // Compute pipeline members
    VkPipelineLayout m_computePipelineLayout;
    VkPipeline m_computePipeline;
    VkPipeline m_passthroughPipeline;
    VkDescriptorSetLayout m_computeDescriptorSetLayout;
    VkDescriptorPool m_computeDescriptorPool;
    VkDescriptorSet m_computeDescriptorSet;

    // Upscaled output image
    VkImage m_upscaledImage;
    VkDeviceMemory m_upscaledDeviceMemory;
    VkImageView m_upscaledImageView;
    VkImageLayout m_upscaledImageLayout = VK_IMAGE_LAYOUT_GENERAL;

    struct ComputeUpscaleParams {
        float rcpCon[4];   // (1.0f, 1.0f, inputWidth, inputHeight)
        float con2[4];     // (outputWidth, outputHeight, 0.0f, 0.0f)
        float sharpen;     // Sharpness factor
        float padding[3];  // Align to 16 bytes
    };

    VkBuffer m_stagingBuffer;
    VkDeviceMemory m_stagingBufferMemory;
    void* m_mappedData = nullptr;
    std::mutex m_stagingMutex;
    std::atomic<bool> m_frameDirty{false};
    uint32_t m_texWidth = 0;
    uint32_t m_texHeight = 0;
    uint32_t m_currTexWidth = 0;
    uint32_t m_currTexHeight = 0;

    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    // Compute pipeline functions
    void CreateComputePipeline();
    void CreateUpscaledTexture();
    void CreateComputeDescriptorPool();
    void AllocateComputeDescriptorSet();
    void UpdateDescriptorSets();

    void RecordCommandBuffer(uint32_t imageIndex, ImGuiHandler* imgui = nullptr);

    void RecreateTexture(uint32_t width, uint32_t height);

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
