#ifndef LIBJOY_GTK_H
#define LIBJOY_GTK_H

#include <libjoy.h>
#include <gtk/gtk.h>

typedef enum gjs_model_columns {
	JOY_COLUMN_DEV,
	JOY_COLUMN_NAME,
	JOY_COLUMN_AXES,
	JOY_COLUMN_BUTTONS,
	JOY_COLUMN_COUNT
} JoyModelColumns;
/**
  * @brief Create a tree model for use in a GtkTreeView
  *
  * This function creates a data model to be used in a GtkTreeView. The
  * current implementation is a GtkListStore; however, implementations
  * should not depend on that.
  *
  * Future versions of this function may return a model that will
  * dynamically be updated as joysticks are added and removed from the
  * system; the current implementation does not do so.
  *
  * @return a tree model ready for use a GtkTreeView, or NULL in case of
  * error (with err set appropriately)
  */
GtkTreeModel* joy_stick_enumerate_model(GError** err);

#endif
