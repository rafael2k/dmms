
#ifndef VOICE_H
#define VOICE_H

#include "config.h"

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "xmms/plugin.h"

extern EffectPlugin voice_ep;

void voice_about(void);

#endif
