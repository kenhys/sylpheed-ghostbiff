#ifndef _GHOSTBIFF_H_INCLUDED_
#define _GHOSTBIFF_H_INCLUDED_

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "account.h"
#include "utils.h"
#include "unmime.h"
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
  gboolean flg_new_subject;
  gboolean flg_new_content;
  gboolean flg_show_subject;
  gboolean flg_show_content;
  GtkWidget *input_entry;

#ifdef DEBUG
  /* debug inbox folder */
  GtkWidget *dfolder_entry;
#endif
};

static void exec_ghostbiff_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_messageview_show_cb(GObject *obj, gpointer msgview, MsgInfo *msginfo, gboolean all_headers);
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

static void summaryview_menu_popup_cb(GObject *obj, GtkItemFactory *ifactory,
				      gpointer data);

static void menu_selected_cb(void);
static void textview_menu_popup_cb(GObject *obj, GtkMenu *menu,
				   GtkTextView *textview,
				   const gchar *uri,
				   const gchar *selected_text,
				   MsgInfo *msginfo);

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

#define GET_RC_BOOLEAN(keyarg) g_key_file_get_boolean(g_opt.rcfile, GHOSTBIFF, keyarg, NULL)
#define SET_RC_BOOLEAN(keyarg,valarg) g_key_file_set_boolean(g_opt.rcfile, GHOSTBIFF, keyarg, valarg)

#endif
