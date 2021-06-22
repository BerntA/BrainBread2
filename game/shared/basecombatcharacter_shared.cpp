//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Switches to the best weapon that is also better than the given weapon.
// Input  : pCurrent - The current weapon used by the player.
// Output : Returns true if the weapon was switched, false if there was no better
//			weapon to switch to.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::SwitchToNextBestWeapon(CBaseCombatWeapon *pCurrent)
{
	CBaseCombatWeapon *pNewWeapon = g_pGameRules->GetNextBestWeapon(this, pCurrent);
	
	if ( ( pNewWeapon != NULL ) && ( pNewWeapon != pCurrent ) )
	{
		return Weapon_Switch(pNewWeapon, true);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Switches to the given weapon (providing it has ammo)
// Input  :
// Output : true is switch succeeded
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_Switch(CBaseCombatWeapon *pWeapon, bool bWantDraw)
{
	if ( pWeapon == NULL )
		return false;

	// Already have it out?
	if ( m_hActiveWeapon.Get() == pWeapon )
	{
		if ( !m_hActiveWeapon->IsWeaponVisible() || m_hActiveWeapon->IsHolstered() )
			return m_hActiveWeapon->Deploy( );

		return false;
	}

	m_hActiveWeapon = pWeapon;
	return pWeapon->Deploy( );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon.
// Input  : pWeapon - 
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)
{
	if (!pWeapon->CanDeploy())
		return false;

	if (m_hActiveWeapon)
	{
		if ( !m_hActiveWeapon->CanHolster() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::GetActiveWeapon() const
{
	return m_hActiveWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapon if already owns a weapon of this class
//-----------------------------------------------------------------------------
CBaseCombatWeapon* CBaseCombatCharacter::Weapon_OwnsThisType( const char *pszWeapon ) const
{
	// Check for duplicates
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (m_hMyWeapons[i].Get() && FClassnameIs(m_hMyWeapons[i], pszWeapon))
			return m_hMyWeapons[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapons with the appropriate weapon slot.
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetBySlot(int slot) const
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (m_hMyWeapons[i].Get())
		{
			if (m_hMyWeapons[i]->GetSlot() == slot)
				return m_hMyWeapons[i];
		}
	}

	return NULL;
}

int CBaseCombatCharacter::BloodColor()
{
	return m_bloodColor;
}

//-----------------------------------------------------------------------------
// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}