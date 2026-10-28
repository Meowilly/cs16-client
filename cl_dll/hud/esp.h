#pragma once
#include "hud.h"

// ch_wh 0 = выкл, 1 = боксы, 2 = боксы + имя + дистанция
// Работает только при sv_cheats 1 (офлайн с ботами)
class CHudESP : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw(float flTime) override;

private:
	cvar_t *m_pCvar;
};
