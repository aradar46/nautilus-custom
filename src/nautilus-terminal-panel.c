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

    AdwTabView *tab_view;
    GtkWidget *title_label;
    GtkWidget *close_button;
    char *current_dir;
};

G_DEFINE_FINAL_TYPE (NautilusTerminalPanel, nautilus_terminal_panel, ADW_TYPE_BIN)

static VteTerminal *add_terminal_tab (NautilusTerminalPanel *self);
static void spawn_shell (NautilusTerminalPanel *self,
                         VteTerminal          *terminal);

static VteTerminal *
get_selected_terminal (NautilusTerminalPanel *self)
{
    AdwTabPage *page = adw_tab_view_get_selected_page (self->tab_view);

    return page != NULL ? VTE_TERMINAL (adw_tab_page_get_child (page)) : NULL;
}

static void
on_child_exited (VteTerminal *terminal,
                 gint         status,
                 gpointer     user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    spawn_shell (self, terminal);
}

static void
on_close_clicked (GtkButton *button,
                  gpointer   user_data)
{
    NautilusTerminalPanel *self = NAUTILUS_TERMINAL_PANEL (user_data);
    gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

static void
spawn_shell (NautilusTerminalPanel *self,
             VteTerminal          *terminal)
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

        vte_terminal_spawn_async (terminal,
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

        vte_terminal_spawn_async (terminal,
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
    VteTerminal *terminal = VTE_TERMINAL (user_data);
    vte_terminal_copy_clipboard_format (terminal, VTE_FORMAT_TEXT);
}

static void
on_paste_action (GtkWidget *widget,
                 gpointer   user_data)
{
    VteTerminal *terminal = VTE_TERMINAL (user_data);
    vte_terminal_paste_clipboard (terminal);
}

static void
on_select_all_action (GtkWidget *widget,
                      gpointer   user_data)
{
    VteTerminal *terminal = VTE_TERMINAL (user_data);
    vte_terminal_select_all (terminal);
}

static void
on_right_click_pressed (GtkGestureClick *gesture,
                        gint             n_press,
                        gdouble          x,
                        gdouble          y,
                        gpointer         user_data)
{
    VteTerminal *terminal = VTE_TERMINAL (
        gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture)));

    GtkWidget *popover = gtk_popover_new ();
    gtk_widget_set_parent (popover, GTK_WIDGET (terminal));

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_top (box, 4);
    gtk_widget_set_margin_bottom (box, 4);
    gtk_widget_set_margin_start (box, 4);
    gtk_widget_set_margin_end (box, 4);

    GtkWidget *btn_copy = gtk_button_new_with_label (_("Copy"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_copy), FALSE);
    gtk_widget_set_sensitive (btn_copy, vte_terminal_get_has_selection (terminal));
    g_signal_connect_swapped (btn_copy, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_copy, "clicked", G_CALLBACK (on_copy_action), terminal);
    gtk_box_append (GTK_BOX (box), btn_copy);

    GtkWidget *btn_paste = gtk_button_new_with_label (_("Paste"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_paste), FALSE);
    g_signal_connect_swapped (btn_paste, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_paste, "clicked", G_CALLBACK (on_paste_action), terminal);
    gtk_box_append (GTK_BOX (box), btn_paste);

    GtkWidget *btn_select_all = gtk_button_new_with_label (_("Select All"));
    gtk_button_set_has_frame (GTK_BUTTON (btn_select_all), FALSE);
    g_signal_connect_swapped (btn_select_all, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
    g_signal_connect (btn_select_all, "clicked", G_CALLBACK (on_select_all_action), terminal);
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
    VteTerminal *terminal = VTE_TERMINAL (
        gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller)));
    GdkModifierType mods = state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK);

    if (mods == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    {
        if (keyval == GDK_KEY_T || keyval == GDK_KEY_t)
        {
            add_terminal_tab (self);
            return GDK_EVENT_STOP;
        }
        else if (keyval == GDK_KEY_W || keyval == GDK_KEY_w)
        {
            AdwTabPage *page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (terminal));
            adw_tab_view_close_page (self->tab_view, page);
            return GDK_EVENT_STOP;
        }
        else if (keyval == GDK_KEY_C || keyval == GDK_KEY_c)
        {
            vte_terminal_copy_clipboard_format (terminal, VTE_FORMAT_TEXT);
            return GDK_EVENT_STOP;
        }
        else if (keyval == GDK_KEY_V || keyval == GDK_KEY_v)
        {
            vte_terminal_paste_clipboard (terminal);
            return GDK_EVENT_STOP;
        }
    }
    else if (mods == GDK_CONTROL_MASK && keyval == GDK_KEY_Page_Up)
    {
        adw_tab_view_select_previous_page (self->tab_view);
        return GDK_EVENT_STOP;
    }
    else if (mods == GDK_CONTROL_MASK && keyval == GDK_KEY_Page_Down)
    {
        adw_tab_view_select_next_page (self->tab_view);
        return GDK_EVENT_STOP;
    }
    else if (mods == GDK_SHIFT_MASK && keyval == GDK_KEY_Insert)
    {
        vte_terminal_paste_clipboard (terminal);
        return GDK_EVENT_STOP;
    }
    else if (mods == GDK_CONTROL_MASK && keyval == GDK_KEY_Insert)
    {
        vte_terminal_copy_clipboard_format (terminal, VTE_FORMAT_TEXT);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static void
set_tab_title (NautilusTerminalPanel *self,
               AdwTabPage           *page)
{
    const char *path = self->current_dir ? self->current_dir : g_get_home_dir ();
    g_autofree char *basename = g_path_get_basename (path);

    adw_tab_page_set_title (page, basename);
}

static VteTerminal *
add_terminal_tab (NautilusTerminalPanel *self)
{
    VteTerminal *terminal = VTE_TERMINAL (vte_terminal_new ());
    AdwTabPage *page;
    GdkRGBA bg_color, fg_color;
    GdkRGBA palette[16];
    static const char *color_palette_hex[16] = {
        "#242424", "#f66151", "#57e389", "#f6d32d",
        "#62a0ea", "#c061cb", "#4cd9e4", "#deddda",
        "#5e5c64", "#ed333b", "#57e389", "#f8e45c",
        "#78aeed", "#dc8add", "#6be5ee", "#ffffff"
    };

    gtk_widget_set_vexpand (GTK_WIDGET (terminal), TRUE);
    gtk_widget_set_hexpand (GTK_WIDGET (terminal), TRUE);
    vte_terminal_set_scrollback_lines (terminal, 10000);
    vte_terminal_set_mouse_autohide (terminal, TRUE);

    gdk_rgba_parse (&bg_color, "#1d1d20");
    gdk_rgba_parse (&fg_color, "#dcdcdc");
    for (gsize i = 0; i < G_N_ELEMENTS (palette); i++)
    {
        gdk_rgba_parse (&palette[i], color_palette_hex[i]);
    }
    vte_terminal_set_colors (terminal, &fg_color, &bg_color, palette, G_N_ELEMENTS (palette));

    g_signal_connect (terminal, "child-exited", G_CALLBACK (on_child_exited), self);

    GtkEventController *key_controller = gtk_event_controller_key_new ();
    g_signal_connect (key_controller, "key-pressed", G_CALLBACK (on_key_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (terminal), key_controller);

    GtkGesture *click_gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), GDK_BUTTON_SECONDARY);
    g_signal_connect (click_gesture, "pressed", G_CALLBACK (on_right_click_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (terminal), GTK_EVENT_CONTROLLER (click_gesture));

    page = adw_tab_view_append (self->tab_view, GTK_WIDGET (terminal));
    set_tab_title (self, page);
    adw_tab_view_set_selected_page (self->tab_view, page);
    spawn_shell (self, terminal);
    gtk_widget_grab_focus (GTK_WIDGET (terminal));

    return terminal;
}

static void
on_new_tab_clicked (GtkButton *button,
                    gpointer   user_data)
{
    add_terminal_tab (NAUTILUS_TERMINAL_PANEL (user_data));
}

static void
on_n_pages_changed (AdwTabView *tab_view,
                    GParamSpec *pspec,
                    gpointer    user_data)
{
    if (adw_tab_view_get_n_pages (tab_view) == 0)
    {
        gtk_widget_set_visible (GTK_WIDGET (user_data), FALSE);
    }
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
    GtkWidget *new_tab_button;
    GtkWidget *separator;
    AdwTabBar *tab_bar;

    main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    self->tab_view = adw_tab_view_new ();

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

    new_tab_button = gtk_button_new_from_icon_name ("list-add-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (new_tab_button), FALSE);
    gtk_widget_set_tooltip_text (new_tab_button, _("New Terminal Tab"));
    g_signal_connect (new_tab_button, "clicked", G_CALLBACK (on_new_tab_clicked), self);
    gtk_box_append (GTK_BOX (header_box), new_tab_button);

    self->close_button = gtk_button_new_from_icon_name ("window-close-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->close_button), FALSE);
    gtk_widget_set_tooltip_text (self->close_button, _("Close Terminal"));
    g_signal_connect (self->close_button, "clicked", G_CALLBACK (on_close_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->close_button);

    gtk_box_append (GTK_BOX (main_box), header_box);

    separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append (GTK_BOX (main_box), separator);

    tab_bar = adw_tab_bar_new ();
    adw_tab_bar_set_view (tab_bar, self->tab_view);
    adw_tab_bar_set_autohide (tab_bar, FALSE);
    gtk_box_append (GTK_BOX (main_box), GTK_WIDGET (tab_bar));

    gtk_widget_set_vexpand (GTK_WIDGET (self->tab_view), TRUE);
    gtk_widget_set_hexpand (GTK_WIDGET (self->tab_view), TRUE);
    gtk_widget_set_size_request (GTK_WIDGET (self->tab_view), -1, 120);
    gtk_box_append (GTK_BOX (main_box), GTK_WIDGET (self->tab_view));
    g_signal_connect (self->tab_view, "notify::n-pages", G_CALLBACK (on_n_pages_changed), self);

    adw_bin_set_child (ADW_BIN (self), main_box);

    add_terminal_tab (self);
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

    VteTerminal *terminal = get_selected_terminal (self);
    if (terminal != NULL)
    {
        AdwTabPage *page = adw_tab_view_get_selected_page (self->tab_view);
        g_autofree char *quoted = g_shell_quote (path);
        g_autofree char *cmd = g_strdup_printf (" cd %s\n", quoted);
        set_tab_title (self, page);
        vte_terminal_feed_child (terminal, cmd, -1);
    }
}

void
nautilus_terminal_panel_grab_focus (NautilusTerminalPanel *self)
{
    g_return_if_fail (NAUTILUS_IS_TERMINAL_PANEL (self));

    VteTerminal *terminal = get_selected_terminal (self);
    if (terminal == NULL)
    {
        terminal = add_terminal_tab (self);
    }

    gtk_widget_grab_focus (GTK_WIDGET (terminal));
}
