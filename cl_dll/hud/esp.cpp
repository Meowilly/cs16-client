// --- в DoAim() --- лол
void CHudESP::DoAim()
{
	if (Cv("ch_aim") == 0.0f)
		return;
	// проверка sv_cheats убрана

	bool always = Cv("ch_aim_always") != 0.0f;
	// ...
}

// --- в ESP_CreateMove() ---
void ESP_CreateMove(struct usercmd_s *cmd)
{
	if (!cmd)
		return;
	// проверка sv_cheats убрана

	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	// ...
}

// --- в Draw() ---
int CHudESP::Draw(float flTime)
{
	DoAim();

	// проверка sv_cheats убрана

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
