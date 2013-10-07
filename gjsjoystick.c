#include <stdint.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include <gjsjoystick.h>
#include <gjs-marshallers.h>

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

typedef struct _GjsJoystickSource GjsJoystickSource;

static GHashTable* object_index = NULL;

struct _GjsJoystickSource {
	GSource parent;
	GjsJoystick* js;
	gpointer tag;
};

struct _GjsJoystickPrivate {
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
	GjsMode mode;
	GjsJoystickSource* source;
};

enum {
	GJS_OPEN = 1,
	GJS_BUTCNT,
	GJS_AXCNT,
	GJS_NAME,
	GJS_DEVNAME,
	GJS_INTV,
};

GjsJoystick* gjs_joystick_open(gchar* devname) {
	/* TODO: canonicalize filename */
	if(!g_hash_table_contains(object_index, devname)) {
		g_hash_table_insert(object_index, devname, g_object_new(GJS_JOYSTICK_TYPE, "devnode", devname, NULL));
	}
	return GJS_JOYSTICK(g_hash_table_lookup(object_index, devname));
}

GArray* gjs_joystick_enumerate(GError** err) {
	struct dirent* de;
	DIR* d;
	GjsDetails det;
	GArray* arr;

	d = opendir("/dev/input");
	if(!d) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_NDIR, "Could not open directory /dev/input: %s", strerror(errno));
		return NULL;
	}

	int errno_s = errno;
	arr = g_array_new(TRUE, FALSE, sizeof(GjsDetails));

	while((de = readdir(d)) != NULL) {
		if(de->d_name[0] == 'j' && de->d_name[1] == 's') {
			det.devname = g_strdup_printf("/dev/input/%s", de->d_name);
			det.model = gjs_joystick_describe_unopened(det.devname, err);
			if(!det.model) {
				goto err_exit;
			}
			g_array_append_val(arr, det);
		}
		errno_s = errno;
	}

	if(errno_s != errno) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_NDIR, "Could not read directory /dev/input: %s", strerror(errno));
err_exit:
		g_array_free(arr, TRUE);
		return NULL;
	}
	return arr;
}

void gjs_enum_free(GArray* value) {
	for(int i=0; i<value->len; i++) {
		GjsDetails det = g_array_index(value, GjsDetails, i);
		g_free(det.devname);
		g_free(det.model);
	}
	g_array_free(value, TRUE);
}

gchar* gjs_joystick_describe_unopened(gchar* devname, GError** err) {
	GjsJoystick* gjs = gjs_joystick_open(devname);
	gchar* retval = gjs_joystick_describe(gjs, err);
	g_object_unref(gjs);
	return retval;
}

gboolean gjs_joystick_reopen(GjsJoystick* self, gchar* devname, GError** err) {
	self->priv->ready = FALSE;
	if(self->priv->fd >= 0) {
		close(self->priv->fd);
		g_array_set_size(self->priv->butvals, 0);
		g_array_set_size(self->priv->axvals, 0);
		g_array_set_size(self->priv->axevts, 0);
	}
	if(devname) {
		if(self->priv->devname) {
			g_free(self->priv->devname);
		}
		self->priv->devname = g_strdup(devname);
	}
	if(!self->priv->devname) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_NDEV, "Could not open joystick: no device name provided!");
		self->priv->fd = -1;
		return FALSE;
	}
	self->priv->fd = open(self->priv->devname, O_RDWR);
	if(self->priv->fd < 0) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_DEV_NREADY, "Could not open %s: %s", devname, strerror(errno));
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
	self->priv->ready = TRUE;
	return TRUE;
}

guint8 gjs_joystick_get_axis_count(GjsJoystick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_DEV_NREADY, "Could not get the axis count: joystick not ready yet!");
		return 0;
	}
	return self->priv->naxes;
}

guchar gjs_joystick_get_button_count(GjsJoystick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_DEV_NREADY, "Could not get the button count: joystick not ready yet!");
		return 0;
	}
	return self->priv->nbuts;
}

gchar* gjs_joystick_describe(GjsJoystick* self, GError** err) {
	if(!self->priv->ready) {
		g_set_error(err, GJS_ERROR_DOMAIN, GJS_ERR_DEV_NREADY, "Could not get the joystick name: joystick not ready yet!");
		return 0;
	}
	return self->priv->name;
}

gchar* gjs_joystick_describe_axis(GjsJoystick* self, guint8 axis) {
	return axis_names[self->priv->axmap[axis]];
}

gchar* gjs_joystick_describe_button(GjsJoystick* self, guint8 button) {
	g_assert(button < self->priv->nbuts);
	return button_names[self->priv->butmap[button] - BTN_MISC];
}

enum gjs_button_type gjs_joystick_get_button_type(GjsJoystick* self, guchar button) {
	g_assert(button < self->priv->nbuts);
	return (enum gjs_button_type)(self->priv->butmap[button] - BTN_MISC);
}

enum gjs_axis_type gjs_joystick_get_axis_type(GjsJoystick* self, guchar axis) {
	return (enum gjs_axis_type)(self->priv->axmap[axis]);
}

static void instance_init(GTypeInstance* instance, gpointer g_class) {
	GjsJoystick *self = GJS_JOYSTICK(instance);

	self->priv = g_new0(GjsJoystickPrivate, 1);
	self->priv->ready = FALSE;
	self->priv->mode = GJS_MODE_MAINLOOP;
	self->priv->fd = -1;
	self->priv->butvals = g_array_new(FALSE, TRUE, sizeof(gboolean));
	self->priv->axvals = g_array_new(FALSE, TRUE, sizeof(gint16));
	self->priv->axevts = g_array_new(FALSE, TRUE, sizeof(guint32));
}

static void get_property(GObject* object, guint property_id, GValue *value, GParamSpec *pspec) {
	GjsJoystick *self = GJS_JOYSTICK(object);
	switch(property_id) {
	case GJS_OPEN:
		g_value_set_boolean(value, self->priv->fd >= 0 ? TRUE : FALSE);
		break;
	case GJS_BUTCNT:
		g_value_set_uchar(value, self->priv->nbuts);
		break;
	case GJS_AXCNT:
		g_value_set_uchar(value, self->priv->naxes);
		break;
	case GJS_NAME:
		g_value_set_string(value, self->priv->name);
		break;
	case GJS_DEVNAME:
		g_value_set_string(value, self->priv->devname);
		break;
	case GJS_INTV:
		g_value_set_uint(value, self->priv->axintv);
		break;
	default:
		g_assert_not_reached();
	}
}

static void finalize(GObject* object) {
	GjsJoystick* self = GJS_JOYSTICK(object);

	if(self->priv->fd >= 0) {
		close(self->priv->fd);
	}
	if(self->priv->source) {
		g_source_remove(g_source_get_id((GSource*)(self->priv->source)));
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
	if(self->priv->devname) {
		g_free(self->priv->devname);
	}
	g_free(self->priv);
}

static void set_property(GObject* object, guint property_id, const GValue *value, GParamSpec *pspec) {
	GjsJoystick *self = GJS_JOYSTICK(object);
	switch(property_id) {
	case GJS_DEVNAME:
		self->priv->devname = g_value_dup_string(value);
		gjs_joystick_reopen(self, NULL, NULL);
		break;
	case GJS_INTV:
		self->priv->axintv = g_value_get_uint(value);
		break;
	default:
		g_assert_not_reached();
	}
}

static gboolean check_fd(GSource* src) {
	GjsJoystickSource* self = (GjsJoystickSource*)src;
	if(g_source_query_unix_fd(src, self->tag) & G_IO_IN) {
		return TRUE;
	}
	return FALSE;
}

static gboolean dispatch_fd(GSource* src, GSourceFunc callback, gpointer user_data) {
	GjsJoystickSource* self = (GjsJoystickSource*)src;
	if(g_source_query_unix_fd(src, self->tag) & G_IO_IN) {
		gjs_joystick_iteration(self->js);
	}
	return TRUE;
}

static void base_init(gpointer* klass G_GNUC_UNUSED) {
	g_assert(object_index == NULL);
	object_index = g_hash_table_new(g_str_hash, g_str_equal);
}

static void base_finalize(gpointer* klass G_GNUC_UNUSED) {
	g_assert(g_hash_table_size(object_index) == 0);
	g_hash_table_destroy(object_index);
}

static void class_init(gpointer g_class, gpointer g_class_data) {
	GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
	GjsJoystickClass *klass = GJS_JOYSTICK_CLASS(g_class);
	GParamSpec *pspec;

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->finalize = finalize;
	klass->button_pressed = g_signal_new("button-pressed",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
	klass->button_released = g_signal_new("button-released",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__UCHAR,
				G_TYPE_NONE,
				1,
				G_TYPE_UCHAR);
	klass->axis_moved = g_signal_new("axis-moved",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
				0,
				NULL,
				NULL,
				gjs_cclosure_marshal_VOID__UCHAR_INT,
				G_TYPE_NONE,
				2,
				G_TYPE_UCHAR,
				G_TYPE_INT);
	pspec = g_param_spec_boolean("open",
				     "Open",
				     "Whether the device has been opened",
				     FALSE,
				     G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					GJS_OPEN,
					pspec);
	pspec = g_param_spec_uchar("button-count",
				   "Button count",
				   "The number of buttons on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					GJS_BUTCNT,
					pspec);
	pspec = g_param_spec_uchar("axis-count",
				   "Axis count",
				   "The number of axes on this joystick device",
				   0,
				   255,
				   0,
				   G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					GJS_AXCNT,
					pspec);
	pspec = g_param_spec_string("name",
				    "name",
				    "The name of the device (as reported by the kernel)",
				    "",
				    G_PARAM_READABLE);
	g_object_class_install_property(gobject_class,
					GJS_NAME,
					pspec);
	pspec = g_param_spec_string("devnode",
				    "device-node",
				    "The device name of this joystick",
				    "/dev/input/js0",
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property(gobject_class,
					GJS_DEVNAME,
					pspec);
	pspec = g_param_spec_uint("axis-interval",
				 "Axis interval",
				 "The minimum interval between two issued axis events (in milliseconds)",
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class,
					GJS_INTV,
					pspec);
}

GType gjs_joystick_get_type(void) {
	static GType type = 0;
	if(!type) {
		static const GTypeInfo info = {
			sizeof(GjsJoystickClass),
			base_init,	/* base_init */
			base_finalize,	/* base_finalize */
			class_init,	/* class_init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GjsJoystick),
			0,	/* n_preallocs */
			instance_init,
		};
		type = g_type_register_static(G_TYPE_OBJECT,
					      "GjsJoystickType",
					      &info, 0);
	}

	return type;
}

void gjs_joystick_iteration(GjsJoystick* self) {
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
				g_signal_emit(self, GJS_JOYSTICK_GET_CLASS(self)->button_pressed, quark, ev.number);
			} else {
				g_signal_emit(self, GJS_JOYSTICK_GET_CLASS(self)->button_released, quark, ev.number);
			}
			break;
		case JS_EVENT_AXIS:
			g_array_index(self->priv->butvals, gint16, ev.number) = ev.value;
			if(ev.time > (g_array_index(self->priv->axevts, guint32, ev.number) + self->priv->axintv)) {
				g_signal_emit(self, GJS_JOYSTICK_GET_CLASS(self)->axis_moved, quark, ev.number, ev.value);
			}
			break;
		default:
			break;
	}
	return;
}

void gjs_joystick_loop(GjsJoystick* self) {
	while(TRUE) {
		gjs_joystick_iteration(self);
	}
}

void gjs_joystick_set_mode(GjsJoystick* self, GjsMode mode) {
	if(self->priv->mode != mode) {
		if(mode == GJS_MODE_MANUAL) {
			g_source_remove(g_source_get_id((GSource*)self->priv->source));
		} else {
			GSourceFuncs fncs;
			memset(&fncs, 0, sizeof(GSourceFuncs));
			fncs.check = check_fd;
			fncs.dispatch = dispatch_fd;
			self->priv->source = (GjsJoystickSource*)g_source_new(&fncs, sizeof(GjsJoystickSource));
			self->priv->source->tag = g_source_add_unix_fd((GSource*)self->priv->source, self->priv->fd, G_IO_IN | G_IO_ERR | G_IO_HUP);
			g_source_attach((GSource*)self->priv->source, NULL);
		}
		self->priv->mode = mode;
	}
}

GQuark gjs_get_errdomain(void) {
	return g_quark_from_string("Gjs");
}

GType gjs_error_get_type(void) {
	/* XXX */

	return -1;
}
