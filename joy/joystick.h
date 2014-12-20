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

G_BEGIN_DECLS

#define JOY_TYPE_STICK		(joy_stick_get_type())
#define JOY_STICK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), JOY_TYPE_STICK, JoyStick))
#define JOY_STICK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST((vtable), JOY_TYPE_STICK, JoyStickClass))
#define JOY_IS_STICK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), JOY_TYPE_STICK))
#define JOY_IS_STICK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE((vtable), JOY_TYPE_STICK))
#define JOY_STICK_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS((inst), JOY_TYPE_STICK, JoyStickClass))
#define JOY_ERROR_DOMAIN		(joy_get_errdomain())

/**
  * JoyError:
  * @JOY_ERR_DEV_NREADY: An operation was performed on a JoyStick which requires a joystick, but none was found on the provided device node.
  * @JOY_ERR_NDEV: Error opening the joystick: device name not given.
  * @JOY_ERR_UDEV: Error while dealing with udev.
  * @JOY_ERR_NJS: No joysticks were found (for "enumerate" style functions).
  *
  * possible error values for libjoy
  */
typedef enum {
	JOY_ERR_DEV_NREADY,
	JOY_ERR_NDEV,
	JOY_ERR_UDEV,
	JOY_ERR_NJS,
} JoyError;

/* The next two enums were defined based on jstest.c */
/**
  * JoyAxisType:
  * @JOY_AXIS_X: X
  * @JOY_AXIS_Y: Y
  * @JOY_AXIS_Z: Z
  * @JOY_AXIS_RX: RX
  * @JOY_AXIS_RY: RY
  * @JOY_AXIS_RZ: RZ
  * @JOY_AXIS_THROTTLE: Throttle
  * @JOY_AXIS_RUDDER: Rudder
  * @JOY_AXIS_WHEEL: Wheel
  * @JOY_AXIS_GAS: Gas
  * @JOY_AXIS_BRAKE: Brake
  * @JOY_AXIS_HAT0X: Hat 0, X
  * @JOY_AXIS_HAT0Y: Hat 0, Y
  * @JOY_AXIS_HAT1X: Hat 1, X
  * @JOY_AXIS_HAT1Y: Hat 1, Y
  * @JOY_AXIS_HAT2X: Hat 2, X
  * @JOY_AXIS_HAT2Y: Hat 2, Y
  * @JOY_AXIS_HAT3X: Hat 3, X
  * @JOY_AXIS_HAT3Y: Hat 3, Y
  *
  * the type of an axis.
  */
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

/**
  * JoyBtnType:
  * @JOY_BTN_0: 0
  * @JOY_BTN_1: 1
  * @JOY_BTN_2: 2
  * @JOY_BTN_3: 3
  * @JOY_BTN_4: 4
  * @JOY_BTN_5: 5
  * @JOY_BTN_6: 6
  * @JOY_BTN_7: 7
  * @JOY_BTN_8: 8
  * @JOY_BTN_9: 9
  * @JOY_BTN_LEFT: Left
  * @JOY_BTN_RIGHT: Right
  * @JOY_BTN_MIDDLE: Middle
  * @JOY_BTN_SIDE: Side
  * @JOY_BTN_EXTRA: Extra
  * @JOY_BTN_FORWARD: Forward
  * @JOY_BTN_BACK: Back
  * @JOY_BTN_TASK: Task
  * @JOY_BTN_TRIGGER: Trigger
  * @JOY_BTN_THUMB: Thumb
  * @JOY_BTN_THUMB2: Thumb 2
  * @JOY_BTN_TOP: Top
  * @JOY_BTN_TOP2: Top 2
  * @JOY_BTN_PINKIE: Pinkie
  * @JOY_BTN_BASE: Base
  * @JOY_BTN_BASE2: Base 2
  * @JOY_BTN_BASE3: Base 3
  * @JOY_BTN_BASE4: Base 4
  * @JOY_BTN_BASE5: Base 5
  * @JOY_BTN_BASE6: Base 6
  * @JOY_BTN_DEAD: Dead
  * @JOY_BTN_A: A
  * @JOY_BTN_B: B
  * @JOY_BTN_C: C
  * @JOY_BTN_X: X
  * @JOY_BTN_Y: Y
  * @JOY_BTN_Z: Z
  * @JOY_BTN_TL: TL
  * @JOY_BTN_TR: TR
  * @JOY_BTN_TL2: TL2
  * @JOY_BTN_TR2: TR2
  * @JOY_BTN_SELECT: Select
  * @JOY_BTN_START: Start
  * @JOY_BTN_MODE: Mode
  * @JOY_BTN_THUMB_L: Thumb L
  * @JOY_BTN_THUMB_R: Thumb R
  * @JOY_BTN_WHEEL: Wheel
  * @JOY_BTN_GEAR_UP: Gear up
  *
  * The type of a button.
  */
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

/**
  * JoyMode:
  * @JOY_MODE_MANUAL: libjoy will do nothing; the program must call
  * joy_stick_iteration() or joy_stick_loop() for events to be issued.
  * @JOY_MODE_MAINLOOP: libjoy will add a #GSource to the glib mainloop, and
  * will issue events from that #GSource.
  *
  * The mode in which a joystick is running.
  */
typedef enum {
	JOY_MODE_MANUAL,
	JOY_MODE_MAINLOOP,
} JoyMode;

typedef struct _JoyStick JoyStick;
typedef struct _JoyStickClass JoyStickClass;
typedef struct _JoyStickPrivate JoyStickPrivate;

/**
  * JoyStick:
  *
  * Opaque object representing a joystick
  */
struct _JoyStick {
	/*< private >*/
	GObject parent;
	JoyStickPrivate *priv;
};

/**
  * JoyStickClass:
  * @button_pressed: signal emitted when a button is pressed
  * @button_released: signal emitted when a button is released
  * @axis_moved: signal emitted when an axis is removed
  * @disconnected: signal emitted when the joystick is disconnected.
  *
  * The signals are only visible so that subclasses (if any) can  use
  * them.
  */
struct _JoyStickClass {
	/*< private >*/
	GObjectClass parent;

	/* signals */
	/*< public >*/
	guint button_pressed;
	guint button_released;
	guint axis_moved;
	guint disconnected;
};

/* constructors & class functions */
JoyStick* joy_stick_open(const gchar* devname);
GList* joy_stick_enumerate(GError** err);
void joy_stick_enum_free(GList* enumeration);
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

G_END_DECLS

#endif // LIBJOY_H
