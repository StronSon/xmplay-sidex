/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002-2006  Felipe Rivera <liebremx at users sourceforge net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: iir_fpu.c,v 1.4 2006/01/15 00:26:32 liebremx Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "iir.h"
#include "eq1.h"
#include "iir_fpu.h"
#include <math.h>
static sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS];
static sXYData data_history2[EQ_MAX_BANDS][EQ_CHANNELS];
float gain[EQ_MAX_BANDS][EQ_CHANNELS];
float gain_raw_value[EQ_MAX_BANDS][EQ_CHANNELS];
/* random noise */
sample_t dither[256];
int di;
//extern int agc_maxslider;
extern int allowagcII;




/* Volume gain 
* values should be between 0.0 and 1.0 
* Use the preamp from XMMS for now
* */


#ifdef BENCHMARK
#include "benchmark.h"
double timex = 0.0;
int count = 0;
unsigned int blength = 0;
#endif

/* 
* Global vars
*/
sIIRCoefficients *g_iir_cf;
float             g_preamp[EQ_CHANNELS];
int               g_rate;
int               g_band_count;

int    get_eq_band_count()
{
	return g_band_count;
}
void set_preamp(int chn, double val)
{
	g_preamp[chn] = (float)val;
}

/* Init the filters */
void init_iir(int nBand)
{
	for(int iBand = 0 ; iBand < EQ_MAX_BANDS ; iBand ++)
	{
		set_eq_value(20.0f , -iBand , 0) ;
		set_eq_value(20.0f , -iBand , 1) ;

		set_eq_value(0.0f , iBand , 0) ;
		set_eq_value(0.0f , iBand , 1) ;
	}
	calc_coeffs();
	/* XXX: Take all the eqcfg. stuff out of this file */
	g_band_count = nBand;
	g_rate = 48000;
	g_iir_cf = get_coeffs(&g_band_count, g_rate, true);
	clean_history();
}


void set_gain(int index, int chn, double val, float rawValue)
{
  gain[index][chn] = (float)val;
  gain_raw_value[index][chn] = rawValue;
}

float  get_eq_value(int index , int chn)
{
	return  gain_raw_value[index][chn] ;
}


float rand_FloatRange(float a, float b)
{

	return ((b-a)*((float)rand()/RAND_MAX))+a;

}

void clean_history()
{
  int n;
  /* Zero the history arrays */
  memset(data_history , 0,  sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
  memset(data_history2, 0,  sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
  /* this is only needed if we use fpu code and there's no other place for
  the moment to init the dither array*/
  for (n = 0; n < 256; n++) 
  {
      dither[n] = (rand_FloatRange(0.1,0.2));
  }
  di = 0;
}


//agc
double eq_agc[2] = { 1, 1 }; // reset in function static DWORD WINAPI SIDex_Open(const char *filename, XMPFILE file) on start of new tune
double eq_agc_maxsample[2] = { 0 , 0 };
float eq_agc_attack[2] = { 0.02 , 0.02 };
float eq_agc_decay[2] = { 0.015 , 0.015 };
float eq_agc_dbmaximum[2] = { 20,20 };
float eq_agc_dbminimum[2] = { 0,0 };
float eq_agc_hr[4] = { 0.6*32768. , 0.6*32768. , 0.6*32768. ,0.6*32768. }; // head room
float eq_agc_distortion[4] = { 32768 - eq_agc_hr[0],32768 - eq_agc_hr[1] ,32768 - eq_agc_hr[2] ,32768 - eq_agc_hr[3] };
float eq_agc_limitsmaxsamplevalue[2] = { eq_agc_distortion[0], eq_agc_distortion[1] };
float eq_agc_limitsminsamplevalue[2] = { eq_agc_distortion[2] ,eq_agc_distortion[3] };
int eq_agc_timervaluereset[2] = { 44100 * 0.015, 44100 * 0.015 };
float eq_agc_maxslider = 0.9;

double eq_simpleagc(double sample, int ch)
{



	eq_agc_limitsmaxsamplevalue[ch] = eq_agc_maxslider;
	eq_agc_limitsminsamplevalue[ch] = eq_agc_maxslider / 2;
	eq_agc_limitsmaxsamplevalue[ch] = eq_agc_maxslider;
	eq_agc_limitsminsamplevalue[ch] = eq_agc_maxslider / 2;

	sample = fabs(sample*eq_agc[ch]);

	if (sample > eq_agc_maxsample[ch])
		eq_agc_maxsample[ch] = sample;

	static int timer[2] = { eq_agc_timervaluereset[ch],eq_agc_timervaluereset[ch] };

	if (timer[ch]-- <= 0)
	{
		timer[ch] = eq_agc_timervaluereset[ch];
		// auto agc
		//if (eq_agc_maxsample[ch] > 32767/2) eq_agc_maxslider -= 0.0011;
		//if (eq_agc_maxsample[ch] < 32767/3) eq_agc_maxslider += 0.001;

		//if (eq_agc_maxsample[ch] < eq_agc_limitsminsamplevalue[ch])
		//	eq_agc[ch] += eq_agc_attack[ch];
		if (eq_agc_maxsample[ch] > eq_agc_limitsmaxsamplevalue[ch])
			eq_agc[ch] -= eq_agc_decay[ch];

		if (eq_agc[ch] > eq_agc_dbmaximum[ch]) eq_agc[ch] = eq_agc_dbmaximum[ch];
		if (eq_agc[ch] < eq_agc_dbminimum[ch]) eq_agc[ch] = eq_agc_dbminimum[ch];
		eq_agc_maxsample[ch] = 0;

		//printf("[DB @ %.2lf 0 = MAX (%.4lf dB) >> %.2i\n",decibel,dbmax,intensify);
	}
	return eq_agc[ch];
}


int iir(float * d, int length, int srate, int nch)
{
//  FTZ_ON;
   float *data = (float*)  d;
  /* Indexes for the history arrays
   * These have to be kept between calls to this function
   * hence they are static */
  static int i = 2, j = 1, k = 0;	

  int index, band, channel;
  double tempgint, halflength;
  sample_t out[EQ_CHANNELS], pcm[EQ_CHANNELS];

  // Load the correct filter table according to the sampling rate if needed
  if (srate != g_rate)
  {
    g_rate = srate;
    g_iir_cf = get_coeffs(&g_band_count, g_rate, false);
    clean_history();
  }

  /**
   * IIR filter equation is
   * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
   *
   * NOTE: The 2 factor was introduced in the coefficients to save
   * 			a multiplication
   *
   * This algorithm cascades two filters to get nice filtering
   * at the expense of extra CPU cycles
   */
  /* 16bit, 2 bytes per sample, so divide by two the length of
   * the buffer (length is in bytes)
   */
  halflength = (length >> 1);
  for (index = 0; index < halflength; index+=nch)
  {
    /* For each channel */
    for (channel = 0; channel < nch; channel++)
    {
      pcm[channel] = data[index+channel];
      /* Preamp gain */
	  pcm[channel] *= (g_preamp[channel]);

      
      /* add random noise */
      //pcm[channel] += dither[di]*0.25;

      out[channel] = 0.;
      /* For each band */
      for (band = 0; band < g_band_count; band++)
      {
        /* Store Xi(n) */
        data_history[band][channel].x[i] = pcm[channel];
        /* Calculate and store Yi(n) */
        data_history[band][channel].y[i] = 
          (
           /* 		= alpha * [x(n)-x(n-2)] */
           g_iir_cf[band].alpha * ( data_history[band][channel].x[i]   -  data_history[band][channel].x[k])
           /* 		+ gamma * y(n-1) */                                 /* 		- beta * y(n-2) */
           + g_iir_cf[band].gamma * data_history[band][channel].y[j]   - g_iir_cf[band].beta * data_history[band][channel].y[k]
       
       
          );
        /* 
         * The multiplication by 2.0 was 'moved' into the coefficients to save
         * CPU cycles here */
        /* Apply the gain  */
        out[channel] +=  data_history[band][channel].y[i]*gain[band][channel]; // * 2.0;
      } /* For each band */

      if (EXTRA_FILTER)
      {
        /* Filter the sample again */
        for (band = 0; band < g_band_count; band++)
        {
          /* Store Xi(n) */
          data_history2[band][channel].x[i] = out[channel];
          /* Calculate and store Yi(n) */
          data_history2[band][channel].y[i] = 
            (
             /* y(n) = alpha * [x(n)-x(n-2)] */
             g_iir_cf[band].alpha * (data_history2[band][channel].x[i]  -  data_history2[band][channel].x[k])
             /* 	    + gamma * y(n-1) */                               /* 		- beta * y(n-2) */ 
             + g_iir_cf[band].gamma * data_history2[band][channel].y[j] - g_iir_cf[band].beta * data_history2[band][channel].y[k]
             
             
            );
          /* Apply the gain */
          //out[channel] +=  data_history2[band][channel].y[i]*(gain[band][channel]+(channel*0.002));
		  out[channel] += data_history[band][channel].y[i] * gain[band][channel]; // * 2.0;
        } /* For each band */
      }

      /* Volume stuff
         Scale down original PCM sample and add it to the filters 
         output. This substitutes the multiplication by 0.25
         Go back to use the floating point multiplication before the
         conversion to give more dynamic range
         */
	 out[channel] += pcm[channel]*0.25;
	 out[channel] *= 1+(dither[di] * 0.25);
// 	 if (GetAsyncKeyState(VK_LSHIFT) & 1)
// 	 {
// 		 char dbug[100];
// 		 sprintf(dbug, "%f", g_preamp[channel]);
// 		 MessageBoxA(0, dbug, "", 0);
// 	 }
	 out[channel] *= fabs(g_preamp[channel])*(5- g_preamp[channel]);
	 /* remove random noise */
	 //out[channel] -= dither[di]*0.25;


	 // auto agc
	 //if(allowagcII)
	 //out[channel] *= eq_simpleagc(out[channel] * eq_agc[channel], channel);

	 
	 data[index + channel] = out[channel] > 1.0 ? 1.0 : (out[channel] < -1.0 ? -1.0 : out[channel]);
      


      /* Round and convert to integer */
#ifdef ARCH_PPC
      tempgint = round_ppc(out[channel]);
#else
#ifdef ARCH_X86
      tempgint = round_trick(out[channel]);
#else
      tempgint = (double)out[channel];
#endif
#endif

      /* Limit the output */
	 /*
      if (tempgint < -32768)
        data[index+channel] = -32768;
      else if (tempgint > 32767)
        data[index+channel] = 32767;
      else 
	 */
// 	 float LIMIT=14;
// 	 if (tempgint>LIMIT) {tempgint=LIMIT;}
// 	 if (tempgint<-LIMIT) {tempgint=-LIMIT;}
//         data[index+channel] = tempgint;
    } /* For each channel */
    
    /* Wrap around the indexes */
    i = (i+1)%3;
    j = (j+1)%3;
    k = (k+1)%3;
    /* random noise index */
    di = (di + 1) % 256;

  }/* For each pair of samples */

//  FTZ_OFF;
  return length;
}
