#include <libjoy-gtk.h>

GtkTreeModel* joy_stick_enumerate_model(GError** err) {
	GtkListStore* l;
	GtkTreeIter iter;
	GArray* arr = joy_stick_enumerate(err);

	if(!arr) {
		return NULL;
	}

	l = gtk_list_store_new(JOY_COLUMN_COUNT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UCHAR, G_TYPE_UCHAR);

	if(arr->len < 1) {
		g_set_error(err, JOY_ERROR_DOMAIN, JOY_ERR_NJS, "No joysticks found!");
		return GTK_TREE_MODEL(l);
	}

	for(int i = 0; i<arr->len; i++) {
		GjsDetails det = g_array_index(arr, GjsDetails, i);

		gtk_list_store_append(l, &iter);
		gtk_list_store_set(l, &iter,
				   JOY_COLUMN_DEV, det.devname,
				   JOY_COLUMN_NAME, det.model,
				   JOY_COLUMN_AXES, det.axes,
				   JOY_COLUMN_BUTTONS, det.buttons,
				   -1);
	}

	return GTK_TREE_MODEL(l);
}
