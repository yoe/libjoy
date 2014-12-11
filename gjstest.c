#include <gjsjoystick.h>
#include <gjsjoystick-gtk.h>
#include <gtk/gtk.h>

void row_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn* col, gpointer data) {
	
}

int main(int argc, char** argv) {
	GjsJoystick* js = gjs_joystick_open("/dev/input/js0");
	GError* err = NULL;
	GtkTreeModel* model;

	gtk_init(&argc, &argv);

	/* Create main window */
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Joystick tester tool");

	model = gjs_joystick_enumerate_model(NULL);

	/* Create paned and tree view */
	GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	GtkWidget* tv = gtk_tree_view_new_with_model(model);
	gtk_widget_set_size_request(GTK_WIDGET(tv), 100, -1);

	gtk_paned_add1(GTK_PANED(paned), tv);

	/* Create table layout, to contain joystick information */
	GtkWidget* layout = gtk_table_new(3, 2, FALSE);

	GtkWidget* label = gtk_label_new("Joystick on /dev/input/js0:");
	gtk_paned_add2(GTK_PANED(paned), layout);

	gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, 0, 1);

	gchar* name = gjs_joystick_describe(js, &err);

	if(!name) {
		GtkWidget* errw = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error opening /dev/input/js0: %s", err->message);
		gtk_dialog_run(GTK_DIALOG(errw));
		gtk_widget_destroy(errw);
		return 1;
	}

	label = gtk_label_new(name);
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 1, 2, 0, 1);

	label = gtk_label_new("Number of axes:");
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, 1, 2);

	guint axes = gjs_joystick_get_axis_count(js, NULL);
	guint buttons = gjs_joystick_get_button_count(js, NULL);

	gchar* axstr = g_strdup_printf("%d", axes);
	label = gtk_label_new(axstr);
	g_free(axstr);
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 1, 2, 1, 2);

	label = gtk_label_new("Number of buttons:");
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, 2, 3);

	gchar* butstr = g_strdup_printf("%d", buttons);
	label = gtk_label_new(butstr);
	g_free(butstr);
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 1, 2, 2, 3);

	gtk_container_add(GTK_CONTAINER(window), paned);

	guchar i;

	guint size = 3;

	for(i=0; i<axes; i++) {
		gtk_table_resize(GTK_TABLE(layout), ++size, 2);
		axstr = g_strdup_printf("Axis %u:", i);
		label = gtk_label_new(axstr);
		g_free(axstr);
		gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, size, size+1);
		label = gtk_label_new(gjs_joystick_describe_axis(js, i));
		gtk_table_attach_defaults(GTK_TABLE(layout), label, 1, 2, size, size+1);
	}
	for(i=0; i<buttons; i++) {
		gtk_table_resize(GTK_TABLE(layout), ++size, 2);
		butstr = g_strdup_printf("Button %u:", i);
		label = gtk_label_new(butstr);
		g_free(butstr);
		gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, size, size+1);
		label = gtk_label_new(gjs_joystick_describe_button(js, i));
		gtk_table_attach_defaults(GTK_TABLE(layout), label, 1, 2, size, size+1);
	}

	g_object_unref(js);

	g_signal_connect_swapped(G_OBJECT(window), "destroy", gtk_main_quit, NULL);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
