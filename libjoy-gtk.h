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
#ifndef LIBJOY_GTK_H
#define LIBJOY_GTK_H

#include <libjoy.h>
#include <gtk/gtk.h>

typedef enum joy_model_columns {
	JOY_COLUMN_DEV,
	JOY_COLUMN_NAME,
	JOY_COLUMN_AXES,
	JOY_COLUMN_BUTTONS,
	JOY_COLUMN_OBJECT,
	JOY_COLUMN_COUNT
} JoyModelColumns;

G_BEGIN_DECLS

typedef struct _JoyModel JoyModel;
typedef struct _JoyModelClass JoyModelClass;
typedef struct _JoyModelPrivate JoyModelPrivate;

#define JOY_TYPE_MODEL	(joy_model_get_type())
#define JOY_MODEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), JOY_TYPE_MODEL, JoyModel))
#define JOY_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), JOY_TYPE_MODEL, JoyModelClass))
#define JOY_IS_MODEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), JOY_TYPE_MODEL))
#define JOY_IS_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), JOY_TYPE_MODEL))
#define JOY_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), JOY_TYPE_MODEL, JoyModelClass))

struct _JoyModel {
	GtkListStore parent;
	JoyModelPrivate *priv;
};

struct _JoyModelClass {
	GtkListStoreClass parent_class;
};

/**
  * @brief Create a tree model for use in a GtkTreeView
  *
  * This function returns a model that will dynamically be updated as joysticks
  * are added and removed from the system.
  *
  * @return a tree model ready for use a GtkTreeView, or NULL in case of
  * error (with err set appropriately)
  */
GtkTreeModel* joy_stick_enumerate_model(GError** err);

GType joy_model_get_type(void) G_GNUC_CONST;

#endif
