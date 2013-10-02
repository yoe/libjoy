#include <gjsjoystick.h>
#include <gtk/gtk.h>

int main(int argc, char** argv) {
	GjsJoystick* js = gjs_joystick_open("/dev/input/js0");
	GError* err;

	gtk_init(&argc, &argv);
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Joystick tester tool");
	GtkWidget* layout = gtk_table_new(3, 2, FALSE);

	GtkWidget* label = gtk_label_new("Joystick on /dev/input/js0:");
	gtk_table_attach_defaults(GTK_TABLE(layout), label, 0, 1, 0, 1);

	gchar* name = gjs_joystick_describe(js, &err);

	if(!name) {
		fprintf(stderr, "E: %s\n", err->message);
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

	gtk_container_add(GTK_CONTAINER(window), layout);

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

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
