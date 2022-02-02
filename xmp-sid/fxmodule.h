////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: sort this test mess out.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define testing
#ifdef testing
#include <windows.h>
#endif
float rand_FloatRangeA(float a, float b)
{
	return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}
int Bass_test_wgc = 1;
int Bass_test_wgctwo = 1;
int Extras = 1;
float surrounddelay_float = 0.015;
float old_surrounddelay_float = ((((int)(surrounddelay_float * 44100)) & 0xFFFFFFFE))/2;
int chips = 0;//remnant's of old sid player , remember to set if sids are actually 2 , 3 etc
int SURROUNDEMP = 0;
int SURROUNDSEPERATION = 0; // allow 2 sids for mono
int SURROUNDSEPFORCE = 0; // forces second sid chip with differences
int BANDEQ = 0;// ENABLES A 10 BAND EQ // MAKES IT SOUNDS LIKE 1 OF MY REAL C64 SID CHIPS DIDNT CHECK THE OTHERS BUT IT SOUNDS LIKE THE ONE I USE FOR TESTING
int HPFILTER = 70;
int LPFILTER = 0;// 100 - 70;
int newtunehasbeenstarted = 0;
int activatefake2sid = 0;
int surroundcount = 0;
int timedsurround = 0;
int PREAMP = 65;
int VOICE_1SID[ 4] = { 0,0,0,0 };
int VOICE_2SID[ 4] = { 0,0,0,0 };
int VOICE_3SID[ 4] = { 0,0,0,0 };
float  fadevolume = 0;
int _8580VOLUMEBOOST = 0;
int surround = 0;
int userstoredvoiceconfig = 0;
float _decode_buffer[44100];
float SDELAY = 0;
int _buffer_fill = 0;
int real2sid = 0;
int real3sid = 0;
int allowagc = 0;
int allowagcII = 0;
int NOSOUNDTIMER = 0;
int SKIPTONEXTTUNE = 0;
int NOSOUNDTIMERCHECK = 20; // check 20 sound buffers not time .. buffers , eg 20 buffers could be 5000 samples times 20 etc etc
int nosounddetected = 0; // if 1 thread will be killed
int allownosoundcheck = 0; // changed if no songlength for this tune has been found and default is loaded eg 2 mins for a 1 second sfx .. not allowing that..
int allownosoundcheckactivate = 0; // actually allow the checking routine

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define f(x,y)*(int*)x^=*(int*)y

int_least32_t xorfloat(int_least32_t a)
{
	if (!a) 	return a;
	if (a > 0)	{ a ^= 0xFFffFFff;	a += 1; return a; }
	if (a < 0)	{ a ^= 0xFFffFFff; }
	a += 1;		return a;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double buf0_[64];
double buf1_[64];
//double cutoff_[ch]=0.10;
//cutoff(0.99),
//1lowpass//2 highpass//3 bandpass
double multi_band_(int mode_, int ax, double cutoff_, double inputSample)
{
	buf0_[ax] += cutoff_ * (inputSample - buf0_[ax]);
	buf1_[ax] += cutoff_ * (buf0_[ax] - buf1_[ax]);
	//switch (mode) {
	//    case FILTER_MODE_LOWPASS:
	if (mode_ == 1)
		return buf1_[ax];
	//    case FILTER_MODE_HIGHPASS:
	if (mode_ == 2)
		return inputSample - buf0_[ax];
	//    case FILTER_MODE_BANDPASS:
	if (mode_ == 3)
		return buf0_[ax] - buf1_[ax];
	//    default:
	return 0.0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_INT16_VALUE       32767.0
#define MIN_INT16_VALUE      -32768.0
double sample_d[2]; // stereo

int softtime = 44100 * 0.005;
int softinc;
float softlspeed = 0.002;
float sofldefault = 1;
float softlmax = 1;

double brickwall(double insamp,int type)
{
	sample_d[0] = (double)insamp;
	if (SURROUNDEMP)
	{
		//if (softinc++ > softtime)
		{
			//softinc = 0;

			//if (abs(multi_band_(2,62,0.5,sample_d[0])) > MAX_INT16_VALUE/2)
			if (fabs(sample_d[0]* softlmax) > MAX_INT16_VALUE)
				softlmax -= softlspeed;
			//printf("%.4f\n", softlmax);

			if (softlmax < 0)
				softlmax = 0;
		}

		sample_d[0] *= softlmax;
	}
	else
		softlmax = sofldefault;
	/* Clipping of overloaded samples */

	if (sample_d[0] > MAX_INT16_VALUE)		sample_d[0] = MAX_INT16_VALUE;
	if (sample_d[0] < MIN_INT16_VALUE)		sample_d[0] = MIN_INT16_VALUE;

	if (type == 0)
		insamp = (int16_t)sample_d[0];//(double)sample_d[0];
	if (type == 1)
		insamp = (float)sample_d[0];//(double)sample_d[0];
	if (type == 2)
		insamp = (double)sample_d[0];//(double)sample_d[0];

	return insamp;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double ___sleft[16] = { 0 };
double ___sright[16] = { 0 };
double maxhigh = 0, maxlow = 0;
double transient(int ch, double sample, float intensity, int origional)
{

	if (ch == 0)
	{
		___sleft[8] = sample;
		if (origional)			sample = ___sleft[8 - 1] + ___sleft[8 - 1] * 2 - ___sleft[8 - 2] - ___sleft[8];
		else { sample *= (1 - intensity); sample += (___sleft[8 - 1] + ___sleft[8 - 1] * 2 - ___sleft[8 - 2] - ___sleft[8])*intensity; }
		for (int v = 0; v < 15; v++)		{			___sleft[v] = ___sleft[v + 1];		}
	}
	else
	{
		___sright[8] = sample;
		if (origional)	sample = ___sright[8 - 1] + ___sright[8 - 1] * 2 - ___sright[8 - 2] - ___sright[8];
		else { sample *= (1 - intensity); 		sample += (___sright[8 - 1] + ___sright[8 - 1] * 2 - ___sright[8 - 2] - ___sright[8])*intensity; }
		for (int v = 0; v < 15; v++)		{			___sright[v] = ___sright[v + 1];		}
	}
	// restores digital sound lost in encoding.

	return sample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


double surroundfx(int ch,double tmp)
{
 					static int checktimemax = 44100*0.05;	//interval in samples to check
					static int checktime;					// counter
					static double lastsample;				// used to grab the sample data
					static double volumex;					// moves up or down in increments to shift left right volume
					static double volumemax = 0.25;			// volume limiter
					static double volumefinall = 1 + volumemax;  //final volume
					static double volumefinalr = 1 + volumemax;  //final volume
					static double volumeinc = 0.01;			// speed of volume shift

					if (checktime++ > checktimemax) { 
						checktime = 0; 

						lastsample = (double)tmp/32768;

						if (lastsample > 0 ) {	volumex += volumeinc;}
						
						if (lastsample < 0 ) volumex -= volumeinc;
						nocheck:

						if (lastsample > volumemax) lastsample = volumemax;	if (lastsample < -volumemax) lastsample = -volumemax;

						if (volumex > volumemax) volumex = volumemax; if (volumex < -volumemax) volumex = -volumemax;

						//printf("volumex : %.4lf lastsample : %.4lf\n", volumex, lastsample);
						
					}
					
					if (ch == 0)
						tmp = brickwall(tmp*(volumefinall - volumex),0);//wmpdelay(ch, tmp*2,0) ); // surround sound // 2 voices .. 0.3*1(missing voice)=0.3=1.3
					else
						tmp = brickwall(tmp*(volumefinalr + volumex ),0);//wmpdelay(ch, tmp*2,0) ); // surround sound // 1 voice = 0.3*2(missing voices)=0.6=1.6
					return tmp;
}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double sparelr[10] = { 0 };

double _StereoEnhanca[4] = { 0 };
int_least32_t _StereoEnhancaint[4] = { 0 };
int_least32_t sa = 0;
int_least32_t sl = 0;
int_least32_t sr = 0;
double MonoSign, stereo;
double SamplL = 0;
double SamplR = 0;
void StereoEnhanca(double SampxL, double SampxR, double WideCoeff,double monoremovepercentage)
{
	SamplL = SampxL; SamplR = SampxR;

	MonoSign = xorfloat((SamplL + SamplR) * monoremovepercentage);
	// SamplL+=xorfloat(SampxL);//test
	// SamplR+=xorfloat(SampxR);//test
	// _StereoEnhanca[0] = (SamplL*(2-WideCoeff));
	// _StereoEnhanca[1] = (SamplR*(2-WideCoeff));



	SamplL += (MonoSign);
	SamplR += (MonoSign);

	_StereoEnhanca[0] = (SamplL * WideCoeff);
	_StereoEnhanca[1] = (SamplR * WideCoeff);

	_StereoEnhanca[2] = stereo;
	_StereoEnhanca[3] = MonoSign;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int_least32_t o1, o2;
int_least32_t swapchanl = 0;
int_least32_t swapchanr = 0;
int a1 = 1, a2 = 0, a3 = 0, a4 = 1, a5 = 2;
int_least32_t easysurround(int ch, double sample, float ad, int removemid = 0)
{
	int_least32_t mid2l, mid2r, sxleft, sxright;

	if (ch == 0)	swapchanl = sample;
	if (ch == 1)	swapchanr = sample;
	//if (ch==0) { sample=swapchanr;sample-=swapchanl*0.5;}	
	//if (ch==1) { sample=swapchanl;sample-=swapchanr*0.5;}

	if (ch == 0)
	{
		mid2l = xorfloat((swapchanl + swapchanr) / a5);		// remove some mono/mid
		if (a1)			sxleft = (swapchanl + xorfloat(swapchanr));	// mono component
		else			sxleft = (swapchanl - xorfloat(swapchanr));	// stereo component
		   // sxleft=(swapchanl);	// seperate left
		if (a2)			sxleft = sxleft - mid2l;						// side band l
		else			sxleft = sxleft + mid2l;						// mid band l

		if (removemid)	sxleft = (swapchanl - xorfloat(swapchanr));		// stereo component

	}

	if (ch == 1)
	{
		mid2r = xorfloat((swapchanl + swapchanr) / a5);		//remove some mono/mid 
		if (a3)			sxright = (swapchanr + xorfloat(swapchanl));	// mono component
		else			sxright = (swapchanr - xorfloat(swapchanl));	// stereo component
			// sxright=(swapchanr);	//seperate right
		if (a4)			sxright -= mid2r;								// side bandr
		else			sxright += mid2r;								//mid bandr

		if (removemid)	sxright = sxright = (swapchanr - xorfloat(swapchanl));	// stereo component
	}

	if (ch == 0) { sample = sxleft; }	if (ch == 1) { sample = sxright; }

	//sample+=mid2;
	//if (ch==0)	sample=wmpdelay(ch,sxleft);
	//if (ch==1)	sample=wmpdelay(ch,sxright);
	//if (ch==0) { sample+=(swapchanl+swapchanr);}	
	//if (ch==1) { sample+=(swapchanl+swapchanr);}

out:;
	double a = (sample);
	if (ch == 0)
	{
		if (Bass_test_wgc)			a += multi_band_(1, 20, 0.04, sample);
		if (Bass_test_wgctwo)			a += multi_band_(2, 21, 0.90, sample);
	}
	if (ch == 1)
	{
		if (Bass_test_wgc)			a += multi_band_(1, 22, 0.04, sample);
		if (Bass_test_wgctwo)			a += multi_band_(2, 23, 0.90, sample);
	}
	return a;

	//return sample;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _sample_format_rate = 44100;
float surround_delay_time = 0.015;
int s__delay = (((uint32_t)((surround_delay_time)* _sample_format_rate)) & 0xFFFFFFFE);
int s__delay_xl = (((uint32_t)((surround_delay_time)* _sample_format_rate)) & 0xFFFFFFFE);
int s_read_l = 0;
int s_read_r = 0;
int s_read_l_delay = s__delay;
int s_read_r_delay = s__delay;
int s_write_l = 0;
int s_write_r = 0;

double s_buffer_l[44100 * 2] = { 0 };
double s_buffer_r[44100 * 2] = { 0 };

double outsample;
double ol, or ,oxl,oxr;
int_least32_t oldsampleX = 0;
// TODO: simplify ( a for loop should do it something along the lines of 
// for (int i=0; i < numberofsamples; ++i){os=sb[smpl+forwarddelay[i]]; // per channel
double wmpdelay(int ch, double sample,int reset)
{

	if (reset)
	{
		for (int x = 0; x < 44100 * 2; x++)		{			s_buffer_l[x] = 0; s_buffer_r[x] = 0;		}
		return 0;
	}
	//if (ch == 1)sample *= 3;
#ifdef testing
	int _type = 2; // 0 return sample unmodded // 1 a delay from previous // 2 delay from look forward
#endif

#ifndef testing
	const int _type = 1; // 0 return sample unmodded // 1 a delay from previous // 2 delay from look forward
#endif

#ifdef xtesting
	if (GetAsyncKeyState(VK_LCONTROL))
		_type=2;
	else
		_type=1;
#endif

#ifdef testing
	if (GetAsyncKeyState(VK_LCONTROL) & 1)
		old_surrounddelay_float -= 1;
		if (GetAsyncKeyState(VK_RCONTROL)&1)
			old_surrounddelay_float += 1;
		if (GetAsyncKeyState(VK_RSHIFT) & 1)
		{
			char f[10];
			sprintf(f, "%f", old_surrounddelay_float);
			MessageBoxA(0, f, 0, MB_ICONEXCLAMATION);
		}
#endif

	//if (chips == 3)
	//	sample = easysurround(ch, sample, 0.5);

	if (!_type)		return sample;
	
// 	if (ch == 1) {		oxr = sample * 0.25; sample *= 0.75;	}
// 	if (ch == 0) {		oxl = sample * 0.25; sample *= 0.75;	}
// 
// 	if (ch == 0) { sample += oxr; }
// 	if (ch == 1) { sample += oxl; }

	if (old_surrounddelay_float != surrounddelay_float)
	{
		if ((int)old_surrounddelay_float % 2 == 0)
		{} else old_surrounddelay_float += 1;
		surrounddelay_float = old_surrounddelay_float;
		s__delay = surrounddelay_float;
		//printf("_dlay=%i\n",s__delay);
		s__delay_xl = s__delay;
		s_read_l = 0;		s_read_r = 0;
		s_read_l_delay = s__delay;		s_read_r_delay = s__delay;
		s_write_l = 0;		s_write_r = 0;
	}

	// TODO: code cleanup and optimize .

	outsample = 0; //sample*=1.5;

	if (ch == 0)		s_write_l++;
	if (ch == 1)		s_write_r++;
	if (s_write_l >= s__delay * 2)				s_write_l = 0;
	if (s_write_r >= s__delay_xl * 2)			s_write_r = 0;
	if (ch == 0)	s_buffer_l[s_write_l] = sample;
	if (ch == 1)	s_buffer_r[s_write_r] = sample;
	if (ch == 0)		s_read_l++;
	if (ch == 1)		s_read_r++;
	if (s_read_l >= s__delay * 2)				s_read_l = 0;
	if (s_read_r >= s__delay_xl * 2)			s_read_r = 0;
	if (ch == 0)		s_read_l_delay++;
	if (ch == 1)		s_read_r_delay++;
	if (s_read_l_delay >= s__delay * 2)				s_read_l_delay = 0;
	if (s_read_r_delay >= s__delay_xl * 2)			s_read_r_delay = 0;

	if (_type == 1)
	{

		if (ch == 0)		outsample = (s_buffer_r[s_read_r] - (s_buffer_l[s_read_l_delay])) / 1.414;
		if (ch == 1)		outsample = (s_buffer_l[s_read_l] - (s_buffer_r[s_read_r_delay])) / 1.414;
		if (ch == 0)		ol = outsample; else or = outsample;

	}

	if (_type == 2)
	{

		if (ch == 0)		outsample = (xorfloat(s_buffer_r[s_read_r]*1.414) - (s_buffer_l[s_read_l_delay])) / 1.414;
		if (ch == 1)		outsample = (xorfloat(s_buffer_l[s_read_l]*1.414) - (s_buffer_r[s_read_r_delay])) / 1.414;
		if (ch == 0)		ol = outsample; else or = outsample;


	}

	if (_type == 3)
	{

		if (ch == 0)		outsample = ((s_buffer_r[s_read_r]) - xorfloat(s_buffer_l[s_read_l_delay] * 1.414)) / 1.414;
		if (ch == 1)		outsample = ((s_buffer_l[s_read_l]) - xorfloat(s_buffer_r[s_read_r_delay] * 1.414)) / 1.414;
		if (ch == 0)		ol = outsample; else or = outsample;

	}


	return outsample;


}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_INT32_VALUE  2147483647.0
#define MIN_INT32_VALUE -2147483648.0
#define MAX_INT24_VALUE     8388607.0
#define MIN_INT24_VALUE    -8388608.0
#define MAX_INT16_VALUE       32767.0
#define MIN_INT16_VALUE      -32768.0
#define MAX_INT8_VALUE          127.0
#define MIN_INT8_VALUE         -128.0

#ifndef _INT16_T_DECLARED
typedef signed   short   int16_t;
#endif

#ifndef _UINT16_T_DECLARED
typedef unsigned short   uint16_t;
#endif

#ifndef _INT32_T_DECLARED
#if UINT_MAX == 0xffff /* 16 bit compiler */
typedef signed   long    int32_t;
#else /* UINT_MAX != 0xffff */ /* 32/64 bit compiler */
typedef signed   int     int32_t;
#endif
#endif /* !_INT32_T_DECLARED */
//double sample_d[2];

double limit_sample(int Int_size, double sample)
{
	sample_d[0] = (double)sample;

	if (Int_size == 32)
	{
		if (sample_d[0] > MAX_INT32_VALUE) sample_d[0] = MAX_INT32_VALUE;
		if (sample_d[0] < MIN_INT32_VALUE) sample_d[0] = MIN_INT32_VALUE;
	}



	if (Int_size == 16)
	{
		/* Clipping of overloaded samples */
		if (sample_d[0] > MAX_INT16_VALUE) sample_d[0] = MAX_INT16_VALUE;
		if (sample_d[0] < MIN_INT16_VALUE) sample_d[0] = MIN_INT16_VALUE;

		sample = (int16_t)sample_d[0];
	}



	return sample;
}


//////////////////////////////////////////////////////////////////////////

double decMain = 0.0001;
double ampMain = 0.000002;
double decRight = 0, decLeft = 0;
double ampRight = 0, ampLeft = 0;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
float globalVolumeOUT = 4;			// Main out > ( surround ) Agc		//
float globalVolumeOUT_MAX = 5;		// max  .. simple Agc				//

									//////////////////////////////////////////////////////////////////////////
double wtf_softlimit(double leftIn, double Threshold = 0.8)
{
	double leftOut;
	double x;

	leftIn /= 32768;


	// Insider the "tight loop."

	if (leftIn > Threshold)
	{
		x = Threshold / leftIn;
		leftOut = (1.0f - x) * (1.0f - Threshold) + Threshold;
	}
	else if (leftIn < -Threshold)
	{
		x = -Threshold / leftIn;
		leftOut = -((1.0f - x) * (1.0f - Threshold) + Threshold);
	}
	else
	{
		leftOut = leftIn;
	}

	if (leftOut > 1)
		leftOut = 1;
	if (leftOut < -1)
		leftOut = -1;
	leftOut = 1.5 * leftOut - 0.5 * leftOut * leftOut * leftOut; // Simple f(x) = 1.5x - 0.5x^3 waveshaper

	leftOut = limit_sample( 16, (leftOut*32768) );

	return leftOut;

}
//////////////////////////////////////////////////////////////////////////
int eqpresetoffset = 0;
int basenumber = 1;
int stseperation = 1;
////////////////////////////
//If number of samples is 200, then step size if 360 / 200 = 1.8
#define M_PI 3.14159265358979323846
int preset[40] = { 0 };
void gensin(int step,int value,int devide,int alpha,int db)
{
	
	step = 360 / step;
	float variance = 360 / (step/devide);
	for (int i = 0; i < step; i++)
	{
		float rads = M_PI / 180;
		preset[i] = (float)(alpha * sin(variance*i*rads)+db);
	}

}
////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

double dbmax = 0;
double db_level = -12; // more minus = lower volume ( looks like -16 is 0 db should test it to make sure looks like it though -16 = 0db so -14 is +2db at a guess)
double db_headroom = 1;
double db_attack = 0.01; // rate of volume increases
double db_decay = 0.01/2; // rate volume decreases

double aplitude(int ch, double insample)
{


	if (insample == 0) insample = 0.00001;
	double s = insample;


	static double sum = 0;
	static double decibel = 0;
	//for (var i = 0; i < _buffer.length; i = i + 2)
	{
		double sample = s / 32768.0;
		sum += (sample * sample);
	}


	static int timer = 44100/10;
	if (timer-- <= 0)
	{
		timer = 44100/10;
		double rms = sqrt(sum / (timer / 2));
		decibel = 20 * log10(rms);
		sum = 0;
		if (decibel < (db_level - db_headroom))
			dbmax += db_attack;
		if (decibel > (db_level + db_headroom) )
			dbmax -= db_decay;

		//printf("[DB @ %.2lf 0 = MAX (%.4lf dB) >> %.2i\n",decibel,dbmax,intensify);
	}

	//return decibel;




	return dbmax;

}

/////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the end ...
//agc
double agc[2] = { 1, 1 };
double agc_maxsample[2] = { 0 , 0 };
float agc_attack[2] = { 0.0001 , 0.0001 };
float agc_decay[2] = { 0.0002 , 0.0002 };
float agc_dbmaximum[2] = { 20,20 };
float agc_dbminimum[2] = { 0,0 };
float agc_hr[4] = { 0.6*32768. , 0.6*32768. , 0.6*32768. ,0.6*32768. }; // head room
float agc_distortion[4] = { 32768- agc_hr[0],32768 - agc_hr[1] ,32768 - agc_hr[2] ,32768 - agc_hr[3] };
float agc_limitsmaxsamplevalue[2] = { agc_distortion[0], agc_distortion[1] };
float agc_limitsminsamplevalue[2] = { agc_distortion[2] ,agc_distortion[3] };
int agc_timervaluereset[2] = { 44100 * 0.015, 44100 * 0.015 };
float agc_maxslider = 0.5;

double simpleagc(double sample,int ch)
{

	

	agc_limitsmaxsamplevalue[ch] = agc_maxslider*32768;
	agc_limitsminsamplevalue[ch] = (agc_maxslider*32768);
	agc_limitsmaxsamplevalue[ch] = agc_maxslider*32768;
	agc_limitsminsamplevalue[ch] = (agc_maxslider*32768);

	sample = fabs(sample*agc[ch]);

	if (sample > agc_maxsample[ch])
		agc_maxsample[ch] = sample;

	static int timer[2] = { agc_timervaluereset[ch],agc_timervaluereset[ch] };

	if (timer[ch]-- <= 0)
	{
		timer[ch] = agc_timervaluereset[ch];
		// auto agc
		//if (agc_maxsample[ch] > 32767/2) agc_maxslider -= 0.0011;
		//if (agc_maxsample[ch] < 32767/3) agc_maxslider += 0.001;

		if (agc_maxsample[ch] < agc_limitsminsamplevalue[ch])
			agc[ch] += agc_attack[ch];
		if (agc_maxsample[ch] > agc_limitsmaxsamplevalue[ch])
			agc[ch] -= agc_decay[ch];

		if (agc[ch] > agc_dbmaximum[ch]) agc[ch] = agc_dbmaximum[ch];
		if (agc[ch] < agc_dbminimum[ch]) agc[ch] = agc_dbminimum[ch];
		agc_maxsample[ch] = 0;

		//printf("[DB @ %.2lf 0 = MAX (%.4lf dB) >> %.2i\n",decibel,dbmax,intensify);
	}
	return agc[ch];
}



// wait ... time passes .. day dawns ..the trolls turn to stone.