#ifndef __YURNAPP_H
#define __YURNAPP_H

#include <gtk/gtk.h>


#define YURN_APP_TYPE (yurn_app_get_type ())
G_DECLARE_FINAL_TYPE (YurnApp, yurn_app, YURN, APP, GtkApplication)


YurnApp                *yurn_app_new                    (void);


#endif /* __YURNAPP_H */
