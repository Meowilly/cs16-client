//666
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "event_api.h"
#include "in_buttons.h"
#include "usercmd.h"
#include "pmtrace.h"
#include <stdio.h>
#include <math.h>

int CHudESP::Init()
{
	m_iFlags = HUD_ACTIVE;
	m_pCvar = CVAR_CREATE("ch_wh", "0", FCVAR_ARCHIVE);
	m_pAim = CVAR_CREATE("ch_aim", "0", FCVAR_ARCHIVE);
	m_pAimFov = CVAR_CREATE("ch_aim_fov", "30", FCVAR_ARCHIVE);
	m_pAimSmooth = CVAR_CREATE("ch_aim_smooth", "5", FCVAR_ARCHIVE);
	CVAR_CREATE("ch_aim_z", "14", FCVAR_ARCHIVE);      // высота точки наводки от центра тела
	CVAR_CREATE("ch_aim_always", "0", FCVAR_ARCHIVE);  // 1 = наводиться всегда, 0 = только при стрельбе
	CVAR_CREATE("ch_trigger", "0", FCVAR_ARCHIVE);     // триггербот
	CVAR_CREATE("ch_trigger_fov", "2", FCVAR_ARCHIVE); // допуск в градусах
	CVAR_CREATE("ch_bhop", "0", FCVAR_ARCHIVE);        // автобхоп
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

static void CalcAngles(const Vector &eye, const Vector &target, float &yaw, float &pitch)
{
	Vector d = target - eye;
	float dist2d = sqrtf(d.x * d.x + d.y * d.y);
	yaw = atan2f(d.y, d.x) * 180.0f / 3.14159265f;
	pitch = -atan2f(d.z, dist2d) * 180.0f / 3.14159265f;
}

// ch_aim 0/1, ch_aim_fov (градусы), ch_aim_smooth (1 = мгновенно, больше = плавнее)
// ch_aim_z (высота точки наводки), ch_aim_always (1 = без нажатия стрельбы)
void CHudESP::DoAim()
{
	if (m_pAim->value == 0.0f)
		return;

	bool always = gEngfuncs.pfnGetCvarFloat("ch_aim_always") != 0.0f;
	if (!always && !(gHUD.m_iKeyBits & IN_ATTACK))
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
	float aimZ = gEngfuncs.pfnGetCvarFloat("ch_aim_z");

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

		Vector target = ent->origin + Vector(0.0f, 0.0f, aimZ);
		float yaw, pitch;
		CalcAngles(eye, target, yaw, pitch);

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

// Вызывается из CL_CreateMove (input.cpp): автобхоп и триггербот.
// ch_bhop 1      - автобхоп (держи пробел)
// ch_trigger 1   - стрельба, когда прицел на враге
// ch_trigger_fov - допуск в градусах (2 по умолчанию)
void ESP_CreateMove(struct usercmd_s *cmd)
{
	if (!cmd)
		return;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	// --- автобхоп: в воздухе отпускаем прыжок, на земле держим ---
	if (gEngfuncs.pfnGetCvarFloat("ch_bhop") != 0.0f && (cmd->buttons & IN_JUMP))
	{
		Vector start = local->origin;
		Vector end = start - Vector(0.0f, 0.0f, 39.0f);
		pmtrace_t *tr = gEngfuncs.PM_TraceLine((float *)&start, (float *)&end, 0, 2, -1);
		bool onGround = tr && tr->fraction < 1.0f;
		if (!onGround)
			cmd->buttons &= ~IN_JUMP;
	}

	// --- триггербот ---
	if (gEngfuncs.pfnGetCvarFloat("ch_trigger") != 0.0f)
	{
		float vh[3] = { 0.0f, 0.0f, 17.0f };
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(vh);
		Vector eye = local->origin + Vector(vh[0], vh[1], vh[2]);

		float maxDiff = gEngfuncs.pfnGetCvarFloat("ch_trigger_fov");
		if (maxDiff < 0.5f)
			maxDiff = 0.5f;
		float aimZ = gEngfuncs.pfnGetCvarFloat("ch_aim_z");

		bool fire = false;
		int maxClients = gEngfuncs.GetMaxClients();
		for (int i = 1; i <= maxClients && !fire; i++)
		{
			if (i == local->index)
				continue;

			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
			if (!ent || !ent->player)
				continue;
			if (g_PlayerExtraInfo[i].dead)
				continue;
			if (g_PlayerExtraInfo[i].teamnumber == g_PlayerExtraInfo[local->index].teamnumber)
				continue;

			Vector target = ent->origin + Vector(0.0f, 0.0f, aimZ);
			float yaw, pitch;
			CalcAngles(eye, target, yaw, pitch);

			float dy = NormalizeAngle(yaw - cmd->viewangles[1]);
			float dp = NormalizeAngle(pitch - cmd->viewangles[0]);
			if (sqrtf(dy * dy + dp * dp) > maxDiff)
				continue;

			// видимость: луч до цели не должен упереться в стену раньше самого игрока
			float dist = (target - eye).Length();
			pmtrace_t *tr = gEngfuncs.PM_TraceLine((float *)&eye, (float *)&target, 0, 2, -1);
			if (tr && tr->fraction * dist < dist - 24.0f)
				continue;

			fire = true;
		}

		// через кадр, чтобы пистолеты и снайперки успевали отпускать кнопку
		static bool toggle = false;
		if (fire)
		{
			toggle = !toggle;
			if (toggle)
				cmd->buttons |= IN_ATTACK;
		}
	}
}

int CHudESP::Draw(float flTime)
{
	DoAim();

	int mode = (int)m_pCvar->value;
	if (mode <= 0)
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

		Vector head = ent->origin + Vector(0.0f, 0.0f, 36.0f);
		Vector feet = ent->origin - Vector(0.0f, 0.0f, 36.0f);

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
