// XMPlay SIDex input plugin (c) 2021 Nathan Hindley & William Cronin
#define _CRT_SECURE_NO_WARNINGS // stops the compiler grumbling.

//##########################################################################################
#define SIDEXVERSION "Rev 3.0 final?"//
#define LIBSIDPLAYVERSION "2.3.1 2022-01-31"//
//#define SIDEXNAME "SodaSID"//
#define SIDEXNAME "SIDex"//
#define DATEXVAR "2022"
//#define debugplayer
//##########################################################################################
char DEBUGAGC[20];//
#include <Windows.h> //wgc
char DEBUGGERCHAR[255] = { 0 };
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>
#include <iostream>
#include <vector>
#include <cctype>
//#include "configdialog.h" //wgc
#include "../../resource.h"

//WGC
#include "../../eq/eq1.cpp"
#include "../../eq/iir_cfs.cpp"
#include "../../eq/iir_fpu.cpp"
#include "../../eq/eq1.h"
//

//TODO: set up defines for all changes 
//TODO: class these
int haltplayer = 0;
//float equalizerl[11] = { 6,5,4,3,2,1,0,2,4,6,6 };//WGC
//float equalizerr[11] = { 6,5,4,3,2,1,0,2,4,6,6 };
//float equalizerl[11] = { 0,0,0,0,0,0,0,0,0,0 };//flat
//float equalizerr[11] = { 0,0,0,0,0,0,0,0,0,0 };
//float equalizerl[11] = { 12,10,4,-3,-0,-4,2,4,12,12,0 };// very nice but if they turn eq on xmplay on it sounds too much treble
//float equalizerr[11] = { 12,10,4,1,-0,-0,5,4,12,12,0 };
//
extern double _StereoEnhanca[4], agc[];
extern double brickwall(double insamp, int type);
extern float rand_FloatRangeA(float a, float b);
extern float agc_maxslider,voices[], softlmax, sofldefault,SDELAY, fadevolume, _decode_buffer[];
extern int allownosoundcheckactivate,	allownosoundcheck,nosounddetected,NOSOUNDTIMERCHECK,SKIPTONEXTTUNE,NOSOUNDTIMER,allowagc, allowagcII,	stseperation,eqpresetoffset,preset[], BANDEQ, surroundcount, LPFILTER, HPFILTER, PREAMP, SURROUNDSEPERATION, SURROUNDSEPFORCE, SURROUNDEMP, surround, VOICE_1SID[], VOICE_2SID[], VOICE_3SID[], _8580VOLUMEBOOST, _buffer_fill, real2sid, real3sid; //wgc
extern void StereoEnhanca(double SampxL, double SampxR, double WideCoeff, double monoremovepercentage), gensin(int step,int value,int devide,int alpha, int db);
int _click_wa = 1;
int eqslider[] = { IDC_EQU1 ,IDC_EQU2 ,IDC_EQU3 ,IDC_EQU4 ,IDC_EQU5 ,IDC_EQU6 ,IDC_EQU7 ,IDC_EQU8 ,IDC_EQU9 ,IDC_EQU10 ,IDC_EQU11 ,IDC_EQU12 ,IDC_EQU13 ,IDC_EQU14 ,IDC_EQU15 ,IDC_EQU16 ,IDC_EQU17 ,IDC_EQU18 ,IDC_EQU19 ,IDC_EQU20 };
int equalizerl[11] = { 12,10,4,-3,-0,-4,2,4,4,12,0 };// sound generally nice with xmp eq on also
int equalizerr[11] = { 12,10,4,1,-0,-0,5,4,12,4,0 };

int eqpresets[200]{
	12/2,10/2,4/2,-3/2,-0,-4/2,2/2,4/2,4/2,12/2,
	12/2,10/2,4/2,1/2,-0,-0,5/2,4/2,12/2,4/2,
	6,4,2,1,0,0,0,1,5,4,
	6,4,2,1,0,0,0,1,5,4,
	6,0,6,0,6,0,6,0,6,0,
	0,6,0,6,0,6,0,6,0,6,
	2,0,2,0,2,0,2,0,2,0,
	0,2,0,2,0,2,0,2,0,2,
	6,0,2,0,1,0,2,0,4,0,
	0,6,0,2,0,1,0,2,0,4,
	0,1,0,1,0,1,0,2,0,2,
	1,0,1,0,1,0,2,0,2,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0
};

#include "xmpin.h"

static XMPFUNC_IN *xmpfin;
static XMPFUNC_MISC *xmpfmisc;
static XMPFUNC_FILE *xmpffile;
static XMPFUNC_TEXT *xmpftext;
static XMPFUNC_REGISTRY *xmpfreg;




#include "utils/STILview/stil.h"
#include "utils/SidDatabase.h"
#include <assert.h>
#include <builders/residfp-builder/residfp.h>
#include <commctrl.h>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <sidid.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/sidplayfp.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#ifdef debugplayer
void printx(char*string)
{
	static int count = 0;
	char buff[100];
	time_t now = time(0);
	strftime(buff, 100, "%H:%M:%S", localtime(&now));
	printf("%s: ", buff);
	//printf("[%.4i] - ", count++);
	printf(string);
	printf("\n");
}
#endif
#ifndef _WIN32
#include <libgen.h>
#else
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#include "c64os.h" // contains kernel (e000) , basic(a000) & chargen.

static HINSTANCE ghInstance;

// fancy data containers
typedef struct
{
    sidplayfp* m_engine;
    ReSIDfpBuilder* m_builder;
    SidTune* p_song;
    SidConfig m_config;
    SidDatabase d_songdbase;
    STIL d_stilbase;
    SidId d_sididbase;
    bool d_loadeddbase;
    bool d_loadedstil;
    bool d_loadedstilmd5;
    bool d_loadedsidid;
    char p_sididplayer[25];
    const SidTuneInfo* p_songinfo;
    int p_songcount;
    int p_subsong;
    int p_songlength;
    int* p_subsonglength;
    char p_sidmodel[100];/*char* p_sidmodel;*/// allowed extra chars while testing 'incase of error' - petite will sort it wgc
	char p_clockspeed[100];/*char* p_clockspeed;*///wgc
    bool b_loaded;

    float fadein;
    float fadeout;
    int fadeouttrigger;
} SIDengine;
static SIDengine sidEngine;

typedef struct
{
    char c_sidmodel[10];
    char c_clockspeed[10];
    int c_powerdelay;
    int c_defaultlength;
    int c_minlength;
    int c_6581filter;
    int c_8580filter;
    int c_fadeinms;
    int c_fadeoutms;
    char c_samplemethod[10];
    char c_dbpath[250];
    bool c_locksidmodel;
    bool c_lockclockspeed;
    bool c_enablefilter;
    bool c_enabledigiboost;
    bool c_powerdelayrandom;
    bool c_forcelength;
    bool c_skipshort;
    bool c_fadein;
    bool c_fadeout;
    bool c_disableseek;
    bool c_detectplayer;
	bool pseudostereo;
	bool c_surround;
	bool c_surround2sid;
} SIDsetting;
static SIDsetting sidSetting;

extern int playsound;
int VOICE_1SIDsave[3] = { 0 };//wgc
int VOICE_2SIDsave[3] = { 0 };
int VOICE_3SIDsave[3] = { 0 };

void setsidchannelssurround()
{
// 	if (!real2sid)
// 		//if (VOICE_1SID[0] == 0 && VOICE_1SID[1] == 0 && VOICE_1SID[2] == 0)//wgc
// 		{
// 			VOICE_1SID[0] = 0;	VOICE_1SID[1] = 1;	VOICE_1SID[2] = 0;
// 			VOICE_2SID[0] = 1;	VOICE_2SID[1] = 0;	VOICE_2SID[2] = 1;
// 		}	
// 	else
// 	{
// 		VOICE_1SID[0] = 0;	VOICE_1SID[1] = 0;	VOICE_1SID[2] = 0;
// 		VOICE_2SID[0] = 0;	VOICE_2SID[1] = 0;	VOICE_2SID[2] = 0;
// 	}

}




// pretty up the format
static const char* simpleFormat(const char* songFormat) {
    if (std::string(songFormat).find("PSID") != std::string::npos) {
        return "PSID";
    } else if (std::string(songFormat).find("RSID") != std::string::npos) {
        return "RSID";
    } else {
        return songFormat;
    }
}
// pretty up the length
static const char* simpleLength(int songLength, char *buf) {
    int rsSecond = songLength;
    int rsMinute = songLength / 60;
    int rsHour = rsMinute / 60;

    if (rsHour > 0) {
		sprintf(buf, "%u:%02u:%02u", rsHour, rsMinute % 60, rsSecond % 60);
    } else {
        sprintf(buf, "%02u:%02u", rsMinute, rsSecond % 60);
    }
    return buf;
}
// config load/save/apply functions
static void loadConfig()
{

		char *eqcharval = new char[10];
		for (int i = 0; i < 10; i++)
		{

			sprintf(eqcharval, "eqvalue_%i", i);
			xmpfreg->GetInt("SIDex", eqcharval, &equalizerl[i]);
		}	
		for (int i = 0; i < 10; i++)
		{

			sprintf(eqcharval, "eqvalue_%i", i+10);
			xmpfreg->GetInt("SIDex", eqcharval, &equalizerr[i]);
			//sprintf(eqcharval, "load eqvalue_%i = %i", i + 10, equalizerr[i]);
			//MessageBoxA(0, eqcharval, "", 0);
		}
	delete eqcharval;


	for (int i = 0; i < 3; i++)
	{
		VOICE_1SID[i] = 0;//0 is unmuted
		VOICE_2SID[i] = 0;
		VOICE_3SID[i] = 0;
	}//


	char *eqcharvalvoices = new char[20];
	for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_1SID_%i", i);
		xmpfreg->GetInt("SIDex", eqcharvalvoices, &VOICE_1SID[i]);
	}//

	for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_2SID_%i", i);
		xmpfreg->GetInt("SIDex", eqcharvalvoices, &VOICE_2SID[i]);
	}//
	for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_3SID_%i", i);
		xmpfreg->GetInt("SIDex", eqcharvalvoices, &VOICE_3SID[i]);
		//sprintf(eqcharval, "save eqvalue_%i = %i", i + 10, ival);
		//MessageBoxA(0, eqcharval, "", 0);
	}//
	delete eqcharvalvoices;


	int agc_temp = 50;
	xmpfreg->GetInt("SIDex", "agc_maxslider", &agc_temp);	
	if (agc_temp != 0)
		agc_maxslider = (float)agc_temp / 100;
	//else
	//	agc_maxslider = 0.5;

	xmpfreg->GetInt("SIDex", "c_twentybandequaliser", &BANDEQ);
	xmpfreg->GetInt("SIDex", "c_preamp", &PREAMP);
	//xmpfreg->GetInt("SIDex", "c_surroundcount", &surroundcount);
	xmpfreg->GetInt("SIDex", "c_allowsecondsid", &SURROUNDSEPERATION);
	xmpfreg->GetInt("SIDex", "c_forcedifference", &SURROUNDSEPFORCE);
	xmpfreg->GetInt("SIDex", "c_enablepassband", &SURROUNDEMP);
	xmpfreg->GetInt("SIDex", "c_passband", &HPFILTER);
	xmpfreg->GetInt("SIDex", "c_passgain", &LPFILTER);
	xmpfreg->GetInt("SIDex", "c_anticlick", &_click_wa);
	int abcv = 0;
	xmpfreg->GetInt("SIDex", "c_alternatesurrounddelay", &abcv);
	SDELAY = abcv;


	//defaults wgc
    if (!sidEngine.b_loaded) {
        strcpy(sidSetting.c_sidmodel, "6581");
        strcpy(sidSetting.c_clockspeed, "PAL");
        strcpy(sidSetting.c_samplemethod, "Normal");
        strcpy(sidSetting.c_dbpath, "");
        sidSetting.c_defaultlength = 120;
        sidSetting.c_minlength = 3;
        sidSetting.c_6581filter = 36;
        sidSetting.c_8580filter = 36;
        sidSetting.c_powerdelay = 0;
        sidSetting.c_fadeinms = 100;
        sidSetting.c_fadeoutms = 500;
        sidSetting.c_lockclockspeed = FALSE;
        sidSetting.c_locksidmodel = FALSE;
        sidSetting.c_enabledigiboost = FALSE;
        sidSetting.c_enablefilter = TRUE;
        sidSetting.c_powerdelayrandom = TRUE;
        sidSetting.c_forcelength = FALSE;
        sidSetting.c_skipshort = FALSE;
        sidSetting.c_fadein = TRUE;
        sidSetting.c_fadeout = FALSE;
        sidSetting.c_disableseek = TRUE;
        sidSetting.c_detectplayer = TRUE;
		sidSetting.c_surround = surround = false; //WGC
		allowagc = 0;
		allowagcII = 0;
		agc_maxslider = 0.25;
		//PREAMP = 65;
        
        if (xmpfreg->GetString("SIDex","c_sidmodel",sidSetting.c_sidmodel,10) != 0) {
            xmpfreg->GetString("SIDex","c_clockspeed",sidSetting.c_clockspeed,10);
            xmpfreg->GetString("SIDex","c_samplemethod",sidSetting.c_samplemethod,10);
            xmpfreg->GetString("SIDex","c_dbpath",sidSetting.c_dbpath,250);
            xmpfreg->GetInt("SIDex", "c_defaultlength", &sidSetting.c_defaultlength);
            xmpfreg->GetInt("SIDex", "c_minlength", &sidSetting.c_minlength);
            xmpfreg->GetInt("SIDex", "c_6581filter", &sidSetting.c_6581filter);
            xmpfreg->GetInt("SIDex","c_8580filter", &sidSetting.c_8580filter);
            xmpfreg->GetInt("SIDex","c_powerdelay", &sidSetting.c_powerdelay);
            xmpfreg->GetInt("SIDex","c_fadeinms", &sidSetting.c_fadeinms);
            xmpfreg->GetInt("SIDex","c_fadeoutms", &sidSetting.c_fadeoutms);
            
			xmpfreg->GetInt("SIDex", "allowagc", &allowagc);
			xmpfreg->GetInt("SIDex", "allowagcII", &allowagcII);


            int ival;

			if (xmpfreg->GetInt("SIDex", "c_surround2sid", &ival))
				sidSetting.c_surround2sid = ival;


			if (xmpfreg->GetInt("SIDex", "c_surround", &ival))
				sidSetting.c_surround = ival;
            if (xmpfreg->GetInt("SIDex", "c_lockclockspeed", &ival))
                sidSetting.c_lockclockspeed = ival;
            if (xmpfreg->GetInt("SIDex", "c_locksidmodel", &ival))
                sidSetting.c_locksidmodel = ival;
            if (xmpfreg->GetInt("SIDex", "c_enabledigiboost", &ival))
                sidSetting.c_enabledigiboost = ival;
            if (xmpfreg->GetInt("SIDex", "c_enablefilter", &ival))
                sidSetting.c_enablefilter = ival;
            if (xmpfreg->GetInt("SIDex", "c_powerdelayrandom", &ival))
                sidSetting.c_powerdelayrandom = ival;
            if (xmpfreg->GetInt("SIDex", "c_forcelength", &ival))
                sidSetting.c_forcelength = ival;
            if (xmpfreg->GetInt("SIDex", "c_skipshort", &ival))
                sidSetting.c_skipshort = ival;
            if (xmpfreg->GetInt("SIDex", "c_fadein", &ival))
                sidSetting.c_fadein = ival;
            if (xmpfreg->GetInt("SIDex", "c_fadeout", &ival))
                sidSetting.c_fadeout = ival;
            if (xmpfreg->GetInt("SIDex", "c_disableseek", &ival))
                sidSetting.c_disableseek = ival;
            if (xmpfreg->GetInt("SIDex", "c_detectplayer", &ival))
                sidSetting.c_detectplayer = ival;

			if (sidSetting.c_locksidmodel)
			{
				if (std::string(sidSetting.c_sidmodel).find("6581") != std::string::npos) {
					_8580VOLUMEBOOST = 0;
				}
				else
				{
					_8580VOLUMEBOOST = 1;
				}
			}
        }
    }
}
static bool applyConfig(bool initThis) {
	//if (sidEngine.m_engine)
	{
		if (initThis) {
			sidEngine.m_config = sidEngine.m_engine->config();

			// apply sid model
			if (std::string(sidSetting.c_sidmodel).find("6581") != std::string::npos) {
				sidEngine.m_config.defaultSidModel = SidConfig::MOS6581;
			}
			else if (std::string(sidSetting.c_sidmodel).find("8580") != std::string::npos) {
				sidEngine.m_config.defaultSidModel = SidConfig::MOS8580;
			}

			// apply clock speed
			if (std::string(sidSetting.c_clockspeed).find("PAL") != std::string::npos) {
				sidEngine.m_config.defaultC64Model = SidConfig::PAL;
			}
			else if (std::string(sidSetting.c_clockspeed).find("NTSC") != std::string::npos) {
				sidEngine.m_config.defaultC64Model = SidConfig::NTSC;
			}

			// apply power delay
			if (sidSetting.c_powerdelayrandom) {
				sidEngine.m_config.powerOnDelay = SidConfig::DEFAULT_POWER_ON_DELAY;
			}
			else {
				sidEngine.m_config.powerOnDelay = sidSetting.c_powerdelay;
			}

			// apply sample method
			if (std::string(sidSetting.c_samplemethod).find("Normal") != std::string::npos) {
				sidEngine.m_config.samplingMethod = SidConfig::INTERPOLATE;
			}
			else if (std::string(sidSetting.c_samplemethod).find("Accurate") != std::string::npos) {
				sidEngine.m_config.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
			}

			// apply sid model & clock speed lock
			sidEngine.m_config.forceSidModel = sidSetting.c_locksidmodel;
			sidEngine.m_config.forceC64Model = sidSetting.c_lockclockspeed;

			// apply digi boost
			sidEngine.m_config.digiBoost = sidSetting.c_enabledigiboost;

			
			// force sid to be 8580 for test
// 			sidEngine.m_config.secondSidAddress = 0xd400;

// 			sidEngine.m_config.forceSecondSidModel = 1;
// 			sidEngine.m_config.forceSidModel = 1;


		}

		// apply filter status & levels
		sidEngine.m_builder->filter(sidSetting.c_enablefilter);
		float temp6581set = (float)sidSetting.c_6581filter / 100;
		sidEngine.m_builder->filter6581Curve(temp6581set);
		float temp8580set = (float)sidSetting.c_8580filter / 100;
		sidEngine.m_builder->filter8580Curve(temp8580set);

		// apply config
		sidEngine.m_config.sidEmulation = sidEngine.m_builder;
		if (sidEngine.m_engine->config(sidEngine.m_config)) {
			return TRUE;
		}
		else {
			return FALSE;
		}

	}
}



static void saveConfig()
{

	
    xmpfreg->SetString("SIDex","c_sidmodel",sidSetting.c_sidmodel);
    xmpfreg->SetString("SIDex","c_samplemethod",sidSetting.c_samplemethod);
    xmpfreg->SetString("SIDex","c_dbpath",sidSetting.c_dbpath);
    xmpfreg->SetString("SIDex","c_clockspeed",sidSetting.c_clockspeed);
    xmpfreg->SetInt("SIDex", "c_powerdelay", &sidSetting.c_powerdelay);
    xmpfreg->SetInt("SIDex", "c_6581filter", &sidSetting.c_6581filter);
    xmpfreg->SetInt("SIDex", "c_8580filter", &sidSetting.c_8580filter);
    xmpfreg->SetInt("SIDex", "c_defaultlength", &sidSetting.c_defaultlength);
    xmpfreg->SetInt("SIDex", "c_minlength", &sidSetting.c_minlength);
    xmpfreg->SetInt("SIDex", "c_fadeinms", &sidSetting.c_fadeinms);
    xmpfreg->SetInt("SIDex", "c_fadeoutms", &sidSetting.c_fadeoutms);

    int ival;
	ival = sidSetting.c_surround2sid;//wgc
	xmpfreg->SetInt("SIDex", "c_surround2sid", &ival);
	ival = sidSetting.c_surround;//wgc
	xmpfreg->SetInt("SIDex", "c_surround", &ival);
    ival = sidSetting.c_lockclockspeed;
    xmpfreg->SetInt("SIDex", "c_lockclockspeed", &ival);
    ival = sidSetting.c_locksidmodel;
    xmpfreg->SetInt("SIDex", "c_locksidmodel", &ival);
    ival = sidSetting.c_enablefilter;
    xmpfreg->SetInt("SIDex", "c_enablefilter", &ival);
    ival = sidSetting.c_enabledigiboost;
    xmpfreg->SetInt("SIDex", "c_enabledigiboost", &ival);
    ival = sidSetting.c_powerdelayrandom;
    xmpfreg->SetInt("SIDex", "c_powerdelayrandom", &ival);
    ival = sidSetting.c_forcelength;
    xmpfreg->SetInt("SIDex", "c_forcelength", &ival);
    ival = sidSetting.c_skipshort;
    xmpfreg->SetInt("SIDex", "c_skipshort", &ival);
    ival = sidSetting.c_fadein;
    xmpfreg->SetInt("SIDex", "c_fadein", &ival);
    ival = sidSetting.c_fadeout;
    xmpfreg->SetInt("SIDex", "c_fadeout", &ival);
    ival = sidSetting.c_disableseek;
    xmpfreg->SetInt("SIDex", "c_disableseek", &ival);
    ival = sidSetting.c_detectplayer;
    xmpfreg->SetInt("SIDex", "c_detectplayer", &ival);

	char *eqcharval = new char[10];
	for (int i = 0; i < 10; i++)
	{
		ival = equalizerl[i];
		sprintf(eqcharval, "eqvalue_%i", i);
		xmpfreg->SetInt("SIDex", eqcharval, &ival);
	}//

	for (int i = 0; i < 10; i++)
	{
		ival = equalizerr[i];
		sprintf(eqcharval, "eqvalue_%i", i+10);
		xmpfreg->SetInt("SIDex", eqcharval, &ival);
		//sprintf(eqcharval, "save eqvalue_%i = %i", i + 10, ival);
		//MessageBoxA(0, eqcharval, "", 0);
	}//
	delete eqcharval;



	char *eqcharvalvoices = new char[20];
	for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_1SID_%i", i);
		xmpfreg->SetInt("SIDex", eqcharvalvoices, &VOICE_1SID[i]);
	}//

	for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_2SID_%i", i);
		xmpfreg->SetInt("SIDex", eqcharvalvoices, &VOICE_2SID[i]);
	}//
		for (int i = 0; i < 3; i++)
	{
		sprintf(eqcharvalvoices, "VOICE_3SID_%i", i);
		xmpfreg->SetInt("SIDex", eqcharvalvoices, &VOICE_3SID[i]);
		//sprintf(eqcharval, "save eqvalue_%i = %i", i + 10, ival);
		//MessageBoxA(0, eqcharval, "", 0);
	}//
	delete eqcharvalvoices;

	xmpfreg->SetInt("SIDex", "allowagc", &allowagc);
		xmpfreg->SetInt("SIDex", "allowagcII", &allowagcII);
	int agc_temp = agc_maxslider * 100;
	xmpfreg->SetInt("SIDex", "agc_maxslider", &agc_temp);
	xmpfreg->SetInt("SIDex", "c_twentybandequaliser", &BANDEQ);
	xmpfreg->SetInt("SIDex", "c_preamp", &PREAMP);
	//xmpfreg->SetInt("SIDex", "c_surroundcount", &surroundcount);//
	xmpfreg->SetInt("SIDex", "c_allowsecondsid", &SURROUNDSEPERATION);
	xmpfreg->SetInt("SIDex", "c_forcedifference", &SURROUNDSEPFORCE);
	xmpfreg->SetInt("SIDex", "c_enablepassband", &SURROUNDEMP);
	xmpfreg->SetInt("SIDex", "c_passband", &HPFILTER);
	xmpfreg->SetInt("SIDex", "c_passgain", &LPFILTER);
	xmpfreg->SetInt("SIDex", "c_anticlick", &_click_wa);
	int abcv = SDELAY;
	xmpfreg->SetInt("SIDex", "c_alternatesurrounddelay", &abcv);



    if (sidEngine.b_loaded) {
        applyConfig(FALSE);
    }
}
// trim from both ends
static inline std::string &trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}
// functions to load and fetch the songlengthdbase
static void loadSonglength() {
    if (!sidSetting.c_forcelength && !sidEngine.d_loadeddbase && strlen(sidSetting.c_dbpath)>10) {
        std::string relpathName = sidSetting.c_dbpath;
        relpathName.append("Songlengths.md5");
        sidEngine.d_loadeddbase = sidEngine.d_songdbase.open(relpathName.c_str());
    }
}
static int fetchSonglength(SidTune* sidSong, int sidSubsong) {
    char md5[SidTune::MD5_LENGTH + 1];
    int32_t md5duration = 0;
    int32_t defaultduration = sidSetting.c_defaultlength;

    if (!sidSetting.c_forcelength && sidEngine.d_loadeddbase) {
        md5duration = sidEngine.d_songdbase.length(sidSong->createMD5New(md5), sidSubsong);
		if (md5duration > 0) {
			defaultduration = md5duration;
			allownosoundcheck = 0; // got length no reason to check as time has been calculated
		}
		else
			allownosoundcheck = 1; // possible over run of sound now allow check of silence
    }
    
    return defaultduration;
}
// get song's tags
static char *GetTags(const SidTuneInfo* p_songinfo)
{
    static const char *tagname[3] = { "title", "artist", "date" };
    std::string taginfo;
    for (int a = 0; a < 3; a++) {
        const char *tag = p_songinfo->infoString(a);
        if (tag[0]) {
            char *tagval = xmpftext->Utf8(tag, -1);
            taginfo.append(tagname[a]);
            taginfo.push_back('\0');
            taginfo.append(tagval);
            taginfo.push_back('\0');
            xmpfmisc->Free(tagval);
        }
    }
    taginfo.append("filetype");
    taginfo.push_back('\0');
    taginfo.append(simpleFormat(p_songinfo->formatString()));
    taginfo.push_back('\0');
    taginfo.push_back('\0');
    int tagsLength = taginfo.size();
    char *tags = (char*)xmpfmisc->Alloc(tagsLength);
    memcpy(tags, taginfo.data(), tagsLength);
    return tags;
}
// try to load STIL database
static void loadSTILbase() {
    if (!sidEngine.d_loadedstil && strlen(sidSetting.c_dbpath)>10) {
        std::string relpathName = sidSetting.c_dbpath;
        relpathName.replace((relpathName.length()-10), 10, "");
        char abspathName[MAX_PATH];
        if(_fullpath(abspathName, relpathName.c_str(), MAX_PATH) != NULL ) {
            sidEngine.d_loadedstil = sidEngine.d_stilbase.setBaseDir(abspathName);
        }
    }
    if (!sidEngine.d_loadedstilmd5 && strlen(sidSetting.c_dbpath)>10) {
        std::string relpathName = sidSetting.c_dbpath;
        relpathName.append("STIL.md5");
        std::ifstream f(relpathName.c_str());
        if (f.good()) {
            sidEngine.d_loadedstilmd5 = TRUE;
        }
    }
}
static char * fetchSTILmd5(char type) {
    std::string stilName = sidSetting.c_dbpath;
    if (type == 'G' || type == 'E') {
        stilName.append("STIL.md5");
    } else if (type == 'B') {
        stilName.append("BUGlist.md5");
    }
    char md5[SidTune::MD5_LENGTH + 1];
    std::string stilMD5 = sidEngine.p_song->createMD5New(md5);
    std::string stilLine;
    std::string stilBuffer;
    BOOL stilFetch = FALSE;
    
    std::ifstream f(stilName.c_str());
    if (f.good()) {
        while(getline(f, stilLine)) {
            if (stilFetch) {
                if (stilLine.empty())
                    break;
                
                stilBuffer.append(stilLine);
                stilBuffer.append("\n");
            } else if (stilLine.find(stilMD5) != std::string::npos && ((stilLine[0] == 'I' && type == 'G') || (stilLine[0] != 'I' && type == 'E') || (type == 'B'))) {
                stilFetch = TRUE;
            }
        }
        f.close();
    }
    
    if (stilFetch) {
        int stilLength = stilBuffer.size()+1;
        //char *stilInfo = (char*)malloc(stilLength);
        char *stilInfo = (char*)xmpfmisc->Alloc(stilLength);
        memcpy(stilInfo, stilBuffer.data(), stilLength);
        return stilInfo;
    } else {
        return NULL;
    }
}
static void formatSTILbase(const char * stilData, char **buf) {
    if (stilData != NULL) {
        std::istringstream stilDatastr(stilData);
        std::string labetTxt;
        std::string stilOutput;
        while (std::getline(stilDatastr, stilOutput)) {
            if (!stilOutput.empty()) {
                stilOutput = trim(stilOutput);
                labetTxt = "";
                if (stilOutput.find("COMMENT: ") != -1) {
                    stilOutput.replace(stilOutput.find("COMMENT: "), 9, "");
                    labetTxt = "Comment";
                } else if (stilOutput.find("AUTHOR: ") != -1) {
                    stilOutput.replace(stilOutput.find("AUTHOR: "), 8, "");
                    labetTxt = "Author";
                } else if (stilOutput.find("ARTIST: ") != -1) {
                    stilOutput.replace(stilOutput.find("ARTIST: "), 8, "");
                    labetTxt = "Artist";
                } else if (stilOutput.find("TITLE: ") != -1) {
                    stilOutput.replace(stilOutput.find("TITLE: "), 7, "");
                    labetTxt = "Title";
                } else if (stilOutput.find("NAME: ") != -1) {
                    stilOutput.replace(stilOutput.find("NAME: "), 6, "");
                    labetTxt = "Name";
                } else if (stilOutput.find("BUG: ") != -1) {
                    stilOutput.replace(stilOutput.find("BUG: "), 5, "");
                    labetTxt = "Bug";
                }
                char *value = xmpftext->Utf8(stilOutput.c_str(), -1);
                *buf = xmpfmisc->FormatInfoText(*buf, labetTxt.c_str(), value);
                xmpfmisc->Free(value);
            } else {
                break;
            }
        }
    }
}
// initialise the plugin
#ifdef debugplayer
void OpenConsole()//wgc
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	printf("\nSidEx v*.** Debug Console:\n\n");
	//printf(disclaimer);



}

#endif

void initallplayer()
{
	{
		if (sidEngine.b_loaded)
		{
			delete sidEngine.m_builder;
			delete sidEngine.m_engine;
			sidEngine.b_loaded = FALSE;
		}
		if (!sidEngine.b_loaded)

		//OpenConsole();//wgc
		if (!sidEngine.b_loaded)
		{
			// set default config
			loadConfig();

			//test remove
			sidEngine.m_config.forceSecondSidModel = -1; //wgc
			sidEngine.m_config.secondSidAddress = 0xd400;
			sidEngine.m_config.secondSidModel = -1;
			sidEngine.m_config.forceSidModel = -1;
			//TO CHANGE !!!!!!! ZR//WGC

			if (sidSetting.c_surround && (sidEngine.m_config.secondSidAddress == 0 || sidEngine.m_config.secondSidAddress == 0xd400))//wgc
			{

				sidEngine.m_config.secondSidAddress = 0xd400;
				surround = 1;
				//setsidchannelssurround();
			}
			else
			{
				sidEngine.m_config.secondSidAddress = 0xd400;

				surround = 0;
// 				VOICE_1SID[0] = VOICE_1SIDsave[0]; // wgc
// 				VOICE_1SID[1] = VOICE_1SIDsave[1];
// 				VOICE_1SID[2] = VOICE_1SIDsave[2];
// 				VOICE_2SID[0] = VOICE_2SIDsave[0];
// 				VOICE_2SID[1] = VOICE_2SIDsave[1];
// 				VOICE_2SID[2] = VOICE_2SIDsave[2];
// 				VOICE_3SID[0] = VOICE_3SIDsave[0];
// 				VOICE_3SID[1] = VOICE_3SIDsave[1];
// 				VOICE_3SID[2] = VOICE_3SIDsave[2];
			}


			// initialise the engine
			
			sidEngine.m_engine = new sidplayfp();
			
			sidEngine.m_engine->setRoms(kernel, basic, chargen);

			 

			sidEngine.m_builder = new ReSIDfpBuilder("ReSIDfp");
			sidEngine.m_builder->create(sidEngine.m_engine->info().maxsids());
			if (!sidEngine.m_builder->getStatus()) {
				delete sidEngine.m_engine;
				sidEngine.b_loaded = FALSE;
			}
			else {
				// apply configuraton
				if (applyConfig(TRUE)) {
					sidEngine.b_loaded = TRUE;
				}
				else {
					delete sidEngine.m_builder;
					delete sidEngine.m_engine;
					sidEngine.b_loaded = FALSE;
				}
			}
		}
	}
}


float gain_scale(float gain/*, bool preamp*/)
{
// 	if (preamp) {
// 		return (9.9999946497217584440165E-01 *
// 			exp(6.9314738656671842642609E-02 * gain)
// 			+ 3.7119444716771825623636E-07);
// 	}
// 	else 
	{
		return (2.5220207857061455181125E-01 *
			exp(8.0178361802353992349168E-02 * gain)
			- 2.5220207852836562523180E-01);
	}
}

static void WINAPI SIDex_Init()
{


		init_equliazer(0);//wgc
		set_eq_value(-2, -1, 0); set_eq_value(-2, -1, 1);//gain

		//float equalizerl[10] = { 6.1 * 2.0,3.7 * 2.0,0.0,2.7,0.0,2.7,0.0,2.7,-2.7,2.7 };
		//float equalizerr[10] = { 6.1 * 2.0,3.7 * 2.0,2.7,0.0,2.7,0.0,2.7,0.0,0.0 ,0.0 };

		for (int si = 0; si <= 10; si++) {

			//set_eq_value(gain_scale(equalizerl[si]*4), si, 0); set_eq_value(gain_scale(equalizerr[si]*4), si, 1);
			set_eq_value((equalizerl[si] + 2), si, 0); set_eq_value((equalizerr[si] + 2), si, 1);
			

	}
	initallplayer();
}

// general purpose
char aboutinfo[] = {
	"XMPlay " SIDEXNAME " plugin (" SIDEXVERSION ")\n"
	"Copyright (c) " DATEXVAR " William Cronin & Nathan Hindley\n\n"
	"This plugin allows XMPlay to load/play sid files with libsidplayfp-" LIBSIDPLAYVERSION ".\n\n"
	"Additional Credits:\n"
	"Copyright(c) 2000 - 2001 Simon White\n"
	"Copyright(c) 2007 - 2010 Antti Lankila\n"
	"Copyright(c) 2010 - " DATEXVAR " Leandro Nini <drfiemost@users.sourceforge.net>\n"
	"FREE FOR USE WITH XMPLAY\n\n"

};
static void WINAPI SIDex_About(HWND win)
{
    MessageBoxA(win, 
		aboutinfo,
 		"About...",
            MB_ICONINFORMATION);
}
static BOOL WINAPI SIDex_CheckFile(const char *filename, XMPFILE file)
{
    DWORD sidHeader;
    xmpffile->Read(file,&sidHeader,4);
    return sidHeader==MAKEFOURCC('P','S','I','D') || sidHeader==MAKEFOURCC('R','S','I','D');
}
static DWORD WINAPI SIDex_GetFileInfo(const char *filename, XMPFILE file, float **length, char **tags)
{
    SidTune* lu_song;
    const SidTuneInfo* lu_songinfo;
    int lu_songcount;

    lu_song = new SidTune(filename);
    if (!lu_song->getStatus()) {
        delete lu_song;
        return 0;
    }

    lu_songinfo = lu_song->getInfo();
    lu_songcount = lu_songinfo->songs();
        
    if (length) {
        // load lengths
        loadSonglength();
        *length = (float*)xmpfmisc->Alloc(lu_songcount * sizeof(float));
        for (int si=1;si<=lu_songcount; si++) {
            (*length)[si - 1] = fetchSonglength(lu_song, si);
        }
    }

    if (tags)
        *tags = GetTags(lu_songinfo);

    delete lu_song;

    return lu_songcount | XMPIN_INFO_NOSUBTAGS;
}
static DWORD WINAPI SIDex_GetSubSongs(float *length) {
    *length = sidEngine.p_songlength;
    return sidEngine.p_songcount;
}
static char * WINAPI SIDex_GetTags()
{
    return GetTags(sidEngine.p_songinfo);
}
static void WINAPI SIDex_GetInfoText(char *format, char *length)
{
    if (format) {
        if (strlen(sidEngine.p_sididplayer)>0)
            sprintf(format, "%s - %s", simpleFormat(sidEngine.p_songinfo->formatString()), sidEngine.p_sididplayer);
        else
            sprintf(format, "%s", simpleFormat(sidEngine.p_songinfo->formatString()));
    }
    if(length) {
        if (length[0]) // got length text in the buffer, append to it
            sprintf(length + strlen(length), " - %s - %s", sidEngine.p_sidmodel, sidEngine.p_clockspeed);
        else
            sprintf(length, "%s - %s", sidEngine.p_sidmodel, sidEngine.p_clockspeed);
    }
}
static void WINAPI SIDex_GetGeneralInfo(char *buf)
{
    static char temp[32]; // buffer for simpleLength

    buf += sprintf(buf, "%s\t%s\r", "Format", sidEngine.p_songinfo->formatString());
    if (strlen(sidEngine.p_sididplayer)>0)
        buf += sprintf(buf, "%s\t%s\r", "Player", sidEngine.p_sididplayer);
    
    if (sidEngine.p_songcount > 1) { // only display subsong info if there are more than 1
        buf += sprintf(buf, "Subsongs");
        for (int si = 1; si <= sidEngine.p_songcount; si++) {
            buf += sprintf(buf, "\t%d. %s\r", si, simpleLength(sidEngine.p_subsonglength[si], temp));
        }
    }
    
    buf += sprintf(buf, "%s\t%s\r", "Length", simpleLength(sidEngine.p_songlength, temp));
    buf += sprintf(buf, "%s\t%s%s\r", "Library", "libsidplayfp-",LIBSIDPLAYVERSION);
}
static void WINAPI SIDex_GetMessage(char *buf)
{
    // Load STIL database
    loadSTILbase();

	const char * stilComment;//wgc
	const char * stilEntry;//wgc
	const char * stilBug;//wgc
	stilComment = NULL;//wgc
	stilEntry = NULL;//wgc
	stilBug = NULL;//wgc
    if (sidEngine.d_loadedstil || sidEngine.d_loadedstilmd5) {

        
        // txt stil lookup
        if (sidEngine.d_loadedstil) {
            std::string searchPath;
                searchPath.append(sidEngine.p_songinfo->path());
                searchPath.append(sidEngine.p_songinfo->dataFileName());
            stilComment = sidEngine.d_stilbase.getAbsGlobalComment(searchPath.c_str());
            stilEntry = sidEngine.d_stilbase.getAbsEntry(searchPath.c_str(),0,STIL::all);
            stilBug = sidEngine.d_stilbase.getAbsBug(searchPath.c_str(),sidEngine.p_subsong);
        }
        
        // md5 stil lookup

        if (sidEngine.d_loadedstilmd5) {
            if (stilComment == NULL) { stilComment = fetchSTILmd5('G'); }
            if (stilEntry == NULL) { stilEntry = fetchSTILmd5('E'); }
            if (stilBug == NULL) { stilBug = fetchSTILmd5('B'); }
        }
        
        if (stilComment != NULL) {
            buf += sprintf(buf, "STIL Global Comment\t-=-\r");
            formatSTILbase(stilComment,&buf);
            buf += sprintf(buf, "\r");
        }
        if (stilEntry != NULL) {
            buf += sprintf(buf, "STIL Tune Entry\t-=-\r");
            formatSTILbase(stilEntry,&buf);
            buf += sprintf(buf, "\r");
        }
        if (stilBug != NULL) {
            buf += sprintf(buf, "STIL Tune Bug\t-=-\r");
            formatSTILbase(stilBug,&buf);
            buf += sprintf(buf, "\r");
        }
    }
}

// handle playback
#define CLICK_ATTENUATION
int attenuate = 0;//wgc
int _target_odometer = 0;//set later in SIDex_Open
int _origtarget_odometer = 0;
int _odometer = 0; //set later in SIDex_Open
int timertomute = 0;//set later in SIDex_Open
int _muting = 1, _mute_size = _target_odometer;
int _attenuation_length = 0;// set later in sidex_open    (rate * channels * ms) / 1000 MsToSamples(25, _sample_format);
int _fadein_length = 44100 * 0.1;// MsToSamples(100, _sample_format);
//int _mute_size = _target_odometer; // start with complete song mute
int _delay = 0;
int MsToSamples(int rate, int channels, int ms)
{
	int ret = (rate * channels * ms) / 1000;
	return ret;
}
extern int chips;
void reset_surrounddelay()
{
	_delay = ((sidEngine.m_config.frequency * 0.015));

	if (SDELAY > 0)		_delay = SDELAY;
//	if (SDELAY > 0)		_delay = (sidEngine.m_config.frequency * (SDELAY*0.0001));


	if (_delay % 2 == 0) {}	else _delay += 1;
}

char chiptype1[5] = { 0 };
wchar_t initinfo[200] = { 0 };
wchar_t initinfochiptype[100] = { 0 };
static DWORD WINAPI SIDex_Open(const char *filename, XMPFILE file)
{
	NOSOUNDTIMER = 0; //reset nosound check to zero
	SKIPTONEXTTUNE = 0; // do not skip to next tune .. wait for no sound check to say .. no sound ie sfx etc with default length in time
	allownosoundcheck = 0;
	eq_agc[0] = 1; eq_agc[1] = 1; // part of equalizer
	attenuate = 0;//wgc
	haltplayer = 0; // allows the replay loop to start
	for (int i=0;i<9;i++)
		voices[i] = 1;

	//HWND abc;
	//xmpfmisc->GetWindow(abc);
	
				// hack for simulation of stereo effect when dual mono sids are active and allow second sid is enabled
	// will reduce to 0 then surround will be off ( sets surround to enable second sid chip effect for 1 sid )
	if (SURROUNDSEPERATION && !real2sid && !real3sid && !sidSetting.c_surround)//WGCCHANGE
	{
		if (SURROUNDSEPFORCE)//WGCCHANGE
		{
			voices[1] = 0.95;	voices[3] = 0.95;	voices[5] = 0.95;
			surroundcount = 10;
		}
	}

	if (!SURROUNDSEPERATION)//WGCCHANGE
	{
		voices[1] = 1;	voices[3] = 1;	voices[5] = 1; // ALLOWS ALL VOICES AT FULL VOLUME
	}

	if ((real2sid || real3sid) && sidSetting.c_surround)//WGCCHANGE
	{
		if (SURROUNDSEPFORCE)//WGCCHANGE
		{
			voices[1] = 1;	voices[3] = 1;	voices[5] = 1;
			surroundcount = 10;
		}
	}

	softlmax = sofldefault;//soft limiter reset on new tune
	reset_surrounddelay(); // resets or sets surround delay if its changed
	
    SIDex_Init();

    if (sidEngine.b_loaded) {
        sidEngine.p_song = new SidTune(filename);
        if (!sidEngine.p_song->getStatus()) {
            return 0;
        } else {
			if (sidEngine.fadein)//wgc
				_buffer_fill = 0;
			
            sidEngine.p_songinfo = sidEngine.p_song->getInfo();
			if (!sidSetting.c_locksidmodel)
			{
				if (sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_8580) _8580VOLUMEBOOST = 1; else _8580VOLUMEBOOST = 0;
// 				char ff[100] = "";
// 				sprintf(ff, "%i", _8580VOLUMEBOOST);
// 				MessageBoxA(0, ff, "", 0);
			}
			//printf("boost %i\n", _8580VOLUMEBOOST);
			if (sidEngine.p_song)
			{
				sidEngine.p_songinfo = sidEngine.p_song->getInfo();
				if (sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_8580)
					wsprintf(initinfochiptype, L"Chip ORIG Model: 8580");
				else
					wsprintf(initinfochiptype, L"Chip ORIG Model: 6581");
				

				wsprintf(initinfo, L"Load: $%.4X Init: $%.4X Play: $%.4X\nPlay Speed: %.2i Sid Chips used: %.2i\nSize on c64 disk: %i Block(s) \n%.3i BLOCKS FREE.\n", 
					sidEngine.p_songinfo->loadAddr(), 
					sidEngine.p_songinfo->initAddr(), 
					sidEngine.p_songinfo->playAddr(),
					sidEngine.p_songinfo->songSpeed()+1,
					sidEngine.p_songinfo->sidChips(),
					sidEngine.p_songinfo->dataFileLen() / 254, // 256 ? debate on size of 1 block , so taking the most common value as first 2 bytes are allocated for next blk
					664 -(sidEngine.p_songinfo->dataFileLen() / 254)
				);

			}

			//sprintf(sidEngine.p_sidmodel, "%i", sidEngine.p_songinfo->sidModel(0));
 			//MessageBoxA(0, sidEngine.p_sidmodel, 0, MB_ICONEXCLAMATION); 
			sidEngine.p_songcount = sidEngine.p_songinfo->songs();
            sidEngine.p_subsong = 1;
            sidEngine.p_song->selectSong(sidEngine.p_subsong);

            // Figure out sid model and clock speed
            if (sidEngine.m_config.forceSidModel || sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_ANY || sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_UNKNOWN) {
                if (sidEngine.m_config.defaultSidModel == SidConfig::MOS6581) {
                    sprintf(sidEngine.p_sidmodel,"%s","6581");//wgc//sidEngine.p_sidmodel = "6581";
                } else if (sidEngine.m_config.defaultSidModel == SidConfig::MOS8580) {
					sprintf(sidEngine.p_sidmodel, "%s", "8580");//sidEngine.p_sidmodel = "8580";
  
              }
            } else if (sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_6581) {
				sprintf(sidEngine.p_sidmodel, "%s", "6581");//wgc//sidEngine.p_sidmodel = "6581";
            } else if (sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_8580) {
				sprintf(sidEngine.p_sidmodel, "%s", "8580");//sidEngine.p_sidmodel = "8580";
            }

			if (sidEngine.m_config.forceC64Model || sidEngine.p_songinfo->clockSpeed() == SidTuneInfo::CLOCK_ANY || sidEngine.p_songinfo->clockSpeed() == SidTuneInfo::CLOCK_UNKNOWN) {
                if (sidEngine.m_config.defaultC64Model == SidConfig::PAL) {
					sprintf(sidEngine.p_clockspeed, "%s", "PAL");//wgc//sidEngine.p_clockspeed = "PAL";
                } else if (sidEngine.m_config.defaultC64Model == SidConfig::NTSC) {
					sprintf(sidEngine.p_clockspeed, "%s", "NTSC");//wgc//sidEngine.p_clockspeed = "NTSC";
                }
            } else if (sidEngine.p_songinfo->clockSpeed() == SidTuneInfo::CLOCK_PAL) {
				sprintf(sidEngine.p_clockspeed, "%s", "PAL");//wgc//sidEngine.p_clockspeed = "PAL";
            } else if (sidEngine.p_songinfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC) {
				sprintf(sidEngine.p_clockspeed, "%s", "NTSC");//wgc//sidEngine.p_clockspeed = "NTSC";
            }
#ifdef debugplayer
			printf("---------------------------------------------------------------\n");
			printf("SidTuneInfo::SIDMODEL_6581 : %i\n", SidTuneInfo::SIDMODEL_6581);
			printf("SidTuneInfo::SIDMODEL_8580 : %i\n", SidTuneInfo::SIDMODEL_8580);
			printf("---------------------------------------------------------------\n");
			printf("sidEngine.m_config.forceSidModel : %i\n", sidEngine.m_config.forceSidModel);
			printf("sidEngine.m_config.defaultSidModel : %i\n", sidEngine.m_config.defaultSidModel);
			printf("sidEngine.p_songinfo->sidModel(0) : %i\n", sidEngine.p_songinfo->sidModel(0));
			printf("sidEngine.p_songinfo->sidModel(1) : %i\n", sidEngine.p_songinfo->sidModel(1));
			printf("sidEngine.p_songinfo->sidModel(2) : %i\n", sidEngine.p_songinfo->sidModel(2));
			printf("sidEngine.p_songinfo->clockSpeed() : %i\n", sidEngine.p_songinfo->clockSpeed());
			printf("sidEngine.m_config.forceSecondSidModel : %i\n", sidEngine.m_config.forceSecondSidModel);
			printf("sidEngine.m_config.secondSidModel : %i\n", sidEngine.m_config.secondSidModel);
			printf("sidEngine.p_songinfo->sidModel(0) : %i\n", sidEngine.p_songinfo->sidModel(0));
			printf("sidEngine.p_songinfo->sidModel(1) : %i\n", sidEngine.p_songinfo->sidModel(1));
			printf("sidEngine.p_songinfo->sidModel(2) : %i\n", sidEngine.p_songinfo->sidModel(2));
			
			printf("---------------------------------------------------------------\n");
			printf("real3sid : %i\n", real3sid);
			printf("real2sid : %i\n", real2sid);
			printf("surround : %i\n", surround);
			printf("sidChipBase(0) : 0x%.4X\n", sidEngine.p_songinfo->sidChipBase(0));
			printf("sidChipBase(1) : 0x%.4X\n", sidEngine.p_songinfo->sidChipBase(1));
			printf("sidChipBase(2) : 0x%.4X\n", sidEngine.p_songinfo->sidChipBase(2));
			printf("chips : %i\n", chips);


#endif


            // detect player
            if (sidSetting.c_detectplayer && !sidEngine.d_loadedsidid && strlen(sidSetting.c_dbpath)>10) {
                std::string relpathName = sidSetting.c_dbpath;
                relpathName.append("sidid.cfg");
                sidEngine.d_loadedsidid = sidEngine.d_sididbase.readConfigFile(relpathName);
            }
            if (sidEngine.d_loadedsidid) {
                // adapted sidid code, could be janky
                std::string c64player;
                //uint_least8_t c64buf[256];//wgc//
				uint_least8_t *c64buf = new uint_least8_t [xmpffile->GetSize(file)];//wgc
                xmpffile->Read(file,c64buf,xmpffile->GetSize(file));
                std::vector<uint_least8_t> buffer(&c64buf[0], &c64buf[xmpffile->GetSize(file)]);//wgc
                c64player = sidEngine.d_sididbase.identify(buffer);
                while (c64player.find("_") != -1)
                    c64player.replace(c64player.find("_"), 1, " ");
                
                memcpy(sidEngine.p_sididplayer, c64player.c_str(), 25);
            }
            
            // load lengths
            loadSonglength();
            sidEngine.p_subsonglength = new int [sidEngine.p_songcount + 1];
            sidEngine.p_songlength = 0;
            for (int si=1;si<=sidEngine.p_songcount; si++) {
                int defaultduration = fetchSonglength(sidEngine.p_song,si);
                sidEngine.p_subsonglength[si] = defaultduration;
                sidEngine.p_songlength += defaultduration;
            }

            if (sidEngine.m_engine->load(sidEngine.p_song)) {
                if (sidSetting.c_defaultlength == 0 || sidSetting.c_disableseek) {
                    xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], FALSE);

					_target_odometer = sidEngine.p_subsonglength[sidEngine.p_subsong] *60 * 60 * 1000;//wgc
					_odometer = 0;
                } else {
                    xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], TRUE);

					_target_odometer = sidEngine.p_subsonglength[sidEngine.p_subsong] * 60 * 60 * 1000;//wgc
					_odometer = 0;
                }
				

				_origtarget_odometer = _target_odometer;

				_click_wa = 1;									// wgc
				_muting = 1;
				_mute_size =			_target_odometer;					// start with full song muted
				_attenuation_length =	MsToSamples(sidEngine.m_config.frequency, sidEngine.m_config.playback,25);	// MsToSamples(25, _sample_format);
				_fadein_length =		MsToSamples(sidEngine.m_config.frequency , sidEngine.m_config.playback, 50);	// MsToSamples(100, _sample_format);
                sidEngine.fadein = 0;								// trigger fade-in
                sidEngine.fadeout = 1;								// trigger fade-out//
				memset(_decode_buffer, 0, 44100 * sizeof(float));
				fadevolume = 0;
				timertomute = 4;// ((sidEngine.m_config.frequency * sidEngine.m_config.playback)* 0.25);

				if ((real2sid || real3sid))//WGCCHANGE
					surround = sidSetting.c_surround2sid;

				if (!real2sid && !real3sid)//WGCCHANGE
					surround = sidSetting.c_surround;

                return 2;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
}
static void WINAPI SIDex_Close()
{
    if (sidEngine.p_song) {
        if (sidEngine.m_engine->isPlaying()) {
            sidEngine.m_engine->stop();
        }
        delete sidEngine.p_subsonglength;
        delete sidEngine.p_sididplayer;
        delete sidEngine.p_song;
    }
	//nosounddetected = 1;
}
bool is_bad_ptr(uint_least32_t* ptr)//wgc
{
	__try
	{
		volatile auto result = *ptr;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return true;
	}
	return false;
}



uint32_t find_sound(float *buffer, uint32_t size)//wgc
{
	for (uint32_t j = 0; j < size; j++)
	{
		if (abs(buffer[j]) > 0.05f)
		{
			return j & ~1;
		}
	}

	return -1;
}
uint32_t has_sound(float *buffer, uint32_t size)//wgc
{
	static int ret;
	ret = 0;
	for (uint32_t j = 0; j < size; j++)
	{
		if (abs(buffer[j]) > 0.01f)
		{
			ret++;
		}
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////

						//If number of samples is 200, then step size if 360/200 = 1.8
int ci = 0;
int numberofsamples = 16/2;
float sinwavefilter(float beta, float alpha)
{
	//	double M_PI = 3.14159;
	float apex = 0;
	float apexi = 360 / numberofsamples;
	//ofstream outfile;
	//outfile.open("data.dat",ios::trunc | ios::out);
	//for(int i=0;i<201;i++)
	ci++;
	if (ci > numberofsamples + 1) ci = 0;

	{
		float rads = M_PI / 180;
		//outfile << 
		apex = (float)(beta*sin(apexi*ci*rads) + alpha);// << endl;
	}
	//outfile.close();
	return apex;
}

///////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

						//If number of samples is 200, then step size if 360/200 = 1.8
int xci = 0;
int xnumberofsamples = numberofsamples/2;
float sinwavefilterr(float beta, float alpha)
{
	//	double M_PI = 3.14159;
	float apex = 0;
	float apexi = 360 / xnumberofsamples;
	//ofstream outfile;
	//outfile.open("data.dat",ios::trunc | ios::out);
	//for(int i=0;i<201;i++)
	xci++;
	if (xci > xnumberofsamples + 1) ci = 0;

	{
		float rads = M_PI / 180;
		//outfile << 
		apex = (float)(beta*sin(apexi*xci*rads) + alpha);// << endl;
	}
	//outfile.close();
	return apex;
}

///////////////////////////////////////////////
extern double _6581_mod_filter[];
static DWORD WINAPI SIDex_Process(float *buffer, DWORD size)
{
	
	
	if (!sidEngine.m_engine)
		return 0;
    // skip short song
    if (sidSetting.c_skipshort && sidSetting.c_minlength >= sidEngine.p_subsonglength[sidEngine.p_subsong]) {
         return 0;
    }

 	if (surround) //wgc
 		setsidchannelssurround();



	if (!size || !sidEngine.m_engine || !is_bad_ptr( (uint_least32_t*)(sidEngine.m_engine->time()) )    )//wgc
	return 0;

	if (sidEngine.m_builder && SURROUNDSEPERATION)
	{
		float perf = 0.05;
		float badbp = (float)sinwavefilter(-1, 0.5);
		if (badbp < -1.)badbp = -1.; if (badbp > 1.)badbp = 1.;
		_6581_mod_filter[0] = badbp* perf;
		    badbp = (float)sinwavefilterr(-1, 0.5);
		if (badbp < -1.)badbp = -1.; if (badbp > 1.)badbp = 1.;
		_6581_mod_filter[1] = badbp* perf;
		sidEngine.m_builder->filter6581Curve((float)sidSetting.c_6581filter/100);
	}
	if (sidEngine.m_builder && !SURROUNDSEPERATION)
	{
		_6581_mod_filter[0] = -0.015;
		_6581_mod_filter[1] = 0.015;
	}

    // process

    if (sidEngine.m_engine->time() < sidEngine.p_subsonglength[sidEngine.p_subsong] || sidSetting.c_defaultlength == 0) {
				

		// set-up fade-in & fade-out // wgc
		float fadestep;
		if (sidEngine.fadein < 1) {
			if (sidSetting.c_fadein && sidSetting.c_fadeinms > 0) {
				if (!sidEngine.fadein) sidEngine.fadein = 0.001;
				fadestep = pow(10, 3000.0 / sidSetting.c_fadeinms / (sidEngine.m_config.frequency * sidEngine.m_config.playback));
			}
			else
				sidEngine.fadein = 1;
		}
		else if (sidEngine.fadeout > 0) {
			if (sidSetting.c_fadeout && sidSetting.c_fadeoutms > 0) {
				sidEngine.fadeouttrigger = sidEngine.p_subsonglength[sidEngine.p_subsong] - sidSetting.c_fadeoutms / 1000; // calc trigger fade-out
				if (sidEngine.fadeouttrigger + 1 > 0) {
					if (sidEngine.fadeout == 1) sidEngine.fadeout = 0.999;
					//fadestep = pow(10, 3000.0 / sidSetting.c_fadeoutms / (sidEngine.m_config.frequency * sidEngine.m_config.playback));
					fadestep = 1.0 / (float)((sidSetting.c_fadeoutms / 1000) * sidEngine.m_config.frequency * sidEngine.m_config.playback);
				}
				else
					sidEngine.fadeout = 0;
			}
			else
				sidEngine.fadeout = 0;
		}

		uint32_t needed = size + _delay - _buffer_fill;

		short *_16bit_buffer = new short[needed];

		// convert 16bit buffer to float
		uint32_t count = sidEngine.m_engine->play(_16bit_buffer, needed);

		for (int i = 0; i < count; i++)
			_decode_buffer[_buffer_fill + i] = ((float)_16bit_buffer[i]) / 32768.0f;

		

		_buffer_fill += count;

		if (_buffer_fill < size)
			size = _buffer_fill;

		//if ((_sample_format.channels == 2) && _surround)
		if (surround)
		{
#ifdef CLICK_ATTENUATION
			if (_click_wa && _muting)
			{
				uint32_t sound_pos = find_sound(&_decode_buffer[0], size);
				if (sound_pos != -1)
				{
					_muting = false;
					_mute_size = sound_pos;
				}

				if (_mute_size > size)
					memset(buffer, 0, size * sizeof(float));
				else
				{
					memset(buffer, 0, _mute_size * sizeof(float));

					for (uint32_t i = _mute_size; i < size; i += 2)
					{
						buffer[i] = (_decode_buffer[i + _mute_size + _delay] + _decode_buffer[i + _mute_size + 1]) / 1.414f;
						buffer[i + 1] = (_decode_buffer[i + _mute_size + _delay + 1] + _decode_buffer[i + _mute_size]) / 1.414f;
					}

					_mute_size += _odometer;
				}
			}
			else
#endif // CLICK_ATTENUATION
			{
				for (uint32_t i = 0; i < size; i += 2)
				{
					buffer[i] = (_decode_buffer[i + _delay] + _decode_buffer[i + 1]) / 1.414;
					buffer[i + 1] = (_decode_buffer[i + _delay + 1] + _decode_buffer[i]) / 1.414f;
				}
			}
		}
		else
		{
#ifdef CLICK_ATTENUATION
			if (_click_wa && _muting)
			{
				uint32_t sound_pos = find_sound(&_decode_buffer[_delay], size);
				if (sound_pos != -1)
				{
					_muting = false;
					_mute_size = sound_pos;
				}

				if (_mute_size > size)
					memset(buffer, 0, size * sizeof(float));
				else
				{
					memset(buffer, 0, _mute_size * sizeof(float));
					memcpy(buffer + _mute_size, &_decode_buffer[_delay + _mute_size], (size - _mute_size) * sizeof(float));
					_mute_size += _odometer;
				}
			}
			else
#endif // CLICK_ATTENUATION
				memcpy(buffer, &_decode_buffer[_delay], size * sizeof(float));
		}

		_buffer_fill -= size;
		memmove(_decode_buffer, _decode_buffer + size, _buffer_fill * sizeof(float));

#ifdef CLICK_ATTENUATION
		//apply anti-click muting/fading
		if (_click_wa && !_muting)
		{
			uint32_t dist = ((_mute_size >= _odometer) ? (_mute_size - _odometer) : 0);
			uint32_t t = dist;

			if (abs(long(_mute_size - _odometer)) <= _attenuation_length)
			{
				for (; (t < size) && (_odometer + t - _mute_size < _attenuation_length); t++)
					buffer[t] = 0;
			}

			if (abs(long(_mute_size + _attenuation_length - _odometer)) <= _fadein_length)
			{
				for (; (t < size) && (_odometer + t - _attenuation_length - _mute_size < _fadein_length); t++)
					buffer[t] *= (float)(_odometer + t - _attenuation_length - _mute_size) / _fadein_length;
			}
		}
#endif // CLICK_ATTENUATION

		//apply fadeout
		uint32_t left = _target_odometer - _odometer;
		if (left < 4)//_fadeout_length);//*60*60*1000
		{
			for (uint32_t i = 0; i < size; i++)
				buffer[i] *= (float)(left - i) / 4;// _fadeout_length;
		}

		//check for stream end
		if (_odometer + size > _target_odometer)
		{
			size = _target_odometer - _odometer;
			_odometer = _target_odometer;
		}
		else
			_odometer += size;

		//if (!playsound)fadevolume = 0;
// 		for (uint32_t i = 0; i < size; ++i)
// 		{
// 				if (fadevolume < 1)fadevolume += 1./(44100*2);				if (fadevolume > 1)fadevolume = 1;
// 				buffer[i] *= fadevolume;
// 		}
		//
		//int sidDone, i;
// 		if (_odometer> 60 * 1000)
// 		if (has_sound(buffer, size) < 5)
// 		{
// 
// 			sidEngine.fadein = 0; // trigger fade-in (needed?)
// 			sidEngine.fadeout = 1; // trigger fade-out (needed?)
// 			xmpfin->UpdateTitle(NULL);
// 			_odometer = 0;
// 			_target_odometer = 4096;// _origtarget_odometer;
// 		}


		

		if (SURROUNDSEPERATION && !real2sid && !SURROUNDEMP)//wgc
		for (int i = 0; i < size; i+=2) {
			static float coef = 1;

			//StereoEnhanca(buffer[i], buffer[i+1],1,1); // stereo only
			StereoEnhanca(buffer[i], buffer[i+1], 1.0,0.5); // enhanced stereo 
			buffer[i] = brickwall(_StereoEnhanca[0] ,1);
			buffer[i+1] = brickwall(_StereoEnhanca[1],1);
		}
		//sidDone = sidEngine.m_engine->play(sidbuffer, count);
		for (int i = 0; i < size; i++) {
			float scale = 1;// / 32768.f;
			// perform fade-in & fade-out
			if (sidEngine.fadein < 1) {
				sidEngine.fadein *= fadestep;
				if (sidEngine.fadein > 1) sidEngine.fadein = 1;
				scale *= sidEngine.fadein;
			}
			else if (sidEngine.fadeout > 0 && sidEngine.m_engine->time() > sidEngine.fadeouttrigger) {
				//sidEngine.fadeout /= sidEngine.fadeoutstep;
				sidEngine.fadeout -= fadestep;
				if (sidEngine.fadeout < 0) sidEngine.fadeout = 0;
				scale *= sidEngine.fadeout;
			}
			//if (GetAsyncKeyState(VK_LSHIFT))
			buffer[i] *= scale;
		}
		//
		
		if (BANDEQ)//WGC
		{
			//if (GetAsyncKeyState(VK_LSHIFT))
			if (size)
				do_equliazer(buffer, size * 2, sidEngine.m_config.frequency, 2);//wgcchange
		}

// 		//if (playsound)
// 		{
// 			if (timertomute > 0)				timertomute--;
// 			else
// 			{
// 				//fadevolume *= 2;
// 				if (fadevolume < 1)fadevolume += 0.5;				if (fadevolume > 1)fadevolume = 1;
// 
// 			}
// 		}
		delete _16bit_buffer;
		if (allownosoundcheck && allownosoundcheckactivate) // allow sound check if unknown tune length only
		{
			int A = has_sound(buffer, size); // check buffers for sound
			if (A == 0) NOSOUNDTIMER++;// if no sound detected timer increases
			if (A > 0) NOSOUNDTIMER = 0; //if sound detected reset count

			if (NOSOUNDTIMER > NOSOUNDTIMERCHECK) //if the Timer is above NOSOUNDTIMERCHECK then
			{
				NOSOUNDTIMER = 0; // reset the timer
				SKIPTONEXTTUNE = 1; // skip to next tune ( see the thread carrienexttunethread( ) for info )


			}
		}
		return size;
    } else {
        return 0;
    }
}
static void WINAPI SIDex_SetFormat(XMPFORMAT *form)
{

    // changing format seems to rewind the SID decoder? so only do that at start
    if (!sidEngine.m_engine->timeMs()) {
        sidEngine.m_config.frequency = std::max<DWORD>(form->rate, 8000);
		sidEngine.m_config.playback = (SidConfig::playback_t)std::min<DWORD>(form->chan, 2);
        applyConfig(FALSE);
    }
    form->rate = sidEngine.m_config.frequency;
    form->chan = sidEngine.m_config.playback;
    form->res = 16 / 8;
}
static double WINAPI SIDex_GetGranularity()
{
    return 0.001;
}
static double WINAPI SIDex_SetPosition(DWORD pos)
{

    if (pos & XMPIN_POS_SUBSONG1 || pos & XMPIN_POS_SUBSONG) {
        if (sidEngine.m_engine->isPlaying()) {
            sidEngine.m_engine->stop();
        }
        sidEngine.p_subsong = LOWORD(pos) + 1;
        sidEngine.p_song->selectSong(sidEngine.p_subsong);
        sidEngine.m_engine->load(sidEngine.p_song);
        //
        if (sidSetting.c_defaultlength == 0 || sidSetting.c_disableseek) {
            xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], FALSE);
        } else {
            xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], TRUE);
        }
        sidEngine.fadein = 0; // trigger fade-in (needed?)
        sidEngine.fadeout = 1; // trigger fade-out (needed?)
        xmpfin->UpdateTitle(NULL);
        //
        return 0;
    } else {
        double seekTarget = pos * SIDex_GetGranularity();
        double seekState = sidEngine.m_engine->timeMs() / 1000.0;
        int seekResult;

        if (seekTarget == seekState)
            return seekTarget;

        //oh dear we have to go back
        if (seekTarget < seekState) {
            if (sidEngine.m_engine->isPlaying()) {
                sidEngine.m_engine->stop();
                sidEngine.m_engine->load(0);
                sidEngine.m_engine->load(sidEngine.p_song);
            }
            seekState = 0;
        }

        //attempt to seek
        int seekCount = (int)((seekTarget - seekState) * sidEngine.m_config.frequency) * sidEngine.m_config.playback;
        short * seekBuffer = new short[seekCount];
        seekResult = sidEngine.m_engine->play(seekBuffer, seekCount);
        delete seekBuffer;
        if (seekResult == seekCount) {
            sidEngine.fadein = pos ? 1 : 0; // trigger fade-in if restarting
            if (!pos || pos < sidEngine.fadeouttrigger)
                sidEngine.fadeout = 1; // trigger fade-out if restarting
            
            return seekTarget;
        } else {
            return -1;
        }
    }
}


void Restart_p1()
{


	//HWND hwnd_xmplay = xmpfmisc->GetWindow();// FindWindowA("XMPLAY-MAIN", NULL);
	//SendMessage(hwnd_xmplay, WM_USER, 0, IPC_SETPLAYLISTPOS); //jump to ms
	//SendMessage(hwnd_xmplay, WM_USER, IPC_STARTPLAY, IPC_STARTPLAY); //jump to ms
	
	//if (sidEngine.p_song) 
	if (sidEngine.m_engine && sidEngine.m_engine->isPlaying()) {
		sidEngine.m_engine->stop();

		haltplayer = 1;//wgc
		Sleep(100);//give the player time to stop TODO should clear buffer here sometimes nasty click with left over samples

		if (sidEngine.p_song) {
			if (sidEngine.m_engine->isPlaying()) {
				sidEngine.m_engine->stop();
			}
			//delete sidEngine.p_subsonglength;
			//delete sidEngine.p_sididplayer;
			//delete sidEngine.p_song;
		}

	}
}

void Restart_p2()
{
	// this is getting technical ... 
	Sleep(100);			// give the engine time to actually stop
	initallplayer();
	Sleep(100);			// necessary? give it more time in case .. should check hasfinished then call this , TODO:
	//////
							sidEngine.fadein = 0; // trigger fade-in (needed?)
							sidEngine.fadeout = 1; // trigger fade-out (needed?)
// 							xmpfin->UpdateTitle(NULL);
							_odometer = 0;
							_origtarget_odometer = 0;
							_target_odometer = _origtarget_odometer;
	haltplayer = 1; //stops player completely // allows the replay loop to start 
// 	if (sidEngine.p_song) 
// 		if (sidEngine.m_engine->isPlaying()) {
// 			sidEngine.m_engine->stop();
// 		}
	Sleep(20);// allow time to restart
	xmpfmisc->DDE("Key81"); xmpfmisc->DDE("Key81"); Sleep(20); xmpfmisc->DDE("Key80");//restart the song .. stop .. stop ... play
	
}


// handle configuration
HWND windowconfig;
bool killthread=0;


#define MESS(id,m,w,l) SendDlgItemMessageA(hWnd,id,m,(WPARAM)(w),(LPARAM)(l))


void readeq(HWND hWnd, int a, int b)
{
	MESS(a, TBM_SETRANGE, TRUE, MAKELONG(-2000, 2000));
	MESS(a, TBM_SETPOS, TRUE, b*-1);
	MESS(a, TBM_SETPAGESIZE, 100, 100);

	//MESS(a, TBM_SETTICFREQ, 1000, 0);
	//MESS(a, TBS_AUTOTICKS, -1000, 1000);
}
void geteql(HWND hWnd, int a, int b)
{
	//MESS(a, TBM_SETRANGE, TRUE, MAKELONG(-600, 600));
	float eqvalue = SendDlgItemMessage(hWnd, a, TBM_GETPOS, 0, 0) *0.01;
	eqvalue *= -1;
	equalizerl[b] = eqvalue;

}
void geteqr(HWND hWnd, int a, int b)
{
	//MESS(a, TBM_SETRANGE, TRUE, MAKELONG(-600, 600));
	float eqvalue = SendDlgItemMessage(hWnd, a, TBM_GETPOS, 0, 0) *0.01;//
	eqvalue *= -1;
	equalizerr[b] = eqvalue;
}


void seteqlevelsonchange(int active)
{

	
	HWND hWnd = windowconfig;
	MESS(IDC_LABEL_DIGIT, WM_SETTEXT, 0, std::to_string((/*(FLOAT)*/SendDlgItemMessage(hWnd, active, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0)*-1)/10).append("%*").c_str());//.substr(0, 4).append("Db*").c_str());
	{
		for (int i = 0; i < 10; i++) {
			//if (IsWindowVisible(GetDlgItem(hWnd, eqslider[i])))
			{
				geteql(hWnd, eqslider[i], i);

				set_eq_value((equalizerl[i] + 2), i, 0);
			}
		}
		for (int i = 0; i < 10; i++) {
			//if (IsWindowVisible(GetDlgItem(hWnd, eqslider[i])))
			{
				geteqr(hWnd, eqslider[i + 10], i);

				set_eq_value((equalizerr[i] + 2), i, 1);
			}
		}
	}
	
}

DWORD WINAPI carrienexttunethread(__in LPVOID lpParameter)
{
	while (!nosounddetected) {
		// Do new thread

		Sleep(100);
		{
			if (SKIPTONEXTTUNE)
			{
				xmpfmisc->DDE("Key128");//next track
				SKIPTONEXTTUNE = 0;
			}

		}
	}

return 0;
}
DWORD WINAPI mythread(__in LPVOID lpParameter)
{
	//printf("Thread inside %d \n", GetCurrentThreadId());
	HWND hWnd = windowconfig;
	while (!killthread) {
		// Do new thread

		Sleep(100);
		{

#ifdef debugplayer



			if (GetAsyncKeyState('0') & 1)
			{
				printf( "float equalizerl[11] = {");
				for (int i = 0; i <= 10; i++)
				{
					printf("%.0f", equalizerl[i]);
					if (i < 10)
						printf(",");

				}
				printf(" };\n");

				printf("float equalizerr[11] = {");
				for (int i = 0; i <= 10; i++)
				{
					printf("%.0f", equalizerr[i]);
					if (i < 10)
						printf(",");

				}
				printf(" };\n");
			}
#endif // debugplayer
			// auto agc
			//MESS(IDC_EQUAGC, TBM_SETPOS, TRUE, (agc_maxslider*-1) * 100);
			sprintf(DEBUGAGC,"%.2f / %.2f", agc[0], agc[1]);
			MESS(IDC_LABEL_PREAMP3, WM_SETTEXT, 0, DEBUGAGC);

			if (sidSetting.c_surround2sid && (real2sid || real3sid))
				surround = 1;

			if (sidSetting.c_surround)//
			{
				surround = 1;
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE1), false);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE2), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE3), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), false);
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_SDELAY), true);
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), true);
				EnableWindow(GetDlgItem(hWnd, IDC_SVINFO), false);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE1), true);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE2), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE3), true);
				if (!real2sid)
				{
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), SURROUNDSEPERATION);//wgc
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), SURROUNDSEPERATION);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), SURROUNDSEPERATION);
				}
				else

				{
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), true);//wgc
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), true);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), true);
				}
				if (!real3sid)
				{
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), false);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), false);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), false);
				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), true);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), true);
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), true);
				}

				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_SDELAY), false);
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), false);
				EnableWindow(GetDlgItem(hWnd, IDC_SVINFO), true);

			}

			if (surround)
			reset_surrounddelay(); // resets or sets surround delay if its changed

	// 					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_SDELAY), sidSetting.c_surround);
	// 					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), sidSetting.c_surround);
						//EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), sidSetting.c_surround);

	//TO CHANGE !!!!!!! ZR//WGC

			if (!surround)
			{
				VOICE_1SIDsave[0] = VOICE_1SID[0]; // wgc
				VOICE_1SIDsave[1] = VOICE_1SID[1];
				VOICE_1SIDsave[2] = VOICE_1SID[2];
				VOICE_2SIDsave[0] = VOICE_2SID[0];
				VOICE_2SIDsave[1] = VOICE_2SID[1];
				VOICE_2SIDsave[2] = VOICE_2SID[2];
				VOICE_3SIDsave[0] = VOICE_3SID[0];
				VOICE_3SIDsave[1] = VOICE_3SID[1];
				VOICE_3SIDsave[2] = VOICE_3SID[2];
			}

			if (sidSetting.c_surround && (sidEngine.m_config.secondSidAddress == 0 || sidEngine.m_config.secondSidAddress == 0xd400))//wgc
			{

				sidEngine.m_config.secondSidAddress = 0xd400;

				surround = 1;
				setsidchannelssurround();
			}
			else
			{

				sidEngine.m_config.secondSidAddress = 0xd400;

				if (real2sid || real3sid)
					surround = sidSetting.c_surround2sid;
				else
				surround = 0;
			}
			// update info for dialog
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_MODEL), WM_SETTEXT, 0, (LPARAM)initinfochiptype);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_INIT), WM_SETTEXT, 0, (LPARAM)initinfo);

			ShowWindow(GetDlgItem(hWnd, IDC_SURROUNDSEPFORCE), SURROUNDSEPERATION);//wgc

			{
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_HPFILTER2), SURROUNDEMP);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_HPFILTER3), SURROUNDEMP);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_LPFILTER2), SURROUNDEMP);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_LPFILTER3), SURROUNDEMP);//wgc
			}
			EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_6581LEVEL), !_8580VOLUMEBOOST);//wgc
			EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_8580LEVEL), _8580VOLUMEBOOST);//wgc
			
			for (int i = 0; i <= 19; i++) 
				EnableWindow(GetDlgItem(hWnd, eqslider[i]), BANDEQ);//wgc
			EnableWindow(GetDlgItem(hWnd, IDC_BANDGROUPBOX), BANDEQ);//wgc
			

		}
	}
	return 0;
}


void timeron(HWND hwndx)
{
	HANDLE myhandle;
	DWORD mythreadid;
	windowconfig = hwndx;
	killthread = 0;
	myhandle = CreateThread(0, 0, mythread, 0, 0, &mythreadid);
}
void timeroff(HWND hwndx)
{
	killthread = 1;
}



#include <tchar.h>
void GetInputFile()
{
	OPENFILENAME ofn = { 0 };
	TCHAR szFileName[MAX_PATH] = _T("");

	ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
	ofn.hwndOwner = 0;
	ofn.lpstrFilter = _T("Songlength Files (*.md5)\0*.md5\0All Files (*.*)\0*.*\0\0");
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = _T("txt");

	if (GetOpenFileName(&ofn)) //
	{
		//MessageBox(NULL, szFileName, _T("name!"), MB_OK);

		std::wstring tmpWString(szFileName);
		std::string result(tmpWString.begin(), tmpWString.end());
		result = result.substr(0, result.find_last_of("\\/"));
		result += "\\";

		SetDlgItemTextA(windowconfig, IDC_EDIT_DBPATH, result.c_str());// sidSetting.c_dbpath);//wgc
	}
}



static BOOL CALLBACK CFGDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_NOTIFY:
			switch (LOWORD(wParam)) {

			case IDC_EQU1:
				seteqlevelsonchange(IDC_EQU1);
				break;
			case IDC_EQU2:
				seteqlevelsonchange(IDC_EQU2);
				break;
			case IDC_EQU3:
				seteqlevelsonchange(IDC_EQU3);
				break;
			case IDC_EQU4:
				seteqlevelsonchange(IDC_EQU4);
				break;
			case IDC_EQU5:
				seteqlevelsonchange(IDC_EQU5);
				break;
			case IDC_EQU6:
				seteqlevelsonchange(IDC_EQU6);
				break;
			case IDC_EQU7:
				seteqlevelsonchange(IDC_EQU7);
				break;
			case IDC_EQU8:
				seteqlevelsonchange(IDC_EQU8);
				break;
			case IDC_EQU9:
				seteqlevelsonchange(IDC_EQU9);
				break;
			case IDC_EQU10:
				seteqlevelsonchange(IDC_EQU10);
				break;
			case IDC_EQU11:
				seteqlevelsonchange(IDC_EQU11);
				break;
			case IDC_EQU12:
				seteqlevelsonchange(IDC_EQU12);
				break;
			case IDC_EQU13:
				seteqlevelsonchange(IDC_EQU13);
				break;
			case IDC_EQU14:
				seteqlevelsonchange(IDC_EQU14);
				break;
			case IDC_EQU15:
				seteqlevelsonchange(IDC_EQU15);
				break;
			case IDC_EQU16:
				seteqlevelsonchange(IDC_EQU16);
				break;
			case IDC_EQU17:
				seteqlevelsonchange(IDC_EQU17);
				break;
			case IDC_EQU18:
				seteqlevelsonchange(IDC_EQU18);
				break;
			case IDC_EQU19:
				seteqlevelsonchange(IDC_EQU19);
				break;
			case IDC_EQU20:
				seteqlevelsonchange(IDC_EQU20);
				break;




			case IDC_SURROUNDEMPH:

			{
				SURROUNDEMP = (BST_CHECKED == MESS(IDC_SURROUNDEMPH, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_BANDEQ:

			{
				BANDEQ = (BST_CHECKED == MESS(IDC_BANDEQ, BM_GETCHECK, 0, 0));//wgc

			}
			break;
			case IDC_SURROUNDSEP:

			{
				SURROUNDSEPERATION = (BST_CHECKED == MESS(IDC_SURROUNDSEP, BM_GETCHECK, 0, 0));//wgc

			}
			break;
			case IDC_SURROUNDSEPFORCE:

			{
				SURROUNDSEPFORCE = (BST_CHECKED == MESS(IDC_SURROUNDSEPFORCE, BM_GETCHECK, 0, 0));//wgc
				// hack for simulation of stereo effect when dual mono sids are active and allow second sid is enabled
	// will reduce to 0 then surround will be off ( sets surround to enable second sid chip effect for 1 sid )
				if (SURROUNDSEPERATION && !real2sid && !real3sid && !sidSetting.c_surround)//WGCCHANGE
				{
					if (SURROUNDSEPFORCE)//WGCCHANGE
					{
						voices[1] = 0.95;	voices[3] = 0.95;	voices[5] = 0.95;
						surroundcount = 10;
					}
				}
				if (!SURROUNDSEPERATION)//WGCCHANGE
				{
					voices[1] = 1;	voices[3] = 1;	voices[5] = 1; // ALLOWS ALL VOICES AT FULL VOLUME
				}

			}
			break;
			case IDC_ANTICLICK:

			{
				_click_wa = (BST_CHECKED == MESS(IDC_ANTICLICK, BM_GETCHECK, 0, 0));//wgc
			}

			break;
			case IDC_CHECK_VOICE1:

			{
				VOICE_1SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE1, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE2:

			{
				VOICE_1SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE2, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE3:

			{
				VOICE_1SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE3, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE4:

			{
				VOICE_2SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE4, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE5:

			{
				VOICE_2SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE5, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE6:

			{
				VOICE_2SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE6, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE7:

			{
				VOICE_3SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE7, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE8:

			{
				VOICE_3SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE8, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_VOICE9:

			{
				VOICE_3SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE9, BM_GETCHECK, 0, 0));//wgc
			}
			break;
			case IDC_CHECK_ENABLEFILTER://
				if (MESS(IDC_CHECK_ENABLEFILTER, BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_6581LEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_8580LEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_6581LEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_8580LEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_TITLE_6581LEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_TITLE_8580LEVEL), TRUE);
					if (sidEngine.m_builder)
						sidEngine.m_builder->filter(1);

				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_6581LEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_8580LEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_6581LEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_8580LEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_TITLE_6581LEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_TITLE_8580LEVEL), FALSE);
					if (sidEngine.m_builder)
						sidEngine.m_builder->filter(0);
				}
				break;
			case IDC_CHECK_FADEIN:
				if (MESS(IDC_CHECK_FADEIN, BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEINLEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEINLEVEL), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEINLEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEINLEVEL), FALSE);
				}
				break;
			case IDC_CHECK_FADEOUT:
				if (MESS(IDC_CHECK_FADEOUT, BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEOUTLEVEL), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEOUTLEVEL), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEOUTLEVEL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEOUTLEVEL), FALSE);
				}
				break;
			case IDC_CHECK_SKIPSHORT:
				if (MESS(IDC_CHECK_SKIPSHORT, BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MINLENGTH), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_MINLENGTH), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MINLENGTH), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LABEL_MINLENGTH), FALSE);
				}
				break;

			case IDC_CHECK_SKIPSILENCE:
				allownosoundcheckactivate = MESS(IDC_CHECK_SKIPSILENCE, BM_GETCHECK, 0, 0);

			break;
                case IDC_CHECK_RANDOMDELAY:
                    if (MESS(IDC_CHECK_RANDOMDELAY, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_POWERDELAY), FALSE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_POWERDELAY), TRUE);
                    }
					break;
                case IDC_SLIDE_6581LEVEL:
                   MESS(IDC_LABEL_6581LEVEL, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_6581LEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0)).append("%").c_str());
				   if (sidEngine.m_builder)
				   {
					   sidEngine.m_builder->filter6581Curve(SendDlgItemMessage(hWnd, IDC_SLIDE_6581LEVEL, TBM_GETPOS, 0, 0) / 100.f);
					   sidSetting.c_6581filter = SendDlgItemMessage(hWnd, IDC_SLIDE_6581LEVEL, TBM_GETPOS, 0, 0);
				   }
				   break;
				case IDC_SLIDE_8580LEVEL:
                    MESS(IDC_LABEL_8580LEVEL, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_8580LEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0)).append("%").c_str());
					if (sidEngine.m_builder)
					{
						sidEngine.m_builder->filter8580Curve(SendDlgItemMessage(hWnd, IDC_SLIDE_8580LEVEL, TBM_GETPOS, 0, 0) / 100.f);
						sidSetting.c_8580filter = SendDlgItemMessage(hWnd, IDC_SLIDE_8580LEVEL, TBM_GETPOS, 0, 0);
					}
					break;
				case IDC_SLIDE_SDELAY:
// 					MESS(IDC_LABEL_SDELAY2, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_SDELAY, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0)/10).append("%").c_str());
					MESS(IDC_LABEL_SDELAY2, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_SDELAY, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0)).append("*").c_str());
					SDELAY = SendDlgItemMessage(hWnd, IDC_SLIDE_SDELAY, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);
					break;
				case IDC_SLIDE_PREAMP:
					MESS(IDC_LABEL_DIGIT, WM_SETTEXT, 0, std::to_string((/*(FLOAT)*/SendDlgItemMessage(hWnd, IDC_SLIDE_PREAMP, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0) - 200)*-1).append("%* Preamp").c_str());//.substr(0, 4).append("Db*").c_str());
					//MESS(IDC_LABEL_PREAMP3, WM_SETTEXT, 0, std::to_string((/*(FLOAT)*/SendDlgItemMessage(hWnd, IDC_SLIDE_PREAMP, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0) -200)*-1).append("%*").c_str());//.substr(0, 4).append("Db*").c_str());
					PREAMP = 200-SendDlgItemMessage(hWnd, IDC_SLIDE_PREAMP, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);
					break;
				case IDC_SLIDE_HPFILTER2:
					MESS(IDC_LABEL_HPFILTER3, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_HPFILTER2, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0)).append("%*").c_str());
					HPFILTER = SendDlgItemMessage(hWnd, IDC_SLIDE_HPFILTER2, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);


					break;
				case IDC_SLIDE_LPFILTER2:
					MESS(IDC_LABEL_LPFILTER3, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_LPFILTER2, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0)).append("%*").c_str());
					LPFILTER = SendDlgItemMessage(hWnd, IDC_SLIDE_LPFILTER2, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);
					break;


				case IDC_CHECK_SURROUND1SID:
					sidSetting.c_surround = (BST_CHECKED == MESS(IDC_CHECK_SURROUND1SID, BM_GETCHECK, 0, 0));//WGC
					if (!real2sid && !real3sid)
					surround = sidSetting.c_surround;
					break;

				case IDC_CHECK_2SIDAUTOSURROUND:
					sidSetting.c_surround2sid = (BST_CHECKED == MESS(IDC_CHECK_2SIDAUTOSURROUND, BM_GETCHECK, 0, 0));//WGC
					if (real2sid || real3sid)
						surround = sidSetting.c_surround2sid;
					break;

				case IDC_SLIDE_FADEINLEVEL:
                    MESS(IDC_LABEL_FADEINLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEINLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 100.f).substr(0,4).append("s").c_str());
					break;
				case IDC_SLIDE_FADEOUTLEVEL:
                    if (SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) >= 100) {
                        MESS(IDC_LABEL_FADEOUTLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 10.f).substr(0,5).append("s").c_str());
                    } else {
                        MESS(IDC_LABEL_FADEOUTLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 10.f).substr(0,4).append("s").c_str());
                        //wgc
                    }
					break;
				case	IDC_EQUAGC:
				{
					agc_maxslider = (SendDlgItemMessage(hWnd, IDC_EQUAGC, TBM_GETPOS, 0, 0)*0.01)*-1;
					MESS(IDC_LABEL_DIGIT, WM_SETTEXT, 0, std::to_string((/*(FLOAT)*/SendDlgItemMessage(hWnd, IDC_EQUAGC, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0) ) *-1).append("%* AGC").c_str());//.substr(0, 4).append("Db*").c_str());
				}
				break;
				
            }
            break;
        case WM_COMMAND:

            switch (LOWORD(wParam)) {

			case IDC_FOLDERX:
			{
				GetInputFile();
			}
			break;


			case IDC_STEREOSEP:
			{





					for (int i = 0; i < 10; i += 4) {

						readeq(hWnd, eqslider[i + 0], (int)(stseperation) * rand_FloatRangeA(-100,100));
						//readeq(hWnd, eqslider[i + 1], (int)(stseperation * 200)*-1);
						readeq(hWnd, eqslider[i + 1], (int)(stseperation) * rand_FloatRangeA(100, -100));
						//equalizerl[i + 1] = stseperation * -1;
						readeq(hWnd, eqslider[i + 2], (int)(stseperation) * rand_FloatRangeA(-100, 100));

						readeq(hWnd, eqslider[i + 3], (int)(stseperation) * rand_FloatRangeA(100, -100));
						}

					for (int i = 0; i < 10; i += 4) {

						readeq(hWnd, eqslider[i+10],  (int)(stseperation) * rand_FloatRangeA(100, -100));
						//readeq(hWnd, eqslider[i + 1], (int)(stseperation * 200)*-1);
						readeq(hWnd, eqslider[i + 11], (int)(stseperation) * rand_FloatRangeA(-100, 100));
						//equalizerl[i + 1] = stseperation * -1;
						readeq(hWnd, eqslider[i + 12], (int)(stseperation) * rand_FloatRangeA(100, -100));

						readeq(hWnd, eqslider[i + 13], (int)(stseperation) * rand_FloatRangeA(-100, 100));
					}

				//if (!GetAsyncKeyState(VK_LCONTROL))
				stseperation += 1;

				if (stseperation > 16)stseperation = 0;
			}
			break;
			case IDC_EQPRESETS:
			{
				

				//step 10 = number of values created do not change , value not used , alpha = sign power , db = volume
				//gensin(10, 100, 2,4,10);
				for (int i = 0; i < 10; i++) {
					//readeq(hWnd, eqslider[i], (int)preset[i]*50);
					readeq(hWnd, eqslider[i], (int)eqpresets[i+eqpresetoffset] * 200);
					//readeq(hWnd, eqslider[i + 10], (int)preset[i]*50);
				}

				//step 10 = number of values created do not change , value not used , alpha = sign power , db = volume
				//gensin(10, 100, 4, -4, 10);
				for (int i = 0; i < 10; i++) {
					//readeq(hWnd, eqslider[i], (int)preset[i] * 50);
					readeq(hWnd, eqslider[i + 10], (int)eqpresets[i+10+ eqpresetoffset] * 200);
					//readeq(hWnd, eqslider[i + 10], (int)preset[i] * 50);
				}
				eqpresetoffset += 2; if (eqpresetoffset > 120)eqpresetoffset = 0;
			}
				break;

			case	IDC_BANDAGC:
			{
				allowagc = (BST_CHECKED == MESS(IDC_BANDAGC, BM_GETCHECK, 0, 0));

			}
			break;			
			case	IDC_BANDAGC2:
			{
				allowagcII = (BST_CHECKED == MESS(IDC_BANDAGC2, BM_GETCHECK, 0, 0));

			}
			break;



                case IDOK:

					//if (sidEngine.m_engine->isPlaying()) //wgc
						//sidEngine.m_engine->stop();

					Restart_p1();// latch to stop tune and delete player...

					sidSetting.c_locksidmodel = (BST_CHECKED==MESS(IDC_CHECK_LOCKSID, BM_GETCHECK, 0, 0));
					



					if (!sidSetting.c_surround)//wgc
					{
						VOICE_1SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE1, BM_GETCHECK, 0, 0));//wgc
						VOICE_1SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE2, BM_GETCHECK, 0, 0));
						VOICE_1SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE3, BM_GETCHECK, 0, 0));
						VOICE_2SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE4, BM_GETCHECK, 0, 0));
						VOICE_2SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE5, BM_GETCHECK, 0, 0));
						VOICE_2SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE6, BM_GETCHECK, 0, 0));
						VOICE_3SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE7, BM_GETCHECK, 0, 0));
						VOICE_3SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE8, BM_GETCHECK, 0, 0));
						VOICE_3SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE9, BM_GETCHECK, 0, 0));


					}

					sidSetting.c_surround2sid = (BST_CHECKED == MESS(IDC_CHECK_2SIDAUTOSURROUND, BM_GETCHECK, 0, 0));//WGC
					sidSetting.c_surround = (BST_CHECKED == MESS(IDC_CHECK_SURROUND1SID, BM_GETCHECK, 0, 0));//WGC
					//TO CHANGE !!!!!!! ZR//WGC

					if (sidSetting.c_surround && (sidEngine.m_config.secondSidAddress == 0 || sidEngine.m_config.secondSidAddress == 0xd400 ))//wgc
					{
						sidEngine.m_config.secondSidAddress = 0xd400;
						surround = 1;
						setsidchannelssurround();
					}
					else
					{
						sidEngine.m_config.secondSidAddress = 0xd400;
						surround = 0;
// 						VOICE_1SID[0] = VOICE_1SIDsave[0]; // wgc
// 						VOICE_1SID[1] = VOICE_1SIDsave[1];
// 						VOICE_1SID[2] = VOICE_1SIDsave[2];
// 						VOICE_2SID[0] = VOICE_2SIDsave[0];
// 						VOICE_2SID[1] = VOICE_2SIDsave[1];
// 						VOICE_2SID[2] = VOICE_2SIDsave[2];
// 						VOICE_3SID[0] = VOICE_3SIDsave[0];
// 						VOICE_3SID[1] = VOICE_3SIDsave[1];
// 						VOICE_3SID[2] = VOICE_3SIDsave[2];
					}



                    sidSetting.c_lockclockspeed = (BST_CHECKED==MESS(IDC_CHECK_LOCKCLOCK, BM_GETCHECK, 0, 0));
                    sidSetting.c_enabledigiboost = (BST_CHECKED==MESS(IDC_CHECK_DIGIBOOST, BM_GETCHECK, 0, 0));
                    sidSetting.c_enablefilter = (BST_CHECKED==MESS(IDC_CHECK_ENABLEFILTER, BM_GETCHECK, 0, 0));
                    sidSetting.c_powerdelayrandom = (BST_CHECKED==MESS(IDC_CHECK_RANDOMDELAY, BM_GETCHECK, 0, 0));
                    sidSetting.c_forcelength = (BST_CHECKED==MESS(IDC_CHECK_FORCELENGTH, BM_GETCHECK, 0, 0));
                    sidSetting.c_skipshort = (BST_CHECKED==MESS(IDC_CHECK_SKIPSHORT, BM_GETCHECK, 0, 0));
                    sidSetting.c_fadein = (BST_CHECKED==MESS(IDC_CHECK_FADEIN, BM_GETCHECK, 0, 0));
                    sidSetting.c_fadeout = (BST_CHECKED==MESS(IDC_CHECK_FADEOUT, BM_GETCHECK, 0, 0));
                    sidSetting.c_disableseek = (BST_CHECKED==MESS(IDC_CHECK_DISABLESEEK, BM_GETCHECK, 0, 0));
                    sidSetting.c_detectplayer = (BST_CHECKED==MESS(IDC_CHECK_DETECTPLAYER, BM_GETCHECK, 0, 0));
                    MESS(IDC_COMBO_SID, WM_GETTEXT, 10, sidSetting.c_sidmodel);
                    MESS(IDC_COMBO_CLOCK, WM_GETTEXT, 10, sidSetting.c_clockspeed);
                    MESS(IDC_COMBO_SAMPLEMETHOD, WM_GETTEXT, 10, sidSetting.c_samplemethod);
                    MESS(IDC_EDIT_DBPATH, WM_GETTEXT, 250, sidSetting.c_dbpath);
                    sidSetting.c_defaultlength = GetDlgItemInt(hWnd, IDC_EDIT_DEFAULTLENGTH, NULL, false);
                    sidSetting.c_minlength = GetDlgItemInt(hWnd, IDC_EDIT_MINLENGTH, NULL, false);
                    sidSetting.c_powerdelay = GetDlgItemInt(hWnd, IDC_EDIT_POWERDELAY, NULL, false);
                    sidSetting.c_6581filter = SendDlgItemMessage(hWnd, IDC_SLIDE_6581LEVEL, TBM_GETPOS, 0, 0);
                    sidSetting.c_8580filter = SendDlgItemMessage(hWnd, IDC_SLIDE_8580LEVEL, TBM_GETPOS, 0, 0);
                    sidSetting.c_fadeinms = SendDlgItemMessage(hWnd, IDC_SLIDE_FADEINLEVEL, TBM_GETPOS, 0, 0) * 10;
                    sidSetting.c_fadeoutms = SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, TBM_GETPOS, 0, 0) * 100;
		
					if (!SendMessage(GetDlgItem(hWnd, IDC_COMBO_SID), CB_GETCURSEL, 0, 0))
					{
						if (sidEngine.m_engine && sidEngine.p_song)
						sidEngine.m_config.defaultSidModel = (SidConfig::sid_model_t)0;
						sprintf(sidSetting.c_sidmodel, "%s", "6581");
						if (sidSetting.c_locksidmodel)
						_8580VOLUMEBOOST = 0;
						
					}
					else
					{
						if (sidEngine.m_engine && sidEngine.p_song)
						sidEngine.m_config.defaultSidModel = (SidConfig::sid_model_t)1;
						sprintf(sidSetting.c_sidmodel, "%s", "8580");
						if (sidSetting.c_locksidmodel)
							_8580VOLUMEBOOST = 1;
							
					}

					if (!SendMessage(GetDlgItem(hWnd, IDC_COMBO_CLOCK), CB_GETCURSEL, 0, 0))
					{
						if (sidEngine.m_engine && sidEngine.p_song)
							sidEngine.m_config.defaultC64Model = SidConfig::PAL;
						sprintf(sidSetting.c_clockspeed, "%s", "PAL");
					}
					else
					{
						if (sidEngine.m_engine && sidEngine.p_song)
							sidEngine.m_config.defaultC64Model = SidConfig::NTSC;
						sprintf(sidSetting.c_clockspeed, "%s", "NTSC");
					}
					

					if (!SendMessage(GetDlgItem(hWnd, IDC_COMBO_SAMPLEMETHOD), CB_GETCURSEL, 0, 0))
					{
						if (sidEngine.m_engine && sidEngine.p_song)
							sidEngine.m_config.samplingMethod = SidConfig::INTERPOLATE;
						sprintf(sidSetting.c_samplemethod, "%s", "Normal");
					}
					else
					{
						if (sidEngine.m_engine && sidEngine.p_song)
							sidEngine.m_config.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
						sprintf(sidSetting.c_samplemethod, "%s", "Accurate");
					}


                    // apply configuraton
					if (sidEngine.m_builder)
					if (sidEngine.m_builder->getStatus()) {
						delete sidEngine.m_engine;
						sidEngine.b_loaded = FALSE;
					}
					
                    saveConfig();//
					
					//if (sidEngine.m_engine)
					//applyConfig(true);
					
					loadConfig();
					


					

					reset_surrounddelay();

					//wgc
					if (sidEngine.m_engine && sidEngine.p_song) {

						sidEngine.fadein = 0;
							//sidEngine.m_engine->load(sidEngine.p_song);//

							
							// wgc seeking broken unable to update player time in xmplay on the fly xmpnewfeaturerequest:::::::::::
// 							 {
// 								if (sidSetting.c_defaultlength == 0 || sidSetting.c_disableseek) {
// 									//xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], FALSE);
// 
// 									_target_odometer = sidEngine.p_subsonglength[sidEngine.p_subsong] * 60 * 60 * 1000;//wgc
// 									_odometer = 0;
// 								}
// 								else {
// 									//xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], TRUE);
// 
// 									_target_odometer = sidEngine.p_subsonglength[sidEngine.p_subsong] * 60 * 60 * 1000;//wgc
// 									_odometer = 0;
// 								}
// 
// 
// 
// 							}
							//

									//
// 							if (sidSetting.c_defaultlength == 0 || sidSetting.c_disableseek) {
// 								xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], FALSE);
// 							}
// 							else {
// 								xmpfin->SetLength(sidEngine.p_subsonglength[sidEngine.p_subsong], TRUE);
// 							}


							//////


							
						Restart_p2();//player restarting...

						//softlmax = 1;//resets quick agc
						timeroff(hWnd);
					}
					timeroff(hWnd);
					Sleep(200);
					
					{
						for (int i = 0; i < 10; i++) {
							geteql(hWnd, eqslider[i], i);
							set_eq_value((equalizerl[i] + 2), i, 0);
						}
						for (int i = 0; i < 10; i++) {
							geteqr(hWnd, eqslider[i + 10], i);
							set_eq_value((equalizerr[i] + 2), i, 1);
							//sprintf(DEBUGGERCHAR, "eqvalue_%i = %i", i + 10, (equalizerr[i] + 2));
							//MessageBoxA(0, DEBUGGERCHAR, "", 0);
						}
					}
					EndDialog(hWnd, 0);
					break;
                case IDCANCEL:
					timeroff(hWnd);
                    EndDialog(hWnd, 0);
					//softlmax = 1;//resets quick agc
					
                    break;

            }
            break;
        case WM_INITDIALOG:

			// set sid model on config dialog

			// update info for dialog
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_MODEL), WM_SETTEXT, 0, (LPARAM)initinfochiptype);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_INIT), WM_SETTEXT, 0, (LPARAM)initinfo);

			timeron(hWnd);

            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SID), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("6581"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SID), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("8580"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SID), CB_SETCURSEL, (WPARAM)CHAR(sidSetting.c_sidmodel[0]=='6' ? 0 : 1), (LPARAM) 0);
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_CLOCK), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("PAL"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_CLOCK), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("NTSC"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_CLOCK), CB_SETCURSEL, (WPARAM)CHAR(sidSetting.c_clockspeed[0] == 'P' ? 0 : 1), (LPARAM)0);
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SAMPLEMETHOD), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("Normal"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SAMPLEMETHOD), (UINT) CB_ADDSTRING, (WPARAM) 0, (LPARAM) TEXT ("Accurate"));
            SendMessage(GetDlgItem(hWnd, IDC_COMBO_SAMPLEMETHOD), CB_SETCURSEL, (WPARAM)CHAR(sidSetting.c_samplemethod[0] == 'N' ? 0 : 1), (LPARAM)0);
            //
			MESS(IDC_SLIDE_SDELAY, TBM_SETRANGE, TRUE, MAKELONG(0, 999));
			MESS(IDC_SLIDE_SDELAY, TBM_SETPOS, TRUE, SDELAY);

			MESS(IDC_EQUAGC, TBM_SETRANGE, TRUE, MAKELONG(-100, 0));
			MESS(IDC_EQUAGC, TBM_SETPOS, TRUE, (agc_maxslider*-1)*100);

			MESS(IDC_SLIDE_PREAMP, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
			MESS(IDC_SLIDE_PREAMP, TBM_SETPOS, TRUE, 200-PREAMP);
			MESS(IDC_SLIDE_PREAMP, TBM_SETTICFREQ, 100, 0);

			MESS(IDC_SLIDE_HPFILTER2, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
			MESS(IDC_SLIDE_HPFILTER2, TBM_SETPOS, TRUE, HPFILTER);

			MESS(IDC_SLIDE_LPFILTER2, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
			MESS(IDC_SLIDE_LPFILTER2, TBM_SETPOS, TRUE, LPFILTER);

            MESS(IDC_SLIDE_6581LEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_6581LEVEL, TBM_SETPOS, TRUE, sidSetting.c_6581filter);

			for (int i = 0; i < 10; i++) {
				readeq(hWnd, eqslider[i], (int)equalizerl[i] * 100);
				readeq(hWnd, eqslider[i+10], (int)equalizerr[i] * 100);
			}
			

            MESS(IDC_SLIDE_8580LEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_8580LEVEL, TBM_SETPOS, TRUE, sidSetting.c_8580filter);
            MESS(IDC_SLIDE_FADEINLEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 30));
            MESS(IDC_SLIDE_FADEINLEVEL, TBM_SETPOS, TRUE, sidSetting.c_fadeinms / 10);
            MESS(IDC_SLIDE_FADEOUTLEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_FADEOUTLEVEL, TBM_SETPOS, TRUE, sidSetting.c_fadeoutms / 100);
            //


			MESS(IDC_BANDAGC, BM_SETCHECK, allowagc ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_BANDAGC2, BM_SETCHECK, allowagcII ? BST_CHECKED : BST_UNCHECKED, 0);

            MESS(IDC_CHECK_LOCKSID, BM_SETCHECK, sidSetting.c_locksidmodel?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_LOCKCLOCK, BM_SETCHECK, sidSetting.c_lockclockspeed?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_ENABLEFILTER, BM_SETCHECK, sidSetting.c_enablefilter?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DIGIBOOST, BM_SETCHECK, sidSetting.c_enabledigiboost?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_FORCELENGTH, BM_SETCHECK, sidSetting.c_forcelength?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_RANDOMDELAY, BM_SETCHECK, sidSetting.c_powerdelayrandom?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DISABLESEEK, BM_SETCHECK, sidSetting.c_disableseek?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DETECTPLAYER, BM_SETCHECK, sidSetting.c_detectplayer?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_SKIPSHORT, BM_SETCHECK, sidSetting.c_skipshort?BST_CHECKED:BST_UNCHECKED, 0);
			MESS(IDC_CHECK_SKIPSILENCE, BM_SETCHECK, allownosoundcheckactivate ? BST_CHECKED : BST_UNCHECKED, 0);
			
            MESS(IDC_CHECK_FADEIN, BM_SETCHECK, sidSetting.c_fadein?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_FADEOUT, BM_SETCHECK, sidSetting.c_fadeout?BST_CHECKED:BST_UNCHECKED, 0);
			MESS(IDC_CHECK_LOCKSID, BM_SETCHECK, sidSetting.c_locksidmodel?BST_CHECKED:BST_UNCHECKED, 0);
			MESS(IDC_SURROUNDEMPH, BM_SETCHECK, SURROUNDEMP ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_SURROUNDSEP, BM_SETCHECK, SURROUNDSEPERATION ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_SURROUNDSEPFORCE, BM_SETCHECK, SURROUNDSEPFORCE ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_ANTICLICK, BM_SETCHECK, _click_wa ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_BANDEQ, BM_SETCHECK, BANDEQ ? BST_CHECKED : BST_UNCHECKED, 0);

			
			
			MESS(IDC_CHECK_SURROUND1SID, BM_SETCHECK, sidSetting.c_surround ? BST_CHECKED : BST_UNCHECKED, 0);
		
			MESS(IDC_CHECK_2SIDAUTOSURROUND, BM_SETCHECK, sidSetting.c_surround2sid ? BST_CHECKED : BST_UNCHECKED, 0);


			
			if (sidSetting.c_surround)//
			{
				
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE1), false);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE2), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE3), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), false);
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_SDELAY), true);
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), true);

				
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE1), true);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE2), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE3), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), true);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), true);
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_SDELAY), false);
				EnableWindow(GetDlgItem(hWnd, IDC_LABEL_SDELAY2), false);

			}


			MESS(IDC_CHECK_VOICE1, BM_SETCHECK, !VOICE_1SID[0] ? BST_CHECKED : BST_UNCHECKED, 0);//wgc
			MESS(IDC_CHECK_VOICE2, BM_SETCHECK, !VOICE_1SID[1] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE3, BM_SETCHECK, !VOICE_1SID[2] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE4, BM_SETCHECK, !VOICE_2SID[0] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE5, BM_SETCHECK, !VOICE_2SID[1] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE6, BM_SETCHECK, !VOICE_2SID[2] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE7, BM_SETCHECK, !VOICE_3SID[0] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE8, BM_SETCHECK, !VOICE_3SID[1] ? BST_CHECKED : BST_UNCHECKED, 0);
			MESS(IDC_CHECK_VOICE9, BM_SETCHECK, !VOICE_3SID[2] ? BST_CHECKED : BST_UNCHECKED, 0);

            SetDlgItemInt(hWnd, IDC_EDIT_DEFAULTLENGTH, sidSetting.c_defaultlength, false);
            SetDlgItemInt(hWnd, IDC_EDIT_MINLENGTH, sidSetting.c_minlength, false);
            SetDlgItemInt(hWnd, IDC_EDIT_POWERDELAY, sidSetting.c_powerdelay, false);
            SetDlgItemTextA(hWnd, IDC_EDIT_DBPATH, sidSetting.c_dbpath);//wgc
			/*MessageBoxA(0, sidSetting.c_dbpath, 0, MB_ICONEXCLAMATION);*///wgc
			
            //
            return TRUE;
    }
    return FALSE;
}
static void WINAPI SIDex_Config(HWND win)
{
    DialogBox(ghInstance, MAKEINTRESOURCE(IDD_DIALOG_CONFIG), win, &CFGDialogProc);
}

// plugin interface
static XMPIN xmpin={
    0,
    "" SIDEXNAME " (" SIDEXVERSION ")",
    "" SIDEXNAME "\0sid",
    SIDex_About,
    SIDex_Config,
    SIDex_CheckFile,
    SIDex_GetFileInfo,
    SIDex_Open,
    SIDex_Close,
    NULL, // reserved1
    SIDex_SetFormat,
    SIDex_GetTags,
    SIDex_GetInfoText,
    SIDex_GetGeneralInfo,
    SIDex_GetMessage,
    SIDex_SetPosition,
    SIDex_GetGranularity,
    NULL, // SIDex_GetBuffering
    SIDex_Process,
    NULL, // WriteFile
    NULL, // GetSamples
    SIDex_GetSubSongs, // GetSubSongs
    NULL, // reserved3
    NULL, // GetDownloaded
    NULL, // visname
    NULL, // VisOpen
    NULL, // VisClose
    NULL, // VisSize
    NULL, // VisRender
    NULL, // VisRenderDC
    NULL, // VisButton
    NULL, // reserved2
    NULL, // GetConfig,
    NULL, // SetConfig
};

// get the plugin's XMPIN interface
#ifdef __cplusplus
extern "C"
#endif
XMPIN *WINAPI XMPIN_GetInterface(DWORD face, InterfaceProc faceproc)
{
    if (face!=XMPIN_FACE) { // unsupported version
        static int shownerror=0;
        if (face<XMPIN_FACE && !shownerror) {
            MessageBoxA(0,"The XMP-SIDex plugin requires XMPlay 3.8 or above",0,MB_ICONEXCLAMATION);
            shownerror=1;
        }
        return NULL;
    }
    xmpfin=(XMPFUNC_IN*)faceproc(XMPFUNC_IN_FACE);
    xmpfmisc=(XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE);
    xmpffile=(XMPFUNC_FILE*)faceproc(XMPFUNC_FILE_FACE);
    xmpftext=(XMPFUNC_TEXT*)faceproc(XMPFUNC_TEXT_FACE);
    xmpfreg=(XMPFUNC_REGISTRY*)faceproc(XMPFUNC_REGISTRY_FACE);
    
    loadConfig();

    return &xmpin;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD reason, LPVOID reserved)
{
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            ghInstance = (HINSTANCE)hDLL;
			srand(time(NULL));
            DisableThreadLibraryCalls((HMODULE)hDLL);
#ifdef debugplayer
			OpenConsole();//wgc
#endif
			HANDLE myhandle;
			DWORD mythreadid;
			myhandle = CreateThread(0, 0, carrienexttunethread, 0, 0, &mythreadid);
            break;
    }
    return TRUE;
}
