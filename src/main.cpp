/**
 * @file main.cpp
 * @brief Entry point for Project-FreeGen.
 * 
 * Orchestrates the XDG Portal request, launches the PipeWire background thread,
 * and manages the SDL2 window and Vulkan rendering loop.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <pipewire/pipewire.h>
#include <iostream>
#include <cstdlib>
#include "PortalHandler.hpp"
#include "PipeWireHandler.hpp"
#include "VulkanHandler.hpp"
#include "LinuxCaptureBackend.hpp"

// Global flag to control the application lifecycle
std::atomic<bool> g_running{true};
VulkanHandler* g_vulkan = nullptr;

int main(int argc, char* argv[]) {
    // Initialize PipeWire environment
    const char* xdg_dir = std::getenv("XDG_RUNTIME_DIR");
    if (xdg_dir) {
        setenv("PIPEWIRE_RUNTIME_DIR", xdg_dir, 1);
        std::cout << "📂 PipeWire Runtime Dir: " << xdg_dir << std::endl;
    }
    pw_init(&argc, &argv);

    // STEP 1: Initialize SDL2 and create a Vulkan-ready window
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "Project FreeGen",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    // STEP 2: Initialize the Vulkan Rendering Engine
    try {
        g_vulkan = new VulkanHandler(window);
    } catch (const std::exception& e) {
        std::cerr << "Vulkan Init Failed: " << e.what() << std::endl;
        return 1;
    }

    // STEP 3: Initialize the Capture Backend
    LinuxCaptureBackend capture;
    if (!capture.Initialize()) {
        std::cerr << "❌ Capture initialization failed!" << std::endl;
        return 1;
    }

    // STEP 4: Initialize Dear ImGui
    ImGuiHandler imgui(window, g_vulkan->GetInstance(), g_vulkan->GetPhysicalDevice(), g_vulkan->GetDevice(), g_vulkan->GetGraphicsQueue(), g_vulkan->GetRenderPass(), g_vulkan->GetQueueFamily());

    capture.SetFrameCallback([&imgui](const CaptureFrame& frame) {
        if (g_vulkan) {
            g_vulkan->UpdateTexture(frame.data, frame.width, frame.height, frame.stride, frame.format);
        }
    });

    // STEP 4: Launch the capture thread
    capture.Start();

    std::cout << "FreeGen: Systems Initialized." << std::endl;

    // STEP 5: Main UI and Rendering Loop
    SDL_Event event;
    while (g_running) {
        while (SDL_PollEvent(&event)) {
            imgui.HandleSDLEvent(event);
            if (event.type == SDL_QUIT) g_running = false;
        }
        
        if (g_vulkan) {
            // Render the current frame (including upscaling/processing)
            g_vulkan->Render(&imgui);
        }
        
        // Target ~60 FPS - handled by Vulkan present mode (MAILBOX/FIFO)
    }

    // Cleanup
    g_running = false;
    delete g_vulkan;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
