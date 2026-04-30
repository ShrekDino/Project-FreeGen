/**
 * @file PortalHandler.hpp
 * @brief Handles XDG Desktop Portal requests for Screen Casting.
 * 
 * This class uses D-Bus to communicate with the XDG Portal. It requests
 * permission from the user to capture a screen or window and retrieves
 * the PipeWire Node ID needed to start the capture stream.
 */

#pragma once
#include <string>
#include <dbus/dbus.h>
#include <cstdint>

class PortalHandler {
public:
    PortalHandler();
    ~PortalHandler();

    /**
     * @brief Initiates the ScreenCast portal sequence.
     * 1. CreateSession: Establishes a portal session.
     * 2. SelectSources: Prompts the user to pick a screen/window.
     * 3. Start: Activates the stream and gets the PipeWire Node ID.
     * @return true if the sequence completed and we have a node ID.
     */
    bool RequestScreenCast();

    /**
     * @return The PipeWire Node ID provided by the portal.
     */
    uint32_t GetPipeWireNode() const { return m_nodeId; }

private:
    DBusConnection* m_conn;
    uint32_t m_nodeId = 0;
};
