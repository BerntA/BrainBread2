//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the client-side representation of CBaseCombatCharacter.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASECOMBATCHARACTER_H
#define C_BASECOMBATCHARACTER_H

#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "c_baseflex.h"

class C_BaseCombatWeapon;

#define BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE 0.9f

class C_BaseCombatCharacter : public C_BaseFlex
{
	DECLARE_CLASS( C_BaseCombatCharacter, C_BaseFlex );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_BaseCombatCharacter( void );
	virtual			~C_BaseCombatCharacter( void );

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual bool	IsBaseCombatCharacter( void ) { return true; };
	virtual C_BaseCombatCharacter *MyCombatCharacterPointer( void ) { return this; }

	C_BaseCombatWeapon*	Weapon_OwnsThisType( const char *pszWeapon ) const;  // True if already owns a weapon of this class
	C_BaseCombatWeapon* Weapon_GetBySlot(int slot) const;
	virtual	bool		Weapon_Switch(C_BaseCombatWeapon *pWeapon, bool bWantDraw = false);
	virtual bool		Weapon_CanSwitchTo(C_BaseCombatWeapon *pWeapon);
	
	// I can't use my current weapon anymore. Switch me to the next best weapon.
	bool SwitchToNextBestWeapon(C_BaseCombatWeapon *pCurrent);

	virtual C_BaseCombatWeapon	*GetActiveWeapon( void ) const;
	int					WeaponCount() const;
	C_BaseCombatWeapon	*GetWeapon( int i ) const;

	float				GetNextAttack() const { return m_flNextAttack; }
	void				SetNextAttack( float flWait ) { m_flNextAttack = flWait; }

	virtual int			BloodColor();

	// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
	void SetBloodColor( int nBloodColor );

	virtual void		DoMuzzleFlash();

public:

	float			m_flNextAttack;

	int GetGibFlags(void) { return m_nGibFlags; }
	bool IsGibFlagActive(int nFlag) { return (m_nGibFlags & nFlag) != 0; }

	bool IsMaterialOverlayFlagActive(int nFlag) { return (m_nMaterialOverlayFlags & nFlag) != 0; }

protected:

	int			m_bloodColor;			// color of blood particless

	// BB2 Gib System:
	int         m_nGibFlags;

	// Material Overlays:
	int         m_nMaterialOverlayFlags;

public:

	CHandle<C_BaseCombatWeapon>		m_hMyWeapons[MAX_WEAPONS];
	CHandle< C_BaseCombatWeapon > m_hActiveWeapon;

private:
	C_BaseCombatCharacter( const C_BaseCombatCharacter & ); // not defined, not accessible
};

inline C_BaseCombatCharacter *ToBaseCombatCharacter( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsBaseCombatCharacter() )
		return NULL;

#if _DEBUG
	return dynamic_cast<C_BaseCombatCharacter *>( pEntity );
#else
	return static_cast<C_BaseCombatCharacter *>( pEntity );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int	C_BaseCombatCharacter::WeaponCount() const
{
	return MAX_WEAPONS;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
//-----------------------------------------------------------------------------
inline C_BaseCombatWeapon *C_BaseCombatCharacter::GetWeapon( int i ) const
{
	Assert( (i >= 0) && (i < MAX_WEAPONS) );
	return m_hMyWeapons[i].Get();
}

EXTERN_RECV_TABLE(DT_BaseCombatCharacter);

#endif // C_BASECOMBATCHARACTER_H
