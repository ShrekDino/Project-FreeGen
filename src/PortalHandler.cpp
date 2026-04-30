/**
 * @file PortalHandler.cpp
 * @brief Handles XDG Desktop Portal communication via D-Bus.
 *
 * Implements the screen capture portal workflow:
 * 1. CreateSession - establishes a portal session
 * 2. SelectSources - lets user choose which screen/window to capture
 * 3. Start - begins the stream and retrieves the PipeWire Node ID
 *
 * Uses low-level D-Bus API to communicate with org.freedesktop.portal.Desktop.
 */

#include "PortalHandler.hpp"
#include <iostream>
#include <dbus/dbus.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

/**
 * @brief PortalHandler Constructor
 * Connects to the D-Bus session bus.
 */
PortalHandler::PortalHandler() : m_conn(nullptr), m_nodeId(0) {
    DBusError err;
    dbus_error_init(&err);

    m_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "❌ D-Bus Connection Error: " << err.message << std::endl;
        dbus_error_free(&err);
    }
}

PortalHandler::~PortalHandler() {
    if (m_conn) dbus_connection_unref(m_conn);
}

/**
 * @brief Helper to add a dictionary entry to a D-Bus message.
 */
void add_dict_entry(DBusMessageIter* dict, const char* key, const char* value) {
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

/**
 * @brief Executes the full XDG Portal ScreenCast request sequence.
 * 
 * This is a state-machine that waits for D-Bus responses to:
 * 1. Create a session.
 * 2. Prompt user to select a window/monitor.
 * 3. Start the stream and obtain the PipeWire node ID.
 */
bool PortalHandler::RequestScreenCast() {
    if (!m_conn) return false;

    // Use a unique token for this session
    std::string token = "freegen_" + std::to_string(getpid());
    std::cout << "🌐 Initializing Session with Token: " << token << std::endl;

    // STEP 1: CreateSession
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "CreateSession");

    DBusMessageIter iter, sub;
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &sub);
    add_dict_entry(&sub, "session_handle_token", token.c_str());
    dbus_message_iter_close_container(&iter, &sub);

    DBusPendingCall* pending;
    if (!dbus_connection_send_with_reply(m_conn, msg, &pending, -1)) return false;
    dbus_connection_flush(m_conn);
    dbus_message_unref(msg);

    std::cout << "📡 CreateSession sent. Waiting for Portal response..." << std::endl;

    bool session_created = false;
    bool sources_selected = false;
    bool started = false;
    std::string session_handle;

    // Main event loop to process D-Bus signals
    while (!started) {
        dbus_connection_read_write_dispatch(m_conn, 100);
        DBusMessage* incoming = dbus_connection_pop_message(m_conn);
        if (!incoming) continue;

        // Check if the signal is a portal response
        if (dbus_message_is_signal(incoming, "org.freedesktop.portal.Request", "Response")) {
            DBusMessageIter response_iter;
            dbus_message_iter_init(incoming, &response_iter);

            uint32_t response_code;
            dbus_message_iter_get_basic(&response_iter, &response_code);

            if (response_code != 0) {
                std::cerr << "❌ Portal Request Failed (code " << response_code << ")" << std::endl;
                dbus_message_unref(incoming);
                return false;
            }

            if (!session_created) {
                // Parse CreateSession response for the handle
                DBusMessageIter array_iter, dict_iter, variant_iter;
                if (dbus_message_iter_next(&response_iter)) {
                    dbus_message_iter_recurse(&response_iter, &array_iter);
                    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_DICT_ENTRY) {
                        dbus_message_iter_recurse(&array_iter, &dict_iter);
                        const char* key;
                        dbus_message_iter_get_basic(&dict_iter, &key);
                        if (std::string(key) == "session_handle") {
                            dbus_message_iter_next(&dict_iter);
                            dbus_message_iter_recurse(&dict_iter, &variant_iter);
                            const char* val;
                            dbus_message_iter_get_basic(&variant_iter, &val);
                            session_handle = val;
                            session_created = true;
                            std::cout << "✅ Session Created! Handle: " << session_handle << std::endl;
                        }
                        dbus_message_iter_next(&array_iter);
                    }
                }

                if (session_created) {
                    // STEP 2: SelectSources
                    std::cout << "🚀 Triggering SelectSources..." << std::endl;
                    DBusMessage* select_msg = dbus_message_new_method_call(
                        "org.freedesktop.portal.Desktop",
                        "/org/freedesktop/portal/desktop",
                        "org.freedesktop.portal.ScreenCast",
                        "SelectSources");

                    DBusMessageIter select_iter, select_sub;
                    dbus_message_iter_init_append(select_msg, &select_iter);
                    const char* handle_ptr = session_handle.c_str();
                    dbus_message_iter_append_basic(&select_iter, DBUS_TYPE_OBJECT_PATH, &handle_ptr);
                    dbus_message_iter_open_container(&select_iter, DBUS_TYPE_ARRAY, "{sv}", &select_sub);
                    
                    DBusMessageIter entry, variant;
                    const char* key_types = "types";
                    uint32_t val_types = 3; // Monitors and Windows
                    dbus_message_iter_open_container(&select_sub, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
                    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key_types);
                    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "u", &variant);
                    dbus_message_iter_append_basic(&variant, DBUS_TYPE_UINT32, &val_types);
                    dbus_message_iter_close_container(&entry, &variant);
                    dbus_message_iter_close_container(&select_sub, &entry);

                    const char* key_multiple = "multiple";
                    dbus_bool_t val_multiple = FALSE;
                    dbus_message_iter_open_container(&select_sub, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
                    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key_multiple);
                    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &variant);
                    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val_multiple);
                    dbus_message_iter_close_container(&entry, &variant);
                    dbus_message_iter_close_container(&select_sub, &entry);

                    dbus_message_iter_close_container(&select_iter, &select_sub);
                    dbus_connection_send(m_conn, select_msg, NULL);
                    dbus_connection_flush(m_conn);
                    dbus_message_unref(select_msg);
                    std::cout << "🖥️ SelectSources sent." << std::endl;
                }
            } else if (!sources_selected) {
                // STEP 3: Start
                sources_selected = true;
                std::cout << "✅ Sources Selected!" << std::endl;
                std::cout << "🚀 Triggering Start..." << std::endl;

                DBusMessage* start_msg = dbus_message_new_method_call(
                    "org.freedesktop.portal.Desktop",
                    "/org/freedesktop/portal/desktop",
                    "org.freedesktop.portal.ScreenCast",
                    "Start");

                DBusMessageIter start_iter, start_sub;
                dbus_message_iter_init_append(start_msg, &start_iter);
                const char* handle_ptr = session_handle.c_str();
                dbus_message_iter_append_basic(&start_iter, DBUS_TYPE_OBJECT_PATH, &handle_ptr);
                const char* parent_window = "";
                dbus_message_iter_append_basic(&start_iter, DBUS_TYPE_STRING, &parent_window);
                dbus_message_iter_open_container(&start_iter, DBUS_TYPE_ARRAY, "{sv}", &start_sub);
                dbus_message_iter_close_container(&start_iter, &start_sub);

                dbus_connection_send(m_conn, start_msg, NULL);
                dbus_connection_flush(m_conn);
                dbus_message_unref(start_msg);
                std::cout << "🎬 Start sent." << std::endl;
            } else if (!started) {
                // Parse Start response for PipeWire node ID
                DBusMessageIter array_iter, dict_iter, variant_iter, stream_array_iter, stream_struct_iter;
                if (dbus_message_iter_next(&response_iter)) {
                    dbus_message_iter_recurse(&response_iter, &array_iter);
                    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_DICT_ENTRY) {
                        dbus_message_iter_recurse(&array_iter, &dict_iter);
                        const char* key;
                        dbus_message_iter_get_basic(&dict_iter, &key);
                        if (std::string(key) == "streams") {
                            dbus_message_iter_next(&dict_iter);
                            dbus_message_iter_recurse(&dict_iter, &variant_iter);
                            dbus_message_iter_recurse(&variant_iter, &stream_array_iter);
                            if (dbus_message_iter_get_arg_type(&stream_array_iter) == DBUS_TYPE_STRUCT) {
                                dbus_message_iter_recurse(&stream_array_iter, &stream_struct_iter);
                                dbus_message_iter_get_basic(&stream_struct_iter, &m_nodeId);
                                started = true;
                                std::cout << "✅ Stream Started! PipeWire Node ID: " << m_nodeId << std::endl;
                            }
                        }
                        dbus_message_iter_next(&array_iter);
                    }
                }
            }
        }
        dbus_message_unref(incoming);
    }

    return started;
}
