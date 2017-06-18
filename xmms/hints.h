#ifndef HINTS_H
#define HINTS_H

/* Window Managers */
#define WM_HINTS_NONE				0
#define WM_HINTS_GNOME				1
#define WM_HINTS_NET				2

void check_wm_hints(void);
void hint_set_always(gboolean always);
void hint_set_skip_winlist(GtkWidget *window);
void hint_set_sticky(gboolean sticky);
gboolean hint_always_on_top_available(void);
gboolean hint_move_resize_available(void);
void hint_move_resize(GtkWidget *window, int x, int y, gboolean move);

#endif
