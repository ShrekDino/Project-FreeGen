/**
 * @file PipeWireHandler.hpp
 * @brief Handles the PipeWire stream for screen capture.
 * 
 * This class manages the connection to the PipeWire daemon, negotiates
 * stream parameters (like resolution and format), and provides a callback
 * mechanism for processing incoming video frames.
 */

#pragma once
#include <pipewire/pipewire.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <spa/param/video/format.h>

class PipeWireHandler {
public:
    PipeWireHandler();
    ~PipeWireHandler();

    /**
     * @brief Connects to a PipeWire node for capture.
     * @param nodeId The ID of the node to connect to (usually obtained from XDG Portal).
     * @return true if connection was successful.
     */
    bool Connect(uint32_t nodeId);

    /**
     * @brief Starts the PipeWire main loop. 
     * This blocks the calling thread, so it should be run in a background thread.
     */
    void Run();

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const struct spa_video_info_raw* GetFormat() const { return &m_format; }

    using FrameCallback = std::function<void(struct pw_buffer*)>;
    /**
     * @brief Sets the function to be called when a new frame is ready.
     * @param cb The callback function.
     */
    void SetFrameCallback(FrameCallback cb) { m_frameCallback = cb; }

    static void OnStreamProcess(void* data);
    static void OnStreamParamChanged(void* data, uint32_t id, const struct spa_pod* param);
    static void OnStreamStateChanged(void* data, enum pw_stream_state old, enum pw_stream_state state, const char* error);

private:
    struct pw_main_loop* m_loop;
    struct pw_context* m_context;
    struct pw_core* m_core;
    struct pw_stream* m_stream;
    struct spa_hook m_stream_listener;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    struct spa_video_info_raw m_format {};

    FrameCallback m_frameCallback;
};
