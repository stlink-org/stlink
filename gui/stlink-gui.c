#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "stlink-common.h"
#include "stlink-gui.h"

#define MEM_READ_SIZE 1024

#ifndef G_VALUE_INIT
#define G_VALUE_INIT {0, {{0}}}
#endif

G_DEFINE_TYPE (STlinkGUI, stlink_gui, G_TYPE_OBJECT);

static void
stlink_gui_dispose (GObject *gobject)
{
    G_OBJECT_CLASS (stlink_gui_parent_class)->dispose (gobject);
}

static void
stlink_gui_finalize (GObject *gobject)
{
    G_OBJECT_CLASS (stlink_gui_parent_class)->finalize (gobject);
}

static void
stlink_gui_class_init (STlinkGUIClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose  = stlink_gui_dispose;
    gobject_class->finalize = stlink_gui_finalize;
}

static void
stlink_gui_init (STlinkGUI *self)
{
    self->sl       = NULL;
    self->filename = NULL;

    self->progress.activity_mode = FALSE;
    self->progress.fraction      = 0;

    self->flash_mem.memory = NULL;
    self->flash_mem.size   = 0;
    self->flash_mem.base   = 0;

    self->file_mem.memory = NULL;
    self->file_mem.size   = 0;
    self->file_mem.base   = 0;
}

static gboolean
set_info_error_message_idle (STlinkGUI *gui)
{
    if (gui->error_message != NULL) {
        gchar *markup;

        markup = g_markup_printf_escaped ("<b>%s</b>", gui->error_message);
        gtk_label_set_markup (gui->infolabel, markup);
        gtk_info_bar_set_message_type (gui->infobar, GTK_MESSAGE_ERROR);
        gtk_widget_show (GTK_WIDGET (gui->infobar));

        g_free (markup);
        g_free (gui->error_message);
        gui->error_message = NULL;
    }
    return FALSE;
}

static void
stlink_gui_set_info_error_message (STlinkGUI *gui, const gchar *message)
{
    gui->error_message = g_strdup (message);
    g_idle_add ((GSourceFunc) set_info_error_message_idle, gui);
}

static void
stlink_gui_set_sensitivity (STlinkGUI *gui, gboolean sensitivity)
{
    gtk_widget_set_sensitive (GTK_WIDGET (gui->open_button), sensitivity);

    if (sensitivity && gui->sl)
        gtk_widget_set_sensitive (GTK_WIDGET (gui->disconnect_button), sensitivity);

    if (sensitivity && !gui->sl)
        gtk_widget_set_sensitive (GTK_WIDGET (gui->connect_button), sensitivity);

    if (sensitivity && gui->sl && gui->filename)
        gtk_widget_set_sensitive (GTK_WIDGET (gui->flash_button), sensitivity);
}

static void
mem_view_init_headers (GtkTreeView *view)
{
    GtkCellRenderer *renderer;
    gint             i;

    g_return_if_fail (view != NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (view,
            -1,
            "Address",
            renderer,
            "text",
            0, /* column */
            NULL);
    for (i = 0; i < 4; i++) {
        gchar *label;

        label = g_strdup_printf ("%X", i * 4);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_insert_column_with_attributes (view,
                -1,
                label,
                renderer,
                "text",
                (i + 1), /* column */
                NULL);
        g_free (label);
    }

    for (i = 0; i < 5; i++) {
        GtkTreeViewColumn *column = gtk_tree_view_get_column (view, i);
        gtk_tree_view_column_set_expand (column, TRUE);
    }
}

static void
mem_view_add_as_hex (GtkListStore *store,
                     GtkTreeIter  *iter,
                     gint          column,
                     guint32       value)
{
    gchar     *hex_str;

    hex_str = g_strdup_printf ("0x%08X", value);
    gtk_list_store_set (store, iter, column, hex_str, -1);
    g_free (hex_str);
}

static void
mem_view_add_buffer (GtkListStore *store,
                     GtkTreeIter  *iter,
                     guint32       address,
                     guchar       *buffer,
                     gint          len)
{
    guint32 *word;
    gint     i, step;
    gint     column = 0;

    step = sizeof (*word);

    for (i = 0; i < len; i += step) {
        word = (guint *) &buffer[i];

        if (column == 0) {
            /* new row */
            gtk_list_store_append (store, iter);

            /* add address */
            mem_view_add_as_hex (store, iter, column, (address + i));
        }
        mem_view_add_as_hex (store, iter, (column + 1), *word);
        column = (column + 1) % step;
    }
}

static guint32
hexstr_to_guint32 (const gchar *str, GError **err)
{
    guint32  val;
    gchar   *end_ptr;

    val = strtoul (str, &end_ptr, 16);
    if ((errno == ERANGE && val == LONG_MAX) || (errno != 0 && val == 0)) {
        g_set_error (err,
                g_quark_from_string ("hextou32"),
                1,
                "Invalid hexstring");
        return LONG_MAX;
    }
    if (end_ptr == str) {
        g_set_error (err,
                g_quark_from_string ("hextou32"),
                2,
                "Invalid hexstring");
        return LONG_MAX;
    }
    return val;
}


static void
stlink_gui_update_mem_view (STlinkGUI *gui, struct mem_t *mem, GtkTreeView *view) {
    GtkListStore *store;
    GtkTreeIter   iter;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (view));

    mem_view_add_buffer (store,
            &iter,
            mem->base,
            mem->memory,
            mem->size);

    gtk_widget_hide (GTK_WIDGET (gui->progress.bar));
    gtk_progress_bar_set_fraction (gui->progress.bar, 0);
    stlink_gui_set_sensitivity (gui, TRUE);
}

static gboolean
stlink_gui_update_devmem_view (STlinkGUI *gui)
{
    stlink_gui_update_mem_view (gui, &gui->flash_mem, gui->devmem_treeview);
    return FALSE;
}


static void
stlink_gui_populate_devmem_view (STlinkGUI *gui)
{
    guint            off;
    stm32_addr_t     addr;

    g_return_if_fail (gui != NULL);
    g_return_if_fail (gui->sl != NULL);

    addr = gui->sl->flash_base;

    if (gui->flash_mem.memory) {
        g_free (gui->flash_mem.memory);
    }
    gui->flash_mem.memory = g_malloc (gui->sl->flash_size);
    gui->flash_mem.size   = gui->sl->flash_size;
    gui->flash_mem.base   = gui->sl->flash_base;

    for (off = 0; off < gui->sl->flash_size; off += MEM_READ_SIZE) {
        guint   n_read = MEM_READ_SIZE;

        if (off + MEM_READ_SIZE > gui->sl->flash_size) {
            n_read = gui->sl->flash_size - off;

            /* align if needed */
            if (n_read & 3) {
                n_read = (n_read + 4) & ~(3);
            }
        }
        /* reads to sl->q_buf */
        stlink_read_mem32(gui->sl, addr + off, n_read);
        if (gui->sl->q_len < 0) {
            stlink_gui_set_info_error_message (gui, "Failed to read memory");
            g_free (gui->flash_mem.memory);
            gui->flash_mem.memory = NULL;
            return;
        }
        memcpy (gui->flash_mem.memory + off, gui->sl->q_buf, n_read);
        gui->progress.fraction = (gdouble) (off + n_read) / gui->sl->flash_size;
    }
    g_idle_add ((GSourceFunc) stlink_gui_update_devmem_view, gui);
}

static gboolean
stlink_gui_update_filemem_view (STlinkGUI *gui)
{
    gchar *basename;

    basename = g_path_get_basename (gui->filename);
    gtk_notebook_set_tab_label_text (gui->notebook,
            GTK_WIDGET (gtk_notebook_get_nth_page (gui->notebook, 1)),
            basename);
    g_free (basename);

    stlink_gui_update_mem_view (gui, &gui->file_mem, gui->filemem_treeview);

    return FALSE;
}

static gpointer
stlink_gui_populate_filemem_view (STlinkGUI *gui)
{
    guchar        buffer[MEM_READ_SIZE];
    GFile        *file;
    GFileInfo    *file_info;
    GInputStream *input_stream;
    gint          off;
    GError       *err = NULL;

    g_return_val_if_fail (gui != NULL, NULL);
    g_return_val_if_fail (gui->filename != NULL, NULL);

    file = g_file_new_for_path (gui->filename);
    input_stream = G_INPUT_STREAM (g_file_read (file, NULL, &err));
    if (err) {
        stlink_gui_set_info_error_message (gui, err->message);
        g_error_free (err);
        goto out;
    }

    file_info = g_file_input_stream_query_info (G_FILE_INPUT_STREAM (input_stream),
            G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, &err);
    if (err) {
        stlink_gui_set_info_error_message (gui, err->message);
        g_error_free (err);
        goto out_input;
    }
    if (gui->file_mem.memory) {
        g_free (gui->file_mem.memory);
    }
    gui->file_mem.size   = g_file_info_get_size (file_info);
    gui->file_mem.memory = g_malloc (gui->file_mem.size);

    for (off = 0; off < gui->file_mem.size; off += MEM_READ_SIZE) {
        guint   n_read = MEM_READ_SIZE;

        if (off + MEM_READ_SIZE > gui->file_mem.size) {
            n_read = gui->file_mem.size - off;
        }

        if (g_input_stream_read (G_INPUT_STREAM (input_stream),
                    &buffer, n_read, NULL, &err) == -1) {
            stlink_gui_set_info_error_message (gui, err->message);
            g_error_free (err);
            goto out_input;
        }
        memcpy (gui->file_mem.memory + off, buffer, n_read);
        gui->progress.fraction = (gdouble) (off + n_read) / gui->file_mem.size;
    }
    g_idle_add ((GSourceFunc) stlink_gui_update_filemem_view, gui);

out_input:
    g_object_unref (input_stream);
out:
    g_object_unref (file);
    return NULL;
}

static void mem_jmp (GtkTreeView *view,
                     GtkEntry    *entry,
                     guint32      base_addr,
                     gsize        size,
                     GError     **err)
{
    GtkTreeModel *model;
    guint32       jmp_addr;
    GtkTreeIter   iter;

    jmp_addr = hexstr_to_guint32 (gtk_entry_get_text (entry), err);
    if (err && *err) {
        return;
    }

    if (jmp_addr < base_addr || jmp_addr > base_addr + size) {
        g_set_error (err,
                g_quark_from_string ("mem_jmp"),
                1,
                "Invalid address");
        return;
    }

    model = gtk_tree_view_get_model (view);
    if (!model) {
        return;
    }

    if (gtk_tree_model_get_iter_first (model, &iter)) {
        do {
            guint32 addr;
            GValue  value = G_VALUE_INIT;
            GError *err   = NULL;

            gtk_tree_model_get_value (model, &iter, 0, &value);
            if (G_VALUE_HOLDS_STRING (&value)) {
                addr = hexstr_to_guint32 (g_value_get_string (&value), &err);
                if (!err) {
                    if (addr == (jmp_addr & 0xFFFFFFF0)) {
                        GtkTreeSelection *selection;
                        GtkTreePath      *path;

                        selection = gtk_tree_view_get_selection (view);
                        path      = gtk_tree_model_get_path (model, &iter);

                        gtk_tree_selection_select_iter (selection, &iter);
                        gtk_tree_view_scroll_to_cell (view,
                                path,
                                NULL,
                                TRUE,
                                0.0,
                                0.0);
                        gtk_tree_path_free (path);
                    }
                }
            }
            g_value_unset (&value);
        } while (gtk_tree_model_iter_next (model, &iter));
    }
}

static void
devmem_jmp_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI *gui;
    GError    *err = NULL;

    gui = STLINK_GUI (data);

    mem_jmp (gui->devmem_treeview,
            gui->devmem_jmp_entry,
            gui->sl->flash_base,
            gui->sl->flash_size,
            &err);

    if (err) {
        stlink_gui_set_info_error_message (gui, err->message);
        g_error_free (err);
    }
}

static void
filemem_jmp_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI *gui;
    GError    *err = NULL;

    gui = STLINK_GUI (data);

    g_return_if_fail (gui->filename != NULL);

    mem_jmp (gui->filemem_treeview,
            gui->filemem_jmp_entry,
            0,
            gui->file_mem.size,
            &err);

    if (err) {
        stlink_gui_set_info_error_message (gui, err->message);
        g_error_free (err);
    }
}

static gchar *
dev_format_chip_id (guint32 chip_id)
{
    gint i;

    for (i = 0; i < sizeof (devices) / sizeof (devices[0]); i++) {
        if (chip_id == devices[i].chip_id) {
            return g_strdup (devices[i].description);
        }
    }
    return g_strdup_printf ("0x%x", chip_id);
}

static gchar *
dev_format_mem_size (gsize flash_size)
{
    return g_strdup_printf ("%u kB", flash_size / 1024);
}


static void
stlink_gui_set_connected (STlinkGUI *gui)
{
    gchar        *tmp_str;
    GtkListStore *store;
    GtkTreeIter   iter;

    gtk_statusbar_push (gui->statusbar,
            gtk_statusbar_get_context_id (gui->statusbar, "conn"),
            "Connected");

    gtk_widget_set_sensitive (GTK_WIDGET (gui->device_frame), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (gui->devmem_box), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (gui->connect_button), FALSE);

    if (gui->filename) {
        gtk_widget_set_sensitive (GTK_WIDGET (gui->flash_button), TRUE);
    }

    tmp_str = dev_format_chip_id (gui->sl->chip_id);
    gtk_label_set_text (gui->chip_id_label, tmp_str);
    g_free (tmp_str);

    tmp_str = g_strdup_printf ("0x%x", gui->sl->core_id);
    gtk_label_set_text (gui->core_id_label, tmp_str);
    g_free (tmp_str);

    tmp_str = dev_format_mem_size (gui->sl->flash_size);
    gtk_label_set_text (gui->flash_size_label, tmp_str);
    g_free (tmp_str);

    tmp_str = dev_format_mem_size (gui->sl->sram_size);
    gtk_label_set_text (gui->ram_size_label, tmp_str);
    g_free (tmp_str);

    tmp_str = g_strdup_printf ("0x%08X", gui->sl->flash_base);
    gtk_entry_set_text (gui->devmem_jmp_entry, tmp_str);
    gtk_editable_set_editable (GTK_EDITABLE (gui->devmem_jmp_entry), TRUE);
    g_free (tmp_str);

    store = GTK_LIST_STORE (gtk_tree_view_get_model (gui->devmem_treeview));
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
        gtk_list_store_clear (store);
    }

    stlink_gui_set_sensitivity (gui, FALSE);
    gtk_notebook_set_current_page (gui->notebook, PAGE_DEVMEM);
    gtk_widget_show (GTK_WIDGET (gui->progress.bar));
    gtk_progress_bar_set_text (gui->progress.bar, "Reading memory");

    g_thread_new ("devmem", (GThreadFunc) stlink_gui_populate_devmem_view, gui);
}

static void
connect_button_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI *gui;
    gint       i;

    gui = STLINK_GUI (data);

    if (gui->sl != NULL)
        return;

    /* try version 1 then version 2 */
    gui->sl = stlink_v1_open(0, 1);
    if (gui->sl == NULL) {
        gui->sl = stlink_open_usb(0, 1);
    }
    if (gui->sl == NULL) {
        stlink_gui_set_info_error_message (gui, "Failed to connect to STLink.");		return;
    }

    /* code below taken from flash/main.c, refactoring might be in order */
    if (stlink_current_mode(gui->sl) == STLINK_DEV_DFU_MODE)
        stlink_exit_dfu_mode(gui->sl);

    if (stlink_current_mode(gui->sl) != STLINK_DEV_DEBUG_MODE)
        stlink_enter_swd_mode(gui->sl);

    /* Disable DMA - Set All DMA CCR Registers to zero. - AKS 1/7/2013 */
    if (gui->sl->chip_id == STM32_CHIPID_F4) {
        memset(gui->sl->q_buf, 0, 4);
        for (i = 0; i < 8; i++) {
            stlink_write_mem32(gui->sl, 0x40026000 + 0x10 + 0x18 * i, 4);
            stlink_write_mem32(gui->sl, 0x40026400 + 0x10 + 0x18 * i, 4);
            stlink_write_mem32(gui->sl, 0x40026000 + 0x24 + 0x18 * i, 4);
            stlink_write_mem32(gui->sl, 0x40026400 + 0x24 + 0x18 * i, 4);
        }
    }
    stlink_gui_set_connected (gui);
}

static void stlink_gui_set_disconnected (STlinkGUI *gui)
{
    gtk_statusbar_push (gui->statusbar,
            gtk_statusbar_get_context_id (gui->statusbar, "conn"),
            "Disconnected");

    gtk_widget_set_sensitive (GTK_WIDGET (gui->device_frame), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (gui->flash_button), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (gui->disconnect_button), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (gui->connect_button), TRUE);
}

static void
disconnect_button_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI *gui;

    gui = STLINK_GUI (data);

    if (gui->sl != NULL) {
        stlink_exit_debug_mode(gui->sl);
        stlink_close(gui->sl);
        gui->sl = NULL;
    }
    stlink_gui_set_disconnected (gui);
}


static void
stlink_gui_open_file (STlinkGUI *gui)
{
    GtkWidget    *dialog;
    GtkListStore *store;
    GtkTreeIter   iter;

    dialog = gtk_file_chooser_dialog_new ("Open file",
            gui->window,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gui->filename =
            gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        store = GTK_LIST_STORE (gtk_tree_view_get_model (gui->filemem_treeview));
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
            gtk_list_store_clear (store);
        }

        stlink_gui_set_sensitivity (gui, FALSE);
        gtk_notebook_set_current_page (gui->notebook, PAGE_FILEMEM);
        gtk_widget_show (GTK_WIDGET (gui->progress.bar));
        gtk_progress_bar_set_text (gui->progress.bar, "Reading file");
        g_thread_new ("file", (GThreadFunc) stlink_gui_populate_filemem_view, gui);
    }
    gtk_widget_destroy (dialog);
}

static void
open_button_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI    *gui;

    gui = STLINK_GUI (data);

    stlink_gui_open_file (gui);
}

static gboolean
stlink_gui_write_flash_update (STlinkGUI *gui)
{
    stlink_gui_set_sensitivity (gui, TRUE);
    gui->progress.activity_mode = FALSE;
    gtk_widget_hide (GTK_WIDGET (gui->progress.bar));

    return FALSE;
}

static void
stlink_gui_write_flash (STlinkGUI *gui)
{
    g_return_if_fail (gui->sl != NULL);
    g_return_if_fail (gui->filename != NULL);

    if (stlink_fwrite_flash(gui->sl, gui->filename, gui->sl->flash_base) < 0) {
        stlink_gui_set_info_error_message (gui, "Failed to write to flash");
    }

    g_idle_add ((GSourceFunc) stlink_gui_write_flash_update, gui);
}

static void
flash_button_cb (GtkWidget *widget, gpointer data)
{
    STlinkGUI *gui;
    gchar     *tmp_str;
    guint32    address;
    gint       result;
    GError    *err = NULL;

    gui = STLINK_GUI (data);
    g_return_if_fail (gui->sl != NULL);

    if (!g_strcmp0 (gtk_entry_get_text (gui->flash_dialog_entry), "")) {
        tmp_str = g_strdup_printf ("0x%08X", gui->sl->flash_base);
        gtk_entry_set_text (gui->flash_dialog_entry, tmp_str);
        g_free (tmp_str);
    }

    result = gtk_dialog_run (gui->flash_dialog);
    if (result == GTK_RESPONSE_OK) {
        address = hexstr_to_guint32 (gtk_entry_get_text (gui->flash_dialog_entry),
                &err);
        if (err) {
            stlink_gui_set_info_error_message (gui, err->message);
        } else {
            if (address > gui->sl->flash_base + gui->sl->flash_size ||
                    address < gui->sl->flash_base) {
                stlink_gui_set_info_error_message (gui, "Invalid address");
            }
            else if (address + gui->file_mem.size >
                    gui->sl->flash_base + gui->sl->flash_size) {
                stlink_gui_set_info_error_message (gui, "Binary overwrites flash");
            } else {
                stlink_gui_set_sensitivity (gui, FALSE);
                gtk_progress_bar_set_text (gui->progress.bar,
                        "Writing to flash");
                gui->progress.activity_mode = TRUE;
                gtk_widget_show (GTK_WIDGET (gui->progress.bar));
                g_thread_new ("flash",
                        (GThreadFunc) stlink_gui_write_flash, gui);
            }
        }
    }
}

static gboolean
progress_pulse_timeout (STlinkGUI *gui) {
    if (gui->progress.activity_mode) {
        gtk_progress_bar_pulse (gui->progress.bar);
    } else {
        gtk_progress_bar_set_fraction (gui->progress.bar, gui->progress.fraction);
    }
    return TRUE;
}

static void
notebook_switch_page_cb (GtkNotebook *notebook,
                         GtkWidget   *widget,
                         guint        page_num,
                         gpointer     data)
{
    STlinkGUI *gui;

    gui = STLINK_GUI (data);

    if (page_num == 1) {
        if (gui->filename == NULL) {
            stlink_gui_open_file (gui);
        }
    }
}

static void
dnd_received_cb (GtkWidget *widget,
                 GdkDragContext *context,
                 gint x,
                 gint y,
                 GtkSelectionData *selection_data,
                 guint target_type,
                 guint time,
                 gpointer data)
{
    GFile        *file_uri;
    gchar       **file_list;
    const guchar *file_data;
    STlinkGUI    *gui = STLINK_GUI (data);
    GtkListStore *store;
    GtkTreeIter   iter;

    if (selection_data != NULL &&
            gtk_selection_data_get_length (selection_data) > 0) {
        switch (target_type) {
        case TARGET_FILENAME:

            if (gui->filename) {
                g_free (gui->filename);
            }

            file_data = gtk_selection_data_get_data (selection_data);
            file_list = g_strsplit ((gchar *)file_data, "\r\n", 0);

            file_uri = g_file_new_for_uri (file_list[0]);
            gui->filename = g_file_get_path (file_uri);

            g_strfreev (file_list);
            g_object_unref (file_uri);


            store = GTK_LIST_STORE (gtk_tree_view_get_model (gui->devmem_treeview));
            if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
                gtk_list_store_clear (store);
            }

            stlink_gui_set_sensitivity (gui, FALSE);
            gtk_notebook_set_current_page (gui->notebook, PAGE_FILEMEM);
            gtk_widget_show (GTK_WIDGET (gui->progress.bar));
            gtk_progress_bar_set_text (gui->progress.bar, "Reading file");
            g_thread_new ("file", (GThreadFunc) stlink_gui_populate_filemem_view, gui);
            break;
        }
    }
    gtk_drag_finish (context,
                     TRUE,
                     gdk_drag_context_get_suggested_action (context) == GDK_ACTION_MOVE,
                     time);
}

void
stlink_gui_init_dnd (STlinkGUI *gui)
{
    GtkTargetEntry target_list[] = {
        { "text/uri-list", 0, TARGET_FILENAME },
    };

    gtk_drag_dest_set (GTK_WIDGET (gui->window),
                       GTK_DEST_DEFAULT_ALL,
                       target_list,
                       G_N_ELEMENTS (target_list),
                       GDK_ACTION_COPY);

    g_signal_connect (gui->window, "drag-data-received",
                      G_CALLBACK (dnd_received_cb), gui);
}

static void
stlink_gui_build_ui (STlinkGUI *gui) {
    GtkBuilder   *builder;
    GtkListStore *devmem_store;
    GtkListStore *filemem_store;
    gchar *ui_file = STLINK_UI_DIR "/stlink-gui.ui";

    if (!g_file_test (ui_file, G_FILE_TEST_EXISTS)) {
        ui_file = "stlink-gui.ui";
    }
    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_file (builder, ui_file, NULL)) {
        g_printerr ("Failed to load UI file: %s\n", ui_file);
        exit (1);
    }

    gui->window = GTK_WINDOW (gtk_builder_get_object (builder, "window"));
    g_signal_connect (G_OBJECT (gui->window), "destroy",
            G_CALLBACK (gtk_main_quit), NULL);

    /* set up toolutton clicked callbacks */
    gui->open_button =
        GTK_TOOL_BUTTON (gtk_builder_get_object (builder, "open_button"));
    g_signal_connect (G_OBJECT (gui->open_button), "clicked",
            G_CALLBACK (open_button_cb), gui);

    gui->connect_button =
        GTK_TOOL_BUTTON (gtk_builder_get_object (builder, "connect_button"));
    g_signal_connect (G_OBJECT (gui->connect_button), "clicked",
            G_CALLBACK (connect_button_cb), gui);

    gui->disconnect_button =
        GTK_TOOL_BUTTON (gtk_builder_get_object (builder, "disconnect_button"));
    g_signal_connect (G_OBJECT (gui->disconnect_button), "clicked",
            G_CALLBACK (disconnect_button_cb), gui);

    gui->flash_button =
        GTK_TOOL_BUTTON (gtk_builder_get_object (builder, "flash_button"));
    g_signal_connect (G_OBJECT (gui->flash_button), "clicked",
            G_CALLBACK (flash_button_cb), gui);

    gui->devmem_treeview =
        GTK_TREE_VIEW (gtk_builder_get_object (builder, "devmem_treeview"));
    gtk_tree_view_set_rules_hint (gui->devmem_treeview, TRUE);
    mem_view_init_headers (gui->devmem_treeview);
    devmem_store = gtk_list_store_new (5,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING);
    gtk_tree_view_set_model (gui->devmem_treeview, GTK_TREE_MODEL (devmem_store));
    g_object_unref (devmem_store);

    gui->filemem_treeview =
        GTK_TREE_VIEW (gtk_builder_get_object (builder, "filemem_treeview"));
    gtk_tree_view_set_rules_hint (gui->filemem_treeview, TRUE);
    mem_view_init_headers (gui->filemem_treeview);
    filemem_store = gtk_list_store_new (5,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_STRING);
    gtk_tree_view_set_model (gui->filemem_treeview, GTK_TREE_MODEL (filemem_store));
    g_object_unref (filemem_store);

    gui->core_id_label =
        GTK_LABEL (gtk_builder_get_object (builder, "core_id_value"));

    gui->chip_id_label =
        GTK_LABEL (gtk_builder_get_object (builder, "chip_id_value"));

    gui->flash_size_label =
        GTK_LABEL (gtk_builder_get_object (builder, "flash_size_value"));

    gui->ram_size_label =
        GTK_LABEL (gtk_builder_get_object (builder, "ram_size_value"));

    gui->device_frame =
        GTK_FRAME (gtk_builder_get_object (builder, "device_frame"));

    gui->notebook =
        GTK_NOTEBOOK (gtk_builder_get_object (builder, "mem_notebook"));
    g_signal_connect (gui->notebook, "switch-page",
            G_CALLBACK (notebook_switch_page_cb), gui);

    gui->devmem_box =
        GTK_BOX (gtk_builder_get_object (builder, "devmem_box"));

    gui->filemem_box =
        GTK_BOX (gtk_builder_get_object (builder, "filemem_box"));

    gui->devmem_jmp_entry =
        GTK_ENTRY (gtk_builder_get_object (builder, "devmem_jmp_entry"));
    g_signal_connect (gui->devmem_jmp_entry, "activate",
            G_CALLBACK (devmem_jmp_cb), gui);

    gui->filemem_jmp_entry =
        GTK_ENTRY (gtk_builder_get_object (builder, "filemem_jmp_entry"));
    g_signal_connect (gui->filemem_jmp_entry, "activate",
            G_CALLBACK (filemem_jmp_cb), gui);
    gtk_editable_set_editable (GTK_EDITABLE (gui->filemem_jmp_entry), TRUE);

    gui->progress.bar =
        GTK_PROGRESS_BAR (gtk_builder_get_object (builder, "progressbar"));
    gtk_progress_bar_set_show_text (gui->progress.bar, TRUE);
    gui->progress.timer = g_timeout_add (100,
            (GSourceFunc) progress_pulse_timeout,
            gui);

    gui->statusbar =
        GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar"));

    gui->infobar =
        GTK_INFO_BAR (gtk_builder_get_object (builder, "infobar"));
    gtk_info_bar_add_button (gui->infobar, GTK_STOCK_OK, GTK_RESPONSE_OK);
    gui->infolabel = GTK_LABEL (gtk_label_new (""));
    gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (gui->infobar)),
            GTK_WIDGET (gui->infolabel));
    g_signal_connect (gui->infobar, "response", G_CALLBACK (gtk_widget_hide), NULL);

    /* flash dialog */
    gui->flash_dialog =
        GTK_DIALOG (gtk_builder_get_object (builder, "flash_dialog"));
    g_signal_connect_swapped (gui->flash_dialog, "response",
            G_CALLBACK (gtk_widget_hide), gui->flash_dialog);

    gui->flash_dialog_ok =
        GTK_BUTTON (gtk_builder_get_object (builder, "flash_dialog_ok_button"));

    gui->flash_dialog_cancel =
        GTK_BUTTON (gtk_builder_get_object (builder, "flash_dialog_cancel_button"));

    gui->flash_dialog_entry =
        GTK_ENTRY (gtk_builder_get_object (builder, "flash_dialog_entry"));

    /* make it so */
    gtk_widget_show_all (GTK_WIDGET (gui->window));
    gtk_widget_hide (GTK_WIDGET (gui->infobar));
    gtk_widget_hide (GTK_WIDGET (gui->progress.bar));

    stlink_gui_set_disconnected (gui);
}

int
main (int argc, char **argv)
{
    STlinkGUI *gui;

    gtk_init (&argc, &argv);

    gui = g_object_new (STLINK_TYPE_GUI, NULL);
    stlink_gui_build_ui (gui);
    stlink_gui_init_dnd (gui);

    gtk_main ();

    return 0;
}
