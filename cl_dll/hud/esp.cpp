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

#define PI_F 3.14159265f

int CHudESP::Init()
{
	m_iFlags = HUD_ACTIVE;
	m_pCvar = CVAR_CREATE("ch_wh", "0", FCVAR_ARCHIVE);
	m_pAim = CVAR_CREATE("ch_aim", "0", FCVAR_ARCHIVE);
	m_pAimFov = CVAR_CREATE("ch_aim_fov", "30", FCVAR_ARCHIVE);
	m_pAimSmooth = CVAR_CREATE("ch_aim_smooth", "5", FCVAR_ARCHIVE);
	CVAR_CREATE("ch_aim_bone", "1", FCVAR_ARCHIVE);    // 0 = голова, 1 = шея, 2 = грудь, 3 = своя высота (ch_aim_z)
	CVAR_CREATE("ch_aim_z", "14", FCVAR_ARCHIVE);      // своя высота точки наводки (для ch_aim_bone 3)
	CVAR_CREATE("ch_aim_always", "0", FCVAR_ARCHIVE);  // 1 = наводиться всегда, 0 = только при стрельбе
	CVAR_CREATE("ch_aim_vis", "1", FCVAR_ARCHIVE);     // 1 = не наводиться сквозь стены
	CVAR_CREATE("ch_aim_prio", "0", FCVAR_ARCHIVE);    // 0 = ближайший к прицелу, 1 = ближайший по расстоянию
	CVAR_CREATE("ch_aim_circle", "0", FCVAR_ARCHIVE);  // 1 = рисовать круг FOV аима
	CVAR_CREATE("ch_wh_team", "1", FCVAR_ARCHIVE);     // 1 = показывать своих, 0 = только врагов
	CVAR_CREATE("ch_wh_lines", "0", FCVAR_ARCHIVE);    // 1 = линии к врагам
	CVAR_CREATE("ch_wh_all", "0", FCVAR_ARCHIVE);      // 1 = показывать и тех, кого клиент давно не обновлял (последняя позиция)
	CVAR_CREATE("ch_radar", "0", FCVAR_ARCHIVE);       // 1 = радар
	CVAR_CREATE("ch_radar_size", "160", FCVAR_ARCHIVE);
	CVAR_CREATE("ch_radar_range", "1500", FCVAR_ARCHIVE);
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

// ---------------------------------------------------------------- helpers

static float Cv(const char *name)
{
	return gEngfuncs.pfnGetCvarFloat(name);
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

static void Rect(int x, int y, int w, int h, int r, int g, int b, int a)
{
	if (w <= 0 || h <= 0)
		return;
	if (x > ScreenWidth || y > ScreenHeight || x + w < 0 || y + h < 0)
		return;
	FillRGBABlend(x, y, w, h, r, g, b, a);
}

static void DrawBox(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	Rect(x1, y1, x2 - x1, 1, r, g, b, 255);
	Rect(x1, y2, x2 - x1, 1, r, g, b, 255);
	Rect(x1, y1, 1, y2 - y1, r, g, b, 255);
	Rect(x2, y1, 1, y2 - y1, r, g, b, 255);
}

static void DrawLine(int x0, int y0, int x1, int y1, int r, int g, int b)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int adx = dx < 0 ? -dx : dx;
	int ady = dy < 0 ? -dy : dy;
	int steps = (adx > ady ? adx : ady) / 3;
	if (steps < 1)
		steps = 1;
	if (steps > 600)
		return; // слишком длинная, цель далеко за экраном

	for (int i = 0; i <= steps; i++)
	{
		int x = x0 + dx * i / steps;
		int y = y0 + dy * i / steps;
		Rect(x, y, 2, 2, r, g, b, 200);
	}
}

static void DrawCircle(int cx, int cy, int radius, int r, int g, int b)
{
	if (radius < 2)
		return;
	const int seg = 72;
	for (int i = 0; i < seg; i++)
	{
		float a = (float)i * 2.0f * PI_F / (float)seg;
		int x = cx + (int)(cosf(a) * radius);
		int y = cy + (int)(sinf(a) * radius);
		Rect(x, y, 2, 2, r, g, b, 200);
	}
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
	yaw = atan2f(d.y, d.x) * 180.0f / PI_F;
	pitch = -atan2f(d.z, dist2d) * 180.0f / PI_F;
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

// Точка наводки: ch_aim_bone 0 = голова, 1 = шея, 2 = грудь, 3 = ch_aim_z
static Vector AimPoint(cl_entity_t *ent)
{
	int bone = (int)Cv("ch_aim_bone");
	float z;
	switch (bone)
	{
	case 0: z = 18.0f; break;
	case 1: z = 12.0f; break;
	case 2: z = 5.0f; break;
	default: z = Cv("ch_aim_z"); break;
	}
	if (ent->curstate.usehull == 1)
		z -= 6.0f; // присел
	return ent->origin + Vector(0.0f, 0.0f, z);
}

static cl_entity_t *GetPlayer(int i, cl_entity_t *local, bool needFresh)
{
	if (i == local->index)
		return NULL;

	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
	if (!ent || !ent->player)
		return NULL;
	if (g_PlayerExtraInfo[i].dead)
		return NULL;
	if (needFresh && ent->curstate.messagenum < local->curstate.messagenum)
		return NULL;
	return ent;
}

static bool IsEnemy(int i, cl_entity_t *local)
{
	return g_PlayerExtraInfo[i].teamnumber != g_PlayerExtraInfo[local->index].teamnumber;
}

// ---------------------------------------------------------------- aim

// ch_aim 0/1, ch_aim_fov (градусы), ch_aim_smooth (1 = мгновенно, больше = плавнее)
// ch_aim_bone, ch_aim_always, ch_aim_vis, ch_aim_prio.
void CHudESP::DoAim()
{
	if (Cv("ch_aim") == 0.0f)
		return;

	bool always = Cv("ch_aim_always") != 0.0f;
	if (!always && !(gHUD.m_iKeyBits & IN_ATTACK))
		return;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	Vector eye = GetEye(local);

	Vector cur;
	gEngfuncs.GetViewAngles((float *)&cur);

	float maxFov = Cv("ch_aim_fov");
	float smooth = Cv("ch_aim_smooth");
	if (smooth < 1.0f)
		smooth = 1.0f;
	bool needVis = Cv("ch_aim_vis") != 0.0f;
	bool byDist = Cv("ch_aim_prio") != 0.0f;

	float bestScore = 99999.0f;
	float bestDP = 0.0f, bestDY = 0.0f;
	bool found = false;

	int maxClients = gEngfuncs.GetMaxClients();
	for (int i = 1; i <= maxClients; i++)
	{
		cl_entity_t *ent = GetPlayer(i, local, true);
		if (!ent || !IsEnemy(i, local))
			continue;

		Vector target = AimPoint(ent);

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

// ---------------------------------------------------------------- trigger + bhop

// Вызывается из CL_CreateMove (input.cpp): автобхоп и триггербот.
void ESP_CreateMove(struct usercmd_s *cmd)
{
	if (!cmd)
		return;

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;

	// --- автобхоп: в воздухе отпускаем прыжок, на земле держим ---
	if (Cv("ch_bhop") != 0.0f && (cmd->buttons & IN_JUMP))
	{
		Vector start = local->origin;
		Vector end = start - Vector(0.0f, 0.0f, 39.0f);
		pmtrace_t *tr = gEngfuncs.PM_TraceLine((float *)&start, (float *)&end, 0, 2, -1);
		bool onGround = tr && tr->fraction < 1.0f;
		if (!onGround)
			cmd->buttons &= ~IN_JUMP;
	}

	// --- триггербот ---
	if (Cv("ch_trigger") != 0.0f)
	{
		Vector eye = GetEye(local);

		float maxDiff = Cv("ch_trigger_fov");
		if (maxDiff < 0.5f)
			maxDiff = 0.5f;

		bool fire = false;
		int maxClients = gEngfuncs.GetMaxClients();
		for (int i = 1; i <= maxClients && !fire; i++)
		{
			cl_entity_t *ent = GetPlayer(i, local, true);
			if (!ent || !IsEnemy(i, local))
				continue;

			Vector target = AimPoint(ent);
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

// ---------------------------------------------------------------- draw parts

// Круг, показывающий радиус захвата аима (ch_aim_fov)
static void DrawFovCircle()
{
	if (Cv("ch_aim_circle") == 0.0f)
		return;

	float aimFov = Cv("ch_aim_fov");
	if (aimFov > 80.0f)
		aimFov = 80.0f;
	float fov = gHUD.m_iFOV > 0 ? (float)gHUD.m_iFOV : 90.0f;

	float k = tanf(aimFov * PI_F / 180.0f) / tanf(fov * 0.5f * PI_F / 180.0f);
	int radius = (int)(k * ScreenWidth * 0.5f);
	if (radius > ScreenWidth)
		radius = ScreenWidth;

	DrawCircle(ScreenWidth / 2, ScreenHeight / 2, radius, 255, 255, 255);
}

// Радар слева по центру экрана. Вверх = куда смотришь.
static void DrawRadar(cl_entity_t *local)
{
	if (Cv("ch_radar") == 0.0f)
		return;

	int size = (int)Cv("ch_radar_size");
	if (size < 60) size = 60;
	if (size > 400) size = 400;
	float range = Cv("ch_radar_range");
	if (range < 200.0f) range = 200.0f;
	bool showTeam = Cv("ch_wh_team") != 0.0f;
	bool all = Cv("ch_wh_all") != 0.0f;

	int x0 = 10;
	int y0 = ScreenHeight / 2 - size / 2;
	int cx = x0 + size / 2;
	int cy = y0 + size / 2;

	Rect(x0, y0, size, size, 0, 0, 0, 110);
	Rect(x0, y0, size, 1, 200, 200, 200, 255);
	Rect(x0, y0 + size - 1, size, 1, 200, 200, 200, 255);
	Rect(x0, y0, 1, size, 200, 200, 200, 255);
	Rect(x0 + size - 1, y0, 1, size, 200, 200, 200, 255);
	Rect(cx, y0, 1, size, 120, 120, 120, 120);
	Rect(x0, cy, size, 1, 120, 120, 120, 120);

	Vector va;
	gEngfuncs.GetViewAngles((float *)&va);
	float rad = va.y * PI_F / 180.0f;
	float cs = cosf(rad);
	float sn = sinf(rad);

	float scale = ((float)size * 0.5f - 4.0f) / range;

	int maxClients = gEngfuncs.GetMaxClients();
	for (int i = 1; i <= maxClients; i++)
	{
		cl_entity_t *ent = GetPlayer(i, local, !all);
		if (!ent)
			continue;

		bool enemy = IsEnemy(i, local);
		if (!enemy && !showTeam)
			continue;

		Vector d = ent->origin - local->origin;
		float fwd = d.x * cs + d.y * sn;
		float right = d.x * sn - d.y * cs;

		// у края радара прижимаем точку, чтобы видеть направление
		float len = sqrtf(fwd * fwd + right * right);
		if (len > range)
		{
			fwd = fwd * range / len;
			right = right * range / len;
		}

		int px = cx + (int)(right * scale);
		int py = cy - (int)(fwd * scale);

		int r = enemy ? 255 : 0;
		int g = enemy ? 50 : 255;
		int b = 50;
		Rect(px - 2, py - 2, 5, 5, r, g, b, 255);
	}

	// ты
	Rect(cx - 2, cy - 2, 5, 5, 255, 255, 255, 255);
}

// Боксы, имена, линии. ch_wh 1/2. Красный = враг за стеной, жёлтый = виден, зелёный = свой.
static void DrawEsp(cl_entity_t *local, int mode)
{
	Vector eye = GetEye(local);
	bool showTeam = Cv("ch_wh_team") != 0.0f;
	bool lines = Cv("ch_wh_lines") != 0.0f;
	bool all = Cv("ch_wh_all") != 0.0f;

	int maxClients = gEngfuncs.GetMaxClients();

	for (int i = 1; i <= maxClients; i++)
	{
		cl_entity_t *ent = GetPlayer(i, local, !all);
		if (!ent)
			continue;

		bool enemy = IsEnemy(i, local);
		if (!enemy && !showTeam)
			continue;

		Vector head = ent->origin + Vector(0.0f, 0.0f, 36.0f);
		Vector feet = ent->origin - Vector(0.0f, 0.0f, 36.0f);

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
		else if (IsVisible(eye, AimPoint(ent)))
		{
			r = 255; g = 255; b = 0;
		}
		else
		{
			r = 255; g = 50; b = 50;
		}

		if (mode >= 1)
			DrawBox(hx - w / 2, hy, hx + w / 2, fy, r, g, b);

		if (lines && enemy)
			DrawLine(ScreenWidth / 2, ScreenHeight, fx, fy, r, g, b);

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
}

int CHudESP::Draw(float flTime)
{
	DoAim();

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return 0;

	DrawFovCircle();
	DrawRadar(local);

	int mode = (int)Cv("ch_wh");
	if (mode > 0 || Cv("ch_wh_lines") != 0.0f)
		DrawEsp(local, mode);

	return 0;
}
