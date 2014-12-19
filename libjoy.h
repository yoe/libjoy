/*
 * libjoy - GObject-based joystick API
 *
 * Copyright(c) Wouter Verhelst, 2014
 *
 * This library is free software; you can copy it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef LIBJOY_H
#define LIBJOY_H

#include <glib-object.h>

#define JOY_STICK_TYPE		(joy_stick_get_type())
#define JOY_STICK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), JOY_STICK_TYPE, JoyStick))
#define JOY_STICK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST((vtable), JOY_STICK_TYPE, JoyStickClass))
#define JOY_IS_STICK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), JOY_STICK_TYPE))
#define JOY_IS_STICK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE((vtable), JOY_STICK_TYPE))
#define JOY_STICK_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS((inst), JOY_STICK_TYPE, JoyStickClass))
#define JOY_ERROR_DOMAIN		(joy_get_errdomain())
#define JOY_ERROR			(joy_error_get_type())

typedef struct _JoyStick JoyStick;
typedef struct _JoyStickClass JoyStickClass;
typedef struct _JoyStickPrivate JoyStickPrivate;

typedef enum {
	JOY_ERR_DEV_NREADY,	/**< An operation was performed on a JoyStick which requires a joystick, but none was found on the provided device node */
	JOY_ERR_NDEV,	/**< Error opening the joystick: device name not given */
	JOY_ERR_UDEV,	/**< Error while dealing with udev */
	JOY_ERR_NJS,	/**< No joysticks were found (for "enumerate" style functions) */
} JoyError;

/* The next two enums were defined based on jstest.c */
typedef enum joy_axis_type {
	JOY_AXIS_X = 0x00,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_RX,
	JOY_AXIS_RY,
	JOY_AXIS_RZ,
	JOY_AXIS_THROTTLE,
	JOY_AXIS_RUDDER,
	JOY_AXIS_WHEEL = 0x08,
	JOY_AXIS_GAS,
	JOY_AXIS_BRAKE,
	JOY_AXIS_HAT0X = 0x10,
	JOY_AXIS_HAT0Y,
	JOY_AXIS_HAT1X,
	JOY_AXIS_HAT1Y,
	JOY_AXIS_HAT2X,
	JOY_AXIS_HAT2Y,
	JOY_AXIS_HAT3X,
	JOY_AXIS_HAT3Y,
} JoyAxisType;

typedef enum joy_button_type {
	JOY_BTN_0 = 0x00,
	JOY_BTN_1,
	JOY_BTN_2,
	JOY_BTN_3,
	JOY_BTN_4,
	JOY_BTN_5,
	JOY_BTN_6,
	JOY_BTN_7,
	JOY_BTN_8,
	JOY_BTN_9,
	JOY_BTN_LEFT = 0x10,
	JOY_BTN_RIGHT,
	JOY_BTN_MIDDLE,
	JOY_BTN_SIDE,
	JOY_BTN_EXTRA,
	JOY_BTN_FORWARD,
	JOY_BTN_BACK,
	JOY_BTN_TASK,
	JOY_BTN_TRIGGER = 0x20,
	JOY_BTN_THUMB,
	JOY_BTN_THUMB2,
	JOY_BTN_TOP,
	JOY_BTN_TOP2,
	JOY_BTN_PINKIE,
	JOY_BTN_BASE,
	JOY_BTN_BASE2,
	JOY_BTN_BASE3,
	JOY_BTN_BASE4,
	JOY_BTN_BASE5,
	JOY_BTN_BASE6,
	JOY_BTN_DEAD,
	JOY_BTN_A = 0x30,
	JOY_BTN_B,
	JOY_BTN_C,
	JOY_BTN_X,
	JOY_BTN_Y,
	JOY_BTN_Z,
	JOY_BTN_TL,
	JOY_BTN_TR,
	JOY_BTN_TL2,
	JOY_BTN_TR2,
	JOY_BTN_SELECT,
	JOY_BTN_START,
	JOY_BTN_MODE,
	JOY_BTN_THUMB_L,
	JOY_BTN_THUMB_R,
	JOY_BTN_WHEEL = 0x50,
	JOY_BTN_GEAR_UP,
} JoyBtnType;

typedef enum {
	JOY_MODE_MANUAL,
	JOY_MODE_MAINLOOP,
} JoyMode;

struct _JoyStick {
	GObject parent;
	JoyStickPrivate *priv;
};

struct _JoyStickClass {
	GObjectClass parent;

	/* mode */

	/* signals */
	guint button_pressed;
	guint button_released;
	guint axis_moved;
	guint disconnected;
};

/* constructors & class functions */
/** @brief Create a joystick object 
  *
  * This function creates a new JoyStick object.
  * @param devname the device node of the joystick to open
  * @return a newly-allocated JoyStick.
  * @warning this function always returns a JoyStick; but if the
  * device node cannot be opened for some reason, then it will not issue
  * any events. Use the joy_stick_is_valid() function to verify that
  * it can do anything useful.
  */
JoyStick* joy_stick_open(const gchar* devname);
/** @brief Enumerate all joystick device nodes on this system
  *
  * @return a GList of JoyStick objects.
  * @see joy_enum_free()
  */
GList* joy_stick_enumerate(GError**);
/** @brief Free the return value of joy_stick_enumerate() */
void joy_enum_free(GArray* enumeration);
/** @brief Describe a joystick without first calling joy_stick_open()
  *
  * @return the identity string of the joystick, or NULL in case of
  * error (with err set appropriately)
  * @param devname the device node of the joystick to identify
  * @note to get the requested information, this function will open the
  * joystick device, perform the correct ioctl() to retrieve the
  * requested data, and close the device again; so this function is no
  * substitute for when joy_stick_open() followed by
  * joy_stick_describe() fails for some reason (e.g., permission
  * issues)
  */
gchar* joy_stick_describe_unopened(gchar* devname, GError** err);
/* instance functions */
/** @brief Returns the devnode for a joystick */
gchar* joy_stick_get_devnode(JoyStick*, GError** err);
/** @brief Get the number of axes on this joystick
  *
  * @return the number of axes found on the given joystick, or 0 in
  * case of error (e.g., the JoyStick is not in a valid state).
  */
guchar joy_stick_get_axis_count(JoyStick*, GError** err);
/** @brief Get the number of buttons on this joystick
  *
  * @return the number of buttons found on this joystick, or 0 in case
  * of error (e.g., the JoyStick is not in a vaid state)
  */
guchar joy_stick_get_button_count(JoyStick*, GError** err);
/** @brief Identify this joystick
  *
  * @return a human-readable string identifying this joystick, as
  * returned by the kernel. This is usually a manufacturer name,
  * followed by a model name.
  */
gchar* joy_stick_describe(JoyStick*, GError** err);
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
  * @see joy_stick_get_axis_type
  */
gchar* joy_stick_describe_axis(JoyStick*, guchar axis);
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
  * @see joy_stick_get_button_type
  */
gchar* joy_stick_describe_button(JoyStick*, guchar button);
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
  * @see joy_stick_describe_button
  */
JoyBtnType joy_stick_get_button_type(JoyStick*, guchar button);
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
  * @see joy_stick_describe_axis
  */
JoyAxisType joy_stick_get_axis_type(JoyStick*, guchar axis);
/** @brief Select the mode in which to issue events
  *
  * @param mode the new mode.
  */
void joy_stick_set_mode(JoyStick*, JoyMode mode);
/** @brief Perform one iteration of handling joystick events and
  * issueing signals.
  *
  * @note This function will do nothing when the joystick is in
  * JOY_MODE_MAINLOOP mode.
  */
void joy_stick_iteration(JoyStick* self);
/** @brief read joystick events (in blocking mode) and issue signals
  *
  * This function will only return when the joystick device is closed.
  * @note This function will do nothing when the joystick is in
  * JOY_MODE_MAINLOOP mode.
  */
void joy_stick_loop(JoyStick* self);

/* type handling functions */
GType joy_stick_get_type(void) G_GNUC_PURE;
GQuark joy_get_errdomain(void) G_GNUC_PURE;
GType joy_error_get_type(void) G_GNUC_PURE;

#endif // LIBJOY_H
