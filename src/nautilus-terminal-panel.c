/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-terminal-panel.h"

#include <glib/gi18n.h>
#include <vte/vte.h>

struct _NautilusTerminalPanel
{
    AdwBin parent_instance;

    VteTerminal *terminal;
    GtkWidget *title_label;
    GtkWidget *close_button;
    char *current_dir;
};

G_DEFINE_FINAL_TYPE (NautilusTerminalPanel, nautilus_terminal_panel, ADW_TYPE_BIN)

static void spawn_shell (NautilusTerminalPanel *self);

static void
on_child_exited (VteTerminal *terminal,
                 gint         status,
                 gpointer     user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    spawn_shell (self);
}

static void
on_close_clicked (GtkButton *button,
                  gpointer   user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

static void
spawn_shell (NautilusTerminalPanel *self)
{
    const char *shell = g_getenv ("SHELL");
    if (shell == NULL || *shell == '\0')
    {
        shell = "/bin/bash";
    }

    const char *working_dir = self->current_dir ? self->current_dir : g_get_home_dir ();
    gboolean in_flatpak = g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);

    if (in_flatpak)
    {
        char *argv[] = {
            "flatpak-spawn",
            "--host",
            "--watch-bus",
            "--env=TERM=xterm-256color",
            "--env=SHELL=/bin/bash",
            "/usr/bin/script",
            "-qefc",
            "/bin/bash -i",
            "/dev/null",
            NULL
        };

        vte_terminal_spawn_async (self->terminal,
                                  VTE_PTY_DEFAULT,
                                  working_dir,
                                  argv,
                                  NULL,
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, NULL, NULL,
                                  -1,
                                  NULL,
                                  NULL, NULL);
    }
    else
    {
        char *argv[] = { (char *) shell, NULL };
        char **envp = g_get_environ ();

        vte_terminal_spawn_async (self->terminal,
                                  VTE_PTY_DEFAULT,
                                  working_dir,
                                  argv,
                                  envp,
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, NULL, NULL,
                                  -1,
                                  NULL,
                                  NULL, NULL);
        g_strfreev (envp);
    }
}

static void
on_copy_action (GtkWidget *widget,
                gpointer   user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    vte_terminal_copy_clipboard_format (self->terminal, VTE_FORMAT_TEXT);
}

static void
on_paste_action (GtkWidget *widget,
                 gpointer   user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    vte_terminal_paste_clipboard (self->terminal);
}

static void
on_select_all_action (GtkWidget *widget,
                      gpointer   user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    vte_terminal_select_all (self->terminal);
}

static void
on_right_click_pressed (GtkGestureClick *gesture,
                        gint             n_press,
                        gdouble          x,
                        gdouble          y,
                        gpointer         user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);

    GtkWidget *popover = gtk_popover_new ();
    gtk_widget_set_parent (popover, GTK_WIDGET (self->terminal));

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_top (box, 4);
    gtk_widget_set_margin_bottom (box, 4);
    gtk_widget_set_margin_start (box, 4);
    gtk_widget_set_margin_end (box, 4);

    GtkWidget *btn_copy = gtk_button_new_with_label (_("Copy"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_copy), FALSE);
    gtk_widget_set_sensitive (btn_copy, vte_terminal_get_has_selection (self->terminal));
    g_signal_connect_swapped (btn_copy, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_copy, "clicked", G_CALLBACK (on_copy_action), self);
    gtk_box_append (GTK_BOX (box), btn_copy);

    GtkWidget *btn_paste = gtk_button_new_with_label (_("Paste"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_paste), FALSE);
    g_signal_connect_swapped (btn_paste, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_paste, "clicked", G_CALLBACK (on_paste_action), self);
    gtk_box_append (GTK_BOX (box), btn_paste);

    GtkWidget *btn_select_all = gtk_button_new_with_label (_("Select All"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_select_all), FALSE);
    g_signal_connect_swapped (btn_select_all, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_select_all, "clicked", G_CALLBACK (on_select_all_action), self);
    gtk_box_append (GTK_BOX (box), btn_select_all);

    gtk_popover_set_child (GTK_POPOVER (popover), box);
    GdkRectangle rect = { (int) x, (int) y, 1, 1 };
    gtk_popover_set_pointing_to (GTK_POPOVER (popover), &rect);
    gtk_popover_set_has_arrow (GTK_POPOVER (popover), FALSE);
    gtk_popover_popup (GTK_POPOVER (popover));
}

static gboolean
on_key_pressed (GtkEventControllerKey *controller,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        state,
                gpointer               user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    GdkModifierType mods = state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK);

    if (mods == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    {
        if (keyval == GDK_KEY_C || keyval == GDK_KEY_c)
        {
            vte_terminal_copy_clipboard_format (self->terminal, VTE_FORMAT_TEXT);
            return GDK_EVENT_STOP;
        }
        else if (keyval == GDK_KEY_V || keyval == GDK_KEY_v)
        {
            vte_terminal_paste_clipboard (self->terminal);
            return GDK_EVENT_STOP;
        }
    }
    else if (mods == GDK_SHIFT_MASK && keyval == GDK_KEY_Insert)
    {
        vte_terminal_paste_clipboard (self->terminal);
        return GDK_EVENT_STOP;
    }
    else if (mods == GDK_CONTROL_MASK && keyval == GDK_KEY_Insert)
    {
        vte_terminal_copy_clipboard_format (self->terminal, VTE_FORMAT_TEXT);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static void
nautilus_terminal_panel_dispose (GObject *object)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (object);

    g_clear_pointer (&self->current_dir, g_free);

    G_OBJECT_CLASS (nautilus_terminal_panel_parent_class)->dispose (object);
}

static void
nautilus_terminal_panel_class_init (NautilusTerminalPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = nautilus_terminal_panel_dispose;
}

static void
nautilus_terminal_panel_init (NautilusTerminalPanel *self)
{
    GtkWidget *main_box;
    GtkWidget *header_box;
    GtkWidget *icon;
    GtkWidget *separator;

    main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* Header Bar */
    header_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start (header_box, 8);
    gtk_widget_set_margin_end (header_box, 6);
    gtk_widget_set_margin_top (header_box, 4);
    gtk_widget_set_margin_bottom (header_box, 4);

    icon = gtk_image_new_from_icon_name ("utilities-terminal-symbolic");
    gtk_box_append (GTK_BOX (header_box), icon);

    self->title_label = gtk_label_new (_("Terminal"));
    gtk_label_set_xalign (GTK_LABEL (self->title_label), 0.0);
    gtk_label_set_ellipsize (GTK_LABEL (self->title_label), PANGO_ELLIPSIZE_START);
    gtk_widget_set_hexpand (self->title_label, TRUE);
    gtk_box_append (GTK_BOX (header_box), self->title_label);

    self->close_button = gtk_button_new_from_icon_name ("window-close-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->close_button), FALSE);
    gtk_widget_set_tooltip_text (self->close_button, _("Close Terminal"));
    g_signal_connect (self->close_button, "clicked", G_CALLBACK (on_close_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->close_button);

    gtk_box_append (GTK_BOX (main_box), header_box);

    separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append (GTK_BOX (main_box), separator);

    /* VTE Terminal */
    self->terminal = VTE_TERMINAL (vte_terminal_new ());
    gtk_widget_set_vexpand (GTK_WIDGET (self->terminal), TRUE);
    gtk_widget_set_hexpand (GTK_WIDGET (self->terminal), TRUE);
    gtk_widget_set_size_request (GTK_WIDGET (self->terminal), -1, 120);
    vte_terminal_set_scrollback_lines (self->terminal, 10000);
    vte_terminal_set_mouse_autohide (self->terminal, TRUE);

    GdkRGBA bg_color, fg_color;
    GdkRGBA palette[16];
    static const char *color_palette_hex[16] = {
        /* Standard 8 colors */
        "#242424", /* Black */
        "#f66151", /* Red */
        "#57e389", /* Green */
        "#f6d32d", /* Yellow */
        "#62a0ea", /* Blue (vibrant, high contrast) */
        "#c061cb", /* Magenta */
        "#4cd9e4", /* Cyan */
        "#deddda", /* White */
        /* Bright 8 colors */
        "#5e5c64", /* Bright Black */
        "#ed333b", /* Bright Red */
        "#57e389", /* Bright Green */
        "#f8e45c", /* Bright Yellow */
        "#78aeed", /* Bright Blue */
        "#dc8add", /* Bright Magenta */
        "#6be5ee", /* Bright Cyan */
        "#ffffff"  /* Bright White */
    };

    gdk_rgba_parse (&bg_color, "#1d1d20");
    gdk_rgba_parse (&fg_color, "#dcdcdc");
    vte_terminal_set_color_background (self->terminal, &bg_color);
    vte_terminal_set_color_foreground (self->terminal, &fg_color);

    for (int i = 0; i < 16; i++)
    {
        gdk_rgba_parse (&palette[i], color_palette_hex[i]);
    }

    vte_terminal_set_colors (self->terminal, &fg_color, &bg_color, palette, 16);

    g_signal_connect (self->terminal, "child-exited", G_CALLBACK (on_child_exited), self);

    /* Shortcuts and context menu */
    GtkEventController *key_controller = gtk_event_controller_key_new ();
    g_signal_connect (key_controller, "key-pressed", G_CALLBACK (on_key_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (self->terminal), key_controller);

    GtkGesture *click_gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), GDK_BUTTON_SECONDARY);
    g_signal_connect (click_gesture, "pressed", G_CALLBACK (on_right_click_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (self->terminal), GTK_EVENT_CONTROLLER (click_gesture));

    gtk_box_append (GTK_BOX (main_box), GTK_WIDGET (self->terminal));

    adw_bin_set_child (ADW_BIN (self), main_box);

    spawn_shell (self);
}

GtkWidget *
nautilus_terminal_panel_new (void)
{
    return g_object_new (NAUTILUS_TYPE_TERMINAL_PANEL, NULL);
}

void
nautilus_terminal_panel_sync_location (NautilusTerminalPanel *self,
                                       GFile                 *location)
{
    g_return_if_fail (NAUTILUS_IS_TERMINAL_PANEL (self));

    if (location == NULL)
    {
        return;
    }

    g_autofree char *path = g_file_get_path (location);
    if (path == NULL)
    {
        return;
    }

    if (self->current_dir != NULL && g_strcmp0 (self->current_dir, path) == 0)
    {
        return;
    }

    g_free (self->current_dir);
    self->current_dir = g_strdup (path);

    if (self->title_label != NULL)
    {
        g_autofree char *title = g_strdup_printf (_("Terminal — %s"), path);
        gtk_label_set_text (GTK_LABEL (self->title_label), title);
    }

    if (self->terminal != NULL)
    {
        g_autofree char *quoted = g_shell_quote (path);
        g_autofree char *cmd = g_strdup_printf (" cd %s\n", quoted);
        vte_terminal_feed_child (self->terminal, cmd, -1);
    }
}

void
nautilus_terminal_panel_grab_focus (NautilusTerminalPanel *self)
{
    g_return_if_fail (NAUTILUS_IS_TERMINAL_PANEL (self));

    if (self->terminal != NULL)
    {
        gtk_widget_grab_focus (GTK_WIDGET (self->terminal));
    }
}
