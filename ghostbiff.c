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
#if 0
#include <gio/gio.h>
#endif
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "utils.h"
#include "alertpanel.h"
#include "foldersel.h"
#include "prefs_common.h"
#include "summaryview.h"
#include "bell_add.xpm"
#include "bell_delete.xpm"

#ifdef USE_AQUESTALK
#ifdef __MINGW32__
#define AqKanji2Koe_Create _AqKanji2Koe_Create
#define AqKanji2Koe_Convert _AqKanji2Koe_Convert
#define AqKanji2Koe_Release _AqKanji2Koe_Release
#endif
#include "AqKanji2Koe.h"
#include "AquesTalk2Da.h"
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>


#define _(String) dgettext("ghostbiff", String)
#define N_(String) gettext_noop(String)
#define gettext_noop(String) (String)

#define GHOSTBIFF "ghostbiff"
#define GHOSTBIFFRC "ghostbiffrc"

#define PLUGIN_NAME N_("Ghostbiff - mail biff plug-in")
#define PLUGIN_DESC N_("Simple biff plug-in for Sylpheed")

static SylPluginInfo info = {
  N_(PLUGIN_NAME),
  "0.2.0",
  "HAYASHI Kentaro",
  N_(PLUGIN_DESC)
};

static gchar* g_copyright = N_("Ghostbiff is distributed under GPL license.\n"
"\n"
"Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>"
"\n"
"ghostbiff contains following resource.\n"
"\n"
"Silk icon set 1.3: Copyright (C) Mark James\n"
"Licensed under a Creative Commons Attribution 2.5 License.\n"
"http://www.famfamfam.com/lab/icons/silk/\n"
"\n"
"If you have proper AquesTalk2,AqKanji2Koe license,\n"
"you can use AquesTalk2 and AqKanji2Koe as TTS engine.\n"
"ghostbiff was tested by evaluation edition, \n"
"so it may not work in official release.");

typedef struct _GhostOption GhostOption;

struct _GhostOption {
  /* General section */

  /* full path to ghostbiffrc*/
  gchar *rcpath;
  /* rcfile */
  GKeyFile *rcfile;

  /* startup check in general */
  GtkWidget *chk_startup;
  /* aquestalk check in general */
  GtkWidget *chk_aquest;

  gboolean enable_aquest;

  GtkWidget *aq_dic_entry;
  GtkWidget *aq_dic_btn;
  GtkWidget *phont_entry;
  GtkWidget *phont_btn;
  GtkWidget *phont_cmb;
  GtkWidget *new_subject_btn;
  GtkWidget *new_content_btn;
  GtkWidget *show_subject_btn;
  GtkWidget *show_content_btn;
  GtkWidget *input_entry;

#ifdef DEBUG
  /* debug inbox folder */
  GtkWidget *dfolder_entry;
#endif
};

static gboolean g_enable = FALSE;
static GhostOption g_opt;
static HANDLE g_aquestalk2da = NULL;
static HANDLE g_aqkanji2koe = NULL;
static void *g_aqkanji = NULL;
static H_AQTKDA g_aqtkda = NULL;
static gchar *g_phont = NULL;

static void exec_ghostbiff_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_messageview_show_cb(GObject *obj, MsgInfo *msginfo, gboolean all_headers);
static void exec_ghostbiff_menu_cb(void);
static void exec_ghostbiff_onoff_cb(void);
static void send_directsstp(MsgInfo *msginfo);
static void read_mail_by_aquestalk(MsgInfo *msginfo);

static void play_btn_clicked(GtkButton *button, gpointer data);
static void aq_dic_btn_clicked(GtkButton *button, gpointer data);
static void phont_btn_clicked(GtkButton *button, gpointer data);
static void phont_file_set(GtkFileChooserButton *widget, gpointer data);

#ifdef DEBUG
static void debug_folder_btn_clicked(GtkButton *button, gpointer data);
static void debug_play_btn_clicked(GtkButton *button, gpointer data);
#endif

static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
static GtkWidget *create_config_aques_page(GtkWidget *notebook, GKeyFile *pkey);
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey);

static gpointer aquestalk_thread_func(gpointer data);

#if USE_AQUESTALK
typedef int __stdcall (*AQUESTALK2DA_PLAYSYNC)(const char *koe, int iSpeed, void *phontDat);
typedef int __stdcall (*AQUESTALK2DA_PLAY)(H_AQTKDA hMe, const char *koe, int iSpeed, void *phontDat, HWND hWnd, unsigned long msg, unsigned long dwUser);
typedef void __stdcall (*AQUESTALK2DA_STOP)(H_AQTKDA hMe);
typedef int __stdcall (*AQUESTALK2DA_ISPLAY)(H_AQTKDA hMe);
typedef H_AQTKDA __stdcall (*AQUESTALK2DA_CREATE)();
typedef void __stdcall (*AQUESTALK2DA_RELEASE)(H_AQTKDA hMe);
typedef void * __stdcall (*AQKANJI2KOE_CREATE)(const char *pathDic, int *pErr);
typedef void  __stdcall (*AQKANJI2KOE_RELEASE)(void *hAqKanji2Koe);
typedef int __stdcall (*AQKANJI2KOE_CONVERT)(void *hAqKanji2Koe, const char *kanji, char *koe, int nBufKoe);
AQKANJI2KOE_CREATE proc_aqkanji_create;
AQKANJI2KOE_RELEASE proc_aqkanji_release;
AQKANJI2KOE_CONVERT proc_aqkanji_convert;
AQUESTALK2DA_CREATE proc_aqda_create;
AQUESTALK2DA_RELEASE proc_aqda_release;
AQUESTALK2DA_PLAYSYNC proc_aqda_playsync;
AQUESTALK2DA_PLAY proc_aqda_play;
AQUESTALK2DA_STOP proc_aqda_stop;
AQUESTALK2DA_ISPLAY proc_aqda_isplay;
#endif

static GtkWidget *g_plugin_on = NULL;
static GtkWidget *g_plugin_off = NULL;
static GtkWidget *g_onoff_switch = NULL;
static GtkTooltips *g_tooltip = NULL;

static GCond *g_cond =NULL;
static GMutex *g_mutex=NULL;
static GList *g_mails=NULL;
static gboolean g_aquestalk_thread = TRUE;

#if 0
static gboolean summary_select_func	(GtkTreeSelection	*treeview,
					 GtkTreeModel		*model,
					 GtkTreePath		*path,
					 gboolean		 cur_selected,
					 gpointer		 data);

static gboolean summary_key_pressed	(GtkWidget		*treeview,
					 GdkEventKey		*event,
					 SummaryView		*summaryview);
#endif

void plugin_load(void)
{
  syl_init_gettext(GHOSTBIFF, "lib/locale");
  
  debug_print(gettext("mail biff Plug-in"));
  debug_print(dgettext(GHOSTBIFF, "mail biff Plug-in"));

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Biff plugin settings [ghostbiff]"), exec_ghostbiff_menu_cb, NULL);

  g_signal_connect(syl_app_get(), "add-msg", G_CALLBACK(exec_ghostbiff_cb), NULL);

  syl_plugin_signal_connect("messageview-show", G_CALLBACK(exec_messageview_show_cb), NULL);

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

  g_opt.rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, GHOSTBIFFRC, NULL);
  g_opt.rcfile = g_key_file_new();
  if (g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
    gboolean startup=g_key_file_get_boolean (g_opt.rcfile, GHOSTBIFF, "startup", NULL);
    debug_print("startup:%s", startup ? "true" : "false");
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

  /* test dll load */
  g_aquestalk2da = LoadLibrary(L"AquesTalk2Da.dll");
  g_aqkanji2koe = LoadLibrary(L"AqKanji2Koe.dll");
  if (g_aquestalk2da != NULL && g_aqkanji2koe != NULL){
    proc_aqkanji_create =(AQKANJI2KOE_CREATE)GetProcAddress(g_aqkanji2koe, "_AqKanji2Koe_Create@8");
    if (proc_aqkanji_create==NULL){
      debug_print("GetProcAddress AqKanji2Koe_Create failed\n");
    }
    proc_aqkanji_convert =(AQKANJI2KOE_CONVERT)GetProcAddress(g_aqkanji2koe, "_AqKanji2Koe_Convert@16");
    if (proc_aqkanji_convert==NULL){
      debug_print("GetProcAddress AqKanji2Koe_Convert failed\n");
    }
    proc_aqkanji_release =(AQKANJI2KOE_RELEASE)GetProcAddress(g_aqkanji2koe, "_AqKanji2Koe_Release@4");
    if (proc_aqkanji_release==NULL){
      debug_print("GetProcAddress AqKanji2Koe_Release failed\n");
    }
    proc_aqda_playsync =(AQUESTALK2DA_PLAYSYNC)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_PlaySync");
    if (proc_aqda_playsync==NULL){
      debug_print("GetProcAddress AquesTalk2Da_PlaySync failed\n");
    }
    proc_aqda_play =(AQUESTALK2DA_PLAY)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_Play");
    if (proc_aqda_play==NULL){
      debug_print("GetProcAddress AquesTalk2Da_Play failed\n");
    }
    proc_aqda_stop =(AQUESTALK2DA_STOP)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_Stop");
    if (proc_aqda_stop==NULL){
      debug_print("GetProcAddress AquesTalk2Da_Stop failed\n");
    }
    proc_aqda_isplay =(AQUESTALK2DA_ISPLAY)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_IsPlay");
    if (proc_aqda_isplay==NULL){
      debug_print("GetProcAddress AquesTalk2Da_IsPlay failed\n");
    }
    proc_aqda_create = (AQUESTALK2DA_CREATE)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_Create");
    if (proc_aqda_create==NULL){
      debug_print("GetProcAddress AquesTalk2Da_Create failed\n");
    }
    proc_aqda_release = (AQUESTALK2DA_RELEASE)GetProcAddress(g_aquestalk2da, "AquesTalk2Da_Release");
    if (proc_aqda_release==NULL){
      debug_print("GetProcAddress AquesTalk2Da_Release failed\n");
    }

    g_aqtkda = proc_aqda_create();

    GError *gerr=NULL;
    g_thread_create(aquestalk_thread_func, g_mails, TRUE, &gerr);

  }else{
    if (g_aquestalk2da == NULL){
      debug_print("LoadLibrary AquesTalk2Da.dll failed\n");
    }
    if (g_aqkanji2koe == NULL){
      debug_print("LoadLibrary AqKanji2Koe.dll failed\n");
    }
    g_aquestalk_thread=FALSE;
  }
  g_cond = g_cond_new();
  g_mutex = g_mutex_new();


}

void plugin_unload(void)
{
  g_free(g_opt.rcpath);

  g_aquestalk_thread=FALSE;
}

SylPluginInfo *plugin_info(void)
{
  return &info;
}

gint plugin_interface_version(void)
{
  return SYL_PLUGIN_INTERFACE_VERSION;
}

static void prefs_ok_cb(GtkWidget *widget, gpointer data)
{

  g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

  gboolean status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.chk_startup));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "startup", status);
  debug_print("startup:%s\n", status ? "TRUE" : "FALSE");

  status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.chk_aquest));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "aquest", status);
  debug_print("aquest:%s\n", status ? "TRUE" : "FALSE");

#if 0
  gchar *buf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(g_opt.aq_dic_btn));
#else
  gchar *buf = gtk_entry_get_text(GTK_ENTRY(g_opt.aq_dic_entry));
#endif
  debug_print("aq_dic:%s\n", buf);
  g_key_file_set_string (g_opt.rcfile, GHOSTBIFF, "aq_dic_path", buf);

#if 0
  buf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(g_opt.phont_btn));
#else
  buf = gtk_entry_get_text(GTK_ENTRY(g_opt.phont_entry));
#endif
  debug_print("phont_path:%s\n", buf);
  g_key_file_set_string (g_opt.rcfile, GHOSTBIFF, "phont_path", buf);
  
  buf = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(g_opt.phont_cmb)->entry));
  g_key_file_set_string (g_opt.rcfile, GHOSTBIFF, "phont", buf);

  status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.new_subject_btn));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "new_subject", status);
  debug_print("new_subject:%s\n", status ? "TRUE" : "FALSE");

  status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.new_content_btn));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "new_content", status);
  debug_print("new_content:%s\n", status ? "TRUE" : "FALSE");

  status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.show_subject_btn));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "show_subject", status);
  debug_print("show_subject:%s\n", status ? "TRUE" : "FALSE");

  status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.show_content_btn));
  g_key_file_set_boolean (g_opt.rcfile, GHOSTBIFF, "show_content", status);
  debug_print("show_content:%s\n", status ? "TRUE" : "FALSE");

  /**/
  gsize sz;
  buf=g_key_file_to_data(g_opt.rcfile, &sz, NULL);
  g_file_set_contents(g_opt.rcpath, buf, sz, NULL);
    
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
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_widget_realize(window);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* notebook */ 
  GtkWidget *notebook = gtk_notebook_new();
  /* main tab */
  create_config_main_page(notebook, g_opt.rcfile);
  /* AquesTalk option tab */
  create_config_aques_page(notebook, g_opt.rcfile);
  /* about, copyright tab */
  create_config_about_page(notebook, g_opt.rcfile);

  gtk_widget_show(notebook);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

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


      
  /* load settings */
  if (g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
    gboolean status=g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "startup", NULL);
    debug_print("startup:%s\n", status ? "TRUE" : "FALSE");
    if (status!=FALSE){
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.chk_startup), TRUE);
    }

    if (g_aquestalk2da == NULL || g_aqkanji2koe == NULL){
      g_opt.enable_aquest=FALSE;
      /* disable check */
      gtk_widget_set_sensitive(GTK_WIDGET(g_opt.chk_aquest), FALSE);
    } else {
      g_opt.enable_aquest=TRUE;
    }

    status=g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "aquest", NULL);
    debug_print("aquest:%s\n", status ? "TRUE" : "FALSE");

    if (status!=FALSE){
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.chk_aquest), TRUE);
    }

    gchar *buf = g_key_file_get_string(g_opt.rcfile, GHOSTBIFF, "aq_dic_path", NULL);
    if (buf != NULL){
      gtk_entry_set_text(GTK_ENTRY(g_opt.aq_dic_entry), buf);
    }

    buf = g_key_file_get_string(g_opt.rcfile, GHOSTBIFF, "phont_path", NULL);
    if (buf != NULL){
      gtk_entry_set_text(GTK_ENTRY(g_opt.phont_entry), buf);

      GDir* gdir = g_dir_open(buf, 0, NULL);
      GList* items = NULL;
      gchar *file =NULL;
      do {
          file = g_dir_read_name(gdir);
          if (file!=NULL){
              debug_print("%s\n", file);
              items = g_list_append(items, g_strdup(file));
          }
      } while (file!=NULL);
    
      gtk_combo_set_popdown_strings(GTK_COMBO(g_opt.phont_cmb), items);
    }

    buf = g_key_file_get_string(g_opt.rcfile, GHOSTBIFF, "phont", NULL);
    if (buf != NULL){
      gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(g_opt.phont_cmb)->entry), buf);
    }

   /*    gchar *to=g_key_file_get_string (g_opt.rcfile, GHOSTBIFF, "to", NULL);
    gtk_entry_set_text(GTK_ENTRY(g_address), to);
    */
  }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

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
  debug_print("exec_ghostbiff_cb called.\n");
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

  MsgInfo *msginfo = folder_item_get_msginfo(item, num);
  
  send_directsstp(msginfo);

  debug_print("mutex_lock\n");
  g_mutex_lock(g_mutex);
  debug_print("append msginfo to list\n");
  g_mails = g_list_append(g_mails, msginfo);
  debug_print("after append stack:%d\n", g_list_length(g_mails));
  g_cond_signal(g_cond);
  g_mutex_unlock(g_mutex);
  debug_print("mutex_unlock\n");
  /*read_mail_by_aquestalk(msginfo);*/
}

static void send_directsstp(MsgInfo *msginfo)
{
  HANDLE hMutex = CreateMutex(NULL,FALSE,L"SakuraFMO");
  if ( hMutex == NULL ) {
    g_print("No SakuraFMO\n");
    return 0;
  }
  if ( WaitForSingleObject(hMutex, 1000) != WAIT_TIMEOUT ) {
    g_print("SakuraFMO\n");

    DWORD dwDesiredAccess = FILE_MAP_READ;
    BOOL bInheritHandle = TRUE;
    HANDLE hFMO = OpenFileMapping(dwDesiredAccess,
                                  bInheritHandle,
                                  L"Sakura");
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
        gchar *msg;

        msg = g_strdup_printf("SEND SSTP/1.4\r\nSender: ghostbiff\r\n"
                              "Script: \\h\\s0"
                              "Date:%s\\n"
                              "From:%s\\n"
                              "To:%s\\n"
                              "Subject:%s\\n\\e\r\n"
                              "HWND: %d\r\nCharset: UTF-8\r\n",
                              msginfo->date,
                              msginfo->from,
                              msginfo->to,
                              unmime_header(msginfo->subject),hwnd);
        
        cds.dwData = 1;
        cds.cbData = strlen(msg);
        cds.lpData = (LPVOID)msg;
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

static void read_mail_by_aquestalk(MsgInfo *msginfo)
{
  gchar *aq_dic = g_key_file_get_string(g_opt.rcfile, GHOSTBIFF, "aq_dic_path", NULL);
  debug_print("aq_dic:%s\n", aq_dic);

  gchar *phont_dic = g_key_file_get_string(g_opt.rcfile, GHOSTBIFF, "phont_path", NULL);
  debug_print("phont_btn:%s\n", phont_dic);

  int nErr=0;
  if (g_aqkanji==NULL){
    if (proc_aqkanji_create==NULL){
      debug_print("AqKanji2Koe_Create symbol not found\n");
      return;
    }

    g_aqkanji=proc_aqkanji_create(aq_dic, &nErr);
    if (g_aqkanji==NULL){
      debug_print("AqKanji2Koe_Create error\n");
      return;
    }
  }
  CodeConverter *cconv = conv_code_converter_new("UTF-8", "Shift_JIS");
  gchar *text = unmime_header(msginfo->subject);
  gchar *sjis = conv_convert(cconv, text);
  char buf[1024];
  if (proc_aqkanji_convert==NULL){
    debug_print("AqKanji2Koe_Convert symbol not found\n");
    return;
  }
  if (proc_aqkanji_convert){
    proc_aqkanji_convert(g_aqkanji, sjis, buf, 1024);
  }
  
  gchar *phont_path = g_strconcat(phont_dic, G_DIR_SEPARATOR_S, "aq_rm.phont", NULL);
  GError *perr = NULL;
  GMappedFile *gmap=NULL;
  g_print("%s\n", phont_path);
  gmap = g_mapped_file_new(phont_path, FALSE, &perr);
  if (gmap!=NULL){
    g_phont = g_mapped_file_get_contents(gmap);
  }
  proc_aqda_play(g_aqtkda, buf, 100, g_phont, 0, 0, 0);

}

static void exec_messageview_show_cb(GObject *obj, MsgInfo *msginfo, gboolean all_headers)
{
    g_print("ghostbiff messageview-show\n");
    g_print(msginfo->subject);
    g_print(msginfo->from);
    g_print(msginfo->to);
    
}

/**/
static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey)
{
  debug_print("create_config_main_page\n");
  if (notebook == NULL){
    return NULL;
  }
  /* startup */
  if (pkey!=NULL){
  }
  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
  g_opt.chk_startup = gtk_check_button_new_with_label(_("Enable plugin on startup."));
  gtk_widget_show(g_opt.chk_startup);
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.chk_startup, FALSE, FALSE, 0);

  g_opt.chk_aquest = gtk_check_button_new_with_label(_("Enable AquesTalk2,AqKanji2Koe.[Experimental]"));
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.chk_aquest, FALSE, FALSE, 0);

#ifdef DEBUG
  /* select folder and read random n mails test */
  GtkWidget *folder_btn = gtk_button_new_from_stock(GTK_STOCK_OPEN);
  g_opt.dfolder_entry = gtk_entry_new();
  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  gtk_entry_set_text(GTK_ENTRY(g_opt.dfolder_entry), "inbox");
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.dfolder_entry, TRUE, TRUE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), folder_btn, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 6);

  g_signal_connect(GTK_BUTTON(folder_btn), "clicked", G_CALLBACK(debug_folder_btn_clicked), g_opt.dfolder_entry);
  
  GtkWidget *spin_btn = gtk_spin_button_new_with_range(1, 10, 1);
  GtkWidget *play_btn = gtk_button_new_from_stock("Play");
  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), spin_btn, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), play_btn, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 6);

  g_signal_connect(GTK_BUTTON(play_btn), "clicked", G_CALLBACK(debug_play_btn_clicked), spin_btn);

#endif
  
  GtkWidget *general_lbl = gtk_label_new(_("General"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, general_lbl);
  gtk_widget_show_all(notebook);
  return NULL;
}

/* AquesTalk option tab */
static GtkWidget *create_config_aques_page(GtkWidget *notebook, GKeyFile *pkey)
{
  debug_print("create_config_aques_page\n");
  if (notebook == NULL){
    return NULL;
  }

  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
  /* set aq_dic file directory */
  GtkWidget *aq_dic_frm = gtk_frame_new(_("AquesTalk2 aq_dic directory"));
  GtkWidget *aq_dic_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(aq_dic_align), 6, 6, 6, 6);
  GtkWidget *aq_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(aq_frm_align), 6, 6, 6, 6);
#if 0
  /* 2.12 or later */
  g_opt.aq_dic_btn = gtk_file_chooser_button_new(_("Select aq_dic directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_container_add(GTK_CONTAINER(aq_dic_align), g_opt.aq_dic_btn);
#else
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  g_opt.aq_dic_entry = gtk_entry_new();
  g_opt.aq_dic_btn = gtk_button_new_from_stock(GTK_STOCK_OPEN);
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.aq_dic_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.aq_dic_btn, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(aq_dic_align), hbox);

  g_signal_connect(GTK_BUTTON(g_opt.aq_dic_btn), "clicked", G_CALLBACK(aq_dic_btn_clicked), g_opt.aq_dic_entry);
#endif
  gtk_container_add(GTK_CONTAINER(aq_dic_frm), aq_dic_align);
  gtk_container_add(GTK_CONTAINER(aq_frm_align), aq_dic_frm);
  gtk_widget_show_all(aq_frm_align);

  /* set phont file directory */
  GtkWidget *phont_frm = gtk_frame_new(_("AquesTalk2 phont directory"));
  GtkWidget *phont_btn_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(phont_btn_align), 6, 6, 6, 6);
  GtkWidget *phont_box = gtk_vbox_new(FALSE, 0);
  GtkWidget *phont_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(phont_frm_align), 6, 6, 6, 6);

#if 0
  g_opt.phont_btn = gtk_file_chooser_button_new(N_("Select phont directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_box_pack_start(GTK_BOX(phont_box), g_opt.phont_btn, FALSE, FALSE, 0);
#else
  hbox = gtk_hbox_new(FALSE, 0);
  g_opt.phont_entry = gtk_entry_new();
  g_opt.phont_btn = gtk_button_new_from_stock(GTK_STOCK_OPEN);
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.phont_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.phont_btn, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(phont_box), hbox, FALSE, FALSE, 0);

  g_signal_connect(GTK_BUTTON(g_opt.phont_btn), "clicked", G_CALLBACK(phont_btn_clicked), g_opt.phont_entry);
#endif

  /* phont selection */
  hbox = gtk_hbox_new(FALSE, 0);
  g_opt.phont_cmb = gtk_combo_new();
  GtkWidget *label = gtk_label_new(_("Phont type:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.phont_cmb, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(phont_box), hbox, FALSE, FALSE, 0);
    
  gtk_container_add(GTK_CONTAINER(phont_btn_align), phont_box);
  gtk_container_add(GTK_CONTAINER(phont_frm), phont_btn_align);
  gtk_container_add(GTK_CONTAINER(phont_frm_align), phont_frm);
  gtk_widget_show_all(phont_frm_align);
    
#if 0    
  g_signal_connect(GTK_FILE_CHOOSER(g_opt.phont_btn), "file-set", G_CALLBACK(phont_file_set), g_opt.phont_cmb);
#endif
  
  /* talk setting when you received new mail.
     read subject,body check button */
  GtkWidget *new_frm = gtk_frame_new(_("Read what when you receive mail"));
  GtkWidget *new_hbox = gtk_hbox_new(FALSE, 6);
  g_opt.new_subject_btn = gtk_check_button_new_with_label(_("subject"));
  g_opt.new_content_btn = gtk_check_button_new_with_label(_("content"));
  GtkWidget *new_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(new_frm_align), 6, 6, 6, 6);
  gtk_box_pack_start(GTK_BOX(new_hbox), g_opt.new_subject_btn, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(new_hbox), g_opt.new_content_btn, FALSE, FALSE, 6);
  gtk_container_add(GTK_CONTAINER(new_frm), new_hbox);
  gtk_container_add(GTK_CONTAINER(new_frm_align), new_frm);
  gtk_widget_show_all(new_frm_align);

  gboolean status = g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "new_subject", NULL);
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.new_subject_btn), TRUE);
  }
  status = g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "new_content", NULL);
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.new_content_btn), TRUE);
  }
    
  /* talk setting when you preview each mail.
     subject,body check button */
  GtkWidget *preview_frm = gtk_frame_new(_("Read what when you preview mail"));
  GtkWidget *pv_hbox = gtk_hbox_new(FALSE, 6);
  g_opt.show_subject_btn = gtk_check_button_new_with_label(_("subject"));
  g_opt.show_content_btn = gtk_check_button_new_with_label(_("content"));
  GtkWidget *pv_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(pv_frm_align), 6, 6, 6, 6);
  gtk_box_pack_start(GTK_BOX(pv_hbox), g_opt.show_subject_btn, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(pv_hbox), g_opt.show_content_btn, FALSE, FALSE, 6);
  gtk_container_add(GTK_CONTAINER(preview_frm), pv_hbox);
  gtk_container_add(GTK_CONTAINER(pv_frm_align), preview_frm);
  gtk_widget_show_all(pv_frm_align);

  status = g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "show_subject", NULL);
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.show_subject_btn), TRUE);
  }
  status = g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, "show_content", NULL);
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.show_content_btn), TRUE);
  }
    

  /* input sample text */
  GtkWidget *sample_frm = gtk_frame_new(_("Test sample"));
  hbox = gtk_hbox_new(FALSE, 6);
  g_opt.input_entry = gtk_entry_new();
  GtkWidget *sample_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(sample_frm_align), 6, 6, 6, 6);
  gtk_entry_set_text(GTK_ENTRY(g_opt.input_entry), _("sample text"));
  /* play sample text */
  GtkWidget *play_btn = gtk_button_new_with_label(_("Play"));
  gtk_box_pack_start(GTK_BOX(hbox), g_opt.input_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), play_btn, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(sample_frm), hbox);
  gtk_container_add(GTK_CONTAINER(sample_frm_align), sample_frm);
  gtk_widget_show_all(sample_frm_align);

  g_signal_connect(GTK_BUTTON(play_btn), "clicked",
                   G_CALLBACK(play_btn_clicked), NULL);
    
  gtk_box_pack_start(GTK_BOX(vbox), aq_frm_align, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), phont_frm_align, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), new_frm_align, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), pv_frm_align, FALSE, FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), sample_frm_align, FALSE, FALSE, 6);
    
  gtk_widget_show_all(vbox);
  GtkWidget *general_lbl = gtk_label_new(_("AquesTalk"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, general_lbl);
  gtk_widget_show_all(notebook);
  return NULL;
}

/* about, copyright tab */
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey)
{
  debug_print("create_config_about_page\n");
  if (notebook == NULL){
    return NULL;
  }
  GtkWidget *hbox = gtk_hbox_new(TRUE, 6);
  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 6);

  GtkWidget *misc = gtk_label_new("Ghostbiff");
  gtk_box_pack_start(GTK_BOX(vbox), misc, FALSE, TRUE, 6);

  misc = gtk_label_new(PLUGIN_DESC);
  gtk_box_pack_start(GTK_BOX(vbox), misc, FALSE, TRUE, 6);

  /* copyright */
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(vbox), scrolled);

  GtkTextBuffer *tbuffer = gtk_text_buffer_new(NULL);
  gtk_text_buffer_set_text(tbuffer, _(g_copyright), strlen(g_copyright));
  GtkWidget *tview = gtk_text_view_new_with_buffer(tbuffer);
  gtk_container_add(GTK_CONTAINER(scrolled), tview);
    
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 6);
    
  /**/
  GtkWidget *general_lbl = gtk_label_new(_("About"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, general_lbl);
  gtk_widget_show_all(notebook);
  return NULL;
}

static void play_btn_clicked(GtkButton *button, gpointer data)
{
#if 0
  gchar *aq_dic = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(g_opt.aq_dic_btn));
#else
  gchar *aq_dic = gtk_entry_get_text(GTK_ENTRY(g_opt.aq_dic_entry));
#endif
  debug_print("aq_dic:%s\n", aq_dic);

#if 0
  gchar *phont_dic = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(g_opt.phont_btn));
#else
  gchar *phont_dic = gtk_entry_get_text(GTK_ENTRY(g_opt.phont_entry));
#endif
  debug_print("phont_btn:%s\n", phont_dic);

  int nErr=0;
  void* aq=NULL;
  if (proc_aqkanji_create==NULL){
    debug_print("AqKanji2Koe_Create symbol not found\n");
    return;
  }

  aq=proc_aqkanji_create(aq_dic, &nErr);
  if (aq==NULL){
    debug_print("AqKanji2Koe_Create error\n");
    return;
  }

  CodeConverter *cconv = conv_code_converter_new("UTF-8", "Shift_JIS");
  gchar *text = gtk_entry_get_text(GTK_ENTRY(g_opt.input_entry));
  gchar *sjis = conv_convert(cconv, text);
  char buf[1024];

  if (proc_aqkanji_convert==NULL){
    debug_print("AqKanji2Koe_Convert symbol not found\n");
    return;
  }
  proc_aqkanji_convert(aq, sjis, buf, 1024);
  
  gchar *phont_path = g_strconcat(phont_dic, G_DIR_SEPARATOR_S, "aq_rm.phont", NULL);
  GError *perr = NULL;
  GMappedFile *gmap=NULL;
  g_print("%s\n", phont_path);
  gmap = g_mapped_file_new(phont_path, FALSE, &perr);
  if (gmap!=NULL){
    gchar *pc = g_mapped_file_get_contents(gmap);

    proc_aqda_playsync(buf, 100, pc);

    g_mapped_file_free(gmap);
  }

  if (proc_aqkanji_release==NULL){
    debug_print("AqKanji2Koe_Release symbol not found\n");
    return;
  }
  proc_aqkanji_release(aq);
    
}

/* GTK 2.12 or later */
static void phont_file_set(GtkFileChooserButton *widget, gpointer data)
{
    debug_print("phont_file_set called.");
    gchar *phont_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    /* use GIO *.phont */
    GList* items = NULL;

    items = g_list_append(items, "test");
    gtk_combo_set_popdown_strings(GTK_COMBO(data), items);
}

static void aq_dic_btn_clicked(GtkButton *button, gpointer data)
{
  GtkWidget *entry = data;
  GtkWidget *dialog = gtk_file_chooser_dialog_new(NULL, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                  GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  NULL);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

    gtk_entry_set_text(GTK_ENTRY(g_opt.aq_dic_entry), filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}
static void phont_btn_clicked(GtkButton *button, gpointer data)
{
  GtkWidget *entry = data;
  GtkWidget *dialog = gtk_file_chooser_dialog_new(NULL, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                  GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  NULL);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

    gtk_entry_set_text(GTK_ENTRY(g_opt.phont_entry), filename);

#if 0
    GFile *gfile = g_file_new_for_path(filename);
    GError *gerr=NULL;
    GFileEnumerator *gfiles=g_file_enumerate_children(gfile,
                                                      G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                      G_FILE_QUERY_INFO_NONE,
                                                      NULL,
                                                      &gerr);
    GList* items = NULL;
    do {
      GFileInfo *file = g_file_enumerator_next_file(gfiles, NULL, &gerr);
      if (file!=NULL){
        items = g_list_append(items, g_file_info_get_name(file));
      }
    } while (file!=NULL);
#else
    GDir* gdir = g_dir_open(filename, 0, NULL);
    GList* items = NULL;
    gchar *file =NULL;
    do {
      file = g_dir_read_name(gdir);
      if (file!=NULL){
        debug_print("%s\n", file);
        items = g_list_append(items, g_strdup(file));
      }
    } while (file!=NULL);
#endif
    
    gtk_combo_set_popdown_strings(GTK_COMBO(g_opt.phont_cmb), items);
    g_free (filename);
    
  }
  gtk_widget_destroy (dialog);
}

gpointer aquestalk_thread_func(gpointer data)
{
  debug_print("aquestalk_thread_func called.\n");
  GTimeVal tval;
  while(g_aquestalk_thread){
    g_get_current_time(&tval);
    /* per 10 second, */
    g_time_val_add(&tval, G_USEC_PER_SEC * 5);
    gboolean ret = g_cond_timed_wait(g_cond, g_mutex, &tval);
    if (ret != FALSE){
      /* signal */
      debug_print("signal in thread\n");
    } else {
      /* timeout */
      debug_print("timeout in thread\n");
    }
    if (proc_aqda_isplay(g_aqtkda) != 0){
      debug_print("now playing\n");
    } else {
      /* now play */
      debug_print("ready to play\n");
      g_mutex_lock(g_mutex);
      if (g_mails!=NULL && g_list_length(g_mails) > 0){
        MsgInfo *msginfo = g_list_nth_data(g_mails, 0);
        /*debug_print("nothing to play %s\n", msginfo->file_path);*/
        debug_print("before play 0 stack:%d\n", g_list_length(g_mails));
        read_mail_by_aquestalk(msginfo);
        g_mails=g_list_remove(g_mails, msginfo);
        debug_print("after play 0 stack:%d\n", g_list_length(g_mails));
      }else {
        debug_print("nothing to play\n");
      }
      g_mutex_unlock(g_mutex);
    }
  }
}

static FolderItem *d_folder = NULL;

static void debug_folder_btn_clicked(GtkButton *button, gpointer data)
{
    d_folder = syl_plugin_folder_sel(NULL, FOLDER_SEL_COPY, NULL);
    if (!d_folder || !d_folder->path){
        return;
    }
    GtkWidget *widget = data;
    gtk_entry_set_text(GTK_ENTRY(widget), d_folder->path);
}

static void debug_play_btn_clicked(GtkButton *button, gpointer data)
{
  debug_print("debug_play_btn_clicked\n");

  if (g_aquestalk2da==NULL || g_aqkanji2koe==NULL){
      return;
  }
  
  gchar *inbox = gtk_entry_get_text(GTK_ENTRY(g_opt.dfolder_entry));

    FolderItem *item = folder_find_item_from_identifier(inbox);
    
    GSList *msglist = folder_item_get_msg_list(item, TRUE);

    int nlstlen = g_slist_length(msglist);

    gint nnum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));
    
                                     /* pick up mail by random */
    GRand *grnd = g_rand_new();
    for (gint ncount = 0; ncount < nnum; ncount++){
      gint32 nindex = g_rand_int_range(grnd, 0, nlstlen-1);
      MsgInfo *msginfo = g_slist_nth_data(msglist, nindex);

      send_directsstp(msginfo);

      debug_print("mutex_lock\n");

      g_mutex_lock(g_mutex);
      debug_print("append msginfo to list\n");
      if (msginfo->file_path){
        debug_print("msginfo file_path:%s\n", msginfo->file_path);
      }
      debug_print("msginfo msgnum:%d\n", msginfo->msgnum);
      debug_print("msginfo subject:%s\n", unmime_header(msginfo->subject));

      g_mails = g_list_append(g_mails, msginfo);
      debug_print("after append stack:%d\n", g_list_length(g_mails));
      g_cond_signal(g_cond);

      g_mutex_unlock(g_mutex);
      debug_print("mutex_unlock\n");
      /*read_mail_by_aquestalk(msginfo);*/
    }
}
