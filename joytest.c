/*
 * libjoy - GObject-based joystick API
 *
 * Copyright(c) Wouter Verhelst, 2014
 *
 * This library is free software; you can copy it under the terms of the
 * GNU General Public License, version 2, as published by the Free Software
 * Foundation
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
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libjoy.h>
#include <libjoy-gtk.h>
#include "joytest-iface.h"

#ifndef _
#define _(s) (s)
#endif

JoyStick* active;

gulong button_p_handler;
gulong button_r_handler;
gulong axis_handler;
gulong lost_handler;

static void button_pressed(JoyStick* stick, guchar butnum) {
	g_message("button %d pressed", butnum);
}

static void button_released(JoyStick* stick, guchar butnum) {
	g_message("button %d released", butnum);
}

static void axis_moved(JoyStick* stick, guchar axis, int newval) {
	g_message("axis %d moved to %d", axis, newval);
}

static void lost(JoyStick* stick) {
	g_message("Joystick on %s disconnected", joy_stick_get_devnode(stick, NULL));
	g_object_unref(G_OBJECT(stick));
	g_message("unref'd");
}

void tree_selection_changed(GtkTreeSelection* sel, gpointer data) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkBuilder *builder = GTK_BUILDER(data);

	if(gtk_tree_selection_get_selected(sel, &model, &iter)) {
		if(button_p_handler) {
			g_signal_handler_disconnect(active, button_p_handler);
		}
		if(button_r_handler) {
			g_signal_handler_disconnect(active, button_r_handler);
		}
		if(axis_handler) {
			g_signal_handler_disconnect(active, axis_handler);
		}
		if(lost_handler) {
			g_signal_handler_disconnect(active, lost_handler);
		}
		if(active) {
			g_object_unref(G_OBJECT(active));
		}
		gtk_tree_model_get(model, &iter, JOY_COLUMN_OBJECT, &active, -1);
		g_object_ref(G_OBJECT(active));
		g_message("Joystick \"%s\" now active", joy_stick_describe(active, NULL));

		GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, "namelabel"));
		gchar* labeltext = g_strdup_printf("%s on %s", joy_stick_describe(active, NULL), joy_stick_get_devnode(active, NULL));
		gtk_label_set_text(GTK_LABEL(widget), labeltext);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "buttonslabel"));
		labeltext = g_strdup_printf("%d", joy_stick_get_button_count(active, NULL));
		gtk_label_set_text(GTK_LABEL(widget), labeltext);
		g_free(labeltext);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "axeslabel"));
		labeltext = g_strdup_printf("%d", joy_stick_get_axis_count(active, NULL));
		gtk_label_set_text(GTK_LABEL(widget), labeltext);
		g_free(labeltext);
		button_p_handler = g_signal_connect(G_OBJECT(active), "button-pressed", G_CALLBACK(button_pressed), NULL);
		button_r_handler = g_signal_connect(G_OBJECT(active), "button-released", G_CALLBACK(button_released), NULL);
		axis_handler = g_signal_connect(G_OBJECT(active), "axis-moved", G_CALLBACK(axis_moved), NULL);
		lost_handler = g_signal_connect(G_OBJECT(active), "disconnected", G_CALLBACK(lost), NULL);
	}
}

int main(int argc, char** argv) {
	GtkBuilder *builder;
	GtkWidget *window, *treeview;
	GtkTreeModel *model;
	GtkCellRenderer *trenderer, *irenderer;
	GtkTreeViewColumn *col;
	GtkTreeSelection *select;
	GError* err = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_string(builder, JOYTEST_IFACE, strlen(JOYTEST_IFACE), NULL);
	window = GTK_WIDGET(gtk_builder_get_object(builder, "mainwin"));

	treeview = GTK_WIDGET(gtk_builder_get_object(builder, "joyview"));
	model = joy_stick_enumerate_model(&err);
	if(!model) {
		g_critical("Could not search for joysticks: %s", err->message);
		exit(EXIT_FAILURE);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);

	trenderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Name"), trenderer, "text", JOY_COLUMN_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	irenderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(irenderer), "xalign", 1.0, NULL);
	col = gtk_tree_view_column_new_with_attributes(_("Axes"), irenderer, "text", JOY_COLUMN_AXES, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	col = gtk_tree_view_column_new_with_attributes(_("Buttons"), irenderer, "text", JOY_COLUMN_BUTTONS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(tree_selection_changed), builder);

	g_signal_connect(G_OBJECT(window), "delete-event", gtk_main_quit, NULL);

	gtk_widget_show_all(window);
	gtk_main();
}
