//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"
#include "input.h"
#include "../hud_crosshair.h"

#include "VGuiMatSurface/IMatSystemSurface.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>

#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SELECTION_TIMEOUT_THRESHOLD		0.5f	// Seconds
#define SELECTION_FADEOUT_TIME			0.75f

#define PLUS_DISPLAY_TIMEOUT			0.5f	// Seconds
#define PLUS_FADEOUT_TIME				0.75f

#define FASTSWITCH_DISPLAY_TIMEOUT		1.5f
#define FASTSWITCH_FADEOUT_TIME			1.5f

#define CAROUSEL_SMALL_DISPLAY_ALPHA	200.0f
#define FASTSWITCH_SMALL_DISPLAY_ALPHA	160.0f

#define MAX_CAROUSEL_SLOTS				5

//-----------------------------------------------------------------------------
// Purpose: hl2 weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void )
	{ 
		return m_hSelectedWeapon;
	}

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void LevelInit();

protected:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

	virtual void SetWeaponSelected()
	{
		CBaseHudWeaponSelection::SetWeaponSelected();
		if (hud_fastswitch.GetInt() > 0)
			ActivateFastswitchWeaponDisplay(GetSelectedWeapon());
	}

private:
	C_BaseCombatWeapon *FindNextWeaponInWeaponSelection(int iCurrentSlot);
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection(int iCurrentSlot);

	void DrawLargeWeaponBox( C_BaseCombatWeapon *pWeapon, bool bSelected, int x, int y, int wide, int tall, Color color, float alpha, int number, const CHudTexture *weaponTexture );
	void ActivateFastswitchWeaponDisplay( C_BaseCombatWeapon *pWeapon );
	void ActivateWeaponHighlight( C_BaseCombatWeapon *pWeapon );
	float GetWeaponBoxAlpha( bool bSelected );

	void FastWeaponSwitch( int iWeaponSlot );

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	virtual	void SetSelectedSlot( int slot ) 
	{ 
		m_iSelectedSlot = slot;
	}

	void SetSelectedSlideDir( int dir )
	{
		m_iSelectedSlideDir = dir;
	}

	void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number, const CHudTexture *weaponTexture, bool bSelected = false);

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionNumbers" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );
	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxSize, "SmallBoxSize", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSmallBoxHeight, "SmallBoxHeight", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flMediumBoxWide, "MediumBoxWide", "75", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMediumBoxTall, "MediumBoxTall", "50", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "54", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "0" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "0" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "SelectionEmptyBoxBg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxColor", "SelectionSelectedBoxBg" );
	CPanelAnimationVar( Color, m_SelectedFgColor, "SelectedFgColor", "FgColor" );
	CPanelAnimationVar( Color, m_BrightBoxColor, "SelectedFgColor", "BgColor" );

	CPanelAnimationVarAliasType(float, weapon_img_width, "weapon_img_width", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, weapon_img_height, "weapon_img_height", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, weapon_width_small, "weapon_width_small", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, weapon_height_small, "weapon_height_small", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, weapon_xpos, "weapon_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, weapon_ypos, "weapon_ypos", "0", "proportional_float");

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	bool m_bFadingOut;

	int						m_iSelectedWeaponBox;
	int						m_iSelectedSlideDir;
	int						m_iSelectedSlot;
	C_BaseCombatWeapon		*m_pLastWeapon;
	CPanelAnimationVar( float, m_flHorizWeaponSelectOffsetPoint, "WeaponBoxOffset", "0" );
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection(pElementName), BaseClass(NULL, "HudWeaponSelection")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bFadingOut = false;
	SetHiddenBits( HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING );
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink( void )
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		if (!m_bFadingOut)
		{
			// start fading out
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FadeOutWeaponSelectionMenu" );
			m_bFadingOut = true;
		}
		else if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			// finished fade, close
			HideSelection();
		}
	}
	else if (m_bFadingOut)
	{
		// stop us fading out, show the animation again
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "OpenWeaponSelectionMenu" );
		m_bFadingOut = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	return ( m_bSelectionVisible ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();

	m_iSelectedWeaponBox = -1;
	m_iSelectedSlideDir  = 0;
	m_pLastWeapon        = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: starts animating the center of the draw point to the newly selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ActivateFastswitchWeaponDisplay( C_BaseCombatWeapon *pSelectedWeapon )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// make sure all our configuration data is read
	MakeReadyForUse();
	m_iSelectedWeaponBox = 0;

	// find out where our selected weapon is in the full list
	int cWeapons = 0;
	int iLastSelectedWeaponBox = -1;
	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i);
		if (!pWeapon)
			continue;

		if (pWeapon == pSelectedWeapon)
		{
			m_iSelectedWeaponBox = cWeapons;
		}
		if (pWeapon == m_pLastWeapon)
		{
			iLastSelectedWeaponBox = cWeapons;
		}
		cWeapons++;
	}

	if ( iLastSelectedWeaponBox == -1 )
	{
		// unexpected failure, no last weapon to scroll from, default to snap behavior
		m_pLastWeapon = NULL;
	}

	// calculate where we would have to start drawing for this weapon to slide into center
	float flStart, flStop, flTime;
	if ( !m_pLastWeapon || m_iSelectedSlideDir == 0 || m_flHorizWeaponSelectOffsetPoint != 0 )
	{
		// no previous weapon or weapon selected directly or selection during slide, snap to exact position
		m_pLastWeapon = pSelectedWeapon;
		flStart = flStop = flTime = 0;
	}
	else
	{
		// offset display for a scroll
		// causing selected weapon to slide into position
		// scroll direction based on user's "previous" or "next" selection
		int numIcons = 0;
		int start    = iLastSelectedWeaponBox;
		for (int i=0; i<cWeapons; i++)
		{
			// count icons in direction of slide to destination
			if ( start == m_iSelectedWeaponBox )
				break;
			if ( m_iSelectedSlideDir < 0 )
			{
				start--;
			}
			else
			{
				start++;
			}
			// handle wraparound in either direction
			start = ( start + cWeapons ) % cWeapons;
			numIcons++;
		}

		flStart = numIcons * (m_flLargeBoxWide + m_flBoxGap);
		if ( m_iSelectedSlideDir < 0 )
			flStart *= -1;
		flStop = 0;

		// shorten duration for scrolling when desired weapon is farther away
		// otherwise a large skip in the same duration causes the scroll to fly too fast
		flTime = numIcons * 0.20f;
		if ( numIcons > 1 )
			flTime *= 0.5f;
	}
	m_flHorizWeaponSelectOffsetPoint = flStart;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "WeaponBoxOffset", flStop, 0, flTime, AnimationController::INTERPOLATOR_LINEAR );

	// start the highlight after the scroll completes
	m_flBlur = 7.f;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "Blur", 0, flTime, 0.75f, AnimationController::INTERPOLATOR_DEACCEL );
}

//-----------------------------------------------------------------------------
// Purpose: starts animating the highlight for the selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ActivateWeaponHighlight( C_BaseCombatWeapon *pSelectedWeapon )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// make sure all our configuration data is read
	MakeReadyForUse();

	C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_iSelectedSlot );
	if ( !pWeapon )
		return;

	// start the highlight after the scroll completes
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "WeaponHighlight" );
}

//-----------------------------------------------------------------------------
// Purpose: returns an (per frame animating) alpha value for different weapon boxes
//-----------------------------------------------------------------------------
float CHudWeaponSelection::GetWeaponBoxAlpha( bool bSelected )
{
	float alpha;
	if ( bSelected )
	{
		alpha = m_flSelectionAlphaOverride;
	}
	else
	{
		alpha = m_flSelectionAlphaOverride * (m_flAlphaOverride / 255.0f);
	}
	return alpha;
}


//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::Paint()
{
	int height;
	int width;
	int xpos;
	int ypos;

	if (!ShouldDraw())
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = GetSelectedWeapon();
	if (hud_fastswitch.GetInt() > 0)
		pSelectedWeapon = pPlayer->GetActiveWeapon();

	if ( !pSelectedWeapon )
		return;

	bool bPushedViewport = false;
	if (hud_fastswitch.GetInt() > 0)
	{
		CMatRenderContextPtr pRenderContext(materials);
		if (pRenderContext->GetRenderTarget())
		{
			vgui::surface()->PushFullscreenViewport();
			bPushedViewport = true;
		}
	}

	// interpolate the selected box size between the small box size and the large box size
	// interpolation has been removed since there is no weapon pickup animation anymore, so it's all at the largest size
	float percentageDone = 1.0f; //min(1.0f, (gpGlobals->curtime - m_flPickupStartTime) / m_flWeaponPickupGrowTime);
	int largeBoxWide = m_flSmallBoxSize + ((m_flLargeBoxWide - m_flSmallBoxSize) * percentageDone);
	int largeBoxTall = m_flSmallBoxSize + ((m_flLargeBoxTall - m_flSmallBoxSize) * percentageDone);

	Color selectedColor;
	for (int i = 0; i < 4; i++)
		selectedColor[i] = m_BoxColor[i] + ((m_SelectedBoxColor[i] - m_BoxColor[i]) * percentageDone);

	if (hud_fastswitch.GetInt() <= 0)
	{
		// bucket style
		height = 0;
		width = largeBoxWide;
		xpos = 0;
		ypos = GetTall();

		int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

		// draw the bucket set
		// iterate over all the weapon slots
		// draw from the bottom and up
		for (int i = 0; i < 5; i++)
		{
			if (i == iActiveSlot)
			{
				if (ypos == GetTall())
					ypos = GetTall() - (largeBoxTall + m_flBoxGap);

				C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i);
				if (pWeapon)
				{
					bool bSelected = (pWeapon == pSelectedWeapon);
					DrawLargeWeaponBox(pWeapon,
						bSelected,
						xpos,
						ypos,
						largeBoxWide,
						largeBoxTall,
						bSelected ? selectedColor : m_BoxColor,
						GetWeaponBoxAlpha(bSelected),
						true, pWeapon->GetSpriteActive());

					// move down to the next bucket
					ypos -= (largeBoxTall + m_flBoxGap);
				}
			}
			else
			{
				if (ypos == GetTall())
					ypos = GetTall() - (m_flSmallBoxHeight + m_flBoxGap);

				// check to see if there is a weapon in this bucket
				CBaseCombatWeapon *pWeaponSmallSlow = GetFirstPos(i);
				if (pWeaponSmallSlow)
				{
					// draw has weapon in slot
					DrawBox(xpos, ypos, m_flSmallBoxSize, m_flSmallBoxHeight, m_BoxColor, m_flAlphaOverride, i + 1, pWeaponSmallSlow->GetSpriteActive());

					ypos -= (m_flSmallBoxHeight + m_flBoxGap);
				}
			}
		}
	}

	if (bPushedViewport)
	{
		vgui::surface()->PopFullscreenViewport();
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws a single weapon selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawLargeWeaponBox( C_BaseCombatWeapon *pWeapon, bool bSelected, int xpos, int ypos, int boxWide, int boxTall, Color selectedColor, float alpha, int number, const CHudTexture *weaponTexture )
{
	Color col = bSelected ? m_SelectedFgColor : GetFgColor();

	// draw box for selected weapon
	DrawBox( xpos, ypos, boxWide, boxTall, selectedColor, alpha, number, weaponTexture, true );

	// draw text
	col = Color( 255, 255, 255, 255 );//m_TextColor;
	const FileWeaponInfo_t &weaponInfo = pWeapon->GetWpnData();

	if ( bSelected )
	{
		wchar_t text[128];
		wchar_t *tempString = g_pVGuiLocalize->Find(weaponInfo.szPrintName);

		// setup our localized string
		if ( tempString )
		{
#ifdef WIN32
			_snwprintf(text, sizeof(text)/sizeof(wchar_t) - 1, L"%s", tempString);
#else
			_snwprintf(text, sizeof(text)/sizeof(wchar_t) - 1, L"%S", tempString);
#endif
			text[sizeof(text)/sizeof(wchar_t) - 1] = 0;
		}
		else
		{
			// string wasn't found by g_pVGuiLocalize->Find()
			g_pVGuiLocalize->ConvertANSIToUnicode(weaponInfo.szPrintName, text, sizeof(text));
		}

		vgui::surface()->DrawSetTextColor( col );
		vgui::surface()->DrawSetTextFont( m_hTextFont );

		// count the position
		int slen = 0, charCount = 0, maxslen = 0;
		int firstslen = 0;
		{
			for (wchar_t *pch = text; *pch != 0; pch++)
			{
				if (*pch == '\n') 
				{
					// newline character, drop to the next line
					if (slen > maxslen)
					{
						maxslen = slen;
					}
					if (!firstslen)
					{
						firstslen = slen;
					}

					slen = 0;
				}
				else if (*pch == '\r')
				{
					// do nothing
				}
				else
				{
					slen += vgui::surface()->GetCharacterWidth( m_hTextFont, *pch );
					charCount++;
				}
			}
		}
		if (slen > maxslen)
		{
			maxslen = slen;
		}
		if (!firstslen)
		{
			firstslen = maxslen;
		}

		int tx = xpos + ((m_flLargeBoxWide - firstslen) / 2);
		int ty = ypos + (int)m_flTextYPos;
		vgui::surface()->DrawSetTextPos( tx, ty );
		// adjust the charCount by the scan amount
		charCount *= m_flTextScan;
		for (wchar_t *pch = text; charCount > 0; pch++)
		{
			if (*pch == '\n')
			{
				// newline character, move to the next line
				vgui::surface()->DrawSetTextPos( xpos + ((boxWide - slen) / 2), ty + (vgui::surface()->GetFontTall(m_hTextFont) * 1.1f));
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				vgui::surface()->DrawUnicodeChar(*pch);
				charCount--;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number, const CHudTexture *weaponTexture, bool bSelected)
{
	// I've removed the square frame around the selected weapon for now...
	//if ( bSelected )
	//BaseClass::DrawHollowBox( x, y, wide, tall, color, normalizedAlpha / 255.0f, 3, 3 );

	// draw the number
	if (number >= 0)
	{
		if ( weaponTexture )
		{
			vgui::surface()->DrawSetColor( ( bSelected ) ? Color( 255, 255, 255, 255 ) : GetFgColor() );
			vgui::surface()->DrawSetTexture( weaponTexture->textureId );
			int xPosNew = ((wide/2) - (weapon_img_width/2));
			vgui::surface()->DrawTexturedRect( xPosNew, y, xPosNew + weapon_img_width, y + weapon_img_height );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);
	SetPaintBackgroundType( 0 );
	SetForceStereoRenderToFrameBuffer( true );
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
	m_iSelectedSlot = -1;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if (pPlayer)
		pPlayer->m_bIsSelectingWeapons = true;
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
	m_bFadingOut = false;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if (pPlayer)
		pPlayer->m_bIsSelectingWeapons = false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindNextWeaponInWeaponSelection(int iCurrentSlot)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) )
		{
			int weaponSlot = pWeapon->GetSlot();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot )
				{
					iLowestNextSlot = weaponSlot;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindPrevWeaponInWeaponSelection(int iCurrentSlot)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) )
		{
			int weaponSlot = pWeapon->GetSlot();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot )
				{
					iLowestPrevSlot = weaponSlot;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = pPlayer->GetActiveWeapon();

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection(-1);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlideDir( 1 );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// Play the "cycle to next weapon" sound
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = pPlayer->GetActiveWeapon();

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection(MAX_WEAPON_SLOTS);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlideDir( -1 );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// Play the "cycle to next weapon" sound
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);
		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot )
			return pWeapon;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = NULL;

	// see where we should start selection
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot);
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot);
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );
	}
	else if ( pNextWeapon != pActiveWeapon )
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	// kill any fastswitch display
	m_flSelectionTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	if (hud_fastswitch.GetInt() <= 0)
	{
		C_BaseCombatWeapon *pActiveWeapon = GetSelectedWeapon();

		// find the weapon in this slot
		pActiveWeapon = GetNextActivePos(iSlot);
		if (!pActiveWeapon)
			pActiveWeapon = GetNextActivePos(iSlot);

		if (pActiveWeapon != NULL)
		{
			if (!IsInSelectionMode())
			{
				// open the weapon selection
				OpenSelection();
			}

			// Mark the change
			SetSelectedWeapon(pActiveWeapon);
			SetSelectedSlideDir(0);
		}
	}
	else
		FastWeaponSwitch(iSlot);

	pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}