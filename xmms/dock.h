#ifndef DOCK_H
#define DOCK_H

void dock_set_uposition(GtkWidget *widget, gint x, gint y);
GList *dock_add_window(GList *window_list, GtkWidget *window);
void dock_move_press(GList *window_list, GtkWidget *w, GdkEventButton *event, gboolean move_list);
void dock_move_motion(GtkWidget *w,GdkEventMotion *event);
void dock_move_release(GtkWidget *w);
void dock_get_widget_pos(GtkWidget *w, gint *x,gint *y);
gboolean dock_is_moving(GtkWidget *w);
void dock_shade(GList *window_list, GtkWidget *widget, gint new_h);
void dock_resize(GList *window_list, GtkWidget *w, gint new_w, gint new_h);

#endif
