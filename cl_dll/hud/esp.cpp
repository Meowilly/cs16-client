#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include <stdio.h>

int CHudESP::Init()
{
	m_iFlags = HUD_ACTIVE;
	m_pCvar = CVAR_CREATE("ch_wh", "0", FCVAR_ARCHIVE);
	gHUD.AddHudElem(this);
	return 1;
}

int CHudESP::VidInit()
{
	return 1;
}

static bool W2S(const Vector &world, int &x, int &y)
{
	float s[3];
	// WorldToScreen возвращает 1, если точка за спиной
	if (gEngfuncs.pTriAPI->WorldToScreen((float *)&world, s))
		return false;

	x = (int)((1.0f + s[0]) * 0.5f * ScreenWidth);
	y = (int)((1.0f - s[1]) * 0.5f * ScreenHeight);
	return true;
}

static void DrawBox(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	FillRGBABlend(x1, y1, x2 - x1, 1, r, g, b, 255);
	FillRGBABlend(x1, y2, x2 - x1, 1, r, g, b, 255);
	FillRGBABlend(x1, y1, 1, y2 - y1, r, g, b, 255);
	FillRGBABlend(x2, y1, 1, y2 - y1, r, g, b, 255);
}

int CHudESP::Draw(float flTime)
{
	int mode = (int)m_pCvar->value;
	if (mode <= 0)
		return 0;

	// только офлайн / с читами
	if (gEngfuncs.pfnGetCvarFloat("sv_cheats") == 0.0f)
		return 0;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return 0;

	int maxClients = gEngfuncs.GetMaxClients();

	for (int i = 1; i <= maxClients; i++)
	{
		if (i == local->index)
			continue;

		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
		if (!ent || !ent->player)
			continue;
		if (g_PlayerExtraInfo[i].dead)
			continue;
		if (ent->curstate.messagenum < local->curstate.messagenum)
			continue; // сущность не обновлялась в этом кадре

		Vector head = ent->origin + Vector(0, 0, 36);
		Vector feet = ent->origin - Vector(0, 0, 36);

		int hx, hy, fx, fy;
		if (!W2S(head, hx, hy) || !W2S(feet, fx, fy))
			continue;

		int h = fy - hy;
		if (h < 4)
			continue;
		int w = h / 2;

		bool enemy = g_PlayerExtraInfo[i].teamnumber != g_PlayerExtraInfo[local->index].teamnumber;
		int r = enemy ? 255 : 0;
		int g = enemy ? 50 : 255;
		int b = 50;

		DrawBox(hx - w / 2, hy, hx + w / 2, fy, r, g, b);

		if (mode >= 2)
		{
			hud_player_info_t info;
			memset(&info, 0, sizeof(info));
			gEngfuncs.pfnGetPlayerInfo(i, &info);

			float dist = (ent->origin - local->origin).Length() / 39.37f; // юниты -> метры
			char text[96];
			snprintf(text, sizeof(text), "%s [%dm]", info.name ? info.name : "?", (int)dist);

			gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
            gEngfuncs.pfnDrawConsoleString(hx - w / 2, hy - 14, text);
		}
	}
	return 0;
}
