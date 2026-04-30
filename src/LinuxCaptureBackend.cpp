/**
 * @file LinuxCaptureBackend.cpp
 * @brief Linux-specific implementation of ICaptureBackend.
 *
 * Bridges the XDG Desktop Portal (for user permission) and PipeWire
 * (for video stream delivery) into a single capture backend.
 * Runs the PipeWire main loop on a background thread and converts
 * pw_buffer objects into CaptureFrame structs for the Vulkan handler.
 */

#include "LinuxCaptureBackend.hpp"
#include <iostream>

LinuxCaptureBackend::LinuxCaptureBackend() {
    m_portal = std::make_unique<PortalHandler>();
    m_pipewire = std::make_unique<PipeWireHandler>();
}

LinuxCaptureBackend::~LinuxCaptureBackend() {
    // In a real app, we'd signal the thread to stop and then join.
    if (m_thread.joinable()) {
        m_thread.detach();
    }
}

bool LinuxCaptureBackend::Initialize() {
    if (!m_portal->RequestScreenCast()) {
        std::cerr << "❌ Portal request failed!" << std::endl;
        return false;
    }
    m_nodeId = m_portal->GetPipeWireNode();
    std::cout << "✅ Captured Node ID: " << m_nodeId << std::endl;
    return true;
}

void LinuxCaptureBackend::Start() {
    m_thread = std::thread(&LinuxCaptureBackend::PipeWireThread, this);
}

void LinuxCaptureBackend::SetFrameCallback(FrameCallback cb) {
    m_frameCallback = std::move(cb);
}

void LinuxCaptureBackend::PipeWireThread() {
    m_pipewire->SetFrameCallback([this](struct pw_buffer* b) {
        if (m_frameCallback && b->buffer->datas[0].data) {
            uint32_t format_id = 0; // Default unknown
            // In a real scenario, we'd get this from negotiated params
            if (m_pipewire->GetFormat()) {
                format_id = m_pipewire->GetFormat()->format;
            }

            CaptureFrame frame{
                .data = b->buffer->datas[0].data,
                .width = m_pipewire->GetWidth(),
                .height = m_pipewire->GetHeight(),
                .stride = static_cast<uint32_t>(b->buffer->datas[0].chunk->stride),
                .format = format_id,
            };
            m_frameCallback(frame);
        }
    });

    if (m_pipewire->Connect(m_nodeId)) {
        m_pipewire->Run();
    }
}
