#ifndef __YURN_APP_WIN_H
#define __YURN_APP_WIN_H

#include <gtk/gtk.h>
#include "yurnapp.h"


#define YURN_APP_WIN_TYPE (yurn_app_win_get_type ())
G_DECLARE_FINAL_TYPE (YurnAppWin, yurn_app_win, YURN, APP_WIN,
                      GtkApplicationWindow)


YurnAppWin             *yurn_app_win_new          (YurnApp *app);
void                    yurn_app_win_open         (YurnAppWin *win,
                                                   const char *file);


#endif /* __YURN_APP_WIN_H */
