#ifndef GJS_JOYSTICK_H
#define GJS_JOYSTICK_H

#include <glib-object.h>

#define GJS_JOYSTICK_TYPE		(gjs_joystick_get_type())
#define GJS_JOYSTICK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GJS_JOYSTICK_TYPE, GjsJoystick))
#define GJS_JOYSTICK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST((vtable), GJS_JOYSTICK_TYPE, GjsJoystickClass))
#define GJS_IS_JOYSTICK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GJS_JOYSTICK_TYPE))
#define GJS_IS_OBJECT_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE((vtable), GJS_JOYSTICK_TYPE))
#define GJS_JOYSTICK_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS((inst), GJS_JOYSTICK_TYPE, GjsJoystickClass))
#define GJS_ERROR_DOMAIN		(gjs_get_errdomain())
#define GJS_ERROR			(gjs_error_get_type())

typedef struct _GjsJoystick GjsJoystick;
typedef struct _GjsJoystickClass GjsJoystickClass;
typedef struct _GjsJoystickPrivate GjsJoystickPrivate;

typedef enum {
	GJS_ERR_DEV_NREADY,
	GJS_ERR_NDEV,
} GjsError;

typedef enum gjs_axis_type {
	GJS_AXIS_X = 0x00,
	GJS_AXIS_Y,
	GJS_AXIS_Z,
	GJS_AXIS_RX,
	GJS_AXIS_RY,
	GJS_AXIS_RZ,
	GJS_AXIS_THROTTLE,
	GJS_AXIS_RUDDER,
	GJS_AXIS_WHEEL = 0x08,
	GJS_AXIS_GAS,
	GJS_AXIS_BRAKE,
	GJS_AXIS_HAT0X = 0x10,
	GJS_AXIS_HAT0Y,
	GJS_AXIS_HAT1X,
	GJS_AXIS_HAT1Y,
	GJS_AXIS_HAT2X,
	GJS_AXIS_HAT2Y,
	GJS_AXIS_HAT3X,
	GJS_AXIS_HAT3Y,
} GjsAxisType;

typedef enum gjs_button_type {
	GJS_BTN_0 = 0x00,
	GJS_BTN_1,
	GJS_BTN_2,
	GJS_BTN_3,
	GJS_BTN_4,
	GJS_BTN_5,
	GJS_BTN_6,
	GJS_BTN_7,
	GJS_BTN_8,
	GJS_BTN_9,
	GJS_BTN_LEFT = 0x10,
	GJS_BTN_RIGHT,
	GJS_BTN_MIDDLE,
	GJS_BTN_SIDE,
	GJS_BTN_EXTRA,
	GJS_BTN_FORWARD,
	GJS_BTN_BACK,
	GJS_BTN_TASK,
	GJS_BTN_TRIGGER = 0x20,
	GJS_BTN_THUMB,
	GJS_BTN_THUMB2,
	GJS_BTN_TOP,
	GJS_BTN_TOP2,
	GJS_BTN_PINKIE,
	GJS_BTN_BASE,
	GJS_BTN_BASE2,
	GJS_BTN_BASE3,
	GJS_BTN_BASE4,
	GJS_BTN_BASE5,
	GJS_BTN_BASE6,
	GJS_BTN_DEAD,
	GJS_BTN_A = 0x30,
	GJS_BTN_B,
	GJC_BTN_C,
	GJC_BTN_X,
	GJS_BTN_Y,
	GJS_BTN_Z,
	GJS_BTN_TL,
	GJS_BTN_TR,
	GJS_BTN_TL2,
	GJS_BTN_TR2,
	GJS_BTN_SELECT,
	GJS_BTN_START,
	GJS_BTN_MODE,
	GJS_BTN_THUMB_L,
	GJS_BTN_THUMB_R,
	GJS_BTN_WHEEL = 0x50,
	GJS_BTN_GEAR_UP,
} GjsBtnType;

typedef enum {
	GJS_MODE_MANUAL,
	GJS_MODE_MAINLOOP,
} GjsMode;

struct _GjsJoystick {
	GObject parent;
	GjsJoystickPrivate *priv;
};

struct _GjsJoystickClass {
	GObjectClass parent;

	/* mode */

	/* signals */
	guint button_pressed;
	guint button_released;
	guint axis_moved;
};

/* constructors & class functions */
/** @brief Create a joystick object 
  *
  * This function creates a new GjsJoystick object.
  * @param devname the device node of the joystick to open
  * @return a newly-allocated GjsJoystick.
  * @warning this function always returns a GjsJoystick; but if the
  * device node cannot be opened for some reason, then it will not issue
  * any events. Use the gjs_joystick_is_valid() function to verify that
  * it can do anything useful.
  */
GjsJoystick* gjs_joystick_open(gchar* devname);
/** @brief Enumerate all joystick device nodes on this system
  *
  * @return an array of strings with filenames of all joystick device
  * nodes on this system
  * @note this function assumes that all joystick device nodes are found
  * under /dev/input. This is usually correct.
  */
gchar** gjs_joystick_enumerate(void);
/** @brief Describe a joystick without first calling gjs_joystick_open()
  *
  * @return the identity string of the joystick, or NULL in case of
  * error (with err set appropriately)
  * @param devname the device node of the joystick to identify
  * @note to get the requested information, this function will open the
  * joystick device, perform the correct ioctl() to retrieve the
  * requested data, and close the device again; so this function is no
  * substitute for when gjs_joystick_open() followed by
  * gjs_joystick_describe() fails for some reason (e.g., permission
  * issues)
  */
gchar* gjs_joystick_describe_unopened(gchar* devname, GError** err);

/* instance functions */
/** @brief Get the number of axes on this joystick
  *
  * @return the number of axes found on the given joystick, or 0 in
  * case of error (e.g., the GjsJoystick is not in a valid state).
  */
guchar gjs_joystick_get_axis_count(GjsJoystick*, GError** err);
/** @brief Get the number of buttons on this joystick
  *
  * @return the number of buttons found on this joystick, or 0 in case
  * of error (e.g., the GjsJoystick is not in a vaid state)
  */
guchar gjs_joystick_get_button_count(GjsJoystick*, GError** err);
/** @brief Identify this joystick
  *
  * @return a human-readable string identifying this joystick, as
  * returned by the kernel. This is usually a manufacturer name,
  * followed by a model name.
  */
gchar* gjs_joystick_describe(GjsJoystick*, GError** err);
/** @brief Identify a given axis
  *
  * There are several types of axes that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick axis is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis in
  * human-readable format.
  * 
  * @param the axis to identify
  * @return a human-readable string describing the axis (e.g.,
  * "Throttle" or "X")
  * @see gjs_joystick_get_axis_type
  */
gchar* gjs_joystick_describe_axis(GjsJoystick*, guchar axis);
/** @brief Identify a given button
  *
  * There are several types of buttons that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick button is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick button in
  * human-readable format.
  * 
  * @param button the button to identify
  * @return a human-readable string describing the button (e.g.,
  * "Trigger" or "A")
  * @see gjs_joystick_get_button_type
  */
gchar* gjs_joystick_describe_button(GjsJoystick*, guchar button);
/** @brief Get the type of a given button
  *
  * There are several types of buttons that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick button is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis
  *
  * @param button the button to identify
  * @return the type of the button
  * @see gjs_joystick_describe_button
  */
GjsBtnType gjs_joystick_get_button_type(GjsJoystick*, guchar button);
/** @brief Identify a given axis
  *
  * There are several types of axes that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick axis is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis.
  * 
  * @param the axis to identify
  * @return the type of the axis
  * @see gjs_joystick_describe_axis
  */
GjsAxisType gjs_joystick_get_axis_type(GjsJoystick*, guchar axis);
/** @brief try to reopen the joystick
  *
  * If the joystick is unplugged from the system, the GjsJoystick will
  * enter an "invalid" state, and stop issueing events. When that
  * happens, this function can be called to reconnect the joystick.
  *
  * Additionally, this function may be used to switch the GjsJoystick to
  * another joystick device node.
  *
  * @param devname the new device name of the joystick. If NULL, the
  * device name chosen at construction time is reused.
  * @return TRUE if the joystick was successfully connected, FALSE
  * otherwise (with err set appropriately).
  */
gboolean gjs_joystick_reopen(GjsJoystick*, gchar* devname, GError** err);
/** @brief Select the mode in which to issue events
  *
  * @param mode the new mode.
  */
void gjs_joystick_set_mode(GjsJoystick*, GjsMode mode);
/** @brief Perform one iteration of handling joystick events and
  * issueing signals.
  *
  * @note This function will do nothing when the joystick is in
  * GJS_MODE_MAINLOOP mode.
  */
void gjs_joystick_iteration(GjsJoystick* self);
/** @brief read joystick events (in blocking mode) and issue signals
  *
  * This function will only return when the joystick device is closed.
  * @note This function will do nothing when the joystick is in
  * GJS_MODE_MAINLOOP mode.
  */
void gjs_joystick_loop(GjsJoystick* self);

/* type handling functions */
GType gjs_joystick_get_type(void) G_GNUC_PURE;
GQuark gjs_get_errdomain(void) G_GNUC_PURE;
GType gjs_error_get_type(void) G_GNUC_PURE;

#endif // GJS_JOYSTICK_H
