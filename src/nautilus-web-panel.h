/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_WEB_PANEL (nautilus_web_panel_get_type())

G_DECLARE_FINAL_TYPE (NautilusWebPanel, nautilus_web_panel, NAUTILUS, WEB_PANEL, GtkWidget)

GtkWidget *nautilus_web_panel_new        (void);
void       nautilus_web_panel_grab_focus (NautilusWebPanel *self);
void       nautilus_web_panel_load_uri   (NautilusWebPanel *self,
                                          const char       *uri);
void       nautilus_web_panel_reset      (NautilusWebPanel *self);

G_END_DECLS
