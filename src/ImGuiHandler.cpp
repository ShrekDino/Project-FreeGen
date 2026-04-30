/**
 * @file ImGuiHandler.cpp
 * @brief Initializes and manages the Dear ImGui overlay.
 *
 * Sets up the ImGui context with SDL2 + Vulkan backends, handles keyboard
 * events (TAB to toggle overlay), and renders the settings panel where users
 * can select upscaler mode (Off/Integer/FSR), adjust sharpness, and change
 * the integer scale factor.
 */

#include "ImGuiHandler.hpp"
#include <iostream>

ImGuiHandler::ImGuiHandler(SDL_Window* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkRenderPass renderPass, uint32_t queueFamily) 
    : m_window(window) 
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = queueFamily;
    init_info.Queue = queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPoolSize = 1000;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.PipelineInfoMain.RenderPass = renderPass;
    init_info.ImageCount = 2;
    init_info.MinImageCount = 2;
    ImGui_ImplVulkan_Init(&init_info);
}

ImGuiHandler::~ImGuiHandler() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiHandler::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Handle keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        m_settings.showOverlay = !m_settings.showOverlay;
    }

    if (m_settings.showOverlay) {
        ImGui::Begin("FreeGen Settings");
        
        const char* modes[] = {"Off", "Integer Scaling", "FSR 1.0"};
        ImGui::Combo("Upscaler", (int*)&m_settings.upscaleMode, modes, IM_ARRAYSIZE(modes));

        if (m_settings.upscaleMode == UpscaleSettings::FSR) {
            ImGui::SliderFloat("Sharpness", &m_settings.sharpness, 0.0f, 1.0f);
            ImGui::Text("Note: Lower value = sharper, Higher = smoother");
            // Sharpness is already 0=smooth, 1=sharp for FSR
        }

        if (m_settings.upscaleMode == UpscaleSettings::Integer) {
            ImGui::SliderInt("Scale Factor", &m_settings.targetScale, 2, 4);
        }

        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("TAB: Toggle Overlay");
        
        ImGui::End();
    }
}

void ImGuiHandler::Render(VkCommandBuffer cmd, VkFramebuffer framebuffer, uint32_t width, uint32_t height) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiHandler::HandleSDLEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}
