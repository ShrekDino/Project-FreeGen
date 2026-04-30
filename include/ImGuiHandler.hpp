#pragma once
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

/**
 * @file ImGuiHandler.hpp
 * @brief Manages the Dear ImGui overlay for settings and stats.
 */

struct UpscaleSettings {
    enum Mode {
        Off = 0,
        Integer = 1,
        FSR = 2
    };
    Mode upscaleMode = Off;
    float sharpness = 0.5f;
    int targetScale = 2; // For integer scaling (2x, 3x, etc.)
    bool showOverlay = true;
};

class ImGuiHandler {
public:
    ImGuiHandler(SDL_Window* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkRenderPass renderPass, uint32_t queueFamily);
    ~ImGuiHandler();

    void NewFrame();
    void Render(VkCommandBuffer cmd, VkFramebuffer framebuffer, uint32_t width, uint32_t height);

    UpscaleSettings& GetSettings() { return m_settings; }

    void HandleSDLEvent(const SDL_Event& event);

private:
    SDL_Window* m_window;
    UpscaleSettings m_settings;
};
