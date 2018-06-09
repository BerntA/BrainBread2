//========= Copyright Reperio-Studios & Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: We use the crosshair convar to decide which crosshair icon we'll use, anyone can add their own crosshairs and set their own crosshairs. It also animates as you shoot, walk & jump.
//
//==========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_crosshair.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "ivrenderview.h"
#include "materialsystem/imaterialsystem.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Shared.h"
#include "in_buttons.h"

#ifdef SIXENSE
#include "sixense/in_sixense.h"
#endif

#ifdef PORTAL
#include "c_portal_player.h"
#endif // PORTAL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CHudCrosshair *m_pCrosshairHUD = NULL;

static void OnCrosshairIconChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (m_pCrosshairHUD)
		m_pCrosshairHUD->SetCrosshair();
}

ConVar crosshair("crosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the crosshair icon to use, see hud_crosshairs for available crosshairs.", OnCrosshairIconChanged);
ConVar crosshair_color_red("crosshair_color_red", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the amount of the color red for your crosshair.", true, 0.0f, true, 255.0f, OnCrosshairIconChanged);
ConVar crosshair_color_green("crosshair_color_green", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the amount of the color green for your crosshair.", true, 0.0f, true, 255.0f, OnCrosshairIconChanged);
ConVar crosshair_color_blue("crosshair_color_blue", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the amount of the color blue for your crosshair.", true, 0.0f, true, 255.0f, OnCrosshairIconChanged);
ConVar crosshair_color_alpha("crosshair_color_alpha", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the amount of opacity for your crosshair.", true, 0.0f, true, 255.0f, OnCrosshairIconChanged);

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

DECLARE_HUDELEMENT_DEPTH(CHudCrosshair, 0);
//DECLARE_HUDELEMENT( CHudCrosshair );

CHudCrosshair::CHudCrosshair( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudCrosshair" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = NULL;
	m_pCursor = NULL;
	m_pDelayedUse = NULL;
	m_clrCrosshair = Color( 0, 0, 0, 0 );
	m_vecCrossHairOffsetAngle.Init();
	m_bHoldingUseButton = false;

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD );

	m_pCrosshairHUD = this;
}

CHudCrosshair::~CHudCrosshair()
{
	m_pCrosshairHUD = NULL;
}

void CHudCrosshair::VidInit(void)
{
	m_pCursor = gHUD.GetIcon("cursor_vguiscreen");
	m_pDelayedUse = gHUD.GetIcon("delayed_use");
	SetCrosshair();
}

void CHudCrosshair::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );

    SetSize( ScreenWidth(), ScreenHeight() );

	SetForceStereoRenderToFrameBuffer( true );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudCrosshair::ShouldDraw( void )
{
	bool bNeedsDraw;

	if ( m_bHideCrosshair )
		return false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if ((pPlayer->GetTeamNumber() < TEAM_HUMANS) || !pPlayer->IsAlive())
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon && !pWeapon->ShouldDrawCrosshair() )
		return false;

#ifdef PORTAL
	C_Portal_Player *portalPlayer = ToPortalPlayer(pPlayer);
	if ( portalPlayer && portalPlayer->IsSuppressingCrosshair() )
		return false;
#endif // PORTAL

	// draw a crosshair only if alive or spectating in eye
	if ( IsX360() )
	{
		bNeedsDraw = m_pCrosshair && 
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			( g_pGameRules->IsMultiplayer() ) &&
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			( pPlayer->IsAlive() );
	}
	else
	{
		bNeedsDraw = m_pCrosshair && 
			crosshair.GetInt() &&
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			( pPlayer->IsAlive() );

		if (!m_pCursor && pPlayer->IsInVGuiInputMode())
			return false;
	}

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

CHudTexture *CHudCrosshair::GetCrosshairIcon(void)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->IsInVGuiInputMode() && m_pCursor)
		return m_pCursor;

	return m_pCrosshair;
}

void CHudCrosshair::GetDrawPosition ( float *pX, float *pY, bool *pbBehindCamera, QAngle angleCrosshairOffset )
{
	QAngle curViewAngles = CurrentViewAngles();
	Vector curViewOrigin = CurrentViewOrigin();

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	float screenWidth = vw;
	float screenHeight = vh;

	float x = screenWidth / 2;
	float y = screenHeight / 2;

	bool bBehindCamera = false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( ( pPlayer != NULL ) && ( pPlayer->GetObserverMode()==OBS_MODE_NONE ) )
	{
		bool bUseOffset = false;
		
		Vector vecStart;
		Vector vecEnd;

		if ( UseVR() )
		{
			// These are the correct values to use, but they lag the high-speed view data...
			vecStart = pPlayer->Weapon_ShootPosition();
			Vector vecAimDirection = pPlayer->GetAutoaimVector( 1.0f );
			// ...so in some aim modes, they get zapped by something completely up-to-date.
			g_ClientVirtualReality.OverrideWeaponHudAimVectors ( &vecStart, &vecAimDirection );
			vecEnd = vecStart + vecAimDirection * MAX_TRACE_LENGTH;

			bUseOffset = true;
		}

#ifdef SIXENSE
		// TODO: actually test this Sixsense code interaction with things like HMDs & stereo.
        if ( g_pSixenseInput->IsEnabled() && !UseVR() )
		{
			// Never autoaim a predicted weapon (for now)
			vecStart = pPlayer->Weapon_ShootPosition();
			Vector aimVector;
			AngleVectors( CurrentViewAngles() - g_pSixenseInput->GetViewAngleOffset(), &aimVector );
			// calculate where the bullet would go so we can draw the cross appropriately
			vecEnd = vecStart + aimVector * MAX_TRACE_LENGTH;
			bUseOffset = true;
		}
#endif

		if ( bUseOffset )
		{
			trace_t tr;
			UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

			Vector screen;
			screen.Init();
			bBehindCamera = ScreenTransform(tr.endpos, screen) != 0;

			x = 0.5f * ( 1.0f + screen[0] ) * screenWidth + 0.5f;
			y = 0.5f * ( 1.0f - screen[1] ) * screenHeight + 0.5f;
		}
	}

	// MattB - angleCrosshairOffset is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the 
	// screen
	if ( angleCrosshairOffset != vec3_angle )
	{
		QAngle angles;
		Vector forward;
		Vector point, screen;

		// this code is wrong
		angles = curViewAngles + angleCrosshairOffset;
		AngleVectors( angles, &forward );
		VectorAdd( curViewOrigin, forward, point );
		ScreenTransform( point, screen );

		x += 0.5f * screen[0] * screenWidth + 0.5f;
		y += 0.5f * screen[1] * screenHeight + 0.5f;
	}

	*pX = x;
	*pY = y;
	*pbBehindCamera = bBehindCamera;
}

void CHudCrosshair::Paint( void )
{
	CHudTexture *pCrosshairIcon = GetCrosshairIcon();
	if (!pCrosshairIcon)
		return;

	if ( !IsCurrentViewAccessAllowed() )
		return;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	float x, y;
	bool bBehindCamera;
	GetDrawPosition ( &x, &y, &bBehindCamera, m_vecCrossHairOffsetAngle );

	if( bBehindCamera )
		return;

	if (m_bHoldingUseButton)
	{
		float timeElapsed = gpGlobals->curtime - m_flTimePressedUse;
		float minimumHoldTime = DELAYED_USE_TIME * 0.25f;
		if ((timeElapsed > minimumHoldTime) && m_pDelayedUse)
		{
			timeElapsed -= minimumHoldTime;
			float fract = 1.0f - clamp((timeElapsed / (DELAYED_USE_TIME - minimumHoldTime)), 0.0f, 1.0f);
			if (fract > 0.0f)
			{
				m_pDelayedUse->DrawCircularProgression(
					Color(255, 255, 255, 255),
					x + use_offset_x,
					y + use_offset_y,
					m_pDelayedUse->GetOrigWide(),
					m_pDelayedUse->GetOrigTall(),
					fract
					);
			}
		}
	}

	float flWeaponScale = 1.f;
	int iTextureW = pCrosshairIcon->Width();
	int iTextureH = pCrosshairIcon->Height();
	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetWeaponCrosshairScale( flWeaponScale );
	}

	float flPlayerScale = 1.0f;
	Color clr = m_clrCrosshair;
	float flWidth = flWeaponScale * flPlayerScale * (float)iTextureW;
	float flHeight = flWeaponScale * flPlayerScale * (float)iTextureH;
	int iWidth = (int)(flWidth + 0.5f);
	int iHeight = (int)(flHeight + 0.5f);
	int iX = (int)(x + 0.5f);
	int iY = (int)(y + 0.5f);

	pCrosshairIcon->DrawSelfCropped(
		iX - (iWidth / 2), iY - (iHeight / 2),
		0, 0,
		iTextureW, iTextureH,
		iWidth, iHeight,
		clr);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshairAngle( const QAngle& angle )
{
	VectorCopy( angle, m_vecCrossHairOffsetAngle );
}

//-----------------------------------------------------------------------------
// Purpose: Set which crosshair + colors to use.
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshair(void)
{
	m_bHoldingUseButton = false;
	m_pCrosshair = gHUD.GetIcon(VarArgs("crosshair_%i", crosshair.GetInt()));
	m_clrCrosshair = Color(crosshair_color_red.GetInt(), crosshair_color_green.GetInt(), crosshair_color_blue.GetInt(), crosshair_color_alpha.GetInt());
}

void CHudCrosshair::ProcessInput()
{
	if (gHUD.m_iKeyBits & IN_USE)
	{
		if (!m_bHoldingUseButton)
		{
			m_bHoldingUseButton = true;
			m_flTimePressedUse = gpGlobals->curtime;
		}
	}
	else
	{
		if (m_bHoldingUseButton)
		{
			m_bHoldingUseButton = false;
			m_flTimePressedUse = 0.0f;
		}
	}
}