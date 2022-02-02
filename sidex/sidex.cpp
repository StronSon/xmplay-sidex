// XMPlay SIDex input plugin (c) 2021 Nathan Hindley & William Cronin
#define _CRT_SECURE_NO_WARNINGS // stops the compiler grumbling.

#include <Windows.h> //wgc

#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>
#include <iostream>
#include <vector>
#include <cctype>

//TODO: set up defines for all changes 
//TODO: class these
int haltplayer = 0;
extern int surround,VOICE_1SID[], VOICE_2SID[], VOICE_3SID[], _8580VOLUMEBOOST, _buffer_fill, real2sid; //wgc
extern float fadevolume, _decode_buffer[];
#include "xmpin.h"

static XMPFUNC_IN *xmpfin;
static XMPFUNC_MISC *xmpfmisc;
static XMPFUNC_FILE *xmpffile;
static XMPFUNC_TEXT *xmpftext;
static XMPFUNC_REGISTRY *xmpfreg;

//#include "configdialog.h" //wgc
#include "../../resource.h"
#include "utils/SidDatabase.h"
#include "utils/STILview/stil.h"
#include <builders/residfp-builder/residfp.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/sidplayfp.h>
#include <sidid.h>

#include <commctrl.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <fstream>
#include <sstream>

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
} SIDsetting;
static SIDsetting sidSetting;

extern int playsound;
int VOICE_1SIDsave[3] = { 0 };//wgc
int VOICE_2SIDsave[3] = { 0 };
int VOICE_3SIDsave[3] = { 0 };

void setsidchannelssurround()
{
	if (!real2sid)
		//if (VOICE_1SID[0] == 0 && VOICE_1SID[1] == 0 && VOICE_1SID[2] == 0)//wgc
		{
			VOICE_1SID[0] = 0;	VOICE_1SID[1] = 1;	VOICE_1SID[2] = 0;
			VOICE_2SID[0] = 1;	VOICE_2SID[1] = 0;	VOICE_2SID[2] = 1;
		}	
	else
	{
		VOICE_1SID[0] = 0;	VOICE_1SID[1] = 0;	VOICE_1SID[2] = 0;
		VOICE_2SID[0] = 0;	VOICE_2SID[1] = 0;	VOICE_2SID[2] = 0;
	}

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
    if (!sidEngine.b_loaded) {
        strcpy(sidSetting.c_sidmodel, "6581");
        strcpy(sidSetting.c_clockspeed, "PAL");
        strcpy(sidSetting.c_samplemethod, "Normal");
        strcpy(sidSetting.c_dbpath, "");
        sidSetting.c_defaultlength = 120;
        sidSetting.c_minlength = 3;
        sidSetting.c_6581filter = 25;
        sidSetting.c_8580filter = 50;
        sidSetting.c_powerdelay = 0;
        sidSetting.c_fadeinms = 80;
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
		sidSetting.c_surround = surround = true; //WGC
        
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
            
            int ival;
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
        }
    }
}
static bool applyConfig(bool initThis) {
    if (initThis) {
        sidEngine.m_config = sidEngine.m_engine->config();
        
        // apply sid model
        if (std::string(sidSetting.c_sidmodel).find("6581") != std::string::npos) {
            sidEngine.m_config.defaultSidModel = SidConfig::MOS6581;
        } else if (std::string(sidSetting.c_sidmodel).find("8580") != std::string::npos) {
            sidEngine.m_config.defaultSidModel = SidConfig::MOS8580;
        }
        
        // apply clock speed
        if (std::string(sidSetting.c_clockspeed).find("PAL") != std::string::npos) {
            sidEngine.m_config.defaultC64Model = SidConfig::PAL;
        } else if (std::string(sidSetting.c_clockspeed).find("NTSC") != std::string::npos) {
            sidEngine.m_config.defaultC64Model = SidConfig::NTSC;
        }
        
        // apply power delay
        if (sidSetting.c_powerdelayrandom) {
            sidEngine.m_config.powerOnDelay = SidConfig::DEFAULT_POWER_ON_DELAY;
        } else {
            sidEngine.m_config.powerOnDelay = sidSetting.c_powerdelay;
        }
        
        // apply sample method
        if (std::string(sidSetting.c_samplemethod).find("Normal") != std::string::npos) {
            sidEngine.m_config.samplingMethod = SidConfig::INTERPOLATE;
        } else if (std::string(sidSetting.c_samplemethod).find("Accurate") != std::string::npos) {
            sidEngine.m_config.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
        }

        // apply sid model & clock speed lock
        sidEngine.m_config.forceSidModel = sidSetting.c_locksidmodel;
        sidEngine.m_config.forceC64Model = sidSetting.c_lockclockspeed;
        
        // apply digi boost
        sidEngine.m_config.digiBoost = sidSetting.c_enabledigiboost;
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
    } else {
        return FALSE;
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
        }
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
void OpenConsole()//wgc
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	printf("\nSidEx v*.** Debug Console:\n\n");
	//printf(disclaimer);



}
static void WINAPI SIDex_Init()
{
	//OpenConsole();//wgc
    if (!sidEngine.b_loaded) {
        // set default config
        loadConfig();

		sidEngine.m_config.forceSecondSidModel = 1; //wgc
		sidEngine.m_config.secondSidAddress = 0xd400;
		sidEngine.m_config.secondSidModel = 0;
		//TO CHANGE !!!!!!! ZR//WGC

		if (sidSetting.c_surround && (sidEngine.m_config.secondSidAddress == 0 || sidEngine.m_config.secondSidAddress == 0xd400))//wgc
		{
			sidEngine.m_config.forceSecondSidModel = 1;
			sidEngine.m_config.secondSidAddress = 0xd400;
			sidEngine.m_config.secondSidModel = 0;
			surround = 1;
			setsidchannelssurround();
		}
		else
		{
			sidEngine.m_config.forceSecondSidModel = 1;//wgc this is not necessary and i should remove it leaving it for future reference
			sidEngine.m_config.secondSidAddress = 0xd400;
			sidEngine.m_config.secondSidModel = 0;
			surround = 0;
			VOICE_1SID[0] = VOICE_1SIDsave[0]; // wgc
			VOICE_1SID[1] = VOICE_1SIDsave[1];
			VOICE_1SID[2] = VOICE_1SIDsave[2];
			VOICE_2SID[0] = VOICE_2SIDsave[0];
			VOICE_2SID[1] = VOICE_2SIDsave[1];
			VOICE_2SID[2] = VOICE_2SIDsave[2];
			VOICE_3SID[0] = VOICE_3SIDsave[0];
			VOICE_3SID[1] = VOICE_3SIDsave[1];
			VOICE_3SID[2] = VOICE_3SIDsave[2];
		}


        // initialise the engine
        sidEngine.m_engine = new sidplayfp();
        sidEngine.m_engine->setRoms(kernel, basic, chargen);

        sidEngine.m_builder = new ReSIDfpBuilder("ReSIDfp");
        sidEngine.m_builder->create(sidEngine.m_engine->info().maxsids());
        if (!sidEngine.m_builder->getStatus()) {
            delete sidEngine.m_engine;
            sidEngine.b_loaded = FALSE;
        } else {
            // apply configuraton
            if (applyConfig(TRUE)) {
                sidEngine.b_loaded = TRUE;
            } else {
                delete sidEngine.m_builder;
                delete sidEngine.m_engine;
                sidEngine.b_loaded = FALSE;
            }
        }
    }
}

// general purpose
char aboutinfo[] = {
	"XMPlay SIDex plugin (v1.3b1)\n"
	"Copyright (c) 2021 Nathan Hindley & William Cronin\n\n"
	"This plugin allows XMPlay to load/play sid files with libsidplayfp-2.3.0.\n\n"
	"Additional Credits:\n"
	"Copyright(c) 2000 - 2001 Simon White\n"
	"Copyright(c) 2007 - 2010 Antti Lankila\n"
	"Copyright(c) 2010 - 2021 Leandro Nini <drfiemost@users.sourceforge.net>\n"
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
    buf += sprintf(buf, "%s\t%s\r", "Library", "libsidplayfp-2.3.0");
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
int _click_wa = 1, _muting = 1, _mute_size = _target_odometer;
int _attenuation_length = 0;// set later in sidex_open    (rate * channels * ms) / 1000 MsToSamples(25, _sample_format);
int _fadein_length = 44100 * 0.1;// MsToSamples(100, _sample_format);
//int _mute_size = _target_odometer; // start with complete song mute
int _delay = 44100 * 0.015 / 2;
int MsToSamples(int rate, int channels, int ms)
{
	int ret = (rate * channels * ms) / 1000;
	return ret;
}
extern int chips;
static DWORD WINAPI SIDex_Open(const char *filename, XMPFILE file)
{



	attenuate = 0;//wgc

	_delay = ((sidEngine.m_config.frequency * 0.015));
	//_delay /= 2;
	if (_delay % 2 == 0)
	{	}else _delay += 1;
	
    SIDex_Init();

    if (sidEngine.b_loaded) {
        sidEngine.p_song = new SidTune(filename);
        if (!sidEngine.p_song->getStatus()) {
            return 0;
        } else {
			if (sidEngine.fadein)//wgc
				_buffer_fill = 0;
			
            sidEngine.p_songinfo = sidEngine.p_song->getInfo();
			if (sidEngine.p_songinfo->sidModel(0) == SidTuneInfo::SIDMODEL_8580) _8580VOLUMEBOOST = 1; else _8580VOLUMEBOOST = 0;
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
				_attenuation_length =	MsToSamples(44100,2,25);	// MsToSamples(25, _sample_format);
				_fadein_length =		MsToSamples(44100, 2, 50);	// MsToSamples(100, _sample_format);
                sidEngine.fadein = 0;								// trigger fade-in
                sidEngine.fadeout = 1;								// trigger fade-out//
				memset(_decode_buffer, 0, 44100 * sizeof(float));
				fadevolume = 0;
				timertomute = 4;// ((sidEngine.m_config.frequency * sidEngine.m_config.playback)* 0.25);



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

// handle configuration
#define MESS(id,m,w,l) SendDlgItemMessageA(hWnd,id,m,(WPARAM)(w),(LPARAM)(l))
static BOOL CALLBACK CFGDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_NOTIFY:
            switch (LOWORD(wParam)) {
                case IDC_CHECK_ENABLEFILTER:
                    if (MESS(IDC_CHECK_ENABLEFILTER, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_6581LEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_8580LEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_6581LEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_8580LEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_TITLE_6581LEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_TITLE_8580LEVEL), TRUE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_6581LEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_8580LEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_6581LEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_8580LEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_TITLE_6581LEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_TITLE_8580LEVEL), FALSE);
                    }
                case IDC_CHECK_FADEIN:
                    if (MESS(IDC_CHECK_FADEIN, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEINLEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEINLEVEL), TRUE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEINLEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEINLEVEL), FALSE);
                    }
                case IDC_CHECK_FADEOUT:
                    if (MESS(IDC_CHECK_FADEOUT, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEOUTLEVEL), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEOUTLEVEL), TRUE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_SLIDE_FADEOUTLEVEL), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_FADEOUTLEVEL), FALSE);
                    }
                case IDC_CHECK_SKIPSHORT:
                    if (MESS(IDC_CHECK_SKIPSHORT, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MINLENGTH), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_MINLENGTH), TRUE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MINLENGTH), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_LABEL_MINLENGTH), FALSE);
                    }
                case IDC_CHECK_RANDOMDELAY:
                    if (MESS(IDC_CHECK_RANDOMDELAY, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_POWERDELAY), FALSE);
                    } else {
                        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_POWERDELAY), TRUE);
                    }
                case IDC_SLIDE_6581LEVEL:
                   MESS(IDC_LABEL_6581LEVEL, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_6581LEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0)).append("%").c_str());
                case IDC_SLIDE_8580LEVEL:
                    MESS(IDC_LABEL_8580LEVEL, WM_SETTEXT, 0, std::to_string(SendDlgItemMessage(hWnd, IDC_SLIDE_8580LEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0)).append("%").c_str());
                case IDC_SLIDE_FADEINLEVEL:
                    MESS(IDC_LABEL_FADEINLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEINLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 100.f).substr(0,4).append("s").c_str());
                case IDC_SLIDE_FADEOUTLEVEL:
                    if (SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) >= 100) {
                        MESS(IDC_LABEL_FADEOUTLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 10.f).substr(0,5).append("s").c_str());
                    } else {
                        MESS(IDC_LABEL_FADEOUTLEVEL, WM_SETTEXT, 0, std::to_string((float)SendDlgItemMessage(hWnd, IDC_SLIDE_FADEOUTLEVEL, (UINT) TBM_GETPOS, (WPARAM) 0, (LPARAM) 0) / 10.f).substr(0,4).append("s").c_str());
                        //wgc
                    }
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
					if (sidEngine.m_engine->isPlaying()) {
						sidEngine.m_engine->stop();

						haltplayer = 1;//wgc
						Sleep(100);//give the player time to stop TODO should clear buffer here sometimes nasty click with left over samples
					}
					//if (sidEngine.m_engine->isPlaying()) //wgc
						//sidEngine.m_engine->stop();

					sidSetting.c_locksidmodel = (BST_CHECKED==MESS(IDC_CHECK_LOCKSID, BM_GETCHECK, 0, 0));
					

					VOICE_1SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE1, BM_GETCHECK, 0, 0));//wgc
					VOICE_1SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE2, BM_GETCHECK, 0, 0));
					VOICE_1SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE3, BM_GETCHECK, 0, 0));
					VOICE_2SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE4, BM_GETCHECK, 0, 0));
					VOICE_2SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE5, BM_GETCHECK, 0, 0));
					VOICE_2SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE6, BM_GETCHECK, 0, 0));
					VOICE_3SID[0] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE7, BM_GETCHECK, 0, 0));
					VOICE_3SID[1] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE8, BM_GETCHECK, 0, 0));
					VOICE_3SID[2] = (!BST_CHECKED == MESS(IDC_CHECK_VOICE9, BM_GETCHECK, 0, 0));

					if (!sidSetting.c_surround)//wgc
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

					sidSetting.c_surround = (BST_CHECKED == MESS(IDC_CHECK_SURROUND1SID, BM_GETCHECK, 0, 0));//WGC
					//TO CHANGE !!!!!!! ZR//WGC

					if (sidSetting.c_surround && (sidEngine.m_config.secondSidAddress == 0 || sidEngine.m_config.secondSidAddress == 0xd400 ))//wgc
					{
						sidEngine.m_config.forceSecondSidModel = 1;
						sidEngine.m_config.secondSidAddress = 0xd400;
						sidEngine.m_config.secondSidModel = 0;
						surround = 1;
						setsidchannelssurround();
					}
					else
					{
// 						sidEngine.m_config.forceSecondSidModel = 0;
// 						//sidEngine.m_config.secondSidAddress = 0;
// 						sidEngine.m_config.secondSidModel = 0;						
						sidEngine.m_config.forceSecondSidModel = 0;
						sidEngine.m_config.secondSidAddress = 0;
						sidEngine.m_config.secondSidModel = -1;
						surround = 0;
						VOICE_1SID[0] = VOICE_1SIDsave[0]; // wgc
						VOICE_1SID[1] = VOICE_1SIDsave[1];
						VOICE_1SID[2] = VOICE_1SIDsave[2];
						VOICE_2SID[0] = VOICE_2SIDsave[0];
						VOICE_2SID[1] = VOICE_2SIDsave[1];
						VOICE_2SID[2] = VOICE_2SIDsave[2];
						VOICE_3SID[0] = VOICE_3SIDsave[0];
						VOICE_3SID[1] = VOICE_3SIDsave[1];
						VOICE_3SID[2] = VOICE_3SIDsave[2];
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
                    
                    // apply configuraton
                    saveConfig();

					//wgc
					if (sidEngine.p_song) {

						sidEngine.fadein = 0;
							sidEngine.m_engine->load(sidEngine.p_song);
							haltplayer = 0; // allows the replay loop to start 
							Sleep(100);// allow time to restart
							
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
							sidEngine.fadein = 0; // trigger fade-in (needed?)
							sidEngine.fadeout = 1; // trigger fade-out (needed?)
							xmpfin->UpdateTitle(NULL);
							_odometer = 0;
							_target_odometer = 4096;// _origtarget_odometer;
							
					}

					
                case IDCANCEL:
                    EndDialog(hWnd, 0);
                    break;
            }
            break;
        case WM_INITDIALOG:
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
            MESS(IDC_SLIDE_6581LEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_6581LEVEL, TBM_SETPOS, TRUE, sidSetting.c_6581filter);
            MESS(IDC_SLIDE_8580LEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_8580LEVEL, TBM_SETPOS, TRUE, sidSetting.c_8580filter);
            MESS(IDC_SLIDE_FADEINLEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 30));
            MESS(IDC_SLIDE_FADEINLEVEL, TBM_SETPOS, TRUE, sidSetting.c_fadeinms / 10);
            MESS(IDC_SLIDE_FADEOUTLEVEL, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            MESS(IDC_SLIDE_FADEOUTLEVEL, TBM_SETPOS, TRUE, sidSetting.c_fadeoutms / 100);
            //
            MESS(IDC_CHECK_LOCKSID, BM_SETCHECK, sidSetting.c_locksidmodel?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_LOCKCLOCK, BM_SETCHECK, sidSetting.c_lockclockspeed?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_ENABLEFILTER, BM_SETCHECK, sidSetting.c_enablefilter?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DIGIBOOST, BM_SETCHECK, sidSetting.c_enabledigiboost?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_FORCELENGTH, BM_SETCHECK, sidSetting.c_forcelength?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_RANDOMDELAY, BM_SETCHECK, sidSetting.c_powerdelayrandom?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DISABLESEEK, BM_SETCHECK, sidSetting.c_disableseek?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_DETECTPLAYER, BM_SETCHECK, sidSetting.c_detectplayer?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_SKIPSHORT, BM_SETCHECK, sidSetting.c_skipshort?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_FADEIN, BM_SETCHECK, sidSetting.c_fadein?BST_CHECKED:BST_UNCHECKED, 0);
            MESS(IDC_CHECK_FADEOUT, BM_SETCHECK, sidSetting.c_fadeout?BST_CHECKED:BST_UNCHECKED, 0);
			MESS(IDC_CHECK_LOCKSID, BM_SETCHECK, sidSetting.c_locksidmodel?BST_CHECKED:BST_UNCHECKED, 0);
			MESS(IDC_CHECK_SURROUND1SID, BM_SETCHECK, sidSetting.c_surround ? BST_CHECKED : BST_UNCHECKED, 0);
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
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE1), true);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE2), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE3), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE4), true);//wgc
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE5), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE6), true);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE7), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE8), false);
				EnableWindow(GetDlgItem(hWnd, IDC_CHECK_VOICE9), false);
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
    "SIDex (v1.3b1)",
    "SIDex\0sid",
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
            DisableThreadLibraryCalls((HMODULE)hDLL);
			//OpenConsole();//wgc
            break;
    }
    return TRUE;
}
