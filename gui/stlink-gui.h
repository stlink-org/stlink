
#ifndef __STLINK_GUI_H__
#define __STLINK_GUI_H__

#include <glib-object.h>

#define STLINK_TYPE_GUI             (stlink_gui_get_type ())
#define STLINK_GUI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), STLINK_TYPE_GUI, STlinkGUI))
#define STLINK_IS_GUI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STLINK_TYPE_GUI))
#define STLINK_GUI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), STLINK_TYPE_GUI, STlinkGUIClass))
#define STLINK_IS_GUI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), STLINK_TYPE_GUI))
#define STLINK_GUI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), STLINK_TYPE_GUI, STlinkGUIlass))
#define STLINK_GUI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), STLINK_TYPE_GUI, STlinkGUIPrivate))

typedef struct _STlinkGUI        STlinkGUI;
typedef struct _STlinkGUIClass   STlinkGUIClass;
typedef struct _STlinkGUIPrivate STlinkGUIPrivate;

enum stlink_gui_pages_t {
    PAGE_DEVMEM,
    PAGE_FILEMEM
};

enum stlink_gui_dnd_targets_t {
    TARGET_FILENAME,
    TARGET_ROOTWIN
};

struct progress_t {
    GtkProgressBar *bar;
    guint           timer;
    gboolean        activity_mode;
    gdouble         fraction;
};

struct mem_t {
    guchar *memory;
    gsize   size;
    guint32 base;
};

struct _STlinkGUI
{
    GObject parent_instance;

    /*< private >*/
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
    GtkToolButton  *connect_button;
    GtkToolButton  *disconnect_button;
    GtkToolButton  *flash_button;
    GtkToolButton  *open_button;

    /* flash dialog */
    GtkDialog  *flash_dialog;
    GtkButton  *flash_dialog_ok;
    GtkButton  *flash_dialog_cancel;
    GtkEntry   *flash_dialog_entry;

    struct progress_t  progress;
    struct mem_t       flash_mem;
    struct mem_t       file_mem;

    gchar    *error_message;
    gchar    *filename;
    stlink_t *sl;
};

struct _STlinkGUIClass
{
    GObjectClass parent_class;

    /* class members */
};

GType stlink_gui_get_type (void);

#endif
