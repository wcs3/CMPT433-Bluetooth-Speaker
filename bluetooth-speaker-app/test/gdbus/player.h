#ifndef __BT_PLAYER_H__
#define __BT_PLAYER_H__

#include <stdint.h>
#include <stdbool.h>

#include <gio/gio.h>

typedef enum
{
    BT_PLAYER_STATUS_STOPPED,
    BT_PLAYER_STATUS_PLAYING,
    BT_PLAYER_STATUS_PAUSED,
    BT_PLAYER_STATUS_cnt,
} bt_player_status_e;

typedef enum
{
    BT_PLAYER_SHUFFLE_OFF,
    BT_PLAYER_SHUFFLE_ALL_TRACKS,
    BT_PLAYER_SHUFFLE_cnt,
} bt_player_shuffle_e;

typedef enum
{
    BT_PLAYER_REPEAT_OFF,
    BT_PLAYER_REPEAT_SINGLE_TRACK,
    BT_PLAYER_REPEAT_ALL_TRACKS,
    BT_PLAYER_REPEAT_cnt,
} bt_player_repeat_e;

typedef struct
{
    char *title;
    char *album;
    char *artist;
    char *genre;

    uint32_t track_number;
    uint32_t track_count;
    uint32_t duration;
} bt_player_track_info_t;

typedef enum
{
    BT_PLAYER_VOLUME_OFF = 0,
    BT_PLAYER_VOLUME_DEFAULT = 100,
    BT_PLAYER_VOLUME_MAX = 127,
} bt_player_volume_e;

void bt_player_init();
void bt_player_cleanup();

bool bt_player_device_available();

typedef enum
{
    BT_PLAYER_CMD_PAUSE,
    BT_PLAYER_CMD_PLAY,
    BT_PLAYER_CMD_STOP,
    BT_PLAYER_CMD_NEXT,
    BT_PLAYER_CMD_PREV,
    BT_PLAYER_CMD_FF, // doesn't appear to work on android
    BT_PLAYER_CMD_RW, // doesn't appear to work on android
    BT_PLAYER_CMD_cnt,
} bt_player_cmd_e;

/**
 * Send a command to the playing bluetooth device, synchronously.
 *
 * This function sends a command to bluez over dbus, waits for bluez to reply
 * with a message indicating if the command executed successfully, then
 * returns.
 *
 * command must be one of the defined bt_player_cmd_e values.
 *
 * Returns true if the command executed successfully, otherwise false
 */
bool bt_player_send_command(bt_player_cmd_e command);

/**
 * Function signature for a command callback
 * Used with bt_player_send_command_async
 */
typedef void (*bt_player_cmd_cb)(bool successful, void *user_data);

/**
 * Send a command to the playing bluetooth device, asynchronously.
 *
 * This function initiates the sending of a command, then immediately returns.
 * When bluez later sends a reply indicating the error status of the command,
 * the supplied callback is called, with its first argument indicating if the
 * command was executed successfully, and its second argument set to the
 * optional user_data variable.
 *
 * This function does not return a value to indicate if an error occured.
 * To see the error status, you need to use the callback mechanism.
 *
 * If you don't care about the error status, you can set callback to NULL.
 */
void bt_player_send_command_async(bt_player_cmd_e command,
                                  bt_player_cmd_cb callback,
                                  void *user_data);

typedef enum
{                                     // Property type:
    BT_PLAYER_PROP_PLAYBACK_STATUS,   // bt_player_status_e
    BT_PLAYER_PROP_SHUFFLE_MODE,      // bt_player_shuffle_e
    BT_PLAYER_PROP_REPEAT_MODE,       // bt_player_repeate_e
    BT_PLAYER_PROP_PLAYBACK_POSITION, // uint32_t
    BT_PLAYER_PROP_TRACK,             // bt_player_track_info_t
    BT_PLAYER_PROP_VOLUME,            // uint8_t (value in 0-127, inclusive)
    BT_PLAYER_PROP_cnt,
} bt_player_prop_e;

/**
 * Get a property value
 *
 * Supply a variable of sufficient size to copy the property value into with
 * the retval argument.
 *
 * Returns true if the value was successfully retrieved, false otherwise.
 *
 * It is not recommended to use this function to continuously poll a property.
 * A more efficient scheme is to use this function only to get the initial
 * value of the property, and use the callback mechanism
 * (bt_player_set_property_changed_cb) to get notified when the value changes.
 */
bool bt_player_get_property(bt_player_prop_e property, void *retval);

/**
 * Set a property value
 *
 * arg is a pointer to the new value of the property, with the stated type in
 * the bt_player_prop_e definition
 *
 * Returns true if set successfully, false otherwise.
 */
bool bt_player_set_property(bt_player_prop_e property, void *arg);

/**
 * Function signature for a property change callback.
 * Used with bt_player_set_property_changed_cb
 *
 * Example callback for playback status:

void on_playback_status_change(const void *changed_val, void *user_data)
{
    (void)user_data;

    const bt_player_status_e *new_status = changed_val;

    switch(new_status)
    {
    case BT_PLAYER_STATUS_STOPPED:
        printf("\n");
        break;
    case BT_PLAYER_STATUS_PLAYING:
        printf("\n");
        break;
    case BT_PLAYER_STATUS_PAUSED:
        printf("\n");
        break;
    default:
        break;
    }
}

 */
typedef void (*bt_player_prop_cb)(const void *changed_val, void *user_data);

/**
 * Methods to set callbacks for property changes
 *
 * If a callback is set for a property, it will be called when the property
 * changes. The first argument to the callback is the new value of the
 * property, and the second is an optional user_data variable which can be
 * set alongside the callback.
 *
 * To stop receiving notifications for a property, set the property's callback
 * to NULL.
 */
void bt_player_set_property_changed_cb(bt_player_prop_e property,
                                       bt_player_prop_cb callback,
                                       void *user_data);

/**
 * Send a "Pause" command to the playing bluetooth device, synchronously.
 *
 * This function sends a "Pause" command to bluez over dbus, waits for bluez to reply
 * with a message indicating if the command executed successfully, then
 * returns.
 */
void bt_player_set_pause_sync(GDBusProxy *player_proxy);

/**
 * Send a "Play" command to the playing bluetooth device, synchronously.
 *
 * This function sends a "Play" command to bluez over dbus, waits for bluez to reply
 * with a message indicating if the command executed successfully, then
 * returns.
 */
void bt_player_set_play_sync(GDBusProxy *player_proxy);

/**
 * Prints data from the proxy's properties.
 *
 * This function parses metadata from the multimedia player proxy's properties
 * and printing them out, then returns.
 */
void print_track_data(GDBusProxy *player_proxy);

bool is_paused_status();

#endif // __BT_PLAYER_H__