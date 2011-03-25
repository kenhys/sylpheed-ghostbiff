#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <gtk/gtk.h>
#include <glib.h>

void destroy_window( GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

int main(int argc, char* argv[])
{
#if 0
    GtkWidget *window;
    gint my_timer;
    guint count = 1;
    gtk_init( &argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_title(GTK_WINDOW(window), "step3");
    /* gtk_window_set_opacity(GTK_WINDOW(window), 0.5);*/
    gtk_signal_connect( GTK_OBJECT(window),"destroy",
                        GTK_SIGNAL_FUNC(destroy_window), NULL);
    gtk_widget_show(window);
    /*my_timer = gtk_timeout_add(2000,(GtkFunction)timer_event,NULL);*/
    gtk_main();
    /*gtk_timeout_remove(my_timer);*/
 	/**/
#endif
    HANDLE hMutex = CreateMutex(NULL,FALSE,"SakuraFMO");
    if ( hMutex == NULL ) {
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
                while (nIndex < 1000){
                    if (p[nIndex] == 0x0d && p[nIndex+1] == 0x0a){
                        puts("");
                        nIndex++;
                    } else if (p[nIndex] == 0x01){
                        printf(" => ");
                    }else if (isascii(p[nIndex])==TRUE){
                        printf("%c",p[nIndex]);
                    }else {
                        g_print(" 0x%02x", p[nIndex]);
                    }
                    nIndex++;
                }

                UnmapViewOfFile(lpMap);
            }
        }
        ReleaseMutex(hMutex);
    }

    CloseHandle(hMutex);
    return 0;                   
}
