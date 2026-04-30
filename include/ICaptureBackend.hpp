#pragma once
#include <cstdint>
#include <functional>

/**
 * @brief Structure representing a single captured frame.
 */
struct CaptureFrame {
    void* data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format; // 11 = RGBA, 12 = BGRA
};

/**
 * @brief Interface for various screen capture backends.
 * This allows us to swap between PipeWire (Linux) and DXGI (Windows) easily.
 */
class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    
    /**
     * @brief Perform initial setup (D-Bus, Portal, etc.)
     */
    virtual bool Initialize() = 0;
    
    /**
     * @brief Start the capture loop (usually in a background thread)
     */
    virtual void Start() = 0;
    
    using FrameCallback = std::function<void(const CaptureFrame&)>;
    virtual void SetFrameCallback(FrameCallback cb) = 0;
};
