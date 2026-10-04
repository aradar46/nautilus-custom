/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-web-panel.h"

#include <glib/gi18n.h>
#include <webkit/webkit.h>

typedef enum
{
    SEARCH_ENGINE_GOOGLE,
    SEARCH_ENGINE_STARTPAGE,
    SEARCH_ENGINE_DUCKDUCKGO,
} SearchEngine;

static const char *
get_engine_home_url (SearchEngine engine)
{
    switch (engine)
    {
        case SEARCH_ENGINE_GOOGLE:
            return "https://www.google.com";
        case SEARCH_ENGINE_STARTPAGE:
            return "https://www.startpage.com";
        case SEARCH_ENGINE_DUCKDUCKGO:
        default:
            return "https://duckduckgo.com";
    }
}

static char *
build_search_url (SearchEngine engine,
                  const char  *escaped_query)
{
    switch (engine)
    {
        case SEARCH_ENGINE_GOOGLE:
            return g_strdup_printf ("https://www.google.com/search?q=%s", escaped_query);
        case SEARCH_ENGINE_STARTPAGE:
            return g_strdup_printf ("https://www.startpage.com/sp/search?query=%s", escaped_query);
        case SEARCH_ENGINE_DUCKDUCKGO:
        default:
            return g_strdup_printf ("https://duckduckgo.com/?q=%s", escaped_query);
    }
}

static const char *vimium_script_source =
    "(function() {"
    "  if (window.__vimium_injected) return;"
    "  window.__vimium_injected = true;"
    "  let hintMode = false;"
    "  let newTabMode = false;"
    "  let insertMode = false;"
    "  let hintOverlay = null;"
    "  let activeHints = [];"
    "  let keyBuffer = '';"
    "  let lastGTime = 0;"
    "  const HINT_CHARS = 'sadfjklewcmpgh';"
    "  function isEditable(el) {"
    "    if (!el) return false;"
    "    const tag = el.tagName ? el.tagName.toLowerCase() : '';"
    "    return tag === 'input' || tag === 'textarea' || tag === 'select' || el.isContentEditable;"
    "  }"
    "  function getClickableElements() {"
    "    const selector = 'a[href], button, input, select, textarea, summary, [role=\"button\"], [role=\"link\"], [onclick], [tabindex]:not([tabindex=\"-1\"])';"
    "    const elements = Array.from(document.querySelectorAll(selector));"
    "    const visible = [];"
    "    const vpWidth = window.innerWidth;"
    "    const vpHeight = window.innerHeight;"
    "    for (const el of elements) {"
    "      const rect = el.getBoundingClientRect();"
    "      if (rect.width > 3 && rect.height > 3 &&"
    "          rect.bottom > 0 && rect.top < vpHeight &&"
    "          rect.right > 0 && rect.left < vpWidth) {"
    "        const style = window.getComputedStyle(el);"
    "        if (style.visibility !== 'hidden' && style.display !== 'none' && parseFloat(style.opacity) > 0.05) {"
    "          visible.push({ el, rect });"
    "        }"
    "      }"
    "    }"
    "    return visible;"
    "  }"
    "  function generateHintStrings(count) {"
    "    const chars = HINT_CHARS;"
    "    const base = chars.length;"
    "    if (count <= base) {"
    "      return chars.slice(0, count).split('');"
    "    }"
    "    const result = [];"
    "    for (let i = 0; i < count; i++) {"
    "      const first = chars[Math.floor(i / base) % base];"
    "      const second = chars[i % base];"
    "      result.push(first + second);"
    "    }"
    "    return result;"
    "  }"
    "  function clearHints() {"
    "    if (hintOverlay) {"
    "      hintOverlay.remove();"
    "      hintOverlay = null;"
    "    }"
    "    activeHints = [];"
    "    hintMode = false;"
    "    keyBuffer = '';"
    "  }"
    "  function showHints(newTab) {"
    "    clearHints();"
    "    const clickables = getClickableElements();"
    "    if (clickables.length === 0) return;"
    "    hintMode = true;"
    "    newTabMode = newTab;"
    "    keyBuffer = '';"
    "    hintOverlay = document.createElement('div');"
    "    hintOverlay.id = '__vimium_hint_overlay';"
    "    hintOverlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;z-index:2147483647;pointer-events:none;';"
    "    const hintStrings = generateHintStrings(clickables.length);"
    "    clickables.forEach((item, index) => {"
    "      const code = hintStrings[index];"
    "      const badge = document.createElement('span');"
    "      badge.textContent = code.toUpperCase();"
    "      badge.dataset.code = code;"
    "      badge.style.cssText = 'position:fixed;' +"
    "        'top:' + Math.max(2, item.rect.top) + 'px;' +"
    "        'left:' + Math.max(2, item.rect.left) + 'px;' +"
    "        'background:#ffe600;color:#111;font-family:monospace,sans-serif;' +"
    "        'font-size:11px;font-weight:700;line-height:1;padding:2px 4px;' +"
    "        'border:1px solid #b39b00;border-radius:3px;' +"
    "        'box-shadow:0 1px 3px rgba(0,0,0,0.4);z-index:2147483647;' +"
    "        'pointer-events:none;user-select:none;';"
    "      hintOverlay.appendChild(badge);"
    "      activeHints.push({ code, el: item.el, badge });"
    "    });"
    "    document.documentElement.appendChild(hintOverlay);"
    "  }"
    "  function handleHintKey(key) {"
    "    keyBuffer += key.toLowerCase();"
    "    let matched = activeHints.filter(h => h.code.startsWith(keyBuffer));"
    "    if (matched.length === 0) {"
    "      clearHints();"
    "      return;"
    "    }"
    "    if (matched.length === 1 && matched[0].code === keyBuffer) {"
    "      const target = matched[0].el;"
    "      clearHints();"
    "      if (newTabMode && target.tagName && target.tagName.toLowerCase() === 'a' && target.href) {"
    "        window.open(target.href, '_blank');"
    "      } else {"
    "        target.focus();"
    "        target.click();"
    "      }"
    "      return;"
    "    }"
    "    for (const h of activeHints) {"
    "      if (h.code.startsWith(keyBuffer)) {"
    "        h.badge.style.display = 'inline-block';"
    "        h.badge.style.background = '#ff9800';"
    "      } else {"
    "        h.badge.style.display = 'none';"
    "      }"
    "    }"
    "  }"
    "  window.addEventListener('keydown', function(e) {"
    "    if (e.key === 'Escape') {"
    "      if (hintMode) {"
    "        e.preventDefault();"
    "        e.stopPropagation();"
    "        clearHints();"
    "        return;"
    "      }"
    "      if (insertMode) {"
    "        insertMode = false;"
    "        return;"
    "      }"
    "      if (isEditable(document.activeElement)) {"
    "        document.activeElement.blur();"
    "        return;"
    "      }"
    "    }"
    "    if (hintMode) {"
    "      if (/^[a-zA-Z]$/.test(e.key) && !e.ctrlKey && !e.altKey && !e.metaKey) {"
    "        e.preventDefault();"
    "        e.stopPropagation();"
    "        handleHintKey(e.key);"
    "      } else {"
    "        clearHints();"
    "      }"
    "      return;"
    "    }"
    "    if (insertMode || isEditable(document.activeElement)) {"
    "      return;"
    "    }"
    "    if (e.ctrlKey || e.altKey || e.metaKey) {"
    "      return;"
    "    }"
    "    const now = Date.now();"
    "    switch (e.key) {"
    "      case 'f': e.preventDefault(); e.stopPropagation(); showHints(false); break;"
    "      case 'F': e.preventDefault(); e.stopPropagation(); showHints(true); break;"
    "      case 'j': e.preventDefault(); e.stopPropagation(); window.scrollBy({ top: 60, behavior: 'smooth' }); break;"
    "      case 'k': e.preventDefault(); e.stopPropagation(); window.scrollBy({ top: -60, behavior: 'smooth' }); break;"
    "      case 'h': e.preventDefault(); e.stopPropagation(); window.scrollBy({ left: -50, behavior: 'smooth' }); break;"
    "      case 'l': e.preventDefault(); e.stopPropagation(); window.scrollBy({ left: 50, behavior: 'smooth' }); break;"
    "      case 'd': e.preventDefault(); e.stopPropagation(); window.scrollBy({ top: window.innerHeight * 0.5, behavior: 'smooth' }); break;"
    "      case 'u': e.preventDefault(); e.stopPropagation(); window.scrollBy({ top: -window.innerHeight * 0.5, behavior: 'smooth' }); break;"
    "      case 'g':"
    "        if (now - lastGTime < 400) {"
    "          e.preventDefault();"
    "          e.stopPropagation();"
    "          window.scrollTo({ top: 0, behavior: 'smooth' });"
    "          lastGTime = 0;"
    "        } else {"
    "          lastGTime = now;"
    "        }"
    "        break;"
    "      case 'G': e.preventDefault(); e.stopPropagation(); window.scrollTo({ top: document.body.scrollHeight, behavior: 'smooth' }); break;"
    "      case 'r': e.preventDefault(); e.stopPropagation(); window.location.reload(); break;"
    "      case 'H': e.preventDefault(); e.stopPropagation(); window.history.back(); break;"
    "      case 'L': e.preventDefault(); e.stopPropagation(); window.history.forward(); break;"
    "      case 'i':"
    "        if (now - lastGTime < 400) {"
    "          e.preventDefault();"
    "          e.stopPropagation();"
    "          const firstInput = document.querySelector('input:not([type=\"hidden\"]), textarea');"
    "          if (firstInput) firstInput.focus();"
    "          lastGTime = 0;"
    "        } else {"
    "          insertMode = true;"
    "        }"
    "        break;"
    "    }"
    "  }, true);"
    "})();";

struct _NautilusWebPanel
{
    AdwBin parent_instance;

    GtkWidget *main_box;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *reload_button;
    GtkWidget *home_button;
    GtkWidget *new_tab_button;
    GtkWidget *url_entry;
    GtkWidget *engine_button;
    GtkWidget *radio_google;
    GtkWidget *radio_startpage;
    GtkWidget *radio_duckduckgo;
    GtkWidget *close_button;
    GtkWidget *progress_bar;

    AdwTabBar *tab_bar;
    AdwTabView *tab_view;

    SearchEngine current_engine;
};

G_DEFINE_FINAL_TYPE (NautilusWebPanel, nautilus_web_panel, ADW_TYPE_BIN)

static AdwTabPage *add_web_tab (NautilusWebPanel *self,
                                const char       *initial_uri);

static WebKitWebView *
get_active_web_view (NautilusWebPanel *self)
{
    if (self->tab_view == NULL)
    {
        return NULL;
    }

    AdwTabPage *page = adw_tab_view_get_selected_page (self->tab_view);
    if (page == NULL)
    {
        return NULL;
    }

    return WEBKIT_WEB_VIEW (adw_tab_page_get_child (page));
}

static void
sync_ui_with_active_tab (NautilusWebPanel *self)
{
    WebKitWebView *view = get_active_web_view (self);

    if (view == NULL)
    {
        gtk_editable_set_text (GTK_EDITABLE (self->url_entry), "");
        gtk_widget_set_sensitive (self->back_button, FALSE);
        gtk_widget_set_sensitive (self->forward_button, FALSE);
        gtk_widget_set_visible (self->progress_bar, FALSE);
        return;
    }

    const char *uri = webkit_web_view_get_uri (view);
    if (!gtk_widget_has_focus (self->url_entry))
    {
        gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri != NULL ? uri : "");
    }

    gtk_widget_set_sensitive (self->back_button, webkit_web_view_can_go_back (view));
    gtk_widget_set_sensitive (self->forward_button, webkit_web_view_can_go_forward (view));

    const char *title = webkit_web_view_get_title (view);
    if (title != NULL && *title != '\0')
    {
        gtk_widget_set_tooltip_text (self->url_entry, title);
    }
}

static void
load_uri_from_text (NautilusWebPanel *self,
                    const char       *text)
{
    if (text == NULL || *text == '\0')
    {
        return;
    }

    g_autofree char *trimmed = g_strstrip (g_strdup (text));
    if (*trimmed == '\0')
    {
        return;
    }

    WebKitWebView *view = get_active_web_view (self);
    if (view == NULL)
    {
        AdwTabPage *page = add_web_tab (self, NULL);
        view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (page));
    }

    g_autofree char *target_uri = NULL;

    if (g_str_has_prefix (trimmed, "http://") ||
        g_str_has_prefix (trimmed, "https://") ||
        g_str_has_prefix (trimmed, "file://") ||
        g_str_has_prefix (trimmed, "about:"))
    {
        target_uri = g_strdup (trimmed);
    }
    else if (strchr (trimmed, ' ') == NULL && (strchr (trimmed, '.') != NULL || g_str_has_prefix (trimmed, "localhost")))
    {
        if (g_str_has_prefix (trimmed, "localhost"))
        {
            target_uri = g_strdup_printf ("http://%s", trimmed);
        }
        else
        {
            target_uri = g_strdup_printf ("https://%s", trimmed);
        }
    }
    else
    {
        g_autofree char *escaped = g_uri_escape_string (trimmed, NULL, TRUE);
        target_uri = build_search_url (self->current_engine, escaped);
    }

    webkit_web_view_load_uri (view, target_uri);
    gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
on_entry_activate (GtkEntry *entry,
                   gpointer  user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));

    load_uri_from_text (self, text);
}

static void
on_back_clicked (GtkButton *button,
                 gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = get_active_web_view (self);

    if (view != NULL && webkit_web_view_can_go_back (view))
    {
        webkit_web_view_go_back (view);
    }
}

static void
on_forward_clicked (GtkButton *button,
                    gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = get_active_web_view (self);

    if (view != NULL && webkit_web_view_can_go_forward (view))
    {
        webkit_web_view_go_forward (view);
    }
}

static void
on_reload_clicked (GtkButton *button,
                   gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = get_active_web_view (self);

    if (view != NULL)
    {
        webkit_web_view_reload (view);
    }
}

static void
on_home_clicked (GtkButton *button,
                 gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = get_active_web_view (self);
    const char *home_url = get_engine_home_url (self->current_engine);

    if (view != NULL)
    {
        webkit_web_view_load_uri (view, home_url);
        gtk_widget_grab_focus (GTK_WIDGET (view));
    }
    else
    {
        add_web_tab (self, home_url);
    }
}

static void
on_new_tab_clicked (GtkButton *button,
                    gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    const char *home_url = get_engine_home_url (self->current_engine);

    add_web_tab (self, home_url);
}

static void
on_close_clicked (GtkButton *button,
                  gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
    nautilus_web_panel_reset (self);
}

static void
on_engine_toggled (GtkCheckButton *button,
                   gpointer        user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    if (!gtk_check_button_get_active (button))
    {
        return;
    }

    if (button == GTK_CHECK_BUTTON (self->radio_google))
    {
        self->current_engine = SEARCH_ENGINE_GOOGLE;
        gtk_entry_set_placeholder_text (GTK_ENTRY (self->url_entry), _("Search Google or address…"));
    }
    else if (button == GTK_CHECK_BUTTON (self->radio_startpage))
    {
        self->current_engine = SEARCH_ENGINE_STARTPAGE;
        gtk_entry_set_placeholder_text (GTK_ENTRY (self->url_entry), _("Search Startpage or address…"));
    }
    else if (button == GTK_CHECK_BUTTON (self->radio_duckduckgo))
    {
        self->current_engine = SEARCH_ENGINE_DUCKDUCKGO;
        gtk_entry_set_placeholder_text (GTK_ENTRY (self->url_entry), _("Search DuckDuckGo or address…"));
    }

    WebKitWebView *view = get_active_web_view (self);
    if (view != NULL)
    {
        const char *uri = webkit_web_view_get_uri (view);
        if (uri == NULL || *uri == '\0' ||
            g_str_has_prefix (uri, "https://www.google.com") ||
            g_str_has_prefix (uri, "https://www.startpage.com") ||
            g_str_has_prefix (uri, "https://duckduckgo.com"))
        {
            webkit_web_view_load_uri (view, get_engine_home_url (self->current_engine));
        }
    }
}

static void
on_view_load_changed (WebKitWebView   *web_view,
                      WebKitLoadEvent  load_event,
                      gpointer         user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    if (web_view == get_active_web_view (self))
    {
        if (load_event == WEBKIT_LOAD_STARTED)
        {
            gtk_widget_set_visible (self->progress_bar, TRUE);
            gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), 0.1);
        }
        else if (load_event == WEBKIT_LOAD_FINISHED)
        {
            gtk_widget_set_visible (self->progress_bar, FALSE);
        }

        gtk_widget_set_sensitive (self->back_button, webkit_web_view_can_go_back (web_view));
        gtk_widget_set_sensitive (self->forward_button, webkit_web_view_can_go_forward (web_view));
    }
}

static void
on_view_progress_changed (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = WEBKIT_WEB_VIEW (object);

    if (view == get_active_web_view (self))
    {
        gdouble progress = webkit_web_view_get_estimated_load_progress (view);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), progress);
    }
}

static void
on_view_uri_changed (GObject    *object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = WEBKIT_WEB_VIEW (object);

    if (view == get_active_web_view (self))
    {
        const char *uri = webkit_web_view_get_uri (view);
        if (uri != NULL && !gtk_widget_has_focus (self->url_entry))
        {
            gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri);
        }
    }
}

static void
on_view_title_changed (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);
    WebKitWebView *view = WEBKIT_WEB_VIEW (object);
    const char *title = webkit_web_view_get_title (view);

    if (self->tab_view != NULL)
    {
        AdwTabPage *page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (view));
        if (page != NULL && title != NULL && *title != '\0')
        {
            adw_tab_page_set_title (page, title);
        }
    }

    if (view == get_active_web_view (self) && title != NULL && *title != '\0')
    {
        gtk_widget_set_tooltip_text (self->url_entry, title);
    }
}

static gboolean
on_view_decide_policy (WebKitWebView           *web_view,
                       WebKitPolicyDecision    *decision,
                       WebKitPolicyDecisionType decision_type,
                       gpointer                 user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
        WebKitNavigationPolicyDecision *nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
        WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action (nav_decision);
        WebKitURIRequest *request = webkit_navigation_action_get_request (action);

        if (request != NULL)
        {
            const char *uri = webkit_uri_request_get_uri (request);
            if (uri != NULL)
            {
                add_web_tab (self, uri);
                webkit_policy_decision_ignore (decision);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void
inject_vimium_script (WebKitWebView *view)
{
    WebKitUserContentManager *ucm = webkit_web_view_get_user_content_manager (view);
    WebKitUserScript *script = webkit_user_script_new (vimium_script_source,
                                                       WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                       WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END,
                                                       NULL, NULL);
    webkit_user_content_manager_add_script (ucm, script);
    webkit_user_script_unref (script);
}

static AdwTabPage *
add_web_tab (NautilusWebPanel *self,
             const char       *initial_uri)
{
    WebKitWebView *view;
    WebKitSettings *settings;
    AdwTabPage *page;

    view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_widget_set_focusable (GTK_WIDGET (view), TRUE);
    gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);

    GtkGesture *click_gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), 0);
    g_signal_connect_swapped (click_gesture, "pressed", G_CALLBACK (gtk_widget_grab_focus), view);
    gtk_widget_add_controller (GTK_WIDGET (view), GTK_EVENT_CONTROLLER (click_gesture));

    settings = webkit_web_view_get_settings (view);
    webkit_settings_set_enable_developer_extras (settings, FALSE);
    webkit_settings_set_enable_javascript (settings, TRUE);

    gtk_widget_set_hexpand (GTK_WIDGET (view), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);

    inject_vimium_script (view);

    g_signal_connect (view, "load-changed", G_CALLBACK (on_view_load_changed), self);
    g_signal_connect (view, "notify::estimated-load-progress", G_CALLBACK (on_view_progress_changed), self);
    g_signal_connect (view, "notify::uri", G_CALLBACK (on_view_uri_changed), self);
    g_signal_connect (view, "notify::title", G_CALLBACK (on_view_title_changed), self);
    g_signal_connect (view, "decide-policy", G_CALLBACK (on_view_decide_policy), self);

    page = adw_tab_view_append (self->tab_view, GTK_WIDGET (view));
    adw_tab_page_set_title (page, _("New Tab"));
    adw_tab_view_set_selected_page (self->tab_view, page);

    if (initial_uri != NULL && *initial_uri != '\0')
    {
        webkit_web_view_load_uri (view, initial_uri);
    }
    else
    {
        webkit_web_view_load_uri (view, get_engine_home_url (self->current_engine));
    }

    gtk_widget_grab_focus (GTK_WIDGET (view));

    return page;
}

static gboolean
on_tab_close_page (AdwTabView *tab_view,
                   AdwTabPage *page,
                   gpointer    user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    adw_tab_view_close_page_finish (tab_view, page, TRUE);

    if (adw_tab_view_get_n_pages (tab_view) == 0)
    {
        gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
        nautilus_web_panel_reset (self);
    }

    return GDK_EVENT_STOP;
}

static void
on_selected_page_changed (AdwTabView *tab_view,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (user_data);

    sync_ui_with_active_tab (self);
}

static void
ensure_tabs (NautilusWebPanel *self)
{
    if (self->tab_view == NULL)
    {
        return;
    }

    if (adw_tab_view_get_n_pages (self->tab_view) == 0)
    {
        add_web_tab (self, get_engine_home_url (self->current_engine));
    }
}

static void
on_map (GtkWidget *widget,
        gpointer   user_data)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (widget);

    ensure_tabs (self);

    GtkWidget *parent = gtk_widget_get_parent (widget);
    if (GTK_IS_PANED (parent))
    {
        int total = gtk_widget_get_width (parent);
        if (total > 550)
        {
            gtk_paned_set_position (GTK_PANED (parent), total - 380);
        }
    }
}

static void
nautilus_web_panel_dispose (GObject *object)
{
    NautilusWebPanel *self = NAUTILUS_WEB_PANEL (object);

    nautilus_web_panel_reset (self);
    adw_bin_set_child (ADW_BIN (self), NULL);

    G_OBJECT_CLASS (nautilus_web_panel_parent_class)->dispose (object);
}

static void
nautilus_web_panel_class_init (NautilusWebPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = nautilus_web_panel_dispose;
}

static void
nautilus_web_panel_init (NautilusWebPanel *self)
{
    GtkWidget *header_box;
    GtkWidget *separator;
    GtkWidget *popover;
    GtkWidget *pbox;
    GtkWidget *plabel;

    self->current_engine = SEARCH_ENGINE_GOOGLE;
    self->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* Navigation / Header Bar */
    header_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start (header_box, 6);
    gtk_widget_set_margin_end (header_box, 6);
    gtk_widget_set_margin_top (header_box, 4);
    gtk_widget_set_margin_bottom (header_box, 4);

    self->back_button = gtk_button_new_from_icon_name ("go-previous-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->back_button), FALSE);
    gtk_widget_set_tooltip_text (self->back_button, _("Back (H)"));
    gtk_widget_set_sensitive (self->back_button, FALSE);
    g_signal_connect (self->back_button, "clicked", G_CALLBACK (on_back_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->back_button);

    self->forward_button = gtk_button_new_from_icon_name ("go-next-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->forward_button), FALSE);
    gtk_widget_set_tooltip_text (self->forward_button, _("Forward (L)"));
    gtk_widget_set_sensitive (self->forward_button, FALSE);
    g_signal_connect (self->forward_button, "clicked", G_CALLBACK (on_forward_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->forward_button);

    self->reload_button = gtk_button_new_from_icon_name ("view-refresh-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->reload_button), FALSE);
    gtk_widget_set_tooltip_text (self->reload_button, _("Reload (r)"));
    g_signal_connect (self->reload_button, "clicked", G_CALLBACK (on_reload_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->reload_button);

    self->home_button = gtk_button_new_from_icon_name ("go-home-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->home_button), FALSE);
    gtk_widget_set_tooltip_text (self->home_button, _("Home"));
    g_signal_connect (self->home_button, "clicked", G_CALLBACK (on_home_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->home_button);

    self->new_tab_button = gtk_button_new_from_icon_name ("list-add-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->new_tab_button), FALSE);
    gtk_widget_set_tooltip_text (self->new_tab_button, _("New Web Tab"));
    g_signal_connect (self->new_tab_button, "clicked", G_CALLBACK (on_new_tab_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->new_tab_button);

    self->url_entry = gtk_entry_new ();
    gtk_widget_set_hexpand (self->url_entry, TRUE);
    gtk_entry_set_placeholder_text (GTK_ENTRY (self->url_entry), _("Search Google or address…"));
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->url_entry), GTK_ENTRY_ICON_PRIMARY, "system-search-symbolic");
    gtk_entry_set_input_purpose (GTK_ENTRY (self->url_entry), GTK_INPUT_PURPOSE_URL);
    g_signal_connect (self->url_entry, "activate", G_CALLBACK (on_entry_activate), self);
    gtk_box_append (GTK_BOX (header_box), self->url_entry);

    /* Engine Selector Menu Button */
    self->engine_button = gtk_menu_button_new ();
    gtk_menu_button_set_has_frame (GTK_MENU_BUTTON (self->engine_button), FALSE);
    gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (self->engine_button), "preferences-system-symbolic");
    gtk_widget_set_tooltip_text (self->engine_button, _("Search Engine: Google, Startpage, DuckDuckGo"));

    popover = gtk_popover_new ();
    pbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start (pbox, 10);
    gtk_widget_set_margin_end (pbox, 10);
    gtk_widget_set_margin_top (pbox, 8);
    gtk_widget_set_margin_bottom (pbox, 8);

    plabel = gtk_label_new (_("Search Engine & Start Page"));
    gtk_label_set_xalign (GTK_LABEL (plabel), 0.0);
    gtk_box_append (GTK_BOX (pbox), plabel);

    self->radio_google = gtk_check_button_new_with_label ("Google");
    self->radio_startpage = gtk_check_button_new_with_label ("Startpage");
    self->radio_duckduckgo = gtk_check_button_new_with_label ("DuckDuckGo");

    gtk_check_button_set_group (GTK_CHECK_BUTTON (self->radio_startpage), GTK_CHECK_BUTTON (self->radio_google));
    gtk_check_button_set_group (GTK_CHECK_BUTTON (self->radio_duckduckgo), GTK_CHECK_BUTTON (self->radio_google));
    gtk_check_button_set_active (GTK_CHECK_BUTTON (self->radio_google), TRUE);

    g_signal_connect (self->radio_google, "toggled", G_CALLBACK (on_engine_toggled), self);
    g_signal_connect (self->radio_startpage, "toggled", G_CALLBACK (on_engine_toggled), self);
    g_signal_connect (self->radio_duckduckgo, "toggled", G_CALLBACK (on_engine_toggled), self);

    gtk_box_append (GTK_BOX (pbox), self->radio_google);
    gtk_box_append (GTK_BOX (pbox), self->radio_startpage);
    gtk_box_append (GTK_BOX (pbox), self->radio_duckduckgo);

    gtk_popover_set_child (GTK_POPOVER (popover), pbox);
    gtk_menu_button_set_popover (GTK_MENU_BUTTON (self->engine_button), popover);
    gtk_box_append (GTK_BOX (header_box), self->engine_button);

    self->close_button = gtk_button_new_from_icon_name ("window-close-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (self->close_button), FALSE);
    gtk_widget_set_tooltip_text (self->close_button, _("Close Web Sidebar"));
    g_signal_connect (self->close_button, "clicked", G_CALLBACK (on_close_clicked), self);
    gtk_box_append (GTK_BOX (header_box), self->close_button);

    gtk_box_append (GTK_BOX (self->main_box), header_box);

    /* Progress Bar */
    self->progress_bar = gtk_progress_bar_new ();
    gtk_widget_set_visible (self->progress_bar, FALSE);
    gtk_box_append (GTK_BOX (self->main_box), self->progress_bar);

    /* Multi-tab View & Bar */
    self->tab_view = adw_tab_view_new ();
    g_signal_connect (self->tab_view, "close-page", G_CALLBACK (on_tab_close_page), self);
    g_signal_connect (self->tab_view, "notify::selected-page", G_CALLBACK (on_selected_page_changed), self);

    self->tab_bar = adw_tab_bar_new ();
    adw_tab_bar_set_view (self->tab_bar, self->tab_view);
    adw_tab_bar_set_autohide (self->tab_bar, FALSE);
    gtk_box_append (GTK_BOX (self->main_box), GTK_WIDGET (self->tab_bar));

    separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append (GTK_BOX (self->main_box), separator);

    gtk_widget_set_hexpand (GTK_WIDGET (self->tab_view), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET (self->tab_view), TRUE);
    gtk_box_append (GTK_BOX (self->main_box), GTK_WIDGET (self->tab_view));

    gtk_widget_set_size_request (GTK_WIDGET (self), 380, -1);
    gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
    g_signal_connect (self, "map", G_CALLBACK (on_map), NULL);

    adw_bin_set_child (ADW_BIN (self), self->main_box);
}

GtkWidget *
nautilus_web_panel_new (void)
{
    return g_object_new (NAUTILUS_TYPE_WEB_PANEL, NULL);
}

void
nautilus_web_panel_grab_focus (NautilusWebPanel *self)
{
    g_return_if_fail (NAUTILUS_IS_WEB_PANEL (self));

    ensure_tabs (self);
    WebKitWebView *view = get_active_web_view (self);
    if (view != NULL)
    {
        gtk_widget_grab_focus (GTK_WIDGET (view));
    }
    else
    {
        gtk_widget_grab_focus (self->url_entry);
        gtk_editable_select_region (GTK_EDITABLE (self->url_entry), 0, -1);
    }
}

void
nautilus_web_panel_load_uri (NautilusWebPanel *self,
                             const char       *uri)
{
    g_return_if_fail (NAUTILUS_IS_WEB_PANEL (self));
    g_return_if_fail (uri != NULL);

    WebKitWebView *view = get_active_web_view (self);
    if (view == NULL)
    {
        add_web_tab (self, uri);
    }
    else
    {
        webkit_web_view_load_uri (view, uri);
    }
}

void
nautilus_web_panel_reset (NautilusWebPanel *self)
{
    g_return_if_fail (NAUTILUS_IS_WEB_PANEL (self));

    if (self->tab_view == NULL)
    {
        return;
    }

    /* Close and destroy all tabs */
    while (adw_tab_view_get_n_pages (self->tab_view) > 0)
    {
        AdwTabPage *page = adw_tab_view_get_nth_page (self->tab_view, 0);
        adw_tab_view_close_page (self->tab_view, page);
        adw_tab_view_close_page_finish (self->tab_view, page, TRUE);
    }

    gtk_editable_set_text (GTK_EDITABLE (self->url_entry), "");
    gtk_widget_set_sensitive (self->back_button, FALSE);
    gtk_widget_set_sensitive (self->forward_button, FALSE);
    gtk_widget_set_visible (self->progress_bar, FALSE);
}
