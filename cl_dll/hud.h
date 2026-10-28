class CHudESP : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void DoAim();
private:
	cvar_t *m_pCvar;
	cvar_t *m_pAim;
	cvar_t *m_pAimFov;
	cvar_t *m_pAimSmooth;
};
