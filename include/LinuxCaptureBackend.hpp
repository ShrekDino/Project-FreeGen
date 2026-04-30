#pragma once
#include "ICaptureBackend.hpp"
#include "PortalHandler.hpp"
#include "PipeWireHandler.hpp"
#include <thread>
#include <memory>

/**
 * @brief Linux-specific capture backend using PipeWire and XDG Portals.
 */
class LinuxCaptureBackend : public ICaptureBackend {
public:
    LinuxCaptureBackend();
    ~LinuxCaptureBackend() override;

    bool Initialize() override;
    void Start() override;
    void SetFrameCallback(FrameCallback cb) override;

private:
    void PipeWireThread();

    std::unique_ptr<PortalHandler> m_portal;
    std::unique_ptr<PipeWireHandler> m_pipewire;
    std::thread m_thread;
    FrameCallback m_frameCallback;
    uint32_t m_nodeId = 0;
};
