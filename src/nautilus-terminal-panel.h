/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_TERMINAL_PANEL (nautilus_terminal_panel_get_type())

G_DECLARE_FINAL_TYPE (NautilusTerminalPanel, nautilus_terminal_panel, NAUTILUS, TERMINAL_PANEL, GtkWidget)

GtkWidget *nautilus_terminal_panel_new (void);
void       nautilus_terminal_panel_sync_location (NautilusTerminalPanel *self,
                                                  GFile                 *location);
void       nautilus_terminal_panel_grab_focus    (NautilusTerminalPanel *self);

G_END_DECLS

