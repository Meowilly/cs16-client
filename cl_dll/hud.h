/***
*
*\tCopyright (c) 1996-2002, Valve LLC. All rights reserved.
*\t
*\tThis product contains software technology licensed from Id 
*\tSoftware, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*\tAll Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//			
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//
#pragma once

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,16,16
#define RGB_GREENISH 0x0000A000 //0,160,0
#define RGB_WHITE 0x00FFFFFF
#define RGB_GRAY 0x00808080

#include <assert.h>
#include <string.h>

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#include "csprite.h"
#include "cvardef.h"
#include "utlstring.h"

#define MIN_ALPHA	 100	
#define	HUDELEM_ACTIVE	1
#define CHudMsgFunc(x) int MsgFunc_##x(const char *pszName, int iSize, void *buf)
#define CHudUserCmd(x) void UserCmd_##x()

class RGBA
{
public:
\tunsigned char r, g, b, a;
};

enum 
{ 
\tMAX_PLAYERS = 64,
\tMAX_TEAMS = 64,
\tMAX_TEAM_NAME = 16,
\tMAX_LOCATION_NAME = 32
};

#define MAX_HOSTAGES 24

#define PLAYERMODEL_PLAYER\t0
#define PLAYERMODEL_LEET\t1
#define PLAYERMODEL_GIGN\t2
#define PLAYERMODEL_VIP\t\t3
extern const char *sPlayerModelFiles[];
extern wrect_t nullrc;

class CClientSprite;

inline bool BIsValidTModelIndex( int i )
{
\tif ( i == 1 || i == 5 || i == 6 || i == 8 || i == 11 )
\t\treturn true;
\telse
\t\treturn false;
}

inline bool BIsValidCTModelIndex( int i )
{
\tif ( i == 7 || i == 2 || i == 10 || i == 4 || i == 9)
\t\treturn true;
\telse
\t\treturn false;
}

inline bool IsASMapType()
{
\tconst char *a = gEngfuncs.pfnGetLevelName();
\treturn a && (!strnicmp( a, "maps/as_", 8 ) || !strnicmp( a, "as_", 3 ));
}

inline bool IsDEMapType()
{
\tconst char *a = gEngfuncs.pfnGetLevelName();
\treturn a && (!strnicmp( a, "maps/de_", 8 ) || !strnicmp( a, "de_", 3 ));
}

#define HUD_DRAW         (1 << 0)
#define HUD_THINK        (1 << 1)
#define HUD_ACTIVE       (HUD_DRAW | HUD_THINK)
#define HUD_INTERMISSION (1 << 2)

#define MAX_PLAYER_NAME_LENGTH\t\t32


extern cvar_t *cl_fog_r;
extern cvar_t *cl_fog_g;
extern cvar_t *cl_fog_b;
extern cvar_t *cl_fog_density;

struct FogParameters {
\tfloat color[3];
\tfloat density;
\tbool affectsSkyBox;
};

extern FogParameters g_FogParameters;

//
//-----------------------------------------------------
//
class CHudBase
{
public:
\tint\t  m_iFlags; // active, moving,
\tvirtual\t\t~CHudBase() {}
\tvirtual int Init( void ) {return 0;}
\tvirtual int VidInit( void ) {return 0;}
\tvirtual int Draw(float flTime) {return 0;}
\tvirtual void Think(void) {return;}
\tvirtual void Reset(void) {return;}
\tvirtual void InitHUDData( void ) {}\t\t// called every time a server is connected to
\tvirtual void Shutdown( void ) {}

};

struct HUDLIST {
\tCHudBase\tp;
\tHUDLIST\t\tpNext;
};



//
//-----------------------------------------------------
//
#include "voice_status.h"
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
\tfriend class WeaponsResource;
\tfriend class HistoryResource;

public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tvoid Think(void);
\tvoid Reset(void);
\tvoid SetCrosshair( HSPRITE hSpr, wrect_t rect, int r, int g, int b );
\tvoid HideCrosshair();

\t// replace engine's buggy crosshair
\tvoid DrawSpriteCrosshair();

\t// dynamic crosshair
\tvoid DrawCrosshair();
\tvoid CalculateCrosshairSize();
\tvoid CalcCrosshairDrawMode();
\tvoid CalcCrosshairColor();

\tstatic int ScaleForRes( float value, int height );
\tfloat GetCrosshairGap( int weaponId );
\tvoid DrawCrosshair( int weaponId );
\tstatic int GetWeaponAccuracyFlags( int weaponId );
\tvoid DrawCrosshairSection( int _x0, int _y0, int _x1, int _y1 );
\tvoid DrawCrosshairPadding( int _pad, int _x0, int _y0, int _x1, int _y1 );

\tint DrawWList(float flTime);
\tCHudMsgFunc(CurWeapon);
\tCHudMsgFunc(WeaponList);
\tCHudMsgFunc(AmmoX);
\tCHudMsgFunc(AmmoPickup);
\tCHudMsgFunc(WeapPickup);
\tCHudMsgFunc(ItemPickup);
\tCHudMsgFunc(HideWeapon);
\tCHudMsgFunc(Crosshair);
\tCHudMsgFunc(Brass);


\tvoid SlotInput( int iSlot );
\tCHudUserCmd(Slot1);
\tCHudUserCmd(Slot2);
\tCHudUserCmd(Slot3);
\tCHudUserCmd(Slot4);
\tCHudUserCmd(Slot5);
\tCHudUserCmd(Slot6);
\tCHudUserCmd(Slot7);
\tCHudUserCmd(Slot8);
\tCHudUserCmd(Slot9);
\tCHudUserCmd(Slot10);
\tCHudUserCmd(Close);
\tCHudUserCmd(NextWeapon);
\tCHudUserCmd(PrevWeapon);
\tCHudUserCmd(Adjust_Crosshair);
\tCHudUserCmd(Rebuy);
\tCHudUserCmd(Autobuy);

private:
\tfloat m_fFade;
\tRGBA  m_rgba;
\tWEAPON *m_pWeapon;
\tint\tm_HUD_bucket0;
\tint m_HUD_selection;

\tint m_iAlpha;
\tint m_R, m_G, m_B;
\tint m_cvarR, m_cvarG, m_cvarB;
\tint m_iCurrentCrosshair;
\tint m_iCrosshairScaleBase;
\tfloat m_flCrosshairDistance;
\tbool m_bAdditive;
\tbool m_bObserverCrosshair ;
\tbool m_bDrawCrosshair;
\tint m_iAmmoLastCheck;

\tconvar_t *m_pClCrosshairColor;
\tconvar_t *m_pClCrosshairTranslucent;
\tconvar_t *m_pClCrosshairSize;
\tcvar_t *m_pClDynamicCrosshair;
\tcvar_t *m_pHud_FastSwitch;
\tcvar_t *m_pHud_DrawHistory_Time;

\t// replace buggy engine's crosshair
\tHSPRITE m_hStaticSpr;
\twrect_t m_rcStaticRc;
\tRGBA m_staticRgba;

\tcvar_t *xhair_enable;

\tcvar_t *xhair_gap;
\tcvar_t *xhair_size;
\tcvar_t *xhair_thick;
\tcvar_t *xhair_pad;
\tcvar_t *xhair_dot;
\tcvar_t *xhair_t;
\tcvar_t *xhair_dynamic_scale;
\tcvar_t *xhair_gap_useweaponvalue;
\tcvar_t *xhair_dynamic_move;

\tcvar_t *xhair_color;
\tcvar_t *xhair_additive;

\tcvar_t *cl_crosshair_color;
\tcvar_t *cl_crosshair_translucent;
\tcvar_t *hud_draw;
};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tvoid Reset( void );
\tint Draw(float flTime);

\tCHudMsgFunc(SecAmmoVal);
\tCHudMsgFunc(SecAmmoIcon);

private:
\tenum {
\t\tMAX_SEC_AMMO_VALUES = 4
\t};

\tint m_HUD_ammoicon; // sprite indices
\tint m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
\tfloat m_fFade;
};


#include "health.h"
#include "radar.h"

#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tCHudMsgFunc(Geiger);
\t
private:
\tint m_iGeigerRange;

};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tCHudMsgFunc(Train);

private:
\tHSPRITE m_hSprite;
\tint m_iPos;

};

//
//-----------------------------------------------------
//
//  MOTD in cs16 must render HTML, so it disabled
//

class CHudMOTD : public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tvoid Reset( void );

\tCHudMsgFunc(MOTD);
\tvoid Scroll( int dir );
\tvoid Scroll( float amount );
\tfloat scroll;
\tbool m_bShow;
\tcvar_t *cl_hide_motd;

protected:
\tstatic int MOTD_DISPLAY_TIME;
\tCUtlString m_szMOTD;
\t
\tint m_iLines;
\tint m_iMaxLength;
\tbool ignoreThisMotd;
};
class CHudESP : public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
private:
\tcvar_t *m_pCvar;
};

class CHudScoreboard: public CHudBase
{
\tfriend class CHudSpectatorGui;
public:
\tint Init( void );
\tvoid InitHUDData( void );
\tint VidInit( void );
\tint Draw( float flTime );

\tint DrawScoreboard( float flTime );
\tint DrawTeams( float listslot );
\tint DrawPlayers( float listslot, int nameoffset = 0, const char *team = NULL ); // returns the ypos where it finishes drawing

\tvoid DeathMsg( int killer, int victim );
\tvoid SetScoreboardDefaults( void );
\tvoid GetAllPlayersInfo( void );

\tbool ShouldDrawScoreboard() const;

\tCHudUserCmd(ShowScores);
\tCHudUserCmd(HideScores);
\tCHudUserCmd(ShowScoreboard2);
\tCHudUserCmd(HideScoreboard2);

\tCHudMsgFunc(ScoreInfo);
\tCHudMsgFunc(TeamInfo);
\tCHudMsgFunc(TeamScore);
\tCHudMsgFunc(TeamNames);

\tint m_iPlayerNum;
\tint m_iNumTeams;

\tbool m_bForceDraw; // if called by showscoreboard2
\tbool m_bShowscoresHeld;

private:
\tint m_iLastKilledBy;
\tint m_fLastKillTime;
\tRGBA m_colors;
\tbool m_bDrawStroke;
\tfloat m_flForceRect[4]; // showscoreboard2 rectangle
\tcvar_t *cl_showpacketloss;
\tcvar_t *cl_showplayerversion;
\tcvar_t *cl_show_scoreboard_on_death;
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tvoid Reset( void );
\tvoid ParseStatusString( int line_num );

\tCHudMsgFunc(StatusText);
\tCHudMsgFunc(StatusValue);

protected:
\tenum {
\t\tMAX_STATUSTEXT_LENGTH = 128,
\t\tMAX_STATUSBAR_VALUES = 8,
\t\tMAX_STATUSBAR_LINES = 1
\t};

\tchar m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
\tchar m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];\t// the constructed bar that is drawn
\tint m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

\tint m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

\t// an array of colors...one color for each line
\tfloat *m_pflNameColors[MAX_STATUSBAR_LINES];

\tcvar_t *hud_centerid;
};

struct extra_player_info_t 
{
\tshort frags;
\tshort deaths;
\tshort team_id;
\tqboolean has_c4;
\tqboolean vip;
\tVector origin;

\t// radar stuff...
\t// float radarflash;
\t// qboolean radarflashon;
\t// int radarflashes;
\tint radarflashes;
\tfloat radarflashtime;
\tfloat radarflashtimedelta;
\tbool nextflash;

\tshort playerclass;
\tshort teamnumber;
\tchar teamname[MAX_TEAM_NAME];
\tbool dead;
\tfloat showhealth;
\tint health;
\tbool talking;
\tchar location[MAX_LOCATION_NAME];
\tint sb_health;
\tint sb_account;
\tqboolean has_defuse_kit;
};

struct team_info_t 
{
\tchar name[MAX_TEAM_NAME];
\tshort frags;
\tshort deaths;
\tshort ownteam;
\tshort players;
\tint already_drawn;
\tint scores_overriden;
\tint sumping;
\tint teamnumber;
};

struct hostage_info_t
{
\tvec3_t origin;
\tfloat radarflashtimedelta;
\tfloat radarflashtime;
\tbool dead;
\tbool nextflash;
\tint radarflashes;
};

extern hud_player_info_t\tg_PlayerInfoList[MAX_PLAYERS+1];\t   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
extern team_info_t\t\t\tg_TeamInfo[MAX_TEAMS+1];
extern hostage_info_t\t\tg_HostageInfo[MAX_HOSTAGES+1];
extern int\t\t\t\t\tg_IsSpectator[MAX_PLAYERS+1];


//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
\tint Init( void );
\tvoid InitHUDData( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tCHudMsgFunc(DeathMsg);

private:
\tint m_HUD_d_skull;  // sprite index of skull icon
\tint m_HUD_d_headshot;
\tcvar_t *hud_deathnotice_time;
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
\tint Init( void );
\tvoid InitHUDData( void );
\tint VidInit( void );
\tvoid Reset( void );
\tint Draw( float flTime );

\tCHudMsgFunc(ShowMenu);
\tCHudMsgFunc(BuyClose);
\tCHudMsgFunc(VGUIMenu);
\t// server sends false when spectating is not allowed, and true when allowed
\tCHudMsgFunc(AllowSpec);

\tCHudUserCmd(OldStyleMenuClose);
\tCHudUserCmd(OldStyleMenuOpen);
\tCHudUserCmd(ShowVGUIMenu);

\tvoid ShowVGUIMenu( int menuType ); // cs16client extension

\tvoid SelectMenuItem( int menu_item );

\tint m_fMenuDisplayed;
\tbool m_bAllowSpec;
\tcvar_t *_extended_menus;
\tint m_bitsValidSlots;
\tfloat m_flShutoffTime;
\tint m_fWaitingForMore;

};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
\tint Init( void );
\tvoid InitHUDData( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tCHudMsgFunc(SayText);
\tvoid SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
\tvoid SayTextPrint( char szBuf[3][256] );
\tvoid EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
\tfriend class CHudSpectator;

private:

\tstruct cvar_s *\tm_HUD_saytext;
\tstruct cvar_s *\tm_HUD_saytext_time;
\tstruct cvar_s *\tm_HUD_saytext_console;
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tCHudMsgFunc(Battery);
\tCHudMsgFunc(ArmorType);
\t
private:
\tenum armortype_t {
\t\tVest = 0,
\t\tVestHelm
\t} m_enArmorType;

\tCClientSprite m_hEmpty[VestHelm + 1];
\tCClientSprite m_hFull[VestHelm + 1];
\tint\t  m_iBat;
\tfloat m_fFade;
\tint\t  m_iHeight;\t\t// width of the battery innards
};


//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tvoid Reset( void );
\tCHudMsgFunc(Flashlight);
\tCHudMsgFunc(FlashBat);
\t
private:
\tCClientSprite m_hSprite1;
\tCClientSprite m_hSprite2;
\tCClientSprite m_hBeam;
\tfloat m_flBat;
\tint\t  m_iBat;
\tint\t  m_fOn;
\tfloat m_fFade;
\tint\t  m_iWidth;\t\t// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
\tclient_textmessage_t\tpMessage;
\tfloat\ttime;
\tint x, y;
\tint totalWidth, totalHeight;
\tint width;
\tint lines;
\tint lineLength;
\tint length;
\tint r, g, b;
\tint text;
\tint fadeBlend;
\tfloat charTime;
\tfloat fadeTime;
};

struct hud_message_t
{
\tclient_textmessage_t\tpMessage;
\t// unsigned int font;
\tchar args[4][256]; // 1024 bytes
\tint arg_count;
};

//
//-----------------------------------------------------
//

class CHudTextMessage: public CHudBase
{
public:
\tint Init( void );
\tstatic char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
\tstatic char *BufferedLocaliseTextString( const char *msg );
\tstatic char *LookupString( char *msg_name, int *msg_dest = NULL );
\tCHudMsgFunc(TextMsg);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
\tfriend class CHudTextMessage;
\tint Init( void );
\tint VidInit( void );
\tint Draw(float flTime);
\tCHudMsgFunc(HudText);
\tCHudMsgFunc(GameTitle);
\tCHudMsgFunc(HudTextArgs);
\tCHudMsgFunc(HudTextPro);

\tfloat FadeBlend( float fadein, float fadeout, float hold, float localTime );
\tint\tXPosition( float x, int width, int lineWidth );
\tint YPosition( float y, int height );

\tvoid MessageAdd( const char *pName, float time, qboolean hintMessage = 0/*, unsigned int font = 0 */ );
\tvoid MessageAdd( client_textmessage_t *newMessage );
\tvoid MessageDrawScan( client_textmessage_t *pMessage, float time );
\tvoid MessageScanStart( void );
\tvoid MessageScanNextChar( void );
\tvoid Reset( void );

\tclient_textmessage_t *AllocMessage( const char *text = NULL, client_textmessage_t *copyFrom = NULL );

private:
\thud_message_t\t\t\tm_pMessages[maxHUDMessages];
\tfloat\t\t\t\t\tm_startTime[maxHUDMessages];
\tmessage_parms_t\t\t\tm_parms;
\tfloat\t\t\t\t\tm_gameTitleTime;
\tclient_textmessage_t\t\tm_pGameTitle;

\tint m_HUD_title_life;
\tint m_HUD_title_half;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH\t24

class CHudStatusIcons: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tvoid Reset( void );
\tint Draw(float flTime);
\tCHudMsgFunc(StatusIcon);

\tenum {
\t\tMAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
\t\tMAX_ICONSPRITES = 4,
\t};

\t
\t//had to make these public so CHud could access them (to enable concussion icon)
\t//could use a friend declaration instead...
\tvoid EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue );
\tvoid DisableIcon( const char *pszIconName );

\tfriend class CHudScoreboard;

private:

\ttypedef struct
\t{
\t\tchar szSpriteName[MAX_ICONSPRITENAME_LENGTH];
\t\tHSPRITE spr;
\t\twrect_t rc;
\t\tunsigned char r, g, b;
\t\tunsigned char secR, secG, secB;
\t\tfloat flTimeToChange;
\t} icon_sprite_t;

\ticon_sprite_t m_IconList[MAX_ICONSPRITES];
};


//
//-----------------------------------------------------
//
#define MONEY_YPOS ScreenHeight - 3 * gHUD.m_iFontHeight

class CHudMoney : public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tCHudMsgFunc(Money);
\tCHudMsgFunc(BlinkAcct);

private:
\tint m_iMoneyCount;
\tint m_iDelta;
\tint m_iBlinkAmt;
\tfloat m_fBlinkTime;
\tfloat m_fFade;
\tCClientSprite m_hDollar;
\tCClientSprite m_hPlus;
\tCClientSprite m_hMinus;
};
//
//-----------------------------------------------------
//
class CHudRadio: public CHudBase
{
public:
\tint Init( void );
\tvoid Voice(int entindex, bool bTalking );
\t// play a sentence from a radio
\t// [byte] unknown (always 1)
\t// [string] sentence name
\t// [short] unknown. (always 100, it's a volume?)
\tCHudMsgFunc(SendAudio);
\tCHudMsgFunc(ReloadSound);
\tCHudMsgFunc(BotVoice);
};

//
//-----------------------------------------------------
//
class CHudTimer: public CHudBase
{
\tfriend class CHudSpectatorGui;
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw(float fTime);
\t// set up the timer.
\t// [short]
\tCHudMsgFunc(RoundTime);
\t// show the timer
\t// [empty]
\tCHudMsgFunc(ShowTimer);

\tint m_right;
private:
\tint m_HUD_timer;
\tint m_iTime;
\tfloat m_fStartTime;
\tbool m_bPanicColorChange;
\tfloat m_flPanicTime;
};
//
//-----------------------------------------------------
//
class CHudProgressBar: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tvoid Reset( void );

\t// start progress bar
\t// [short] Duration
\tCHudMsgFunc(BarTime);

\t// [short] Duration
\t// [short] percent
\tCHudMsgFunc(BarTime2);
\tCHudMsgFunc(BotProgress);

private:
\tint m_iDuration;
\tfloat m_fPercent;
\tfloat m_fStartTime;
\tchar m_szHeader[256];
\tconst char *m_szLocalizedHeader;
};

//
//-----------------------------------------------------
//
// class for drawing sniper scope
class CHudSniperScope: public CHudBase
{
public:
\tint Init( void );
\tint VidInit( void );
\tint Draw( float flTime );
\tvoid Shutdown( void );

private:
\tfloat left, right, centerx, centery;
\tint m_iScopeArc[4];
};

//
//-----------------------------------------------------
//

class CHudNVG: public CHudBase
{
public:
\tint Init( void );
\tint Draw( float flTime );
\tvoid Reset( void );
\tCHudMsgFunc(NVGToggle);

\tCHudUserCmd(NVGAdjustUp);
\tCHudUserCmd(NVGAdjustDown);
private:
\tint m_iAlpha;
\tcvar_t *cl_fancy_nvg;
\tdlight_t *m_pLight;
};

//
//-----------------------------------------------------
//

class CHudScenario : public CHudBase
{
public:
\tint Init();
\tint VidInit();
\tint Draw(float flTime);
\tvoid Reset();

\tCHudMsgFunc(Scenario);
private:
\tCClientSprite m_sprite;
\tint m_iAlpha;
\tint m_iFlashAlpha;
\tfloat m_fFlashRate;
\tfloat m_fNextFlash;
};

//
//-----------------------------------------------------
//

class CHudSpectatorGui: public CHudBase
{
public:
\tint Init();
\tint VidInit();
\tint Draw( float flTime );
\tvoid InitHUDData();
\tvoid Reset();
\tvoid Shutdown();

\tCHudMsgFunc( SpecHealth );
\tCHudMsgFunc( SpecHealth2 );

\tCHudUserCmd( ToggleSpectatorMenu );
\tCHudUserCmd( ToggleSpectatorMenuOptions );
\tCHudUserCmd( ToggleSpectatorMenuOptionsSettings );
\tCHudUserCmd( ToggleSpectatorMenuSpectateOptions );

\tvoid CalcAllNeededData( );

\tbool m_bBombPlanted;

private:\t
\t// szMapName is 64 bytes only. Removing "maps/" and ".bsp" gived me this result
\tclass Labels
\t{
\tpublic:
\t\tshort m_iTerrorists;
\t\tshort m_iCounterTerrorists;
\t\tchar m_szTimer[64];
\t\tchar m_szMap[64];
\t\tchar m_szNameAndHealth[80];
\t} label;
\tint m_hTimerTexture;
\tint m_hChecked;
\tint m_hArrowDown;
\tint m_hArrowUp;
\tint m_hArrowLeft;
\tint m_hArrowRight;

\tenum {
\t\tROOT_MENU = (1<<0),
\t\tMENU_OPTIONS = (1<<1),
\t\tMENU_OPTIONS_SETTINGS = (1<<2),
\t\tMENU_SPEC_OPTIONS = (1<<3)
\t};
\tbyte m_menuFlags;
};

//
//-----------------------------------------------------
//

class CHud
{
public:
\tCHud() : m_pHudList(NULL), m_iSpriteCount(0), m_iRes(640)  {}
\t~CHud();\t\t\t// destructor, frees allocated memory // thanks, Captain Obvious

\tvoid Init( void );
\tvoid VidInit( void );
\tvoid Think( void );
\tvoid Reset( void );
\tvoid Shutdown( void );
\tint Redraw( float flTime, int intermission );
\tint UpdateClientData( client_data_t *cdata, float time );
\tvoid AddHudElem(CHudBase *p);
\tCHudESP m_ESP;

\tinline float GetSensitivity() { return m_flMouseSensitivity; }
\tinline HSPRITE GetSprite( int index )
\t{
\t\tassert( index >= -1 && index <= m_iSpriteCount );

\t\treturn (index >= 0) ? m_rghSprites[index] : 0;
\t}

\tinline wrect_t& GetSpriteRect( int index )
\t{
\t\tassert( index >= -1 && index <= m_iSpriteCount );

\t\treturn (index >= 0) ? m_rgrcRects[index]: nullrc;
\t}

\t// GetSpriteIndex()
\t// searches through the sprite list loaded from hud.txt for a name matching SpriteName
\t// returns an index into the gHUD.m_rghSprites[] array
\t// returns -1 if sprite not found
\tinline int GetSpriteIndex( const char *SpriteName )
\t{
\t\t// look through the loaded sprite name list for SpriteName
\t\tfor ( int i = 0; i < m_iSpriteCount; i++ )
\t\t{
\t\t\tif ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
\t\t\t\treturn i;
\t\t}

\t\tgEngfuncs.Con_Printf( "GetSpriteIndex: %s sprite not found\n", SpriteName );
\t\treturn -1; // invalid sprite
\t}

\tinline short GetCharWidth ( unsigned char ch )
\t{
\t\treturn m_scrinfo.charWidths[ ch ];
\t}

\tinline int GetCharHeight( )
\t{
\t\treturn m_scrinfo.iCharHeight;
\t}

\tinline int GetGameType( )
\t{
\t\treturn m_iGameType;
\t}

\tinline int GetSpriteRes()
\t{
\t\treturn m_iRes;
\t}


\tfloat   m_flTime;      // the current client time
\tfloat   m_fOldTime;    // the time at which the HUD was last redrawn
\tdouble  m_flTimeDelta; // the difference between flTime and fOldTime
\tfloat   m_flScale;     // hud_scale->value
\tVector\tm_vecOrigin;
\tVector\tm_vecAngles;
\tint\t\tm_iKeyBits;
\tint\t\tm_iHideHUDDisplay;
\tint\t\tm_iFOV;
\tint\t\tm_Teamplay;
\tcvar_t *m_pCvarDraw;
\tcvar_t *fastsprites;
\tcvar_t *cl_nopred;
\tcvar_t *cl_weapon_wallpuff;
\tcvar_t *cl_weapon_sparks;
\tcvar_t *cl_lw;
\tcvar_t *cl_righthand;
\tcvar_t *cl_weather;
\tcvar_t *cl_minmodels;
\tcvar_t *cl_min_t;
\tcvar_t *cl_min_ct;
\tcvar_t *cl_gunsmoke;
\tcvar_t *hud_textmode;
\tcvar_t *hud_colored;
\tcvar_t *cscl_currentmoney;
\tcvar_t *cscl_currentmap;
\tcvar_t *cscl_mapprefix;
\tcvar_t *cl_viewbob;

\tcvar_t* m_pCvarColor;
\tunsigned long m_iDefaultHUDColor;
\tvoid UpdateDefaultHUDColor();

\tHSPRITE m_hGasPuff;

\tint m_iFontHeight, m_iFontWidth;
\tCHudAmmo        m_Ammo;
\tCHudHealth      m_Health;
\tCHudSpectator   m_Spectator;
\tCHudGeiger      m_Geiger;
\tCHudBattery\t    m_Battery;
\tCHudTrain       m_Train;
\tCHudFlashlight  m_Flash;
\tCHudMessage     m_Message;
\tCHudStatusBar   m_StatusBar;
\tCHudDeathNotice m_DeathNotice;
\tCHudSayText     m_SayText;
\tCHudMenu        m_Menu;
\tCHudAmmoSecondary m_AmmoSecondary;
\tCHudTextMessage m_TextMessage;
\tCHudStatusIcons m_StatusIcons;
\tCHudScoreboard  m_Scoreboard;
\tCHudMOTD        m_MOTD;
\tCHudMoney       m_Money;
\tCHudTimer       m_Timer;
\tCHudRadio       m_Radio;
\tCHudProgressBar m_ProgressBar;
\tCHudSniperScope m_SniperScope;
\tCHudNVG         m_NVG;
\tCHudRadar       m_Radar;
\tCHudSpectatorGui m_SpectatorGui;
\tCHudScenario\tm_Scenario;

\t// user messages
\tCHudMsgFunc(Damage);
\tCHudMsgFunc(GameMode);
\tCHudMsgFunc(Logo);
\tCHudMsgFunc(ResetHUD);
\tCHudMsgFunc(InitHUD);
\tCHudMsgFunc(ViewMode);
\tCHudMsgFunc(SetFOV);
\tCHudMsgFunc(Concuss);
\tCHudMsgFunc(ShadowIdx);
\tCHudMsgFunc(ServerName);

\tCHudMsgFunc(Fog);

\t// Screen information
\tSCREENINFO\tm_scrinfo;
\t// As Xash3D can fake m_scrinfo for hud scailing
\t// we will use a real screen parameters
\tSCREENINFO  m_truescrinfo;

\tint\tm_iWeaponBits;
\tint\tm_fPlayerDead;
\tint m_iIntermission;
\tint m_iNoConsolePrint;

\t// sprite indexes
\tint m_HUD_number_0;

\tchar m_szServerName[64];

\tint m_WhiteTex;

\tcvar_t *m_pShowHealth;
\tcvar_t *m_pShowMoney;

private:
\tvoid SetGameType();

\tHUDLIST\t*m_pHudList;
\tHSPRITE\tm_hsprLogo;
\tint\tm_iLogo;
\tclient_sprite_t\t*m_pSpriteList;
\tint\tm_iSpriteCount;
\tint\tm_iSpriteCountAllRes;
\tfloat m_flMouseSensitivity;
\tint\tm_iConcussionEffect;
\tint\tm_iForceCamera;
\tint m_iForceChaseCam;
\tint m_iFadeToBlack;
\tint m_iGameType;
\tint\tm_iRes;

\t// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
\t// freed in ~CHud()
\tHSPRITE *m_rghSprites;\t/*[HUD_SPRITE_COUNT]*/\t\t\t// the sprites loaded from hud.txt
\twrect_t *m_rgrcRects;\t/*[HUD_SPRITE_COUNT]*/
\tchar *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

\tcvar_t *hud_draw;
\tcvar_t *default_fov;
\tcvar_t *zoom_sens_ratio;
};

extern CHud gHUD;
extern cvar_t *sensitivity;

extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;
