#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "event_api.h"
#include "in_buttons.h"
#include <stdio.h>
#include <math.h>

int CHudESP::Init()
{
	m_iFlags = HUD_ACTIVE;
	m_pCvar = CVAR_CREATE("ch_wh", "0", FCVAR_ARCHIVE);
	m_pAim = CVAR_CREATE("ch_aim", "0", FCVAR_ARCHIVE);
	m_pAimFov = CVAR_CREATE("ch_aim_fov", "30", FCVAR_ARCHIVE);
	m_pAimSmooth = CVAR_CREATE("ch_aim_smooth", "5", FCVAR_ARCHIVE);
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

static float NormalizeAngle(float a)
{
	while (a > 180.0f) a -= 360.0f;
	while (a < -180.0f) a += 360.0f;
	return a;
}

// ch_aim 0/1, ch_aim_fov (градусы), ch_aim_smooth (1 = мгновенно, больше = плавнее)
// Наводится только пока зажата кнопка стрельбы. Только при sv_cheats 1.
void CHudESP::DoAim()
{
	if (m_pAim->value == 0.0f)
		return;
	if (gEngfuncs.pfnGetCvarFloat("sv_cheats") == 0.0f)
		return;
	if (!(gHUD.m_iKeyBits & IN_ATTACK))
		return;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	float vh[3] = { 0.0f, 0.0f, 17.0f };
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(vh);
	Vector eye = local->origin + Vector(vh[0], vh[1], vh[2]);

	Vector cur;
	gEngfuncs.GetViewAngles((float *)&cur);

	float maxFov = m_pAimFov->value;
	float smooth = m_pAimSmooth->value;
	if (smooth < 1.0f)
		smooth = 1.0f;

	float bestDiff = 99999.0f;
	float bestDP = 0.0f, bestDY = 0.0f;
	bool found = false;

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
			continue;
		if (g_PlayerExtraInfo[i].teamnumber == g_PlayerExtraInfo[local->index].teamnumber)
			continue; // только враги

		Vector target = ent->origin + Vector(0, 0, 28); // голова
		Vector d = target - eye;

		float dist2d = sqrtf(d.x * d.x + d.y * d.y);
		float yaw = atan2f(d.y, d.x) * 180.0f / 3.14159265f;
		float pitch = -atan2f(d.z, dist2d) * 180.0f / 3.14159265f;

		float dy = NormalizeAngle(yaw - cur.y);
		float dp = NormalizeAngle(pitch - cur.x);
		float diff = sqrtf(dy * dy + dp * dp);

		if (diff > maxFov || diff >= bestDiff)
			continue;

		bestDiff = diff;
		bestDY = dy;
		bestDP = dp;
		found = true;
	}

	if (!found)
		return;

	Vector na;
	na.x = cur.x + bestDP / smooth;
	na.y = cur.y + bestDY / smooth;
	na.z = 0.0f;

	if (na.x > 89.0f) na.x = 89.0f;
	if (na.x < -89.0f) na.x = -89.0f;

	gEngfuncs.SetViewAngles((float *)&na);
}

int CHudESP::Draw(float flTime)
{
	DoAim();

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
