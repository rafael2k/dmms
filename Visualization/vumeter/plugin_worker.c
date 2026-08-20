#include <gtk/gtk.h>
#include <unistd.h>
#include <sched.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "config.h"

#include "plugin_main.h"

extern gint	worker_can_quit,
		num_of_samples,
		num_of_windows,
		worker_state,
		decay_pct,
		target_fps,

		devmode_enabled;

extern float	devmode_left_value,
		devmode_right_value;

extern vumeter_window  plugin_win[MAX_INSTANCES];

extern gint16	shared_data[2][512];

float		audio_rms[2][10],
		audio_peak[2][10],
		rms_values[3],
		peak_values[3];

/************************************************************
 * Shift + store values
 ************************************************************/
void vumeter_worker_helper_1(float lrms,float rrms,float lpeak,float rpeak)
{
	gint l1,l2;
	float tmp_val[4];

	// Shift values
	for(l1=9; l1>=1; l1--)
	{
		audio_rms[0][l1]=audio_rms[0][l1-1];
		audio_rms[1][l1]=audio_rms[1][l1-1];

		audio_peak[0][l1]=audio_peak[0][l1-1];
		audio_peak[1][l1]=audio_peak[1][l1-1];
	}

	// Store values
	if(lrms<-93.0)	audio_rms[0][0]=-93.0;
	else		audio_rms[0][0]=lrms;

	if(rrms<-93.0)	audio_rms[1][0]=-93.0;
	else		audio_rms[1][0]=rrms;

	if(lpeak<-93.0)	audio_peak[0][0]=-93.0;
	else		audio_peak[0][0]=lpeak;

	if(rpeak<-93.0)	audio_peak[1][0]=-93.0;
	else		audio_peak[1][0]=rpeak;

	// Calculate averages
	l2=num_of_samples;

	for(l1=0; l1<4; l1++)
		tmp_val[l1]=0.0;

	for(l1=0; l1<l2; l1++)
	{
		tmp_val[0]+=audio_rms[0][l1];
		tmp_val[1]+=audio_rms[1][l1];
		tmp_val[2]+=audio_peak[0][l1];
		tmp_val[3]+=audio_peak[1][l1];
	}

	for(l1=0; l1<4; l1++)
		tmp_val[l1]/=(float)l2;

	// Store new values
	rms_values[0]=(tmp_val[0]+tmp_val[1])/2.0;
	rms_values[1]=tmp_val[0];
	rms_values[2]=tmp_val[1];

	peak_values[0]=(tmp_val[2]+tmp_val[3])/2.0;
	peak_values[1]=tmp_val[2];
	peak_values[2]=tmp_val[3];
}

/************************************************************
 * Worker thread
 ************************************************************/
void *vumeter_worker (void *ptr)
{
	GTimer 	*timer1,
		*timer2,
		*timer3;
	gint	timer1_value;
	float	chandata[2],
		chanpeak[2],
		tmp2;
	int	l1,
		redraw;

	// Reset values
	for(l1=0; l1<10; l1++)
	{
		audio_rms[0][l1]=-93.0;
		audio_rms[1][l1]=-93.0;
		audio_peak[0][l1]=-93.0;
		audio_peak[1][l1]=-93.0;
	}

	for(l1=0; l1<3; l1++)
	{
		peak_values[l1]=-93.0;
		rms_values[l1]=-93.0;
	}

	// Start timers
	timer1 = g_timer_new();
	timer2 = g_timer_new();
	timer3 = g_timer_new();

	// Worker thread
	while(worker_can_quit==0)
	{
		// zzzZZzzZZzzz
		g_timer_start(timer1);
		sched_yield();
		timer1_value = floor(g_timer_elapsed(timer1,NULL)*1000000.0);

		if(timer1_value<10000 && timer1_value>=0)
			usleep(10000-timer1_value);

		// Quit worker ?
		if(worker_can_quit == 1)
		{
			pthread_exit(NULL);
		}

		// Continue sleeping, if no new packets have arrived
		if(worker_state==0)
		{
			// Decay values every 100ms
			if( floor(g_timer_elapsed(timer3,NULL)*1000.0) > 100 )
			{
				chandata[0]=-93;
				chandata[1]=-93;
				chanpeak[0]=-93;
				chanpeak[1]=-93;
				redraw = 0;

				if(audio_rms[0][0]>-93)
				{
					redraw=1;
					chandata[0]=audio_rms[0][0]+audio_rms[0][0]*((float)decay_pct/100.0);
				}

				if(audio_rms[1][0]>-93)
				{
					redraw=1;
					chandata[1]=audio_rms[1][0]+audio_rms[1][0]*((float)decay_pct/100.0);
				}

				if(audio_peak[0][0]>-93)
				{
					redraw=1;
					chanpeak[0]=audio_peak[0][0]+audio_peak[0][0]*((float)decay_pct/100.0);
				}

				if(audio_peak[1][0]>-93)
				{
					redraw=1;
					chanpeak[1]=audio_peak[1][0]+audio_peak[1][0]*((float)decay_pct/100.0);
				}

				if(devmode_enabled == 1)
				{
					chandata[0] = chanpeak[0] = devmode_left_value;
					chandata[1] = chanpeak[1] = devmode_right_value;
				}

				vumeter_worker_helper_1(chandata[0],chandata[1],chanpeak[0],chanpeak[1]);

				if(redraw==1)
				{
					GDK_THREADS_ENTER();
					g_timer_start(timer2);
					for(l1=0; l1<MAX_INSTANCES; l1++)
					if(plugin_win[l1].win!=NULL)
					{
						vumeter_redraw_window(&plugin_win[l1]);
					}
					GDK_THREADS_LEAVE();
				}
			}
			continue;
		}

		// Calculate RMS values for both channels
		// [ http://en.wikipedia.org/wiki/Root_mean_square ]
		chandata[0]=0.0;
		chandata[1]=0.0;
		chanpeak[0]=0;
		chanpeak[1]=0;

		for(l1=0; l1<512; l1++)
		{
			// Calc RMS
			if( (l1%2)==0 )
			{
				chandata[0]+=pow( (float)shared_data[0][l1], 2 );
				chandata[1]+=pow( (float)shared_data[1][l1], 2 );
			}

			// Do not process first & last sample
			if(l1==0 || l1==511) continue;

			// Find peaks
			tmp2 =	(float)abs(shared_data[0][l1-1])+
				(float)abs(shared_data[0][l1])+
				(float)abs(shared_data[0][l1+1]);

			if( (tmp2/3.0) > chanpeak[0]) chanpeak[0]=(tmp2/3.0);

			tmp2 =	(float)abs(shared_data[1][l1-1])+
				(float)abs(shared_data[1][l1])+
				(float)abs(shared_data[1][l1+1]);
			if( (tmp2/3.0) > chanpeak[1]) chanpeak[1]=(tmp2/3.0);

		}

		chandata[0]=sqrt(chandata[0]/256.0);
		chandata[1]=sqrt(chandata[1]/256.0);

		// Calculate dBFS values for both channels
		// [ http://www.kgw.tu-berlin.de/~lac2007/papers/lac07_cabrera.pdf ]
		chandata[0]=20*log10(chandata[0]/32767.0);
		chandata[1]=20*log10(chandata[1]/32767.0);

		chanpeak[0]=20*log10(chanpeak[0]/32767.0);
		chanpeak[1]=20*log10(chanpeak[1]/32767.0);

		if(chanpeak[0]>=-0.1) chanpeak[0]=-0.1;
		if(chanpeak[1]>=-0.1) chanpeak[1]=-0.1;
		if(chandata[0]>=-0.1) chandata[0]=-0.1;
		if(chandata[1]>=-0.1) chandata[1]=-0.1;

		// Decay values (if needed)
		if(chandata[0]<audio_rms[0][0])
		{
			tmp2=audio_rms[0][0]+audio_rms[0][0]*((float)decay_pct/100.0);
			if(tmp2 > chandata[0]) chandata[0] = tmp2;
		}
		if(chandata[1]<audio_rms[1][0])
		{
			tmp2=audio_rms[1][0]+audio_rms[1][0]*((float)decay_pct/100.0);
			if(tmp2 > chandata[1]) chandata[1] = tmp2;
		}
		if(chanpeak[0]<audio_peak[0][0])
		{
			tmp2=audio_peak[0][0]+audio_peak[0][0]*((float)decay_pct/100.0);
			if(tmp2 > chanpeak[0]) chanpeak[0] = tmp2;
		}
		if(chanpeak[1]<audio_peak[1][0])
		{
			tmp2=audio_peak[1][0]+audio_peak[1][0]*((float)decay_pct/100.0);
			if(tmp2 > chanpeak[1]) chanpeak[1] = tmp2;
		}

		//
		if(devmode_enabled == 1)
		{
			chandata[0] = chanpeak[0] = devmode_left_value;
			chandata[1] = chanpeak[1] = devmode_right_value;
		}

		// Store values
		vumeter_worker_helper_1(chandata[0],chandata[1],chanpeak[0],chanpeak[1]);

		// Redraw? (40ms delay ~= 25fps)
		if( floor(g_timer_elapsed(timer2,NULL)*1000.0) > (1000/target_fps) )
		{
			if(worker_can_quit == 1)
			{
				pthread_exit(NULL);
			}

			GDK_THREADS_ENTER();
			g_timer_start(timer2);
			for(l1=0; l1<MAX_INSTANCES; l1++)
			if(plugin_win[l1].win!=NULL)
			{
				vumeter_redraw_window(&plugin_win[l1]);
			}
			GDK_THREADS_LEAVE();
		}

		// Ready for new pcm packet
		worker_state=0;
	}

	g_timer_destroy(timer1);
	g_timer_destroy(timer2);
	g_timer_destroy(timer3);

	return(NULL);
}
