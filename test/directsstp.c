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
                        printf("\nkey:%s\n", kbuf);
                        vbuf[nvbuf]='\0';
                        printf("value:%s\n", vbuf);
                        nIndex++;
                        nkbuf=0;
                        nvbuf=0;
                        flg = 0;
                        if (strstr(kbuf, ".hwnd")!=NULL){
                            hwnd = (HANDLE)atoi(vbuf);
                            printf("hwnd:%d\n", hwnd);
                        }
                    } else if (p[nIndex] == 0x01){
                        printf(" => ");
                        kbuf[nkbuf] = '\0';
                        nvbuf=0;
                        flg = 1;
                    }else if (isascii(p[nIndex])==TRUE){
                        printf("%c",p[nIndex]);
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
    return 0;                   
}
    
