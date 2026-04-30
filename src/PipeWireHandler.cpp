/**
 * @file PipeWireHandler.cpp
 * @brief Manages the PipeWire stream lifecycle for screen capture.
 *
 * Creates a PipeWire main loop, connects to a stream on a specific Node ID,
 * negotiates video formats (RGBA/BGRA), maps MemFd buffers to memory, and
 * delivers frames to the registered callback. Handles both MemFd and direct
 * memory buffer types.
 */

#include "PipeWireHandler.hpp"
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

/**
 * PipeWire Stream Events
 * These callbacks are triggered by PipeWire during the stream lifecycle.
 */
static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = PipeWireHandler::OnStreamStateChanged,
    .control_info = nullptr,
    .param_changed = PipeWireHandler::OnStreamParamChanged,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = PipeWireHandler::OnStreamProcess,
    .trigger_done = nullptr,
};

void PipeWireHandler::OnStreamStateChanged(void* data, enum pw_stream_state old, enum pw_stream_state state, const char* error) {
    std::cout << "🔄 PipeWire Stream State: " << pw_stream_state_as_string(old) << " -> " << pw_stream_state_as_string(state) << std::endl;
    if (error) {
        std::cerr << "❌ PipeWire Stream Error: " << error << std::endl;
    }
}

PipeWireHandler::PipeWireHandler() : m_loop(nullptr), m_context(nullptr), m_core(nullptr), m_stream(nullptr) {
    // Initialize PipeWire structures
    m_loop = pw_main_loop_new(nullptr);
    m_context = pw_context_new(pw_main_loop_get_loop(m_loop), nullptr, 0);
    m_core = pw_context_connect(m_context, nullptr, 0);

    if (!m_core) {
        std::cerr << "❌ Failed to connect to PipeWire core" << std::endl;
    }
}

PipeWireHandler::~PipeWireHandler() {
    if (m_stream) pw_stream_destroy(m_stream);
    if (m_core) pw_core_disconnect(m_core);
    if (m_context) pw_context_destroy(m_context);
    if (m_loop) pw_main_loop_destroy(m_loop);
}

bool PipeWireHandler::Connect(uint32_t nodeId) {
    if (!m_core) return false;

    // Create a new stream for input (capture)
    m_stream = pw_stream_new(m_core, "FreeGen Stream",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Video",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "DSP",
            NULL));

    if (!m_stream) return false;

    pw_stream_add_listener(m_stream, &m_stream_listener, &stream_events, this);

    // Negotiate multiple video formats (RGBA and BGRA)
    uint8_t buffer[2048];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod* params[2];
    
    struct spa_video_info_raw rgba_info = SPA_VIDEO_INFO_RAW_INIT(.format = SPA_VIDEO_FORMAT_RGBA);
    params[0] = spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &rgba_info);
    
    struct spa_video_info_raw bgra_info = SPA_VIDEO_INFO_RAW_INIT(.format = SPA_VIDEO_FORMAT_BGRA);
    params[1] = spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &bgra_info);

    std::cout << "📡 PipeWire: Connecting to node " << nodeId << " with RGBA/BGRA options..." << std::endl;

    // Connect the stream to the specified Node ID
    int res = pw_stream_connect(m_stream,
        PW_DIRECTION_INPUT,
        nodeId,
        (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params, 2);

    if (res < 0) {
        std::cerr << "❌ Failed to connect stream: " << spa_strerror(res) << std::endl;
        return false;
    }

    return true;
}

void PipeWireHandler::Run() {
    std::cout << "🚀 PipeWire Thread: Main loop starting..." << std::endl;
    if (m_loop) pw_main_loop_run(m_loop);
    std::cout << "⚠️ PipeWire Thread: Main loop exited!" << std::endl;
}

/**
 * Called by PipeWire whenever a new frame buffer is available.
 */
void PipeWireHandler::OnStreamProcess(void* data) {
    PipeWireHandler* self = static_cast<PipeWireHandler*>(data);
    struct pw_buffer* b;

    // Dequeue the buffer from the stream
    if (!(b = pw_stream_dequeue_buffer(self->m_stream))) {
        return;
    }

    struct spa_buffer* buf = b->buffer;
    
    // Support both direct memory pointers and MemFd (common on Steam Deck)
    if (buf->datas[0].type == SPA_DATA_MemFd) {
        if (buf->datas[0].data == nullptr) {
            // Map the file descriptor to a memory pointer if not already mapped
            buf->datas[0].data = mmap(NULL, buf->datas[0].maxsize, PROT_READ, MAP_SHARED, buf->datas[0].fd, buf->datas[0].mapoffset);
            if (buf->datas[0].data == MAP_FAILED) {
                std::cerr << "❌ Failed to mmap MemFd buffer!" << std::endl;
                buf->datas[0].data = nullptr;
            } else {
                static bool mapped_once = false;
                if (!mapped_once) {
                    std::cout << "🗺️ Successfully mapped MemFd buffer of size " << buf->datas[0].maxsize << std::endl;
                    mapped_once = true;
                }
            }
        }
    } else if (buf->datas[0].type == SPA_DATA_DmaBuf) {
        static bool dmabuf_warned = false;
        if (!dmabuf_warned) {
             std::cerr << "⚠️ Received DMA-BUF - Direct GPU import not yet implemented, will be black!" << std::endl;
             dmabuf_warned = true;
        }
    }

    // Trigger the registered frame callback
    if (self->m_frameCallback && buf->datas[0].data) {
        self->m_frameCallback(b);
    }

    // Re-queue the buffer for the next frame
    pw_stream_queue_buffer(self->m_stream, b);
}

/**
 * Called when the stream format is negotiated or changed by the compositor.
 */
void PipeWireHandler::OnStreamParamChanged(void* data, uint32_t id, const struct spa_pod* param) {
    PipeWireHandler* self = static_cast<PipeWireHandler*>(data);
    if (param == NULL || id != SPA_PARAM_Format) return;

    if (spa_format_video_raw_parse(param, &self->m_format) < 0) return;

    // Update captured resolution
    self->m_width = self->m_format.size.width;
    self->m_height = self->m_format.size.height;
    std::cout << "📺 Stream Format Changed: " << self->m_width << "x" << self->m_height 
              << " (Format ID: " << self->m_format.format << ")" << std::endl;
}
