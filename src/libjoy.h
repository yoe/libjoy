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
JoyStick* joy_stick_open(const gchar* devname);
GList* joy_stick_enumerate(GError** err);
void joy_stick_enum_free(GArray* enumeration);
gchar* joy_stick_describe_unopened(gchar* devname, GError** err);
/* instance functions */
gchar* joy_stick_get_devnode(JoyStick* stick, GError** err);
guint8 joy_stick_get_axis_count(JoyStick* stick, GError** err);
guint8 joy_stick_get_button_count(JoyStick* stick, GError** err);
gchar* joy_stick_describe(JoyStick* stick, GError** err);
gchar* joy_stick_describe_axis(JoyStick* stick, guchar axis);
gchar* joy_stick_describe_button(JoyStick* stick, guchar button);
JoyBtnType joy_stick_get_button_type(JoyStick* stick, guchar button);
JoyAxisType joy_stick_get_axis_type(JoyStick* stick, guchar axis);
void joy_stick_set_mode(JoyStick* stick, JoyMode mode);
void joy_stick_iteration(JoyStick* stick);
void joy_stick_loop(JoyStick* stick);

/* type handling functions */
GType joy_stick_get_type(void) G_GNUC_PURE;
GQuark joy_get_errdomain(void) G_GNUC_PURE;

#endif // LIBJOY_H
