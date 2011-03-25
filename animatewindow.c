#include <stdio.h>

#include <gtk/gtk.h>

void destroy_window( GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

int main(int argc, char* argv[])
{
    GtkWidget *window;
    gint my_timer;
    guint count = 1;
    gtk_init( &argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_title(GTK_WINDOW(window), "step3");
    gtk_window_set_opacity(GTK_WINDOW(window), 0.5);
    gtk_signal_connect( GTK_OBJECT(window),"destroy",
                        GTK_SIGNAL_FUNC(destroy_window), NULL);
    gtk_widget_show(window);
    /*my_timer = gtk_timeout_add(2000,(GtkFunction)timer_event,NULL);*/
    gtk_main();
    /*gtk_timeout_remove(my_timer);*/
 	
    return 0;                   
}
