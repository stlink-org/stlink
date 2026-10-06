/**
  ******************************************************************************
  * @file             gui.h
  * @brief            stlink-gui
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <glib-object.h>

#define STLINK_TYPE_GUI             (stlink_gui_get_type())
#define STLINK_GUI(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), STLINK_TYPE_GUI, STlinkGUI))
#define STLINK_IS_GUI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), STLINK_TYPE_GUI))
#define STLINK_GUI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), STLINK_TYPE_GUI, STlinkGUIClass))
#define STLINK_IS_GUI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), STLINK_TYPE_GUI))
#define STLINK_GUI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), STLINK_TYPE_GUI, STlinkGUIClass))

typedef struct _STlinkGUI STlinkGUI;
typedef struct _STlinkGUIClass STlinkGUIClass;

enum stlink_gui_pages_t {
    PAGE_DEVMEM,
    PAGE_FILEMEM
};

struct progress_t {
    GtkProgressBar *bar;
    guint timer;
    gboolean activity_mode;
    gdouble fraction;
};

struct mem_t {
    guchar *memory;
    gsize size;
    guint32 base;
};

struct _STlinkGUI {
    GObject parent_instance;

    /* < private > */
    GtkWindow      *window;
    GtkTreeView    *devmem_treeview;
    GtkTreeView    *filemem_treeview;
    GtkSpinner     *spinner;
    GtkStatusbar   *statusbar;
    GtkInfoBar     *infobar;
    GtkLabel       *infolabel;
    GtkNotebook    *notebook;
    GtkFrame       *device_frame;
    GtkLabel       *chip_id_label;
    GtkLabel       *core_id_label;
    GtkLabel       *flash_size_label;
    GtkLabel       *ram_size_label;
    GtkBox         *devmem_box;
    GtkEntry       *devmem_jmp_entry;
    GtkBox         *filemem_box;
    GtkEntry       *filemem_jmp_entry;
    GtkButton      *open_button;
    GtkButton      *connect_button;
    GtkButton      *disconnect_button;
    GtkButton      *flash_button;
    GtkButton      *reset_button;
    GtkButton      *export_button;
    GtkButton      *erase_button;

    /* Flash dialog */
    GtkDialog  *flash_dialog;
    GtkButton  *flash_dialog_ok;
    GtkButton  *flash_dialog_cancel;
    GtkEntry   *flash_dialog_entry;

    /* Erase dialog */
    GtkDialog  *erase_dialog;
    GtkButton  *erase_dialog_ok;
    GtkButton  *erase_dialog_cancel;

    struct progress_t progress;
    struct mem_t flash_mem;
    struct mem_t file_mem;

    gchar    *error_message;
    gchar    *filename;
    stlink_t *sl;
};

struct _STlinkGUIClass {
    GObjectClass parent_class;

    /* class members */
};

GType stlink_gui_get_type(void);
int32_t export_to_file(const char*filename, const struct mem_t flash_mem);

#endif // GUI_H
