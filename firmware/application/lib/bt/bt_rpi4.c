#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "bt_rpi4.h"
#include "../log.h"

#define BLUETOOTH_PROFILE_DEFAULT "org.bluez.Profile1"
#define BLUETOOTH_PROFILE_MANAGER_DEFAULT "org.bluez.ProfileManager1"

#define BLUEZ_BUS_NAME             "org.bluez"
#define ADAPTER_OBJECT_PATH        "/org/bluez/hci0"
#define DEVICE_PATH_PREFIX         "/org/bluez/hci0/dev_"
#define ADAPTER_INTERFACE          "org.bluez.Adapter1"
#define DEVICE_INTERFACE           "org.bluez.Device1"
#define MEDIAPLAYER_INTERFACE      "org.bluez.MediaPlayer1"
#define MEDIACONTROL_INTERFACE     "org.bluez.MediaControl1"
#define PROFILEMANAGER_INTERFACE   "org.bluez.Profile1"
#define PROPERTIES_CHANGED_SIGNAL  "PropertiesChanged"

#define A2DP_PROFILE_UUID "0000110A-0000-1000-8000-00805F9B34FB" //"0000110e-0000-1000-8000-00805f9b34fb"
#define HFP_PROFILE_UUID  "0000111E-0000-1000-8000-00805F9B34FB"
#define VRP_PROFILE_UUID  "0000111F-0000-1000-8000-00805F9B34FB"

char device_addr[256];
gboolean loop_exit = FALSE;
guint mediaplayer_changed;
guint mediacontrol_changed;

char* uint8_array_to_mac(const uint8_t mac_array[6]) {
    // Allocate memory for the MAC address string
    char *mac_str = malloc(18); // 17 characters for the MAC address + 1 for the null terminator
    if (mac_str == NULL) {
        return NULL; // Handle memory allocation failure
    }

    // Format the MAC address into the string
    snprintf(mac_str, 18, "%02X_%02X_%02X_%02X_%02X_%02X",
             mac_array[0], mac_array[1], mac_array[2],
             mac_array[3], mac_array[4], mac_array[5]);

    return mac_str;
}

void mac_to_uint8_array(const char *mac_str, uint8_t mac_array[6]) {
    // Use sscanf to parse the MAC address string
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_array[0], &mac_array[1], &mac_array[2],
           &mac_array[3], &mac_array[4], &mac_array[5]);
}

// Function to handle property values
void bluez_property_value(const gchar *key, GVariant *value) {
    const gchar *type = g_variant_get_type_string(value);

    LogDebug(LOG_SOURCE_BT, "\t%s : ", key);
    switch (*type) {
        case 'o': // Object path
        case 's': // String
            LogDebug(LOG_SOURCE_BT, "%s", g_variant_get_string(value, NULL));
            break;
        case 'b': // Boolean
            LogDebug(LOG_SOURCE_BT, "%d", g_variant_get_boolean(value));
            break;
        case 'u': // Unsigned integer
            LogDebug(LOG_SOURCE_BT, "%u", g_variant_get_uint32(value));
            break;
        case 'i': // Signed integer
            LogDebug(LOG_SOURCE_BT, "%d", g_variant_get_int32(value));
            break;
        case 'd': // Double
            LogDebug(LOG_SOURCE_BT, "%f", g_variant_get_double(value));
            break;
        case 'a': // Array
            if (g_strcmp0(type, "as") == 0) { // Array of strings
                LogDebug(LOG_SOURCE_BT, "Array of strings:");
                const gchar *str_value;
                GVariantIter i;
                g_variant_iter_init(&i, value);
                while (g_variant_iter_next(&i, "s", &str_value)) {
                    LogDebug(LOG_SOURCE_BT, "\t\t%s", str_value);
                }
            } else if (g_str_has_prefix(type, "a{")) { // Array of dictionaries
                LogDebug(LOG_SOURCE_BT, "Array of dictionaries:");
                GVariantIter dict_iter;
                g_variant_iter_init(&dict_iter, value);
                GVariant *dict_entry;

                while (g_variant_iter_next(&dict_iter, "a{sa{sv}}", &dict_entry)) {
                    LogDebug(LOG_SOURCE_BT, "\tArray entry parsing");
                    const gchar *dict_key;
                    GVariant *dict_value;
                    GVariantIter entry_iter;

                    g_variant_iter_init(&entry_iter, dict_entry);
                    while (g_variant_iter_next(&entry_iter, "{&sv}", &dict_key, &dict_value)) {
                        // Log the key and value of the dictionary
                        LogDebug(LOG_SOURCE_BT, "\t\tKey: %s, Value Type: %s", dict_key, g_variant_get_type_string(dict_value));

                        // Handle specific value types
                        if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_STRING)) {
                            const gchar *str_value = g_variant_get_string(dict_value, NULL);
                            LogDebug(LOG_SOURCE_BT, "Value: %s", str_value);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_BOOLEAN)) {
                            gboolean bool_value = g_variant_get_boolean(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %s", bool_value ? "true" : "false");
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_UINT32)) {
                            guint32 uint_value = g_variant_get_uint32(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %u", uint_value);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_INT32)) {
                            gint32 int_value = g_variant_get_int32(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %d", int_value);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_DOUBLE)) {
                            gdouble double_value = g_variant_get_double(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %f", double_value);
                        } else {
                            LogDebug(LOG_SOURCE_BT, "Value: Unknown type");
                        }
                    }
                }
            }
            break;
        default:
            LogDebug(LOG_SOURCE_BT, "Value: Unknown type\n");
            break;
    }
}

void on_properties_changed(GDBusConnection *connection,
                      const gchar *sender_name,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *signal_name,
                      GVariant *parameters,
                      gpointer user_data)
{
    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;
    BT_t *bt = (BT_t*)user_data;
    const gchar *type_string = g_variant_get_type_string(parameters);
    //LogDebug(LOG_SOURCE_BT, "Properties Changed - Parameters: %s", type_string);
    GVariantIter *properties_iter;
    const gchar *key;
    GVariant *value;
    g_variant_get(parameters, "(sa{sv}as)", NULL, &properties_iter, NULL);
    while (g_variant_iter_next(properties_iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "Status") == 0) {
            const char* status = g_variant_get_string(value, NULL);
            if (g_strcmp0(g_ascii_strdown(status, -1), "playing") == 0) {
                bt->playbackStatus = BT_AVRCP_STATUS_PLAYING;
                EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, 0);
            } else if (g_strcmp0(g_ascii_strdown(status, -1), "paused") == 0) {
                bt->playbackStatus = BT_AVRCP_STATUS_PAUSED;
                EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, 0);
            }
        } else if (g_strcmp0(key, "Track") == 0) {
            const gchar *type = g_variant_get_type_string(value);

            //LogDebug(LOG_SOURCE_BT, "\t%s - %s : ", key, type);
            
            // Unpack the GVariant
            GVariantIter iter;
            g_variant_iter_init(&iter, value);

            GVariant *track_value;
            const gchar *track_key;

            // Iterate over the array of key-value pairs
            while (g_variant_iter_next(&iter, "{sv}", &track_key, &track_value)) {
                // Print the key
                //LogDebug(LOG_SOURCE_BT, "Key: %s-\n", track_key);
                bt->metadataTimestamp = TimerGetMillis();
                if (g_strcmp0(track_key, "Title") == 0) {
                    char title[BT_METADATA_MAX_SIZE] = {0};
                    const char* g_title = g_variant_get_string(track_value, NULL);
                    UtilsNormalizeText(title, g_title, BT_METADATA_MAX_SIZE);
                    if (strncmp(bt->title, title, BT_METADATA_FIELD_SIZE - 1) != 0) {
                        bt->metadataStatus = BT_METADATA_STATUS_UPD;
                        memset(bt->title, 0, BT_METADATA_FIELD_SIZE);
                        memset(bt->artist, 0, BT_METADATA_FIELD_SIZE);
                        memset(bt->album, 0, BT_METADATA_FIELD_SIZE);
                        UtilsStrncpy(bt->title, title, BT_METADATA_FIELD_SIZE);
                    }
                } else if (g_strcmp0(track_key, "Artist") == 0) {
                    char artist[BT_METADATA_MAX_SIZE] = {0};
                    const char* g_artist = g_variant_get_string(track_value, NULL);
                    UtilsNormalizeText(artist, g_artist, BT_METADATA_MAX_SIZE);
                    if (strncmp(bt->artist, artist, BT_METADATA_FIELD_SIZE - 1) != 0) {
                        bt->metadataStatus = BT_METADATA_STATUS_UPD;
                        memset(bt->artist, 0, BT_METADATA_FIELD_SIZE);
                        UtilsStrncpy(bt->artist, artist, BT_METADATA_FIELD_SIZE);
                    }
                } else if (g_strcmp0(track_key, "Album") == 0) {
                    char album[BT_METADATA_MAX_SIZE] = {0};
                    const char* g_album = g_variant_get_string(track_value, NULL);
                    UtilsNormalizeText(album, g_album, BT_METADATA_MAX_SIZE);
                    if (strncmp(bt->album, album, BT_METADATA_FIELD_SIZE - 1) != 0) {
                        bt->metadataStatus = BT_METADATA_STATUS_UPD;
                        memset(bt->album, 0, BT_METADATA_FIELD_SIZE);
                        UtilsStrncpy(bt->album, album, BT_METADATA_FIELD_SIZE);
                    }
                }

                // Clean up the value if necessary
                g_variant_unref(track_value);
            }
            //LogError("BT: title=%s,artist=%s,album=%s - %u", bt->title, bt->artist, bt->album, bt->metadataStatus);
            if (bt->metadataStatus == BT_METADATA_STATUS_UPD) {
                LogDebug(
                    LOG_SOURCE_BT,
                    "BT: title=%s,artist=%s,album=%s",
                    bt->title,
                    bt->artist,
                    bt->album
                );
                EventTriggerCallback(BT_EVENT_METADATA_UPDATE, 0);
            }
            bt->metadataStatus = BT_METADATA_STATUS_CUR;
        } else {
            //bluez_property_value(key, value);
        }
        g_variant_unref(value);
    }
    g_variant_iter_free(properties_iter);
}

typedef void (*method_cb_t)(GObject *, GAsyncResult *, gpointer);
int bluez_adapter_call_method(GDBusConnection *conn, const char *method, GVariant *param, method_cb_t method_cb)
{
    GError *error = NULL;

    g_dbus_connection_call(conn,
                 BLUEZ_BUS_NAME,
                 ADAPTER_OBJECT_PATH,
                 ADAPTER_INTERFACE,
                 method,
                 param,
                 NULL,
                 G_DBUS_CALL_FLAGS_NONE,
                 -1,
                 NULL,
                 method_cb,
                 &error);
    if(error != NULL)
        return 1;
    return 0;
}

guint subscribe_to_mediaplayer_events(BT_t *bt, const char* playerPath) {
    LogDebug(LOG_SOURCE_BT, "Subscribed to MediaPlayer events");
    return g_dbus_connection_signal_subscribe(bt->connection,
            BLUEZ_BUS_NAME,
            "org.freedesktop.DBus.Properties",
            PROPERTIES_CHANGED_SIGNAL,
            (gchar*)playerPath,
            MEDIAPLAYER_INTERFACE,
            G_DBUS_SIGNAL_FLAGS_NONE,
            on_properties_changed,
            bt,
            NULL);
}

guint subscribe_to_mediacontrol_events(BT_t *bt, const char* playerPath) {
    LogDebug(LOG_SOURCE_BT, "Subscribed to MediaControl events");
    return g_dbus_connection_signal_subscribe(bt->connection,
            BLUEZ_BUS_NAME,
            "org.freedesktop.DBus.Properties",
            PROPERTIES_CHANGED_SIGNAL,
            (gchar*)playerPath,
            MEDIACONTROL_INTERFACE,
            G_DBUS_SIGNAL_FLAGS_NONE,
            on_properties_changed,
            bt,
            NULL);
}

static void bluez_device_appeared(GDBusConnection *sig,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    GVariant *properties;
    BT_t *bt = (BT_t*)user_data;

    const gchar *type_string = g_variant_get_type_string(parameters);
    //LogDebug(LOG_SOURCE_BT, "Received parameters type: %s", type_string);
    g_assert(g_str_equal(type_string, "(oa{sa{sv}})"));
    g_variant_get(parameters, "(oa{sa{sv}})", &object, &interfaces);
    LogDebug(LOG_SOURCE_BT, "Device Connected: %s", object);
    //bt->status = BT_STATUS_CONNECTED;
    //EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, 0);
    //type_string = g_variant_get_type_string(interfaces);
    //LogDebug(LOG_SOURCE_BT, "Received interfaces type: %s", type_string);
    while (g_variant_iter_next(interfaces, "{&s@a{sv}}", &interface_name, &properties))
    {
        LogDebug(LOG_SOURCE_BT, "Interface Added: %s", interface_name);
        if (g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "mediaplayer"))
        {
            mediaplayer_changed = subscribe_to_mediaplayer_events(bt, object);
            mediacontrol_changed = subscribe_to_mediacontrol_events(bt, object);
            uint8_t linkType = BT_LINK_TYPE_A2DP;
            EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED, &linkType);
            const gchar *property_name;
            GVariantIter i;
            GVariant *prop_val;
            g_variant_iter_init(&i, properties);
            while (g_variant_iter_next(&i, "{&sv}", &property_name, &prop_val))
            {
                bluez_property_value(property_name, prop_val);
            }

            g_variant_unref(prop_val);
        }
        g_variant_unref(properties);
    }
}

#define BT_ADDRESS_STRING_SIZE 18
static void bluez_device_disappeared(GDBusConnection *sig,
                const gchar *sender_name,
                const gchar *object_path,
                const gchar *interface,
                const gchar *signal_name,
                GVariant *parameters,
                gpointer user_data)
{
    BT_t *bt = (BT_t*)user_data;

    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    char address[BT_ADDRESS_STRING_SIZE] = {'\0'};

    g_variant_get(parameters, "(&oas)", &object, &interfaces);
    LogDebug(LOG_SOURCE_BT, "Device Disconnected: %s", object);
    while(g_variant_iter_next(interfaces, "s", &interface_name)) {
        if(g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "device")) {
            int i;
            char *tmp = g_strstr_len(object, -1, "dev_") + 4;

            for(i = 0; *tmp != '\0'; i++, tmp++) {
                if(*tmp == '_') {
                    address[i] = ':';
                    continue;
                }
                address[i] = *tmp;
            }
            LogDebug(LOG_SOURCE_BT, "\nDevice %s removed\n", address);
        }
    }
    return;
}

void RPI4CommandBtState(BT_t *bt, uint8_t connectable, uint8_t discoverable) {
    bt->connectable = connectable;
    bt->discoverable = discoverable;
}

void RPI4CommandCallAnswer(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Accept", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Call accepted.");
}

void RPI4CommandClose(BT_t *bt, uint8_t id) {
    bluez_adapter_call_method(bt->connection, "Disconnect", g_variant_new("", id), NULL);
    LogDebug(LOG_SOURCE_BT, "Disconnected from device with ID: %d.", id);
}

void RPI4CommandCallEnd(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "EndCall", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Call ended.");
}

void RPI4CommandDial(BT_t *bt, const char *number) {
    bluez_adapter_call_method(bt->connection, "Dial", g_variant_new("s", number), NULL);
    LogDebug(LOG_SOURCE_BT, "Dialing %s.", number);
}

void RPI4CommandRedial(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Redial", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Redialing last number.");
}

// Callback function to handle the result of the disconnection attempt
void disconnect_callback(GDBusConnection *connection, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    BT_t *bt = (BT_t*)user_data;

    // Finish the asynchronous call
    g_dbus_connection_call_finish(connection, res, &error);

    if (error) {
        bt->status = BT_STATUS_CONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, 0);
        LogError("Error disconnecting from device: %s\n", error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT, "CB::Device Disconnected: %s", bt->activeDevice.deviceName);
        bt->playbackStatus = BT_AVRCP_STATUS_PAUSED;
        // Notify the world that the device disconnected
        memset(&bt->activeDevice, 0, sizeof(BTConnection_t));
        bt->activeDevice = BTConnectionInit();
        bt->status = BT_STATUS_DISCONNECTED;
        EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, 0);
        EventTriggerCallback(BT_EVENT_DEVICE_LINK_DISCONNECTED, 0);
    }
}

// Function to disconnect from a connected Bluetooth device
void disconnect_from_device(BT_t *bt, uint8_t shutdown) {
    const char *device_address = uint8_array_to_mac(bt->activeDevice.macId);
    memcpy(device_addr, device_address, sizeof(device_addr));
    char device_path[256];
    snprintf(device_path, sizeof(device_path), "%s%s", DEVICE_PATH_PREFIX, device_addr);
    GError *error = NULL;
    if (bt->status == BT_STATUS_CONNECTED) {
        if (shutdown == 1) {
            g_dbus_connection_call_sync(
                bt->connection,
                BLUEZ_BUS_NAME,                  // Destination bus name
                device_path,                  // Object path
                DEVICE_INTERFACE,          // Interface name
                "Disconnect",                    // Method name
                NULL,                         // Input parameters (none)
                NULL,                         // Expected output parameters (none)
                G_DBUS_CALL_FLAGS_NONE,      // Flags
                -1,                           // Timeout
                NULL,                         // Cancellable
                &error                         // Error
            );
            
            if (error) {
                bt->status = BT_STATUS_CONNECTED;
                EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, 0);
                LogError("Error disconnecting from device: %s\n", error->message);
                g_error_free(error);
            } else {
                LogDebug(LOG_SOURCE_BT, "Device Disconnected: %s", bt->activeDevice.deviceName);
                bt->playbackStatus = BT_AVRCP_STATUS_PAUSED;
                // Notify the world that the device disconnected
                memset(&bt->activeDevice, 0, sizeof(BTConnection_t));
                bt->activeDevice = BTConnectionInit();
                bt->status = BT_STATUS_DISCONNECTED;
                EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, 0);
                EventTriggerCallback(BT_EVENT_DEVICE_LINK_DISCONNECTED, 0);
            }
        } else {
            g_dbus_connection_call(
                bt->connection,
                BLUEZ_BUS_NAME,                  // Destination bus name
                device_path,                  // Object path
                DEVICE_INTERFACE,          // Interface name
                "Disconnect",                    // Method name
                NULL,                         // Input parameters (none)
                NULL,                         // Expected output parameters (none)
                G_DBUS_CALL_FLAGS_NONE,      // Flags
                -1,                           // Timeout
                NULL,                         // Cancellable
                (GAsyncReadyCallback)disconnect_callback,             // Callback function
                (gpointer)bt      // User data (device address)
            );
        }
    }
}

void RPI4CommandDisconnect(BT_t *bt) {
    const char *mac_str = uint8_array_to_mac(bt->activeDevice.macId);
    LogDebug(LOG_SOURCE_BT, "Disconnecting from %s (%s).", bt->activeDevice.deviceName, mac_str);
    disconnect_from_device(bt, 0);
}

// Function to register a profile
void register_profile(BT_t *bt, const char *profile_uuid, uint8_t linkType) {
    GError *error = NULL;

    // Call the RegisterProfile method on the ProfileManager interface
    g_dbus_connection_call_sync(
        bt->connection,
        BLUEZ_BUS_NAME,                  // Destination bus name
        ADAPTER_OBJECT_PATH,            // Object path for the adapter
        PROFILEMANAGER_INTERFACE,   // Interface name
        "RegisterProfile",             // Method name
        g_variant_new("(ss)", profile_uuid, "Profile"), // Input parameters (UUID and options)
        NULL,                          // Expected output parameters (none)
        G_DBUS_CALL_FLAGS_NONE,        // Flags
        -1,                            // Timeout
        NULL,                          // Cancellable
        &error                         // Error
    );

    if (error) {
        LogError("Error registering profile %s: %s\n", profile_uuid, error->message);
        g_error_free(error);
    } else {
        EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED, &linkType);
        LogDebug(LOG_SOURCE_BT, "Successfully registered profile %s\n", profile_uuid);
    }
}


// Function to connect to a profile
void connect_profile(BT_t *bt, const char *profile_uuid) {
    char profile_path[256];
    snprintf(profile_path, sizeof(profile_path), "/org/bluez/hci0/dev_%s/profile_%s", device_addr, profile_uuid);

    GError *error = NULL;

    // Call the Connect method on the Profile1 interface
    g_dbus_connection_call_sync(
        bt->connection,
        BLUEZ_BUS_NAME,                  // Destination bus name
        profile_path,                 // Object path for the profile
        "org.bluez.Profile1",         // Interface name
        "NewConnection",                    // Method name
        NULL,                         // Input parameters (none)
        NULL,                         // Expected output parameters (none)
        G_DBUS_CALL_FLAGS_NONE,      // Flags
        -1,                           // Timeout
        NULL,                         // Cancellable
        &error                        // Error
    );

    if (error) {
        LogError("Error connecting to profile %s: %s\n", profile_path, error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT, "Successfully connected to profile %s for device %s\n", profile_uuid, device_addr);
    }
}

// Function to enable A2DP and HFP profiles
void enable_profiles(BT_t *bt) {
    // Register A2DP Profile
    uint8_t linkType = BT_LINK_TYPE_A2DP;
    //register_profile(bt, A2DP_PROFILE_UUID, linkType);
    connect_profile(bt, A2DP_PROFILE_UUID);

    // Register HFP Profile
    linkType = BT_LINK_TYPE_HFP;
    //register_profile(bt, HFP_PROFILE_UUID, linkType);
    connect_profile(bt, HFP_PROFILE_UUID);

    // Register VRP Profile
    linkType = BT_LINK_TYPE_AVRCP;
    //register_profile(bt, VRP_PROFILE_UUID, linkType);
    connect_profile(bt, VRP_PROFILE_UUID);
}

// Callback function to handle the result of the connection attempt
void connect_callback(GDBusConnection *connection, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    BT_t *bt = (BT_t*)user_data;

    // Finish the asynchronous call
    g_dbus_connection_call_finish(connection, res, &error);

    if (error) {
        bt->status = BT_STATUS_DISCONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_DISCONNECTED, 0);
        LogError("Error connecting to device: %s\n", error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT, "CB::Device Connected: %s", bt->activeDevice.deviceName);
        bt->status = BT_STATUS_CONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, 0);
        // Call enable_profiles after successful connection
        enable_profiles(bt);
    }
}

// Function to connect to a paired Bluetooth device
void connect_to_device(BT_t *bt, const char *device_address) {
    memcpy(device_addr, device_address, sizeof(device_addr));
    char device_path[256];
    snprintf(device_path, sizeof(device_path), "%s%s", DEVICE_PATH_PREFIX, device_addr);

    // Call the Connect method on the Device1 interface asynchronously
    g_dbus_connection_call(
        bt->connection,
        BLUEZ_BUS_NAME,                  // Destination bus name
        device_path,                  // Object path
        DEVICE_INTERFACE,          // Interface name
        "Connect",                    // Method name
        NULL,                         // Input parameters (none)
        NULL,                         // Expected output parameters (none)
        G_DBUS_CALL_FLAGS_NONE,      // Flags
        -1,                           // Timeout
        NULL,                         // Cancellable
        (GAsyncReadyCallback)connect_callback,             // Callback function
        (gpointer)bt      // User data (device address)
    );
}

void RPI4CommandConnect(BT_t *bt, BTPairedDevice_t *dev) {
    const uint8_t* device_address = dev->macId;
    memcpy(bt->activeDevice.macId, dev->macId, BT_MAC_ID_LEN);
    bt->status = BT_STATUS_CONNECTING;
    const char *mac_str = uint8_array_to_mac(device_address);
    LogDebug(LOG_SOURCE_BT, "Connecting to %s (%s).", dev->deviceName, mac_str);
    connect_to_device(bt, mac_str);
}

void RPI4CommandGetMetadata(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "GetMetadata", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Getting metadata.");
}

static void bluez_list_devices(GDBusConnection *con,
                GAsyncResult *res,
                gpointer data)
{
    BT_t *bt = (BT_t*)data;
    GVariant *result = NULL;
    GVariantIter i;
    const gchar *object_path;
    GVariant *ifaces_and_properties;

    result = g_dbus_connection_call_finish(con, res, NULL);
    if(result == NULL)
        LogError("Unable to get result for GetManagedObjects\n");

    /* Parse the result */
    if(result) {
        EventTriggerCallback(BT_EVENT_DEVICE_FOUND, 0);
        result = g_variant_get_child_value(result, 0);
        g_variant_iter_init(&i, result);
        uint8_t number = 1;
        while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &object_path, &ifaces_and_properties)) {
            const gchar *interface_name;
            GVariant *properties;
            GVariantIter ii;
            g_variant_iter_init(&ii, ifaces_and_properties);
            while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                if(g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "device")) {
                    //g_print("%d - [ %s ]\n", number, object_path);
                     const gchar *property_name;
                    GVariantIter iii;
                    GVariant *prop_val;
                    g_variant_iter_init(&iii, properties);
                    char deviceName[12];
                    uint8_t macId[6];
                    while(g_variant_iter_next(&iii, "{&sv}", &property_name, &prop_val)) {
                        bluez_property_value(property_name, prop_val);
                        if (strcmp(property_name, "Address") == 0) {
                            mac_to_uint8_array(g_variant_get_string(prop_val, NULL), macId);
                        } else if (strcmp(property_name, "Name") == 0) {
                            strcpy(deviceName, g_variant_get_string(prop_val, NULL));
                        }
                    }
                    g_variant_unref(prop_val);
                    BTPairedDeviceInit(bt, macId, deviceName, number++);
                }
                g_variant_unref(properties);
            }
            g_variant_unref(ifaces_and_properties);
        }
        g_variant_unref(result);
    }
}

void RPI4CommandList(BT_t *bt) {
    g_dbus_connection_call(bt->connection,
        BLUEZ_BUS_NAME,
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        (GAsyncReadyCallback)bluez_list_devices,
        bt);
    LogDebug(LOG_SOURCE_BT, "Listing paired devices.");
}

void RPI4CommandPause(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Pause", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Playback paused.");
}

void RPI4CommandPlay(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Play", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Playback started.");
}

void RPI4CommandPlaybackTrackFastforwardStart(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "FastForward", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Fast forwarding track.");
}

void RPI4CommandPlaybackTrackFastforwardStop(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "StopFastForward", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Stopped fast forwarding track.\n");
}

void RPI4CommandPlaybackTrackRewindStart(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Rewind", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Rewinding track.\n");
}

void RPI4CommandPlaybackTrackRewindStop(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "StopRewind", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Stopped rewinding track.\n");
}

void RPI4CommandPlaybackTrackNext(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Next", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Playing next track.\n");
}

void RPI4CommandPlaybackTrackPrevious(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Previous", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Playing previous track.\n");
}

void RPI4CommandProfileOpen(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "Open", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Profile opened.\n");
}

void RPI4CommandSetConnectable(BT_t *bt, unsigned char connectable) {
    bluez_adapter_call_method(bt->connection, "SetConnectable", g_variant_new("b", connectable), NULL);
    LogDebug(LOG_SOURCE_BT, "Set connectable: %d.\n", connectable);
}

void RPI4CommandSetDiscoverable(BT_t *bt, unsigned char discoverable) {
    bluez_adapter_call_method(bt->connection, "SetDiscoverable", g_variant_new("b", discoverable), NULL);
    LogDebug(LOG_SOURCE_BT, "Set discoverable: %d.\n", discoverable);
}

void RPI4CommandStatus(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "GetProperties", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Bluetooth status retrieved.\n");
}

void RPI4CommandStatusAVRCP(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "GetStatus", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "AVRCP status retrieved.\n");
}

void RPI4CommandToggleVR(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "ToggleVoiceRecognition", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Voice recognition toggled.\n");
}

guint subscribe_to_bluetooth_events(BT_t *bt, GMainLoop *loop) {
    LogDebug(LOG_SOURCE_BT, "Subscribed to BT events");
    return g_dbus_connection_signal_subscribe(bt->connection,
            BLUEZ_BUS_NAME,
            "org.freedesktop.DBus.Properties",
            PROPERTIES_CHANGED_SIGNAL,
            ADAPTER_OBJECT_PATH,
            ADAPTER_INTERFACE,
            G_DBUS_SIGNAL_FLAGS_NONE,
            on_properties_changed,
            bt,
            NULL);
}

guint subscribe_to_added_bluetooth_interface(BT_t *bt, GMainLoop *loop) {
    LogDebug(LOG_SOURCE_BT, "Subscribed to InterfacesAdded events");
    return g_dbus_connection_signal_subscribe(bt->connection,
            BLUEZ_BUS_NAME,
            "org.freedesktop.DBus.ObjectManager",
            "InterfacesAdded",
            NULL,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            bluez_device_appeared,
            bt,
            NULL);
}

guint subscribe_to_removed_bluetooth_interface(BT_t *bt, GMainLoop *loop) {
    LogDebug(LOG_SOURCE_BT, "Subscribed to InterfacesRemoved events");
    return g_dbus_connection_signal_subscribe(bt->connection,
            BLUEZ_BUS_NAME,
            "org.freedesktop.DBus.ObjectManager",
            "InterfacesRemoved",
            NULL,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            bluez_device_disappeared,
            bt,
            NULL);
}

gboolean on_loop_idle(gpointer user_data)
{
    if (loop_exit == TRUE) {
        GMainLoop *loop = (GMainLoop*)user_data;
        g_main_loop_quit(loop);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

// Signal handler for SIGINT
void handle_sigint(int signo) {
    LogDebug(LOG_SOURCE_BT, "\nCaught signal %d, disconnecting from device...\n", signo);
    if (signo == SIGINT) loop_exit = TRUE;
}

void RPI4Process(BT_t *bt) {
    LogDebug(LOG_SOURCE_BT, "Processing BT events");
    // Subscribe to Bluetooth events
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    // Set up signal handler for SIGINT
    signal(SIGINT, handle_sigint);
    guint prop_changed = subscribe_to_bluetooth_events(bt, loop);
    guint iface_added = subscribe_to_added_bluetooth_interface(bt, loop);
    guint iface_removed = subscribe_to_removed_bluetooth_interface(bt, loop);

    RPI4CommandList(bt);

    g_idle_add(on_loop_idle, loop);
    g_main_loop_run(loop);

    disconnect_from_device(bt, 1);

    g_dbus_connection_signal_unsubscribe(bt->connection, prop_changed);
    g_dbus_connection_signal_unsubscribe(bt->connection, iface_added);
    g_dbus_connection_signal_unsubscribe(bt->connection, iface_removed);
    g_dbus_connection_signal_unsubscribe(bt->connection, mediaplayer_changed);
    g_dbus_connection_signal_unsubscribe(bt->connection, mediacontrol_changed);
    g_main_loop_unref(loop);
    g_object_unref(bt->connection);
    exit(0);
}

