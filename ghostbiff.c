/*
 * Ghostbiff Plug-in
 *  -- simple biff program via DirectSSTP.
 * Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "defs.h"

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "utils.h"
#include "alertpanel.h"
#include "prefs_common.h"
#include "bell_add.xpm"
#include "bell_delete.xpm"


#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

#define _(String) dgettext("ghostbiff", String)
#define N_(String) gettext_noop(String)
#define gettext_noop(String) (String)

#define PLUGIN_NAME N_("Ghostbiff mail biff Plug-in")
#define PLUGIN_DESC N_("Simple biff plug-in for Sylpheed")

static SylPluginInfo info = {
	N_(PLUGIN_NAME),
	"0.1.0",
	"HAYASHI Kentaro",
	N_(PLUGIN_DESC)
};

static gboolean g_enable = FALSE;

static void exec_ghostbiff_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_ghostbiff_menu_cb(void);
static void exec_ghostbiff_onoff_cb(void);

static GtkWidget *g_plugin_on = NULL;
static GtkWidget *g_plugin_off = NULL;
static GtkWidget *g_onoff_switch = NULL;
static GtkTooltips *g_tooltip = NULL;
static GKeyFile *g_keyfile=NULL;

void plugin_load(void)
{
  syl_init_gettext("ghostbiff", "lib/locale");
  
  debug_print(gettext("mail biff Plug-in"));
  debug_print(dgettext("ghostbiff", "mail biff Plug-in"));

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Ghostbiff Settings"), exec_ghostbiff_menu_cb, NULL);

  g_signal_connect(syl_app_get(), "add-msg", G_CALLBACK(exec_ghostbiff_cb), NULL);

  GtkWidget *mainwin = syl_plugin_main_window_get();
    GtkWidget *statusbar = syl_plugin_main_window_get_statusbar();
    GtkWidget *plugin_box = gtk_hbox_new(FALSE, 0);

    GdkPixbuf* on_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)bell_add);
    g_plugin_on=gtk_image_new_from_pixbuf(on_pixbuf);
    
    GdkPixbuf* off_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)bell_delete);
    g_plugin_off=gtk_image_new_from_pixbuf(off_pixbuf);

    gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_on, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_off, FALSE, FALSE, 0);
    
    g_tooltip = gtk_tooltips_new();
    
    g_onoff_switch = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(g_onoff_switch), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS(g_onoff_switch, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(g_onoff_switch, 20, 20);

    gtk_container_add(GTK_CONTAINER(g_onoff_switch), plugin_box);
	g_signal_connect(G_OBJECT(g_onoff_switch), "clicked",
                     G_CALLBACK(exec_ghostbiff_onoff_cb), mainwin);
	gtk_box_pack_start(GTK_BOX(statusbar), g_onoff_switch, FALSE, FALSE, 0);

    gtk_widget_show_all(g_onoff_switch);
    gtk_widget_hide(g_plugin_on);
    info.name = g_strdup(_(PLUGIN_NAME));
    info.description = g_strdup(_(PLUGIN_DESC));

	gchar *rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "ghostbiffrc", NULL);
    g_keyfile = g_key_file_new();
    if (g_key_file_load_from_file(g_keyfile, rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
        gboolean startup=g_key_file_get_boolean (g_keyfile, "ghostbiff", "startup", NULL);
        debug_print("startup:%s", startup ? "true" : "false");
        g_free(rcpath);
        if (startup){
            g_enable=TRUE;
            gtk_widget_hide(g_plugin_off);
            gtk_widget_show(g_plugin_on);
            gtk_tooltips_set_tip
                (g_tooltip, g_onoff_switch,
                 _("Ghostbiff is enabled. Click the icon to disable plugin."),
                 NULL);
        }
    }
}

void plugin_unload(void)
{
}

SylPluginInfo *plugin_info(void)
{
	return &info;
}

gint plugin_interface_version(void)
{
	return SYL_PLUGIN_INTERFACE_VERSION;
}

static GtkWidget *g_address;
static GtkWidget *g_startup;

static void prefs_ok_cb(GtkWidget *widget, gpointer data)
{
	gchar *rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "ghostbiffrc", NULL);
    g_keyfile = g_key_file_new();
    g_key_file_load_from_file(g_keyfile, rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

    gboolean startup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_startup));
    g_key_file_set_boolean (g_keyfile, "ghostbiff", "startup", startup);
    debug_print("startup:%d\n", startup);

    /**/
    gsize sz;
    gchar *buf=g_key_file_to_data(g_keyfile, &sz, NULL);
    g_file_set_contents(rcpath, buf, sz, NULL);
    
	g_free(rcpath);

    gtk_widget_destroy(GTK_WIDGET(data));
}
static void prefs_cancel_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void exec_ghostbiff_menu_cb(void)
{
    /* show modal dialog */
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *confirm_area;
    GtkWidget *ok_btn;
    GtkWidget *cancel_btn;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);
	/*gtk_widget_set_size_request(window, 200, 100);*/
	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
	gtk_widget_realize(window);

    vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	confirm_area = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_area), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(confirm_area), 6);


    ok_btn = gtk_button_new_from_stock(GTK_STOCK_OK);
    GTK_WIDGET_SET_FLAGS(ok_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(confirm_area), ok_btn, FALSE, FALSE, 0);
    gtk_widget_show(ok_btn);

    cancel_btn = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    GTK_WIDGET_SET_FLAGS(cancel_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(confirm_area), cancel_btn, FALSE, FALSE, 0);
    gtk_widget_show(cancel_btn);

    gtk_widget_show(confirm_area);
	
    gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_btn);

    gtk_window_set_title(GTK_WINDOW(window), _("Ghostbiff Settings"));

    g_signal_connect(G_OBJECT(ok_btn), "clicked",
                     G_CALLBACK(prefs_ok_cb), window);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
                     G_CALLBACK(prefs_cancel_cb), window);

	/* startup settings */
	g_startup = gtk_check_button_new_with_label(_("Enable plugin on startup."));
	gtk_widget_show(g_startup);
	gtk_box_pack_start(GTK_BOX(vbox), g_startup, FALSE, FALSE, 0);

    /* load settings */
    gchar *rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "ghostbiffrc", NULL);
    g_keyfile = g_key_file_new();
    if (g_key_file_load_from_file(g_keyfile, rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
        gchar *startup=g_key_file_get_string (g_keyfile, "ghostbiff", "startup", NULL);
        debug_print("startup:%s", startup);
        if (strcmp(startup, "true")==0){
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_startup), TRUE);
        }
        gchar *to=g_key_file_get_string (g_keyfile, "ghostbiff", "to", NULL);
        gtk_entry_set_text(GTK_ENTRY(g_address), to);
    }
    g_free(rcpath);

    gtk_widget_show(window);
}


static void exec_ghostbiff_onoff_cb(void)
{

    if (g_enable != TRUE){
        syl_plugin_alertpanel_message(_("Ghostbiff"), _("ghostbiff plugin is enabled."), ALERT_NOTICE);
        g_enable=TRUE;
        gtk_widget_hide(g_plugin_off);
        gtk_widget_show(g_plugin_on);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Ghostbiff is enabled. Click the icon to disable plugin."),
			 NULL);
    }else{
        syl_plugin_alertpanel_message(_("Ghostbiff"), _("ghostbiff plugin is disabled."), ALERT_NOTICE);
        g_enable=FALSE;
        gtk_widget_hide(g_plugin_on);
        gtk_widget_show(g_plugin_off);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Ghostbiff is disabled. Click the icon to enable plugin."),
			 NULL);
    }
}

void exec_ghostbiff_cb(GObject *obj, FolderItem *item, const gchar *file, guint num)
{
    if (g_enable!=TRUE){
        return;
    }
    if (item->stype != F_NORMAL && item->stype != F_INBOX){
        return;
    }

    PrefsCommon *prefs_common = prefs_common_get();
    if (prefs_common->online_mode != TRUE){
        return;
    }
    
    PrefsAccount *ac = (PrefsAccount*)account_get_default();
    g_return_if_fail(ac != NULL);


     HANDLE hMutex = CreateMutex(NULL,FALSE,"SakuraFMO");
    if ( hMutex == NULL ) {
        g_print("No SakuraFMO\n");
        return 0;
    }
    if ( WaitForSingleObject(hMutex, 1000) != WAIT_TIMEOUT ) {
        g_print("SakuraFMO\n");

        DWORD dwDesiredAccess = FILE_MAP_READ;
        BOOL bInheritHandle = TRUE;
        LPCTSTR lpName;
        HANDLE hFMO = OpenFileMapping(dwDesiredAccess,
                                      bInheritHandle,
                                      "Sakura");
        HANDLE hwnd;
        if (hFMO != NULL){
            DWORD dwFileOffsetHigh = 0;
            DWORD dwFileOffsetLow = 0;
            /* map all */
            DWORD dwNumberOfBytesToMap = 0;
            LPVOID lpBaseAddress = NULL;
            LPVOID lpMap = MapViewOfFileEx(hFMO,
                                           dwDesiredAccess,
                                           dwFileOffsetHigh,
                                           dwFileOffsetLow,
                                           dwNumberOfBytesToMap,
                                           lpBaseAddress);
            if (lpMap != NULL) {
                unsigned char *p = (unsigned char*)lpMap;
                DWORD *lp = (DWORD*)lpMap;
                int nIndex = 0;
                g_print("%p 0x%04x %ld\n", lp, lp[0], lp[0]);
                nIndex = 4;
                unsigned char kbuf[1024];
                unsigned char vbuf[1024];
                int nkbuf=-1;
                int nvbuf=-1;
                int flg = 0;
                while (nIndex < 1000){
                    if (p[nIndex] == 0x0d && p[nIndex+1] == 0x0a){
                        g_print("\nkey:%s\n", kbuf);
                        vbuf[nvbuf]='\0';
                        g_print("value:%s\n", vbuf);
                        nIndex++;
                        nkbuf=0;
                        nvbuf=0;
                        flg = 0;
                        if (strstr(kbuf, ".hwnd")!=NULL){
                            hwnd = (HANDLE)atoi(vbuf);
                            g_print("hwnd:%d\n", hwnd);
                        }
                    } else if (p[nIndex] == 0x01){
                        g_print(" => ");
                        kbuf[nkbuf] = '\0';
                        nvbuf=0;
                        flg = 1;
                    }else if (isascii(p[nIndex])==TRUE){
                        g_print("%c",p[nIndex]);
                        if (flg){
                            vbuf[nvbuf++] = p[nIndex];
                        } else {
                            kbuf[nkbuf++] = p[nIndex];
                        }
                    }else {
                        g_print(" 0x%02x", p[nIndex]);
                    }
                    nIndex++;
                }

                COPYDATASTRUCT cds;
                CHAR lpszData[1024];

                wsprintf(lpszData, "SEND SSTP/1.4\r\nSender: ghostbiff\r\nScript: \\h\\s0 HWND:%d\\e\r\nHWnd: %dCharset: Shift_JISThis is DATA.\r\n", hwnd, hwnd);
                
                cds.dwData = 1;
                cds.cbData = sizeof(lpszData);
                cds.lpData = (LPVOID)lpszData;
                WPARAM wParam = (WPARAM)hwnd;/*hwnd;*/
                LPARAM lParam = (LPARAM)&cds;

                SendMessage((HANDLE)hwnd, WM_COPYDATA, wParam, lParam);
                UnmapViewOfFile(lpMap);
            }
        }
        ReleaseMutex(hMutex);
    }

    CloseHandle(hMutex);

}
