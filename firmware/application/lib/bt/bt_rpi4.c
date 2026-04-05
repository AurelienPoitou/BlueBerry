#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <gio/gio.h>
#include <dbus/dbus.h>
#include <bluetooth/bluetooth.h>

#include "bt_common.h"
#include "bt_rpi4.h"
#include "../timer.h"
#include "../utils.h"
#include "../log.h"
#include "../event.h"
#include "../shutdown.h"

/* ------------------------------------------------------------------------- */
/* BlueZ constants                                                           */
/* ------------------------------------------------------------------------- */

#ifndef BLUEZ_BUS_NAME
#define BLUEZ_BUS_NAME                  "org.bluez"
#endif

#ifndef ADAPTER_OBJECT_PATH
#define ADAPTER_OBJECT_PATH             "/org/bluez/hci0"
#endif

#ifndef ADAPTER_INTERFACE
#define ADAPTER_INTERFACE               "org.bluez.Adapter1"
#endif

#ifndef DEVICE_INTERFACE
#define DEVICE_INTERFACE                "org.bluez.Device1"
#endif

#ifndef DEVICE_PATH_PREFIX
#define DEVICE_PATH_PREFIX              "/org/bluez/hci0/dev_"
#endif

#ifndef PROFILEMANAGER_OBJECT_PATH
#define PROFILEMANAGER_OBJECT_PATH      "/org/bluez"
#endif

#ifndef PROFILEMANAGER_INTERFACE
#define PROFILEMANAGER_INTERFACE        "org.bluez.ProfileManager1"
#endif

#ifndef MEDIAPLAYER_INTERFACE
#define MEDIAPLAYER_INTERFACE           "org.bluez.MediaPlayer1"
#endif

#ifndef MEDIACONTROL_INTERFACE
#define MEDIACONTROL_INTERFACE          "org.bluez.MediaControl1"
#endif

#ifndef PROPERTIES_CHANGED_SIGNAL
#define PROPERTIES_CHANGED_SIGNAL       "PropertiesChanged"
#endif

/* Profiles */
#ifndef A2DP_PROFILE_UUID
#define A2DP_PROFILE_UUID               "0000110A-0000-1000-8000-00805F9B34FB"
#endif

#ifndef HFP_PROFILE_UUID
#define HFP_PROFILE_UUID                "0000111E-0000-1000-8000-00805F9B34FB"
#endif

#ifndef AVRCP_PROFILE_UUID
#define AVRCP_PROFILE_UUID              "0000111F-0000-1000-8000-00805F9B34FB"
#endif

#ifndef A2DP_PROFILE_PATH
#define A2DP_PROFILE_PATH               "/com/myapp/a2dp"
#endif

#ifndef HFP_PROFILE_PATH
#define HFP_PROFILE_PATH                "/com/myapp/hfp"
#endif

#ifndef AVRCP_PROFILE_PATH
#define AVRCP_PROFILE_PATH              "/com/myapp/avrcp"
#endif

/* ------------------------------------------------------------------------- */
/* Globals                                                                   */
/* ------------------------------------------------------------------------- */

char device_addr[256];
gboolean loop_exit = FALSE;
guint mediaplayer_changed = 0;
guint mediacontrol_changed = 0;

/* ------------------------------------------------------------------------- */
/* Helper: MAC conversions                                                   */
/* ------------------------------------------------------------------------- */

char *uint8_array_to_mac(const uint8_t mac_array[6])
{
    char *mac_str = malloc(18);
    if (mac_str == NULL) {
        return NULL;
    }
    snprintf(mac_str, 18, "%02X_%02X_%02X_%02X_%02X_%02X",
             mac_array[0], mac_array[1], mac_array[2],
             mac_array[3], mac_array[4], mac_array[5]);
    return mac_str;
}

void mac_to_uint8_array(const char *mac_str, uint8_t mac_array[6])
{
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_array[0], &mac_array[1], &mac_array[2],
           &mac_array[3], &mac_array[4], &mac_array[5]);
}

/* ------------------------------------------------------------------------- */
/* BlueZ adapter helper                                                      */
/* ------------------------------------------------------------------------- */

typedef void (*method_cb_t)(GObject *, GAsyncResult *, gpointer);

int bluez_adapter_call_method(GDBusConnection *conn,
                              const char *method,
                              GVariant *param,
                              method_cb_t method_cb)
{
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
                           NULL);
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Property dump helper                                                      */
/* ------------------------------------------------------------------------- */

void bluez_property_value(const gchar *key, GVariant *value)
{
    const gchar *type = g_variant_get_type_string(value);

    LogDebug(LOG_SOURCE_BT, "\t%s : ", key);
    switch (*type) {
        case 'o':
        case 's':
            LogDebug(LOG_SOURCE_BT, "%s", g_variant_get_string(value, NULL));
            break;
        case 'b':
            LogDebug(LOG_SOURCE_BT, "%d", g_variant_get_boolean(value));
            break;
        case 'u':
            LogDebug(LOG_SOURCE_BT, "%u", g_variant_get_uint32(value));
            break;
        case 'i':
            LogDebug(LOG_SOURCE_BT, "%d", g_variant_get_int32(value));
            break;
        case 'd':
            LogDebug(LOG_SOURCE_BT, "%f", g_variant_get_double(value));
            break;
        case 'a':
            if (g_strcmp0(type, "as") == 0) {
                const gchar *str_value;
                GVariantIter i;
                LogDebug(LOG_SOURCE_BT, "Array of strings:");
                g_variant_iter_init(&i, value);
                while (g_variant_iter_next(&i, "s", &str_value)) {
                    LogDebug(LOG_SOURCE_BT, "\t\t%s", str_value);
                }
            } else if (g_str_has_prefix(type, "a{")) {
                GVariantIter dict_iter;
                GVariant *dict_entry;
                LogDebug(LOG_SOURCE_BT, "Array of dictionaries:");
                g_variant_iter_init(&dict_iter, value);
                while (g_variant_iter_next(&dict_iter, "a{sa{sv}}", &dict_entry)) {
                    const gchar *dict_key;
                    GVariant *dict_value;
                    GVariantIter entry_iter;
                    g_variant_iter_init(&entry_iter, dict_entry);
                    while (g_variant_iter_next(&entry_iter, "{&sv}", &dict_key, &dict_value)) {
                        LogDebug(LOG_SOURCE_BT,
                                 "\t\tKey: %s, Value Type: %s",
                                 dict_key,
                                 g_variant_get_type_string(dict_value));
                        if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_STRING)) {
                            const gchar *sv = g_variant_get_string(dict_value, NULL);
                            LogDebug(LOG_SOURCE_BT, "Value: %s", sv);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_BOOLEAN)) {
                            gboolean bv = g_variant_get_boolean(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %s", bv ? "true" : "false");
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_UINT32)) {
                            guint32 uv = g_variant_get_uint32(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %u", uv);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_INT32)) {
                            gint32 iv = g_variant_get_int32(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %d", iv);
                        } else if (g_variant_is_of_type(dict_value, G_VARIANT_TYPE_DOUBLE)) {
                            gdouble dv = g_variant_get_double(dict_value);
                            LogDebug(LOG_SOURCE_BT, "Value: %f", dv);
                        } else {
                            LogDebug(LOG_SOURCE_BT, "Value: Unknown type");
                        }
                        g_variant_unref(dict_value);
                    }
                    g_variant_unref(dict_entry);
                }
            }
            break;
        default:
            LogDebug(LOG_SOURCE_BT, "Value: Unknown type");
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* Media player / metadata handling                                          */
/* ------------------------------------------------------------------------- */

static void on_properties_changed(GDBusConnection *connection,
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

    BT_t *bt = (BT_t *)user_data;
    GVariantIter *properties_iter;
    const gchar *key;
    GVariant *value;

    g_variant_get(parameters, "(sa{sv}as)", NULL, &properties_iter, NULL);
    while (g_variant_iter_next(properties_iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "Status") == 0) {
            const char *status = g_variant_get_string(value, NULL);
            gchar *lower = g_ascii_strdown(status, -1);
            if (g_strcmp0(lower, "playing") == 0) {
                bt->playbackStatus = BT_AVRCP_STATUS_PLAYING;
                EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, NULL);
            } else if (g_strcmp0(lower, "paused") == 0) {
                bt->playbackStatus = BT_AVRCP_STATUS_PAUSED;
                EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, NULL);
            }
            g_free(lower);
        } else if (g_strcmp0(key, "Track") == 0) {
            GVariantIter iter;
            GVariant *track_value;
            const gchar *track_key;

            g_variant_iter_init(&iter, value);
            while (g_variant_iter_next(&iter, "{sv}", &track_key, &track_value)) {
                bt->metadataTimestamp = TimerGetMillis();
                if (g_strcmp0(track_key, "Title") == 0) {
                    char title[BT_METADATA_MAX_SIZE] = {0};
                    const char *g_title = g_variant_get_string(track_value, NULL);
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
                    const char *g_artist = g_variant_get_string(track_value, NULL);
                    UtilsNormalizeText(artist, g_artist, BT_METADATA_MAX_SIZE);
                    if (strncmp(bt->artist, artist, BT_METADATA_FIELD_SIZE - 1) != 0) {
                        bt->metadataStatus = BT_METADATA_STATUS_UPD;
                        memset(bt->artist, 0, BT_METADATA_FIELD_SIZE);
                        UtilsStrncpy(bt->artist, artist, BT_METADATA_FIELD_SIZE);
                    }
                } else if (g_strcmp0(track_key, "Album") == 0) {
                    char album[BT_METADATA_MAX_SIZE] = {0};
                    const char *g_album = g_variant_get_string(track_value, NULL);
                    UtilsNormalizeText(album, g_album, BT_METADATA_MAX_SIZE);
                    if (strncmp(bt->album, album, BT_METADATA_FIELD_SIZE - 1) != 0) {
                        bt->metadataStatus = BT_METADATA_STATUS_UPD;
                        memset(bt->album, 0, BT_METADATA_FIELD_SIZE);
                        UtilsStrncpy(bt->album, album, BT_METADATA_FIELD_SIZE);
                    }
                }
                g_variant_unref(track_value);
            }

            if (bt->metadataStatus == BT_METADATA_STATUS_UPD) {
                LogDebug(LOG_SOURCE_BT,
                         "BT: title=%s,artist=%s,album=%s",
                         bt->title,
                         bt->artist,
                         bt->album);
                EventTriggerCallback(BT_EVENT_METADATA_UPDATE, NULL);
            }
            bt->metadataStatus = BT_METADATA_STATUS_CUR;
        }
        g_variant_unref(value);
    }
    g_variant_iter_free(properties_iter);
}

guint subscribe_to_mediaplayer_events(BT_t *bt, const char *playerPath)
{
    LogDebug(LOG_SOURCE_BT, "Subscribed to MediaPlayer events");
    return g_dbus_connection_signal_subscribe(bt->connection,
                                              BLUEZ_BUS_NAME,
                                              "org.freedesktop.DBus.Properties",
                                              PROPERTIES_CHANGED_SIGNAL,
                                              (gchar *)playerPath,
                                              MEDIAPLAYER_INTERFACE,
                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                              on_properties_changed,
                                              bt,
                                              NULL);
}

guint subscribe_to_mediacontrol_events(BT_t *bt, const char *playerPath)
{
    LogDebug(LOG_SOURCE_BT, "Subscribed to MediaControl events");
    return g_dbus_connection_signal_subscribe(bt->connection,
                                              BLUEZ_BUS_NAME,
                                              "org.freedesktop.DBus.Properties",
                                              PROPERTIES_CHANGED_SIGNAL,
                                              (gchar *)playerPath,
                                              MEDIACONTROL_INTERFACE,
                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                              on_properties_changed,
                                              bt,
                                              NULL);
}

/* ------------------------------------------------------------------------- */
/* Device appeared / disappeared                                             */
/* ------------------------------------------------------------------------- */

static void bluez_device_appeared(GDBusConnection *sig,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data)
{
    (void)sig;
    (void)sender_name;
    (void)object_path;
    (void)interface;
    (void)signal_name;

    BT_t *bt = (BT_t *)user_data;
    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    GVariant *properties;

    g_assert(g_str_equal(g_variant_get_type_string(parameters), "(oa{sa{sv}})"));
    g_variant_get(parameters, "(oa{sa{sv}})", &object, &interfaces);
    LogDebug(LOG_SOURCE_BT, "Device Connected: %s", object);

    while (g_variant_iter_next(interfaces, "{&s@a{sv}}", &interface_name, &properties)) {
        LogDebug(LOG_SOURCE_BT, "Interface Added: %s", interface_name);
        if (g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "mediaplayer")) {
            mediaplayer_changed = subscribe_to_mediaplayer_events(bt, object);
            mediacontrol_changed = subscribe_to_mediacontrol_events(bt, object);
            uint8_t linkType = BT_LINK_TYPE_A2DP;
            EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED,
                                 (unsigned char *)&linkType);

            const gchar *property_name;
            GVariantIter i;
            GVariant *prop_val;
            g_variant_iter_init(&i, properties);
            while (g_variant_iter_next(&i, "{&sv}", &property_name, &prop_val)) {
                bluez_property_value(property_name, prop_val);
                g_variant_unref(prop_val);
            }
        }

        if (strcmp(interface_name, "org.bluez.MediaTransport1") == 0) {
            bt->activeDevice.a2dpId = 1;
            LogDebug(LOG_SOURCE_BT, "A2DP profile connected");
        }

        if (strcmp(interface_name, "org.bluez.MediaPlayer1") == 0) {
            bt->activeDevice.avrcpId = 1;
            LogDebug(LOG_SOURCE_BT, "AVRCP profile connected");
        }

        if (strcmp(interface_name, "org.bluez.HandsfreeGateway1") == 0 ||
            strcmp(interface_name, "org.bluez.HandsfreeAudioGateway1") == 0) {
            bt->activeDevice.hfpId = 1;
            LogDebug(LOG_SOURCE_BT, "HFP profile connected");
        }
        g_variant_unref(properties);
    }
    g_variant_iter_free(interfaces);
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
    BT_t *bt = (BT_t *)user_data;

    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    char address[BT_ADDRESS_STRING_SIZE] = {'\0'};

    g_variant_get(parameters, "(&oas)", &object, &interfaces);
    LogDebug(LOG_SOURCE_BT, "Device Disconnected: %s", object);
    while (g_variant_iter_next(interfaces, "s", &interface_name)) {
        LogDebug(LOG_SOURCE_BT, "Interface Disconnected: %s", interface_name);
        if (g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "device")) {
            int i;
            char *tmp = g_strstr_len(object, -1, "dev_") + 4;
            for (i = 0; *tmp != '\0'; i++, tmp++) {
                if (*tmp == '_') {
                    address[i] = ':';
                    continue;
                }
                address[i] = *tmp;
            }
            LogDebug(LOG_SOURCE_BT, "Device %s removed", address);
            /* Compare with active device */
            char *active_mac = uint8_array_to_mac(bt->activeDevice.macId);

            if (active_mac && strcmp(active_mac, address) == 0) {
                LogDebug(LOG_SOURCE_BT,
                         "Active device %s disconnected — resetting BT state",
                         address);

                /* Reset state */
                bt->activeDevice.a2dpId = 0;
                bt->activeDevice.avrcpId = 0;
                bt->activeDevice.hfpId  = 0;
                bt->activeDevice.bleId  = 0;
                bt->activeDevice.mapId  = 0;
                bt->activeDevice.pbapId = 0;

                bt->status = BT_STATUS_DISCONNECTED;
            }
        }
        if (strcmp(interface_name, "org.bluez.MediaTransport1") == 0) {
            bt->activeDevice.a2dpId = 0;
            LogDebug(LOG_SOURCE_BT, "A2DP profile disconnected");
        }

        if (strcmp(interface_name, "org.bluez.MediaPlayer1") == 0) {
            bt->activeDevice.avrcpId = 0;
            LogDebug(LOG_SOURCE_BT, "AVRCP profile disconnected");
        }

        if (strcmp(interface_name, "org.bluez.HandsfreeGateway1") == 0 ||
            strcmp(interface_name, "org.bluez.HandsfreeAudioGateway1") == 0) {
            bt->activeDevice.hfpId = 0;
            LogDebug(LOG_SOURCE_BT, "HFP profile disconnected");
        }

    }
    g_variant_iter_free(interfaces);
}

/* ------------------------------------------------------------------------- */
/* Device connect / disconnect callbacks                                     */
/* ------------------------------------------------------------------------- */

static void enable_profiles(BT_t *bt);

static void connect_callback(GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
    GError *error = NULL;
    BT_t *bt = (BT_t *)user_data;

    g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);
    if (error) {
        bt->status = BT_STATUS_DISCONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_DISCONNECTED, NULL);
        LogError("Error connecting to device: %s", error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT, "CB::Device Connected: %s", bt->activeDevice.deviceName);
        bt->status = BT_STATUS_CONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, NULL);
        enable_profiles(bt);
    }
}

static void disconnect_callback(GObject *source_object,
                                GAsyncResult *res,
                                gpointer user_data)
{
    GError *error = NULL;
    BT_t *bt = (BT_t *)user_data;

    g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);
    if (error) {
        bt->status = BT_STATUS_CONNECTED;
        EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, NULL);
        LogError("Error disconnecting from device: %s", error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT, "CB::Device Disconnected: %s", bt->activeDevice.deviceName);
        bt->playbackStatus = BT_AVRCP_STATUS_PAUSED;
        memset(&bt->activeDevice, 0, sizeof(BTConnection_t));
        bt->activeDevice = BTConnectionInit();
        bt->status = BT_STATUS_DISCONNECTED;
        EventTriggerCallback(BT_EVENT_PLAYBACK_STATUS_CHANGE, NULL);
        EventTriggerCallback(BT_EVENT_DEVICE_LINK_DISCONNECTED, NULL);
    }
}

/* ------------------------------------------------------------------------- */
/* Device connect / disconnect wrappers                                      */
/* ------------------------------------------------------------------------- */

static void bt_rpi4_connect_to_device(BT_t *bt, const char *device_address)
{
    memcpy(device_addr, device_address, sizeof(device_addr));
    char device_path[256];
    snprintf(device_path, sizeof(device_path), "%s%s", DEVICE_PATH_PREFIX, device_addr);

    LogDebug(LOG_SOURCE_BT, "Device.Connect(%s)", device_path);

    g_dbus_connection_call(bt->connection,
                           BLUEZ_BUS_NAME,
                           device_path,
                           DEVICE_INTERFACE,
                           "Connect",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           connect_callback,
                           bt);
}

static void bt_rpi4_disconnect_from_device(BT_t *bt, int force)
{
    (void)force;

    char device_path[256];
    snprintf(device_path, sizeof(device_path), "%s%s", DEVICE_PATH_PREFIX, device_addr);

    LogDebug(LOG_SOURCE_BT, "Device.Disconnect(%s)", device_path);

    g_dbus_connection_call(bt->connection,
                           BLUEZ_BUS_NAME,
                           device_path,
                           DEVICE_INTERFACE,
                           "Disconnect",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           disconnect_callback,
                           bt);
}

/* ------------------------------------------------------------------------- */
/* Profile registration                                                      */
/* ------------------------------------------------------------------------- */

static void register_profile(BT_t *bt, const char *profile_uuid, const char *profile_path)
{
    GError *error = NULL;
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));

    g_variant_builder_add(&options, "{sv}", "Name", g_variant_new_string("Profile"));
    g_variant_builder_add(&options, "{sv}", "Role", g_variant_new_string("client"));
    g_variant_builder_add(&options, "{sv}", "RequireAuthentication", g_variant_new_boolean(FALSE));
    g_variant_builder_add(&options, "{sv}", "RequireAuthorization", g_variant_new_boolean(FALSE));

    /* Correct order: (OBJECT profile_path, STRING profile_uuid, DICT options) */
    GVariant *params = g_variant_new("(osa{sv})",
                                     profile_path,   /* object path */
                                     profile_uuid,   /* UUID string */
                                     &options);

    g_dbus_connection_call_sync(bt->connection,
                                BLUEZ_BUS_NAME,
                                PROFILEMANAGER_OBJECT_PATH,
                                PROFILEMANAGER_INTERFACE,
                                "RegisterProfile",
                                params,
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (error) {
        LogError("RegisterProfile(%s at %s) failed: %s",
                 profile_uuid, profile_path, error->message);
        g_error_free(error);
    } else {
        LogDebug(LOG_SOURCE_BT,
                 "Registered profile %s at %s",
                 profile_uuid,
                 profile_path);
    }
}

static void enable_profiles(BT_t *bt)
{
    uint8_t linkType;

    linkType = BT_LINK_TYPE_A2DP;
//    register_profile(bt, A2DP_PROFILE_UUID, A2DP_PROFILE_PATH);
    EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED,
                         (unsigned char *)&linkType);

    linkType = BT_LINK_TYPE_HFP;
//    register_profile(bt, HFP_PROFILE_UUID, HFP_PROFILE_PATH);
    EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED,
                         (unsigned char *)&linkType);

    linkType = BT_LINK_TYPE_AVRCP;
//    register_profile(bt, AVRCP_PROFILE_UUID, AVRCP_PROFILE_PATH);
    EventTriggerCallback(BT_EVENT_DEVICE_LINK_CONNECTED,
                         (unsigned char *)&linkType);
}

/* ------------------------------------------------------------------------- */
/* Public commands: Connect / List / Disconnect                              */
/* ------------------------------------------------------------------------- */

void RPI4CommandConnect(BT_t *bt, BTPairedDevice_t *dev)
{
    /* Avoid spamming Connect() if we’re already in flight or up */
    if (bt->status == BT_STATUS_CONNECTING || bt->status == BT_STATUS_CONNECTED) {
        LogDebug(LOG_SOURCE_BT,
                 "RPI4CommandConnect: ignoring request, status=%u",
                 bt->status);
        return;
    }

    const uint8_t *device_address = dev->macId;
    memcpy(bt->activeDevice.macId, dev->macId, BT_MAC_ID_LEN);
    bt->status = BT_STATUS_CONNECTING;

    char *mac_str = uint8_array_to_mac(device_address);
    if (!mac_str) {
        LogError("RPI4CommandConnect: failed to allocate MAC string");
        return;
    }

    LogDebug(LOG_SOURCE_BT, "Connecting to %s (%s).", dev->deviceName, mac_str);
    UtilsStrncpy(bt->activeDevice.deviceName, dev->deviceName, BT_DEVICE_NAME_LEN);
    bt_rpi4_connect_to_device(bt, mac_str);
    free(mac_str);
}

static void bluez_list_devices(GObject *source_object,
                               GAsyncResult *res,
                               gpointer data)
{
    GDBusConnection *con = G_DBUS_CONNECTION(source_object);
    BT_t *bt = (BT_t *)data;
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_finish(con, res, &error);

    if (error) {
        LogError("GetManagedObjects failed: %s", error->message);
        g_error_free(error);
        return;
    }
    if (!result) {
        LogError("GetManagedObjects returned NULL");
        return;
    }

    /* Notify system that device enumeration has started */
    EventTriggerCallback(BT_EVENT_DEVICE_FOUND, 0);

    /* Extract the dictionary of objects */
    GVariant *objects = g_variant_get_child_value(result, 0);
    g_variant_unref(result);

    GVariantIter iter_objects;
    const gchar *object_path;
    GVariant *ifaces_and_properties;

    g_variant_iter_init(&iter_objects, objects);

    uint8_t number = 1;

    while (g_variant_iter_next(&iter_objects,
                               "{&o@a{sa{sv}}}",
                               &object_path,
                               &ifaces_and_properties))
    {
        GVariantIter iter_ifaces;
        const gchar *iface_name;
        GVariant *properties;

        g_variant_iter_init(&iter_ifaces, ifaces_and_properties);

        while (g_variant_iter_next(&iter_ifaces,
                                   "{&s@a{sv}}",
                                   &iface_name,
                                   &properties))
        {
            /* Only process Device1 interfaces */
            if (g_strstr_len(g_ascii_strdown(iface_name, -1), -1, "device"))
            {
                gboolean connected = FALSE;

                GVariantIter iter_props;
                const gchar *property_name;
                GVariant *prop_val;

                char deviceName[BT_DEVICE_NAME_LEN] = {0};
                uint8_t macId[6] = {0};

                g_variant_iter_init(&iter_props, properties);

                while (g_variant_iter_next(&iter_props,
                                           "{&sv}",
                                           &property_name,
                                           &prop_val))
                {
                    bluez_property_value(property_name, prop_val);

                    if (strcmp(property_name, "Connected") == 0) {
                        connected = g_variant_get_boolean(prop_val);
                    } else if (strcmp(property_name, "Address") == 0) {
                        mac_to_uint8_array(g_variant_get_string(prop_val, NULL), macId);
                    }
                    else if (strcmp(property_name, "Name") == 0) {
                        UtilsStrncpy(deviceName,
                                     g_variant_get_string(prop_val, NULL),
                                     BT_DEVICE_NAME_LEN);
                    }

                    g_variant_unref(prop_val);
                }

                /* Register paired device */
                if (macId[0] != 0 || macId[1] != 0) {
                    if (connected) {
                        memcpy(bt->activeDevice.macId, macId, BT_MAC_ID_LEN);
                        bt->status = BT_STATUS_CONNECTED;
                        EventTriggerCallback(BT_EVENT_DEVICE_CONNECTED, 0);
                    } else {
                        BTPairedDeviceInit(bt, macId, deviceName, number++);
                    }
                }
            }

            g_variant_unref(properties);
        }

        g_variant_unref(ifaces_and_properties);
    }

    g_variant_unref(objects);
}

void RPI4CommandList(BT_t *bt)
{
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
                           bluez_list_devices,
                           bt);
    LogDebug(LOG_SOURCE_BT, "Listing paired devices.");
}

void RPI4CommandDisconnect(BT_t *bt)
{
    char *mac_str = uint8_array_to_mac(bt->activeDevice.macId);
    if (!mac_str) {
        LogError("RPI4CommandDisconnect: failed to allocate MAC string");
        return;
    }
    LogDebug(LOG_SOURCE_BT, "Disconnecting from %s (%s).",
             bt->activeDevice.deviceName,
             mac_str);
    free(mac_str);
    bt_rpi4_disconnect_from_device(bt, 0);
}

/* ------------------------------------------------------------------------- */
/* Subscriptions for device / interface events                               */
/* ------------------------------------------------------------------------- */

guint subscribe_to_bluetooth_events(BT_t *bt, GMainLoop *loop)
{
    (void)loop;
    LogDebug(LOG_SOURCE_BT, "Subscribed to BT events");
    return g_dbus_connection_signal_subscribe(bt->connection,
                                              BLUEZ_BUS_NAME,
                                              "org.freedesktop.DBus.Properties",
                                              PROPERTIES_CHANGED_SIGNAL,
                                              NULL,
                                              DEVICE_INTERFACE,
                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                              on_properties_changed,
                                              bt,
                                              NULL);
}

guint subscribe_to_added_bluetooth_interface(BT_t *bt, GMainLoop *loop)
{
    (void)loop;
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

guint subscribe_to_removed_bluetooth_interface(BT_t *bt, GMainLoop *loop)
{
    (void)loop;
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

/* ------------------------------------------------------------------------- */
/* Main loop / signal handling                                               */
/* ------------------------------------------------------------------------- */

void RPI4Process(BT_t *bt)
{
    LogDebug(LOG_SOURCE_BT, "BT thread starting (no internal GMainLoop)");

    /* Use the default context (owned by main thread) */
    GMainContext *ctx = g_main_context_default();

    /* Subscribe to D-Bus signals — these attach to the default context */
    guint prop_changed  = subscribe_to_bluetooth_events(bt, NULL);
    guint iface_added   = subscribe_to_added_bluetooth_interface(bt, NULL);
    guint iface_removed = subscribe_to_removed_bluetooth_interface(bt, NULL);

    /* Initial commands */
    RPI4CommandList(bt);

    /* Worker loop — no GLib loop here */
    while (!shutting_down) {
        /* If you need to wake the main loop, do it here */
        g_main_context_wakeup(ctx);

        /* Sleep a bit to avoid busy looping */
        g_usleep(10000);
    }

    LogDebug(LOG_SOURCE_BT, "BT thread shutting down, cleaning up");

    /* Disconnect if needed */
    if (bt->status == BT_STATUS_CONNECTED) {
        bt_rpi4_disconnect_from_device(bt, 1);
    }

    /* Unsubscribe from all D-Bus signals */
    g_dbus_connection_signal_unsubscribe(bt->connection, prop_changed);
    g_dbus_connection_signal_unsubscribe(bt->connection, iface_added);
    g_dbus_connection_signal_unsubscribe(bt->connection, iface_removed);

    if (mediaplayer_changed) {
        g_dbus_connection_signal_unsubscribe(bt->connection, mediaplayer_changed);
        mediaplayer_changed = 0;
    }

    if (mediacontrol_changed) {
        g_dbus_connection_signal_unsubscribe(bt->connection, mediacontrol_changed);
        mediacontrol_changed = 0;
    }

    /* Release D-Bus connection */
    g_object_unref(bt->connection);

    LogDebug(LOG_SOURCE_BT, "BT thread terminated cleanly");
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

void RPI4CommandGetMetadata(BT_t *bt) {
    bluez_adapter_call_method(bt->connection, "GetMetadata", NULL, NULL);
    LogDebug(LOG_SOURCE_BT, "Getting metadata.");
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

