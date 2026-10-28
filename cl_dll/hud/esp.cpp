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
	CVAR_CREATE("ch_aim_vis", "1", FCVAR_ARCHIVE);     // 1 = не наводиться сквозь стены
	CVAR_CREATE("ch_aim_prio", "0", FCVAR_ARCHIVE);    // 0 = ближайший к прицелу, 1 = ближайший по расстоянию
	CVAR_CREATE("ch_wh_team", "1", FCVAR_ARCHIVE);     // 1 = показывать своих, 0 = только врагов
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

// Видна ли цель: луч от глаз до цели не должен упереться в стену раньше самого игрока
static bool IsVisible(const Vector &eye, const Vector &target)
{
	Vector s = eye;
	Vector e = target;
	pmtrace_t *tr = gEngfuncs.PM_TraceLine((float *)&s, (float *)&e, 0, 2, -1);
	if (!tr)
		return true;

	float dist = (target - eye).Length();
	return tr->fraction * dist >= dist - 24.0f;
}

static Vector GetEye(cl_entity_t *local)
{
	float vh[3] = { 0.0f, 0.0f, 17.0f };
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(vh);
	return local->origin + Vector(vh[0], vh[1], vh[2]);
}

// ch_aim 0/1, ch_aim_fov (градусы), ch_aim_smooth (1 = мгновенно, больше = плавнее)
// ch_aim_z (высота точки наводки), ch_aim_always (1 = без нажатия стрельбы)
// ch_aim_vis (1 = не сквозь стены), ch_aim_prio (0 = к прицелу, 1 = по расстоянию)
// Только при sv_cheats 1.
void CHudESP::DoAim()

	bool always = gEngfuncs.pfnGetCvarFloat("ch_aim_always") != 0.0f;
	if (!always && !(gHUD.m_iKeyBits & IN_ATTACK))
		return;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	Vector eye = GetEye(local);

	Vector cur;
	gEngfuncs.GetViewAngles((float *)&cur);

	float maxFov = m_pAimFov->value;
	float smooth = m_pAimSmooth->value;
	if (smooth < 1.0f)
		smooth = 1.0f;
	float aimZ = gEngfuncs.pfnGetCvarFloat("ch_aim_z");
	bool needVis = gEngfuncs.pfnGetCvarFloat("ch_aim_vis") != 0.0f;
	bool byDist = gEngfuncs.pfnGetCvarFloat("ch_aim_prio") != 0.0f;

	float bestScore = 99999.0f;
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

		if (diff > maxFov)
			continue;
		if (needVis && !IsVisible(eye, target))
			continue; // за стеной

		float score = byDist ? (target - eye).Length() : diff;
		if (score >= bestScore)
			continue;

		bestScore = score;
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
// ch_trigger 1   - стрельба, когда прицел на видимом враге
// ch_trigger_fov - допуск в градусах (2 по умолчанию)
// Только при sv_cheats 1.
void ESP_CreateMove(struct usercmd_s *cmd)
{

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	// --- автобхоп: в воздухе отпускаем прыжок, на земле держим ---
	if (gEngfuncs.pfnGetCvarFloat("ch_bhop") != 0.0f && (cmd->buttons & IN_JUMP))
	{
		Vector start = local->origin;
		Vector end = start - Vector(0, 0, 39);
		pmtrace_t *tr = gEngfuncs.PM_TraceLine((float *)&start, (float *)&end, 0, 2, -1);
		bool onGround = tr && tr->fraction < 1.0f;
		if (!onGround)
			cmd->buttons &= ~IN_JUMP;
	}

	// --- триггербот ---
	if (gEngfuncs.pfnGetCvarFloat("ch_trigger") != 0.0f)
	{
		Vector eye = GetEye(local);

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

			if (!IsVisible(eye, target))
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

// ch_wh 1/2. Красный бокс = враг за стеной, жёлтый = враг виден, зелёный = свой.
// ch_wh_team 0 скрывает своих.
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

	Vector eye = GetEye(local);
	float aimZ = gEngfuncs.pfnGetCvarFloat("ch_aim_z");
	bool showTeam = gEngfuncs.pfnGetCvarFloat("ch_wh_team") != 0.0f;

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

		bool enemy = g_PlayerExtraInfo[i].teamnumber != g_PlayerExtraInfo[local->index].teamnumber;
		if (!enemy && !showTeam)
			continue;

		Vector head = ent->origin + Vector(0, 0, 36);
		Vector feet = ent->origin - Vector(0, 0, 36);

		int hx, hy, fx, fy;
		if (!W2S(head, hx, hy) || !W2S(feet, fx, fy))
			continue;

		int h = fy - hy;
		if (h < 4)
			continue;
		int w = h / 2;

		int r, g, b;
		if (!enemy)
		{
			r = 0; g = 255; b = 50;
		}
		else if (IsVisible(eye, ent->origin + Vector(0.0f, 0.0f, aimZ)))
		{
			r = 255; g = 255; b = 0;
		}
		else
		{
			r = 255; g = 50; b = 50;
		}

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
