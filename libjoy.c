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

#include <libjoy.h>
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
};

JoyStick* joy_stick_open(const gchar* devname) {
	JoyStick* js;
	if(!object_index || !g_hash_table_contains(object_index, devname)) {
		js = g_object_new(JOY_STICK_TYPE, "devnode", devname, NULL);
		/* Since it's base_init which creates our hash table, it might
		 * not actually exist until the above returns */
		g_hash_table_insert(object_index, (gchar*)devname, js);
	} else {
		js = JOY_STICK(g_hash_table_lookup(object_index, devname));
		g_object_ref(G_OBJECT(js));
	}
	return js;
}

void joy_enum_free(GArray* enumeration) {
	for(int i=0; i<enumeration->len; i++) {
		g_object_unref(g_array_index(enumeration, JoyStick*, i));
	}
	g_array_free(enumeration, TRUE);
}

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

guint8 joy_stick_get_axis_count(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the axis count: joystick not ready yet!");
		return 0;
	}
	return self->priv->naxes;
}

guchar joy_stick_get_button_count(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the button count: joystick not ready yet!");
		return 0;
	}
	return self->priv->nbuts;
}

gchar* joy_stick_describe(JoyStick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_DEV_NREADY, "Could not get the joystick name: joystick not ready yet!");
		return 0;
	}
	return self->priv->name;
}

gchar* joy_stick_get_devnode(JoyStick* self, GError** err) {
	return self->priv->devname;
}

gchar* joy_stick_describe_axis(JoyStick* self, guint8 axis) {
	return axis_names[self->priv->axmap[axis]];
}

gchar* joy_stick_describe_button(JoyStick* self, guint8 button) {
	g_assert(button < self->priv->nbuts);
	return button_names[self->priv->butmap[button] - BTN_MISC];
}

enum joy_button_type joy_stick_get_button_type(JoyStick* self, guchar button) {
	g_assert(button < self->priv->nbuts);
	return (enum joy_button_type)(self->priv->butmap[button] - BTN_MISC);
}

enum joy_axis_type joy_stick_get_axis_type(JoyStick* self, guchar axis) {
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
	GParamSpec *pspec;

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->finalize = finalize;
	klass->button_pressed = g_signal_new("button-pressed",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
	klass->button_released = g_signal_new("button-released",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
	klass->axis_moved = g_signal_new("axis-moved",
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
	klass->disconnected = g_signal_new("disconnected",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0);
	pspec = g_param_spec_boolean("open",
				     "Open",
				     "Whether the device has been opened",
				     FALSE,
				     G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					JOY_OPEN,
					pspec);
	pspec = g_param_spec_uchar("button-count",
				   "Button count",
				   "The number of buttons on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					JOY_BUTCNT,
					pspec);
	pspec = g_param_spec_uchar("axis-count",
				   "Axis count",
				   "The number of axes on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					JOY_AXCNT,
					pspec);
	pspec = g_param_spec_string("name",
				    "name",
				    "The name of the device (as reported by the kernel)",
				    "",
				    G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					JOY_NAME,
					pspec);
	pspec = g_param_spec_string("devnode",
				    "device-node",
				    "The device name of this joystick",
				    "/dev/input/js0",
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property(gobject_class,
					JOY_DEVNAME,
					pspec);
	pspec = g_param_spec_uint("axis-interval",
				 "Axis interval",
				 "The minimum interval between two issued axis events (in milliseconds)",
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class,
					JOY_INTV,
					pspec);
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
					      "JoyStickType",
					      &info, 0);
	}

	return type;
}

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

void joy_stick_loop(JoyStick* self) {
	while(TRUE) {
		joy_stick_iteration(self);
	}
}

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

GQuark joy_get_errdomain(void) {
	return g_quark_from_string("Joy");
}

GType joy_error_get_type(void) {
	/* XXX */

	return -1;
}
