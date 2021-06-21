//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side implementation of CBaseCombatWeapon.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "history_resource.h"
#include "iclientmode.h"
#include "iinput.h"
#include "weapon_selection.h"
#include "hud_crosshair.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"
#include "hltvcamera.h"
#include "tier1/KeyValues.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "GameBase_Client.h"
#include "c_hl2mp_player.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Gets the local client's active weapon, if any.
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *GetActiveWeapon( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	return player->GetActiveWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatWeapon::SetDormant(bool bDormant)
{
	// If I'm going from active to dormant and I'm carried by another player/npc, holster me.
	if ((IsDormant() != bDormant) && GetOwner() && !IsCarriedByLocalPlayer())
		SetWeaponVisible(!bDormant);

	BaseClass::SetDormant(bDormant);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatWeapon::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit(state);

	if (state == SHOULDTRANSMIT_END)
	{
		if (m_iState == WEAPON_IS_ACTIVE)
		{
			m_iState = WEAPON_IS_CARRIED_BY_PLAYER;
		}
	}
	else if( state == SHOULDTRANSMIT_START )
	{
		if( m_iState == WEAPON_IS_CARRIED_BY_PLAYER )
		{
			if( GetOwner() && GetOwner()->GetActiveWeapon() == this )
			{
				// Restore the Activeness of the weapon if we client-twiddled it off in the first case above.
				m_iState = WEAPON_IS_ACTIVE;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static inline bool ShouldDrawLocalPlayerViewModel( void )
{
	return !C_BasePlayer::ShouldDrawLocalPlayer();
}

int C_BaseCombatWeapon::GetWorldModelIndex( void )
{
	return m_iWorldModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseCombatWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged(updateType);

	// If it's being carried by the *local* player, on the first update,
	// find the registered weapon for this ID

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatCharacter *pOwner = GetOwner();

	// check if weapon is carried by local player
	bool bIsLocalPlayer = pPlayer && (pPlayer == pOwner);
	if ( bIsLocalPlayer && ShouldDrawLocalPlayerViewModel() )		
	{
		// If I was just picked up, or created & immediately carried, add myself to this client's list of weapons
		if ( (m_iState != WEAPON_NOT_CARRIED ) && (m_iOldState == WEAPON_NOT_CARRIED) )
		{
			// Tell the HUD this weapon's been picked up
			if ( ShouldDrawPickup() )
			{
				CBaseHudWeaponSelection *pHudSelection = GetHudWeaponSelection();
				if ( pHudSelection )
				{
					pHudSelection->OnWeaponPickup( this );
				}

				pPlayer->EmitSound( "Player.PickupWeapon" );
			}
		}
	}
	else // weapon carried by other player or not at all
	{
		if (!EnsureCorrectRenderingModel())
		{
			int overrideModelIndex = CalcOverrideModelIndex();
			if (overrideModelIndex != -1 && overrideModelIndex != GetModelIndex())
			{
				SetModelIndex(overrideModelIndex);
			}
		}
	}

	UpdateVisibility();

	m_iOldState = m_iState;
}

//-----------------------------------------------------------------------------
// Is anyone carrying it?
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::IsBeingCarried() const
{
	return ( m_hOwner.Get() != NULL );
}

//-----------------------------------------------------------------------------
// Is the carrier alive?
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::IsCarrierAlive() const
{
	if ( !m_hOwner.Get() )
		return false;

	return m_hOwner.Get()->GetHealth() > 0;
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_BaseCombatWeapon::ShadowCastType()
{
	if ( IsEffectActive( /*EF_NODRAW |*/ EF_NOSHADOW ) )
		return SHADOWS_NONE;

	if (!IsBeingCarried())
		return SHADOWS_RENDER_TO_TEXTURE;

	if (IsCarriedByLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer())
		return SHADOWS_NONE;

	C_HL2MP_Player *pOwnerEnt = ToHL2MPPlayer(m_hOwner.Get());
	if (pOwnerEnt)
	{
		if (!pOwnerEnt->IsDormant() && pOwnerEnt->IsPerkFlagActive(PERK_POWERUP_PREDATOR))
			return SHADOWS_NONE;
	}

	return SHADOWS_RENDER_TO_TEXTURE;
}

//-----------------------------------------------------------------------------
// Purpose: This weapon is the active weapon, and the viewmodel for it was just drawn.
//-----------------------------------------------------------------------------
void C_BaseCombatWeapon::ViewModelDrawn( C_BaseViewModel *pViewModel )
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this client's carrying this weapon
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::IsCarriedByLocalPlayer( void )
{
	if ( !GetOwner() )
		return false;

	return ( GetOwner() == C_BasePlayer::GetLocalPlayer() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this client is carrying this weapon and is
//			using the view models
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::ShouldDrawUsingViewModel( void )
{
	return IsCarriedByLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this weapon is the local client's currently wielded weapon
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::IsActiveByLocalPlayer( void )
{
	if ( IsCarriedByLocalPlayer() )
	{
		return (m_iState == WEAPON_IS_ACTIVE);
	}

	return false;
}

bool C_BaseCombatWeapon::GetShootPosition( Vector &vOrigin, QAngle &vAngles )
{
	// Get the entity because the weapon doesn't have the right angles.
	C_BaseCombatCharacter *pEnt = ToBaseCombatCharacter( GetOwner() );
	if ( pEnt )
	{
		if ( pEnt == C_BasePlayer::GetLocalPlayer() )
		{
			vAngles = pEnt->EyeAngles();
		}
		else
		{
			vAngles = pEnt->GetRenderAngles();	
		}
	}
	else
	{
		vAngles.Init();
	}

	QAngle vDummy;
	if ( IsActiveByLocalPlayer() && ShouldDrawLocalPlayerViewModel() )
	{
		C_BasePlayer *player = ToBasePlayer( pEnt );
		C_BaseViewModel *vm = player ? player->GetViewModel() : NULL;
		if ( vm )
		{
			int iAttachment = vm->LookupAttachment( "muzzle" );
			if ( vm->GetAttachment( iAttachment, vOrigin, vDummy ) )
			{
				return true;
			}
		}
	}
	else
	{
		// Thirdperson
		int iAttachment = LookupAttachment( "muzzle" );
		if ( GetAttachment( iAttachment, vOrigin, vDummy ) )
		{
			return true;
		}
	}

	vOrigin = GetRenderOrigin();
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::ShouldDraw( void )
{
	if ( m_iWorldModelIndex == 0 )
		return false;

	// FIXME: All weapons with owners are set to transmit in CBaseCombatWeapon::UpdateTransmitState,
	// even if they have EF_NODRAW set, so we have to check this here. Ideally they would never
	// transmit except for the weapons owned by the local player.
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	C_BaseCombatCharacter *pOwner = GetOwner();

	// weapon has no owner, always draw it
	if ( !pOwner )
		return true;

	bool bIsActive = ( m_iState == WEAPON_IS_ACTIVE );

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	 // carried by local player?
	if ( pOwner == pLocalPlayer )
	{
		// Only ever show the active weapon
		if ( !bIsActive )
			return false;

		if ( !pOwner->ShouldDraw() )
		{
			// Our owner is invisible.
			// This also tests whether the player is zoomed in, in which case you don't want to draw the weapon.
			return false;
		}

		if (bb2_render_client_in_mirrors.GetBool())
			return VisibleInWeaponSelection();

		// 3rd person mode? 
		// Bernt - Slot must be under 4 so we don't show hands and other awkward weapons.
		if (!ShouldDrawLocalPlayerViewModel() && VisibleInWeaponSelection())
			return true;

		// don't draw active weapon if not in some kind of 3rd person mode, the viewmodel will do that
		return false;
	}

	// If it's a player, then only show active weapons
	if ( pOwner->IsPlayer() )
	{
		// Show it if it's active...
		// Bernt - We will always draw every weapon that the player has but not slots above 3 which are weapons like hands, random items, etc...
		return (VisibleInWeaponSelection() && bIsActive);
	}

	// FIXME: We may want to only show active weapons on NPCs
	// These are carried by AIs; always show them
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if a weapon-pickup icon should be displayed when this weapon is received
//-----------------------------------------------------------------------------
bool C_BaseCombatWeapon::ShouldDrawPickup( void )
{
	return !(GetWeaponFlags() & ITEM_FLAG_NOITEMPICKUP);
}
		   
//-----------------------------------------------------------------------------
// Purpose: Render the weapon. Draw the Viewmodel if the weapon's being carried
//			by this player, otherwise draw the worldmodel.
//-----------------------------------------------------------------------------
int C_BaseCombatWeapon::DrawModel( int flags )
{
	VPROF_BUDGET( "C_BaseCombatWeapon::DrawModel", VPROF_BUDGETGROUP_MODEL_RENDERING );
	if (!m_bReadyToDraw || !IsVisible())
		return 0;

	// check if local player chases owner of this weapon in first person
	C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
	if ( localplayer && localplayer->IsObserver() && GetOwner() )
	{
		// don't draw weapon if chasing this guy as spectator
		// we don't check that in ShouldDraw() since this may change
		// without notification 
		
		if ( localplayer->GetObserverMode() == OBS_MODE_IN_EYE &&
			 localplayer->GetObserverTarget() == GetOwner() ) 
			return 0;
	}

	EnsureCorrectRenderingModel();

	return BaseClass::DrawModel( flags );
}

int C_BaseCombatWeapon::InternalDrawModel(int flags)
{
	if (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES))
	{
		C_HL2MP_Player *pOwnerEnt = ToHL2MPPlayer(m_hOwner.Get());
		if (pOwnerEnt && !pOwnerEnt->IsDormant() && pOwnerEnt->IsPerkFlagActive(PERK_POWERUP_PREDATOR))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetCloakOverlay());
			int retVal = BaseClass::InternalDrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
			return retVal;
		}
	}

	return BaseClass::InternalDrawModel(flags);
}

//-----------------------------------------------------------------------------
// Allows the client-side entity to override what the network tells it to use for
// a model. This is used for third person mode, specifically in HL2 where the
// the weapon timings are on the view model and not the world model. That means the
// server needs to use the view model, but the client wants to use the world model.
//-----------------------------------------------------------------------------
int C_BaseCombatWeapon::CalcOverrideModelIndex() 
{ 
	C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
	if ( localplayer && 
		(localplayer == GetOwner()) &&
		ShouldDrawLocalPlayerViewModel() )
	{
		return BaseClass::CalcOverrideModelIndex();
	}
	else
	{
		return GetWorldModelIndex();
	}
}

//-----------------------------------------------------------------------------
// tool recording
//-----------------------------------------------------------------------------
void C_BaseCombatWeapon::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	int nModelIndex = GetModelIndex();
	int nWorldModelIndex = GetWorldModelIndex();
	if ( nModelIndex != nWorldModelIndex )
	{
		SetModelIndex( nWorldModelIndex );
	}

	BaseClass::GetToolRecordingState( msg );

	if ( m_iState == WEAPON_NOT_CARRIED )
	{
		BaseEntityRecordingState_t *pBaseEntity = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );
		pBaseEntity->m_nOwner = -1;
	}
	else
	{
		msg->SetInt( "worldmodel", 1 );
		if ( m_iState == WEAPON_IS_ACTIVE )
		{
			BaseEntityRecordingState_t *pBaseEntity = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );
			pBaseEntity->m_bVisible = true;
		}
	}

	if ( nModelIndex != nWorldModelIndex )
	{
		SetModelIndex( nModelIndex );
	}
}

// Prevents multiple animation events when using mirror rendering.
bool C_BaseCombatWeapon::ShouldDoAnimEvents()
{
	if (!bb2_render_client_in_mirrors.GetBool())
		return true;

	C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
	if (localplayer && (localplayer == GetOwner()))
		return false;

	return true;
}

// Make sure we render the right model and play the right anim in the mirror.
bool C_BaseCombatWeapon::EnsureCorrectRenderingModel()
{
	if (!bb2_render_client_in_mirrors.GetBool())
		return false;

	C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
	if (localplayer && (localplayer != GetOwner()))
		return false;

	SetModelIndex(GetWorldModelIndex());

	// Validate our current sequence just in case ( in theory the view and weapon models should have the same sequences for sequences that overlap at least )
	CStudioHdr *pStudioHdr = GetModelPtr();
	if (pStudioHdr && (GetSequence() >= pStudioHdr->GetNumSeq()))
		SetSequence(0);

	return true;
}

unsigned char C_BaseCombatWeapon::GetClientSideFade(void)
{
	// Weapon should dupe fade value of npc!
	C_BaseCombatCharacter *pOwner = GetOwner();
	if (pOwner && pOwner->IsNPC() && pOwner->MyNPCPointer())
		return pOwner->MyNPCPointer()->GetClientSideFade();

	return BaseClass::GetClientSideFade();
}