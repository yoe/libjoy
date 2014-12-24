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
#include <stdint.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include <libudev.h>

#include <glib-unix.h>

#include <joy/joystick.h>
#include <joy-marshallers.h>

/* These two were shamelessly stolen from jstest.c */
char* axis_names[ABS_MAX + 1] = {
"X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder", 
"Wheel", "Gas", "Brake", "?", "?", "?", "?", "?",
"Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat3Y",
"?", "?", "?", "?", "?", "?", "?", 
};

char* button_names[KEY_MAX - BTN_MISC + 1] = {
"Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7", "Btn8", "Btn9", "?", "?", "?", "?", "?", "?",
"LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn", "TaskBtn", "?", "?", "?", "?", "?", "?", "?", "?",
"Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn", "BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6", "BtnDead",
"BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR", "BtnTL2", "BtnTR2", "BtnSelect", "BtnStart", "BtnMode", "BtnThumbL", "BtnThumbR", "?",
"?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", 
"WheelBtn", "Gear up",
};

/**
  * SECTION:joystick
  * @short_description: a joystick interface
  * @see_also: #JoyModel
  * @stability: Unstable
  * @include: joy/joystick.h
  */

#define NAME_LEN 128

static GHashTable* object_index = NULL;

struct _JoyStickPrivate {
	int fd;
	gboolean ready;
	uint8_t axmap[ABS_MAX + 1];
	uint16_t butmap[KEY_MAX - BTN_MISC + 1];
	uint8_t nbuts;
	uint8_t naxes;
	GArray* butvals;
	GArray* axvals;
	GArray* axevts;
	guint axintv;
	gchar name[NAME_LEN];
	gchar* devname;
	JoyMode mode;
	guint watch;
};

enum {
	JOY_OPEN = 1,
	JOY_BUTCNT,
	JOY_AXCNT,
	JOY_NAME,
	JOY_DEVNAME,
	JOY_INTV,
	JOY_PROP_COUNT,
};

static GParamSpec *props[JOY_PROP_COUNT] = { NULL, };

/** 
  * joy_stick_open: (constructor)
  * @devname: the device node of the joystick to open
  * 
  * This function creates a new #JoyStick object.
  *
  * Warning: this function always returns a #JoyStick; but if the
  * device node cannot be opened for some reason, then it will not issue
  * any events. Use the #JoyStick:open property to verify that
  * it can do anything useful.
  *
  * Returns: a newly-allocated #JoyStick.
  */
JoyStick* joy_stick_open(const gchar* devname) {
	JoyStick* js;
	if(!object_index || !g_hash_table_contains(object_index, devname)) {
		js = g_object_new(JOY_TYPE_STICK, "devnode", devname, NULL);
		/* Since it's base_init which creates our hash table, it might
		 * not actually exist until the above returns */
		g_hash_table_insert(object_index, (gchar*)devname, js);
	} else {
		js = JOY_STICK(g_hash_table_lookup(object_index, devname));
		g_object_ref(G_OBJECT(js));
	}
	return js;
}

/** 
  * joy_stick_enum_free: (skip)
  * @enumeration: the enumeration to free.
  *
  * Free the return value of joy_stick_enumerate(). This is a
  * convenience function; it simply calls g_object_unref() on each of
  * the elements of the list, and frees the list nodes.
  *
  */
void joy_stick_enum_free(GList* enumeration) {
	while(enumeration) {
		g_object_unref(JOY_STICK(enumeration->data));
		GList* next = enumeration->next;
		g_free(enumeration);
		enumeration = next;
	}
}

/** 
  * joy_stick_enumerate:
  * @err: a #GError
  *
  * Enumerate all joystick device nodes on this system
  *
  * See also joy_stick_enum_free()
  *
  * Returns: (element-type Joy.Stick) (transfer full): a #GList of #JoyStick
  * objects, or %NULL in case of error (with @err set appropriately)
  */
GList* joy_stick_enumerate(GError** err) {
	GList* retval = NULL;
	struct udev* udev;
	struct udev_enumerate *enumer;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	JoyStick *joy;

	udev = udev_new();
	if(!udev) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_UDEV, "Could not initialize udev");
		return NULL;
	}

	enumer = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumer, "input");
	udev_enumerate_scan_devices(enumer);
	devices = udev_enumerate_get_list_entry(enumer);
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char* path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		const char* name = udev_device_get_sysname(dev);
		char* devnode = g_strdup(udev_device_get_devnode(dev));
		if(name[0] == 'j' && name[1] == 's') {
			joy = joy_stick_open(devnode);
			retval = g_list_append(retval, joy);
		}
	}
	return retval;
}

/** 
  * joy_stick_describe_unopened:
  * @devname: the path of the joystick device node to describe.
  * @err: a #GError
  *
  * Describe a joystick without first calling joy_stick_open().
  *
  * Note that to get the requested information, this function will open
  * the joystick device, perform the correct ioctl() to retrieve the
  * requested data, and close the device again; so this function is no
  * substitute for when joy_stick_open() followed by
  * joy_stick_describe() fails for some reason (e.g., permission issues)
  *
  * Returns: (transfer full): the identity string of the joystick, or %NULL in
  * case of error (with @err set appropriately)
  */
gchar* joy_stick_describe_unopened(gchar* devname, GError** err) {
	JoyStick* joy = joy_stick_open(devname);
	gchar* retval = joy_stick_describe(joy, err);
	g_object_unref(joy);
	return retval;
}

static gboolean handle_joystick_event(gint fd, GIOCondition cond, gpointer user_data) {
	JoyStick* self = JOY_STICK(user_data);
	if(cond & G_IO_IN) {
		joy_stick_iteration(self);
	} else {
		g_hash_table_remove(object_index, self->priv->devname);
		close(self->priv->fd);
		g_signal_emit(self, JOY_STICK_GET_CLASS(self)->disconnected, 0, NULL);
		self->priv->ready = FALSE;
		return FALSE;
	}
	return TRUE;
}

static gboolean joy_stick_reopen(JoyStick* self, GError** err) {
	self->priv->ready = FALSE;
	if(self->priv->fd >= 0) {
		close(self->priv->fd);
		g_array_set_size(self->priv->butvals, 0);
		g_array_set_size(self->priv->axvals, 0);
		g_array_set_size(self->priv->axevts, 0);
	}
	if(!self->priv->devname) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_NDEV, "Could not open joystick: no device name provided!");
		self->priv->fd = -1;
		return FALSE;
	}
	self->priv->fd = open(self->priv->devname, O_RDONLY);
	if(self->priv->fd < 0) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not open %s: %s", self->priv->devname, strerror(errno));
		return FALSE;
	}
	ioctl(self->priv->fd, JSIOCGAXMAP, self->priv->axmap);
	ioctl(self->priv->fd, JSIOCGBTNMAP, self->priv->butmap);
	ioctl(self->priv->fd, JSIOCGAXES, &(self->priv->naxes));
	g_array_set_size(self->priv->axvals, self->priv->naxes);
	g_array_set_size(self->priv->axevts, self->priv->naxes);
	ioctl(self->priv->fd, JSIOCGBUTTONS, &(self->priv->nbuts));
	g_array_set_size(self->priv->butvals, self->priv->nbuts);
	ioctl(self->priv->fd, JSIOCGNAME(NAME_LEN), self->priv->name);
	self->priv->watch = g_unix_fd_add(self->priv->fd, G_IO_IN | G_IO_ERR | G_IO_HUP, handle_joystick_event, self);
	self->priv->ready = TRUE;
	return TRUE;
}

/** 
  * joy_stick_get_axis_count:
  * @self: a #JoyStick
  * @err: a #GError
  *
  * Get the number of axes on this joystick
  *
  * Returns: the number of axes found on the given joystick, or 0 in
  * case of error (e.g., the #JoyStick is not in a valid state).
  */
guint8 joy_stick_get_axis_count(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the axis count: joystick not ready yet!");
		return 0;
	}
	return self->priv->naxes;
}

/** 
  * joy_stick_get_button_count:
  * @self: a #JoyStick
  * @err: a #GError
  *
  * Get the number of buttons on this joystick.
  *
  * Returns: the number of buttons found on this joystick, or 0 in case
  * of error (e.g., the JoyStick is not in a vaid state)
  */
guint8 joy_stick_get_button_count(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the button count: joystick not ready yet!");
		return 0;
	}
	return self->priv->nbuts;
}

/** 
  * joy_stick_describe:
  * @self: a #JoyStick
  * @err: a #GError
  * 
  * Identify this joystick.
  *
  * Returns: (transfer none): a human-readable string identifying this
  * joystick, as returned by the kernel. This is usually a manufacturer name,
  * followed by a model name. In case of error, %NULL is returned (with @err
  * set appropriately)
  */
gchar* joy_stick_describe(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the joystick name: joystick not ready yet!");
		return 0;
	}
	return self->priv->name;
}

/** 
  * joy_stick_get_devnode:
  * @self: a #JoyStick
  * @err: a #GError
  *
  * Look up the `/dev` entry to which this joystick is connected.
  *
  * Returns: (transfer none): the devnode for a joystick, or %NULL in case of
  * error (with @err set appropriately)
  */
gchar* joy_stick_get_devnode(JoyStick* self, GError** err) {
	return self->priv->devname;
}

/** 
  * joy_stick_describe_axis:
  * @self: a #JoyStick
  * @axis: the axis to identify
  *
  * Identify a given axis.
  *
  * There are several types of axes that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick axis is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis in
  * human-readable format.
  * 
  * See also joy_stick_get_axis_type()
  *
  * Returns: (transfer none): a human-readable string describing the axis
  * (e.g., "Throttle" or "X")
  */
gchar* joy_stick_describe_axis(JoyStick* self, guint8 axis) {
	return axis_names[self->priv->axmap[axis]];
}

/** 
  * joy_stick_describe_button:
  * @self: a #JoyStick
  * @button: the button to identify
  *
  * Identify a given button
  *
  * There are several types of buttons that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick button is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick button in
  * human-readable format.
  * 
  * See also joy_stick_get_button_type()
  *
  * Returns: (transfer none): a human-readable string describing the button
  * (e.g., "Trigger" or "A")
  */
gchar* joy_stick_describe_button(JoyStick* self, guint8 button) {
	g_assert(button < self->priv->nbuts);
	return button_names[self->priv->butmap[button] - BTN_MISC];
}

/** 
  * joy_stick_get_button_type:
  * @self: a #JoyStick
  * @button: the button to identify
  *
  * Get the type of a given button
  *
  * There are several types of buttons that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick button is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis
  *
  * See also joy_stick_describe_button()
  *
  * Returns: the type of the button
  */
JoyBtnType joy_stick_get_button_type(JoyStick* self, guchar button) {
	g_assert(button < self->priv->nbuts);
	return (enum joy_button_type)(self->priv->butmap[button] - BTN_MISC);
}

/** 
  * joy_stick_get_axis_type:
  * @self: a #JoyStick
  * @axis: the axis to identify
  * 
  * Identify a given axis
  *
  * There are several types of axes that can be found on joysticks, and
  * they each have different functions. The exact intended functionality
  * of a joystick axis is assigned by the manufacturer, and is passed on
  * to the system as part of the HID protocol.
  *
  * This function will return the type of a joystick axis.
  *
  * See also joy_stick_describe_axis()
  * 
  * Returns: the type of the axis
  */
JoyAxisType joy_stick_get_axis_type(JoyStick* self, guchar axis) {
	return (enum joy_axis_type)(self->priv->axmap[axis]);
}

static void instance_init(GTypeInstance* instance, gpointer g_class) {
	JoyStick *self = JOY_STICK(instance);

	self->priv = g_new0(JoyStickPrivate, 1);
	self->priv->ready = FALSE;
	self->priv->mode = JOY_MODE_MAINLOOP;
	self->priv->fd = -1;
	self->priv->butvals = g_array_new(FALSE, TRUE, sizeof(gboolean));
	self->priv->axvals = g_array_new(FALSE, TRUE, sizeof(gint16));
	self->priv->axevts = g_array_new(FALSE, TRUE, sizeof(guint32));
}

static void get_property(GObject* object, guint property_id, GValue *value, GParamSpec *pspec) {
	JoyStick *self = JOY_STICK(object);
	switch(property_id) {
	case JOY_OPEN:
		g_value_set_boolean(value, self->priv->fd >= 0 ? TRUE : FALSE);
		break;
	case JOY_BUTCNT:
		g_value_set_uchar(value, self->priv->nbuts);
		break;
	case JOY_AXCNT:
		g_value_set_uchar(value, self->priv->naxes);
		break;
	case JOY_NAME:
		g_value_set_string(value, self->priv->name);
		break;
	case JOY_DEVNAME:
		g_value_set_string(value, self->priv->devname);
		break;
	case JOY_INTV:
		g_value_set_uint(value, self->priv->axintv);
		break;
	default:
		g_assert_not_reached();
	}
}

static void finalize(GObject* object) {
	JoyStick* self = JOY_STICK(object);

	if(self->priv->fd >= 0) {
		close(self->priv->fd);
	}
	if(self->priv->ready) {
		g_source_remove(self->priv->watch);
	}
	if(self->priv->butvals) {
		g_array_free(self->priv->butvals, TRUE);
	}
	if(self->priv->axvals) {
		g_array_free(self->priv->axvals, TRUE);
	}
	if(self->priv->axevts) {
		g_array_free(self->priv->axevts, TRUE);
	}
	g_hash_table_remove(object_index, self->priv->devname);
	if(self->priv->devname) {
		g_free(self->priv->devname);
	}
	g_free(self->priv);
}

static void set_property(GObject* object, guint property_id, const GValue *value, GParamSpec *pspec) {
	JoyStick *self = JOY_STICK(object);
	switch(property_id) {
	case JOY_DEVNAME:
		self->priv->devname = g_value_dup_string(value);
		joy_stick_reopen(self, NULL);
		break;
	case JOY_INTV:
		self->priv->axintv = g_value_get_uint(value);
		break;
	default:
		g_assert_not_reached();
	}
}

static void base_init(gpointer klass G_GNUC_UNUSED) {
	g_assert(object_index == NULL);
	object_index = g_hash_table_new(g_str_hash, g_str_equal);
}

static void base_finalize(gpointer klass G_GNUC_UNUSED) {
	g_assert(g_hash_table_size(object_index) == 0);
	g_hash_table_destroy(object_index);
}

static void class_init(gpointer g_class, gpointer g_class_data) {
	GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
	JoyStickClass *klass = JOY_STICK_CLASS(g_class);

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->finalize = finalize;
/**
  * JoyStick::button-pressed:
  * @object: the object which received the signal.
  * @button: the number of the button that was pressed.
  *
  * The #JoyStick::button-pressed signal is emitted when a button on the
  * joystick is pressed.
  *
  * The signal will have a detail of the button. E.g., when button 0 is
  * pressed, the detailed event will be `button-pressed:0`. As such, it
  * is possible to only connect to this event for the button(s) one is
  * interested in.
  */
	klass->button_pressed =
	  g_signal_new("button-pressed",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
/**
  * JoyStick::button-released:
  * @object: the object which received the signal.
  * @button: the number of the button that was released.
  *
  * The #JoyStick::button-released signal is emitted when a button on the
  * joystick is released.
  *
  * The signal will have a detail of the button. E.g., when button 0 is
  * released, the detailed event will be `button-released:0`.  As such,
  * it is possible to only connect to this event for the button(s) one
  * is interested in.
  */
	klass->button_released =
	  g_signal_new("button-released",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
/**
  * JoyStick::axis-moved:
  * @object: the object which received the signal.
  * @axis: the number of the axis that was moved.
  * @newval: the new value of the axis.
  *
  * The #JoyStick::axis-moved signal is emitted when an axis on the
  * joystick changes its value. However, it will never be issued
  * more often than permitted by the #JoyStick:axis-interval
  * property.
  *
  * The signal will have a detail of the button. E.g., when axis 0
  * changes its value, the detailed event will be `axis-moved:0`. As
  * such, it is possible to only connect to this event for the axis (or
  * axes) one is interested in.
  */
	klass->axis_moved =
	  g_signal_new("axis-moved",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED,
				0,
				NULL,
				NULL,
				joy_cclosure_marshal_VOID__UCHAR_INT,
				G_TYPE_NONE,
				2,
				G_TYPE_UCHAR,
				G_TYPE_INT);
/**
  * JoyStick::disconnected:
  * @object: the object which received the signal.
  *
  * When this signal is emitted, the joystick that was in use is
  * no longer available (e.g., because the user disconnected the
  * joystick from the system).
  *
  * An application should g_object_unref() the joystick, and
  * possibly check to see if the user wants to use another
  * joystick instead.
  *
  * It is not possible to reconnect a #JoyStick object that has lost its
  * hardware.
  */
	klass->disconnected =
	  g_signal_new("disconnected",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0);
/**
 * JoyStick:open:
 *
 * Whether the device is open and available for use. This should always
 * be the case, except in case of error, or after the
 * #JoyStick::disconnected signal has been emitted.
 */
	props[JOY_OPEN] = 
	  g_param_spec_boolean("open",
				     "Open",
				     "Whether the device has been opened.",
				     FALSE,
				     G_PARAM_READABLE);
/**
 * JoyStick:button-count:
 *
 * The number of buttons this joystick has.
 */
	props[JOY_BUTCNT] = 
	  g_param_spec_uchar("button-count",
				   "Button count",
				   "The number of buttons on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
/**
 * JoyStick:axis-count:
 *
 * The number of axes this joystick has.
 */
	props[JOY_AXCNT] =
	  g_param_spec_uchar("axis-count",
				   "Axis count",
				   "The number of axes on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
/**
 * JoyStick:name:
 */
	props[JOY_NAME] =
	  g_param_spec_string("name",
				    "name",
				    "The name of the device (as reported by the kernel)",
				    "",
				    G_PARAM_READABLE);
/**
 * JoyStick:devnode:
 */
	props[JOY_DEVNAME] =
	  g_param_spec_string("devnode",
				    "device-node",
				    "The device name of this joystick",
				    "/dev/input/js0",
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
/**
 * JoyStick:axis-interval:
 *
 * The minimum interval between axis events, in milliseconds.
 */
	props[JOY_INTV] =
	  g_param_spec_uint("axis-interval",
				 "Axis interval",
				 "The minimum interval between two issued axis events (in milliseconds)",
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READWRITE);
	g_object_class_install_properties(gobject_class, JOY_PROP_COUNT, props);
}

GType joy_stick_get_type(void) {
	static GType type = 0;
	if(!type) {
		static const GTypeInfo info = {
			sizeof(JoyStickClass),
			base_init,	/* base_init */
			base_finalize,	/* base_finalize */
			class_init,	/* class_init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(JoyStick),
			0,	/* n_preallocs */
			instance_init,
		};
		type = g_type_register_static(G_TYPE_OBJECT,
					      "JoyStick",
					      &info, 0);
	}

	return type;
}

/** 
  * joy_stick_iteration:
  * @self: a #JoyStick
  *
  * Perform one iteration of handling joystick events and
  * issuing signals.
  *
  * Note: This function will do nothing when the joystick is in
  * %JOY_MODE_MAINLOOP mode.
  */
void joy_stick_iteration(JoyStick* self) {
	struct js_event ev;
	int rv;
	if((rv = read(self->priv->fd, &ev, sizeof(ev))) < 0) {
		return;
	}
	/* XXX if(ev.type & JS_EVENT_INIT) */
	ev.type &= ~JS_EVENT_INIT;
	gchar* name = g_strdup_printf("%u", ev.number);
	GQuark quark = g_quark_from_string(name);
	g_free(name);
	switch(ev.type) {
		case JS_EVENT_BUTTON:
			g_array_index(self->priv->butvals, gboolean, ev.number) = ev.value ? TRUE : FALSE;
			if(ev.value) {
				g_signal_emit(self, JOY_STICK_GET_CLASS(self)->button_pressed, quark, ev.number);
			} else {
				g_signal_emit(self, JOY_STICK_GET_CLASS(self)->button_released, quark, ev.number);
			}
			break;
		case JS_EVENT_AXIS:
			g_array_index(self->priv->butvals, gint16, ev.number) = ev.value;
			if(ev.time > (g_array_index(self->priv->axevts, guint32, ev.number) + self->priv->axintv)) {
				g_signal_emit(self, JOY_STICK_GET_CLASS(self)->axis_moved, quark, ev.number, ev.value);
			}
			break;
		default:
			break;
	}
	return;
}

/** 
  * joy_stick_loop:
  * @self: a #JoyStick
  *
  * Read joystick events (in blocking mode) and issue signals.
  *
  * This function will only return when the joystick device is closed.
  * 
  * Note: This function will do nothing when the joystick is in
  * %JOY_MODE_MAINLOOP mode.
  */
void joy_stick_loop(JoyStick* self) {
	while(TRUE) {
		joy_stick_iteration(self);
	}
}

/** 
  * joy_stick_set_mode:
  * @self: a #JoyStick
  * @mode: the new mode.
  *
  * Select the mode in which to issue events
  *
  */
void joy_stick_set_mode(JoyStick* self, JoyMode mode) {
	if(self->priv->mode != mode) {
		if(mode == JOY_MODE_MANUAL) {
			g_source_remove(self->priv->watch);
		} else {
			self->priv->watch = g_unix_fd_add(self->priv->fd, G_IO_IN | G_IO_ERR | G_IO_HUP, handle_joystick_event, self);
		}
		self->priv->mode = mode;
	}
}

/**
  * joy_stick_get_typed_axis:
  * @self: a #JoyStick
  * @type: the wanted axis type
  * @err: a #GError
  *
  * Check for an axis with the given type. If the joystick does not have such
  * an axis, -1 is returned. If an error occurs, -2 is returned.
  *
  * Returns: the number of the axis of the given type (a number from 0 to 255),
  * -1, or -2 (with @err set appropriately).
  */
gint16 joy_stick_get_typed_axis(JoyStick* self, JoyAxisType type, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not look up axis: joystick not initialized!");
		return -2;
	}
	for(int i=0; i<self->priv->naxes; i++) {
		if((JoyAxisType)(self->priv->axmap[i]) == type) {
			return i;
		}
	}
	return -1;
}

/**
  * joy_stick_get_typed_button:
  * @self: a #JoyStick
  * @type: the wanted button type
  * @err: a #GError
  *
  * Check for a button with the given type. If the joystick does not have such
  * a button or an error occurs, -1 is returned.
  *
  * Returns: the number of the axis of the given type (a number from 0 to 255),
  * or -1 (with @err set appropriately).
  */
gint16 joy_stick_get_typed_button(JoyStick* self, JoyBtnType type, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not look up axis: joystick not initialized!");
		return -2;
	}
	for(int i=0; i<self->priv->nbuts; i++) {
		if((JoyBtnType)(self->priv->butmap[i]) == type) {
			return i;
		}
	}
	return -1;
}

GQuark joy_get_errdomain(void) {
	return g_quark_from_string("Joy");
}
