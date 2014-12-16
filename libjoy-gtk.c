#include <string.h>
#include <libjoy-gtk.h>
#include <libudev.h>

#include <glib-unix.h>

typedef struct _JoyModelSource JoyModelSource;

struct _JoyModelPrivate {
	GList* objs;
	GList* iters;
	struct udev_monitor* mon;
	guint watch;
};

struct udev* udev;

GtkTreeModel* joy_stick_enumerate_model(GError** err) {
	JoyModel* mod;
	GtkListStore* l;

	mod = g_object_new(JOY_TYPE_MODEL, NULL);
	l = GTK_LIST_STORE(mod);

	if(!mod->priv->objs) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_NJS, "No joysticks found!");
		return GTK_TREE_MODEL(l);
	}

	GList* obj = mod->priv->objs;

	while(obj != NULL) {
		JoyStick* stick = JOY_STICK(obj->data);
		GtkTreeIter* iter = g_new0(GtkTreeIter, 1);
		mod->priv->iters = g_list_append(mod->priv->iters, iter);

		gtk_list_store_append(l, iter);
		gtk_list_store_set(l, iter,
				   JOY_COLUMN_DEV, joy_stick_get_devnode(stick, err),
				   JOY_COLUMN_NAME, joy_stick_describe(stick, err),
				   JOY_COLUMN_AXES, joy_stick_get_axis_count(stick, err),
				   JOY_COLUMN_BUTTONS, joy_stick_get_button_count(stick, err),
				   JOY_COLUMN_OBJECT, stick,
				   -1);
		obj = obj->next;
	}

	return GTK_TREE_MODEL(l);
}

static gboolean handle_udev_event(gint fd, GIOCondition cond, gpointer user_data) {
	JoyModel* self = JOY_MODEL(user_data);
	if(cond & G_IO_IN) {
		struct udev_device *dev = udev_monitor_receive_device(self->priv->mon);
		if(!dev) return TRUE;
		const char* name = udev_device_get_sysname(dev);
		if(name[0] != 'j' || name[1] != 's') return TRUE;
		GList* obj = self->priv->objs;
		GList* it = self->priv->iters;
		const char* act = udev_device_get_action(dev);
		if(!strcmp(act, "remove")) {
			while(strcmp(joy_stick_get_devnode(JOY_STICK(obj->data), NULL), udev_device_get_devnode(dev))) {
				obj = obj->next;
				it = it->next;
			}
			gtk_list_store_remove(GTK_LIST_STORE(self), it->data);
			self->priv->objs = g_list_remove_link(self->priv->objs, obj);
			self->priv->iters = g_list_remove_link(self->priv->iters, it);
			g_object_unref(G_OBJECT(obj->data));
			g_free(it->data);
			obj = g_list_delete_link(obj, obj);
			it = g_list_delete_link(it, it);
		}
		if(!strcmp(act, "add")) {
			JoyStick* stick = joy_stick_open(udev_device_get_devnode(dev));
			GtkTreeIter* iter = g_new0(GtkTreeIter, 1);
			self->priv->iters = g_list_append(self->priv->iters, iter);
			self->priv->objs = g_list_append(self->priv->objs, stick);

			gtk_list_store_append(GTK_LIST_STORE(self), iter);
			gtk_list_store_set(GTK_LIST_STORE(self), iter,
							JOY_COLUMN_DEV, joy_stick_get_devnode(stick, NULL),
							JOY_COLUMN_NAME, joy_stick_describe(stick, NULL),
							JOY_COLUMN_AXES, joy_stick_get_axis_count(stick, NULL),
							JOY_COLUMN_BUTTONS, joy_stick_get_button_count(stick, NULL),
							JOY_COLUMN_OBJECT, stick,
							-1);
		}
	}
	return TRUE;
}

static void instance_init(GTypeInstance* instance, gpointer g_class) {
	JoyModel *self = JOY_MODEL(instance);
	GType types[] = {
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_UCHAR,
		G_TYPE_UCHAR,
		JOY_STICK_TYPE,
	};

	gtk_list_store_set_column_types(GTK_LIST_STORE(self), JOY_COLUMN_COUNT, types);

	self->priv = g_new0(JoyModelPrivate, 1);
	self->priv->objs = joy_stick_enumerate(NULL);
	self->priv->mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(self->priv->mon, "input", NULL);
	udev_monitor_enable_receiving(self->priv->mon);
	int fd = udev_monitor_get_fd(self->priv->mon);
	self->priv->watch = g_unix_fd_add(fd, G_IO_IN | G_IO_HUP, handle_udev_event, self);
}

static void instance_finalize(GObject* self) {
	udev_monitor_unref(JOY_MODEL(self)->priv->mon);
}

static void class_init(gpointer klass, gpointer data G_GNUC_UNUSED) {
	udev = udev_new();
	G_OBJECT_CLASS(klass)->finalize = instance_finalize;
}

GType joy_model_get_type(void) {
	static GType type = 0;
	if(!type) {
		static const GTypeInfo info = {
			sizeof(JoyModelClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			class_init,	/* class_init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(JoyModel),
			0,	/* n_preallocs */
			instance_init,
		};
		type = g_type_register_static(GTK_TYPE_LIST_STORE,
					      "JoyModelType",
					      &info, 0);
	}
	return type;
}
