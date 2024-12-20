//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawn and use functions for editor-placed triggers.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "filters.h"
#include "vstdlib/random.h"
#include "triggers.h"
#include "hierarchy.h"
#include "bspfile.h"
#include "te_effect_dispatch.h"
#include "ammodef.h"
#include "movevars_shared.h"
#include "physics_prop_ragdoll.h"
#include "props.h"
#include "RagdollBoogie.h"
#include "EntityParticleTrail.h"
#include "in_buttons.h"
#include "gameinterface.h"
#include "team.h"

#ifdef HL2_DLL
#include "hl2_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar showtriggers( "showtriggers", "0", FCVAR_CHEAT, "Shows trigger brushes" );

bool IsTriggerClass( CBaseEntity *pEntity );

// Command to dynamically toggle trigger visibility
void Cmd_ShowtriggersToggle_f( const CCommand &args )
{
	// Loop through the entities in the game and make visible anything derived from CBaseTrigger
	CBaseEntity *pEntity = gEntList.FirstEnt();
	while ( pEntity )
	{
		if ( IsTriggerClass(pEntity) )
		{
			// If a classname is specified, only show triggles of that type
			if ( args.ArgC() > 1 )
			{
				const char *sClassname = args[1];
				if ( sClassname && sClassname[0] )
				{
					if ( !FClassnameIs( pEntity, sClassname ) )
					{
						pEntity = gEntList.NextEnt( pEntity );
						continue;
					}
				}
			}

			if ( pEntity->IsEffectActive( EF_NODRAW ) )
			{
				pEntity->RemoveEffects( EF_NODRAW );
			}
			else
			{
				pEntity->AddEffects( EF_NODRAW );
			}
		}

		pEntity = gEntList.NextEnt( pEntity );
	}
}

static ConCommand showtriggers_toggle( "showtriggers_toggle", Cmd_ShowtriggersToggle_f, "Toggle show triggers", FCVAR_CHEAT );

// Global Savedata for base trigger
BEGIN_DATADESC( CBaseTrigger )

	// Keyfields
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_KEYFIELD( m_bDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD(m_flPercentRequired, FIELD_FLOAT, "PercentRequired"),

	// Inputs	
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TouchTest", InputTouchTest ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartTouch", InputStartTouch ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EndTouch", InputEndTouch ),

	// Outputs
	DEFINE_OUTPUT( m_OnStartTouch, "OnStartTouch"),
	DEFINE_OUTPUT( m_OnStartTouchAll, "OnStartTouchAll"),
	DEFINE_OUTPUT( m_OnEndTouch, "OnEndTouch"),
	DEFINE_OUTPUT( m_OnEndTouchAll, "OnEndTouchAll"),
	DEFINE_OUTPUT( m_OnTouching, "OnTouching" ),
	DEFINE_OUTPUT( m_OnNotTouching, "OnNotTouching" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );


CBaseTrigger::CBaseTrigger()
{
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
	m_bSkipFilterCheck = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputEnable( inputdata_t &inputdata )
{ 
	Enable();
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputDisable( inputdata_t &inputdata )
{ 
	Disable();
}

void CBaseTrigger::InputTouchTest( inputdata_t &inputdata )
{
	TouchTest();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CBaseTrigger::Spawn()
{
	m_flPercentRequired = MAX(m_flPercentRequired, 1.0f);

	if (HasSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS))
	{
		// Automatically set this trigger to work with NPC's.
		AddSpawnFlags(SF_TRIGGER_ALLOW_NPCS);
	}

	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CBaseTrigger::UpdateOnRemove( void )
{
	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->RemoveTrigger();
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose: Turns on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Enable( void )
{
	m_bDisabled = false;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( true );
	}

	if (!IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		AddSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CBaseTrigger::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose: Called after player becomes active in the game
//-----------------------------------------------------------------------------
void CBaseTrigger::PostClientActive( void )
{
	BaseClass::PostClientActive();

	if ( !m_bDisabled )
	{
		PhysicsTouchTriggers();
	}
}

//------------------------------------------------------------------------------
// Purpose: Turns off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Disable( void )
{ 
	m_bDisabled = true;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( false );
	}

	if (IsSolidFlagSet(FSOLID_TRIGGER))
	{
		RemoveSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}
//------------------------------------------------------------------------------
// Purpose: Tests to see if anything is touching this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::TouchTest( void )
{
	// If the trigger is disabled don't test to see if anything is touching it.
	if ( !m_bDisabled )
	{
		if ( m_hTouchingEntities.Count() !=0 )
		{
			
			m_OnTouching.FireOutput( this, this );
		}
		else
		{
			m_OnNotTouching.FireOutput( this, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CBaseTrigger::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (IsSolidFlagSet(FSOLID_TRIGGER)) 
		{
			Q_strncpy(tempstr,"State: Enabled",sizeof(tempstr));
		}
		else
		{
			Q_strncpy(tempstr,"State: Disabled",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CBaseTrigger::PointIsWithin( const Vector &vecPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( vecPoint, vecPoint );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTrigger::InitTrigger( )
{
	SetSolid( GetParent() ? SOLID_VPHYSICS : SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );
	if (m_bDisabled)
	{
		RemoveSolidFlags( FSOLID_TRIGGER );	
	}
	else
	{
		AddSolidFlags( FSOLID_TRIGGER );	
	}

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		AddEffects( EF_NODRAW );
	}

	m_hTouchingEntities.Purge();

	if ( HasSpawnFlags( SF_TRIG_TOUCH_DEBRIS ) )
	{
		CollisionProp()->AddSolidFlags( FSOLID_TRIGGER_TOUCH_DEBRIS );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CBaseTrigger::PassesTriggerFilters(CBaseEntity *pOther)
{
	// First test spawn flag filters
	if (HasSpawnFlags(SF_TRIGGER_ALLOW_ALL) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS)
		)
	{
		if (pOther->GetFlags() & FL_NPC)
		{
			CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

			if (HasSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS))
			{
				if (!pNPC || !pNPC->IsPlayerAlly())
				{
					return false;
				}
			}
		}

		bool bOtherIsPlayer = pOther->IsPlayer();

		if (bOtherIsPlayer)
		{
			CBasePlayer *pPlayer = (CBasePlayer*)pOther;
			if (!pPlayer->IsAlive())
				return false;

			if (HasSpawnFlags(SF_TRIGGER_DISALLOW_BOTS))
			{
				if (pPlayer->IsFakeClient())
					return false;
			}
		}

		return (m_bSkipFilterCheck ? true : IsFilterPassing(pOther));
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called to simulate what happens when an entity touches the trigger.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputStartTouch( inputdata_t &inputdata )
{
	//Pretend we just touched the trigger.
	StartTouch( inputdata.pCaller );
}
//-----------------------------------------------------------------------------
// Purpose: Called to simulate what happens when an entity leaves the trigger.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputEndTouch( inputdata_t &inputdata )
{
	//And... pretend we left the trigger.
	EndTouch( inputdata.pCaller );	
}

//-----------------------------------------------------------------------------
// Purpose: Called when an entity starts touching us.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::StartTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther) )
	{
		EHANDLE hOther;
		hOther = pOther;
		
		bool bAdded = false;
		if ( m_hTouchingEntities.Find( hOther ) == m_hTouchingEntities.InvalidIndex() )
		{
			m_hTouchingEntities.AddToTail( hOther );
			bAdded = true;
		}

		m_OnStartTouch.FireOutput(pOther, this);

		if ( bAdded && ( m_hTouchingEntities.Count() == 1 ) )
		{
			// First entity to touch us that passes our filters
			m_OnStartTouchAll.FireOutput( pOther, this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when an entity stops touching us.
// Input  : pOther - The entity that was touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::EndTouch(CBaseEntity *pOther)
{
	if ( IsTouching( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;
		m_hTouchingEntities.FindAndRemove( hOther );
		
		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		//if ( !m_bDisabled )
		//{
			m_OnEndTouch.FireOutput(pOther, this);
		//}

		// If there are no more entities touching this trigger, fire the lost all touches
		// Loop through the touching entities backwards. Clean out old ones, and look for existing
		bool bFoundOtherTouchee = false;
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther;
			hOther = m_hTouchingEntities[i];

			if ( !hOther )
			{
				m_hTouchingEntities.Remove( i );
			}
			else if ( hOther->IsPlayer() && !hOther->IsAlive() )
			{
#ifdef STAGING_ONLY
				if ( !HushAsserts() )
				{
					AssertMsg( false, "Dead player [%s] is still touching this trigger at [%f %f %f]", hOther->GetEntityName().ToCStr(), XYZ( hOther->GetAbsOrigin() ) );
				}
				Warning( "Dead player [%s] is still touching this trigger at [%f %f %f]", hOther->GetEntityName().ToCStr(), XYZ( hOther->GetAbsOrigin() ) );
#endif
				m_hTouchingEntities.Remove( i );
			}
			else
			{
				bFoundOtherTouchee = true;
			}
		}

		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		// Didn't find one?
		if ( !bFoundOtherTouchee /*&& !m_bDisabled*/ )
		{
			m_OnEndTouchAll.FireOutput(pOther, this);
			EndTouchAll();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching us
//-----------------------------------------------------------------------------
bool CBaseTrigger::IsTouching( CBaseEntity *pOther )
{
	EHANDLE hOther;
	hOther = pOther;
	return ( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the first entity of the specified type being touched by this trigger
//-----------------------------------------------------------------------------
CBaseEntity *CBaseTrigger::GetTouchedEntityOfType( const char *sClassName )
{
	int iCount = m_hTouchingEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CBaseEntity *pEntity = m_hTouchingEntities[i];
		if ( FClassnameIs( pEntity, sClassName ) )
			return pEntity;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Toggles this trigger between enabled and disabled.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputToggle( inputdata_t &inputdata )
{
	if (IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		RemoveSolidFlags(FSOLID_TRIGGER);
	}
	else
	{
		AddSolidFlags(FSOLID_TRIGGER);
	}

	PhysicsTouchTriggers();
}

bool CBaseTrigger::IsEnoughPlayersInVolume(int iTeam)
{
	int players = 0;
	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBaseEntity *pToucher = m_hTouchingEntities[i].Get();
		if (!pToucher || !pToucher->IsPlayer() || !pToucher->IsAlive() || (pToucher->GetTeamNumber() != iTeam) || !IsFilterPassing(pToucher))
			continue;
		players++;
	}

	CTeam *pTeam = GetGlobalTeam(iTeam);
	float teamSize = (pTeam ? ((float)pTeam->GetNumPlayers()) : 0.0f),
		teamSizeInVolume = ((float)players);
	float flPercentInVolume = floor((teamSizeInVolume / teamSize) * 100.0f);

	return (flPercentInVolume >= m_flPercentRequired);
}

bool CBaseTrigger::IsFilterPassing(CBaseEntity *pOther)
{
	CBaseFilter *pFilter = m_hFilter.Get();
	return (pFilter ? pFilter->PassesFilter(this, pOther) : true);
}

//-----------------------------------------------------------------------------
// Purpose: Removes anything that touches it. If the trigger has a targetname,
//			firing it will toggle state.
//-----------------------------------------------------------------------------
class CTriggerRemove : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerRemove, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	
	DECLARE_DATADESC();
	
	// Outputs
	COutputEvent m_OnRemove;
};

BEGIN_DATADESC( CTriggerRemove )

	// Outputs
	DEFINE_OUTPUT( m_OnRemove, "OnRemove" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_remove, CTriggerRemove );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerRemove::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CTriggerRemove::Touch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	UTIL_Remove( pOther );
}


BEGIN_DATADESC( CTriggerHurt )

	// Function Pointers
	DEFINE_THINKFUNC(RadiationThink),
	DEFINE_THINKFUNC(HurtThink),

	// Fields
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flDamageCap, FIELD_FLOAT, "damagecap" ),
	DEFINE_KEYFIELD( m_bitsDamageInflict, FIELD_INTEGER, "damagetype" ),
	DEFINE_KEYFIELD( m_damageModel, FIELD_INTEGER, "damagemodel" ),
	DEFINE_KEYFIELD( m_bNoDmgForce, FIELD_BOOLEAN, "nodmgforce" ),

	// Inputs
	DEFINE_INPUT( m_flDamage, FIELD_FLOAT, "SetDamage" ),

	// Outputs
	DEFINE_OUTPUT( m_OnHurt, "OnHurt" ),
	DEFINE_OUTPUT( m_OnHurtPlayer, "OnHurtPlayer" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

IMPLEMENT_AUTO_LIST( ITriggerHurtAutoList );

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerHurt::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	m_flOriginalDamage = m_flDamage;

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( &CTriggerHurt::RadiationThink );
		SetNextThink( gpGlobals->curtime + random->RandomFloat(0.0, 0.5) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CTriggerHurt::RadiationThink( void )
{
	// check to see if a player is in pvs
	// if not, continue	
	Vector vecSurroundMins, vecSurroundMaxs;
	CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );

	float dt = gpGlobals->curtime - m_flLastDmgTime;
	if ( dt >= 0.5 )
	{
		HurtAllTouchers( dt );
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
}


//-----------------------------------------------------------------------------
// Purpose: When touched, a hurt trigger does m_flDamage points of damage each half-second.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
bool CTriggerHurt::HurtEntity( CBaseEntity *pOther, float damage )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther) )
		return false;
	
	// If player is disconnected, we're probably in this routine via the
	//  PhysicsRemoveTouchedList() function to make sure all Untouch()'s are called for the
	//  player. Calling TakeDamage() in this case can get into the speaking criteria, which
	//  will then loop through the control points and the touched list again. We shouldn't
	//  need to hurt players that are disconnected, so skip all of this...
	bool bPlayerDisconnected = pOther->IsPlayer() && ( ((CBasePlayer *)pOther)->IsConnected() == false );
	if ( bPlayerDisconnected )
		return false;	

	if ( damage < 0 )
	{
		pOther->TakeHealth( -damage, m_bitsDamageInflict );
	}
	else
	{
		// The damage position is the nearest point on the damaged entity
		// to the trigger's center. Not perfect, but better than nothing.
		Vector vecCenter = CollisionProp()->WorldSpaceCenter();

		Vector vecDamagePos;
		pOther->CollisionProp()->CalcNearestPoint( vecCenter, &vecDamagePos );

		CTakeDamageInfo info( this, this, damage, m_bitsDamageInflict );
		info.SetDamagePosition( vecDamagePos );
		if ( !m_bNoDmgForce )
		{
			GuessDamageForce( &info, ( vecDamagePos - vecCenter ), vecDamagePos );
		}
		else
		{
			info.SetDamageForce( vec3_origin );
		}
		
		pOther->TakeDamage( info );
	}

	if (pOther->IsPlayer())
	{
		m_OnHurtPlayer.FireOutput(pOther, this);
	}
	else
	{
		m_OnHurt.FireOutput(pOther, this);
	}
	m_hurtEntities.AddToTail( EHANDLE(pOther) );
	//NDebugOverlay::Box( pOther->GetAbsOrigin(), pOther->WorldAlignMins(), pOther->WorldAlignMaxs(), 255,0,0,0,0.5 );
	return true;
}

void CTriggerHurt::HurtThink()
{
	// if I hurt anyone, think again
	if ( HurtAllTouchers( 0.5 ) <= 0 )
	{
		SetThink(NULL);
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

void CTriggerHurt::EndTouch( CBaseEntity *pOther )
{
	if (PassesTriggerFilters(pOther))
	{
		EHANDLE hOther;
		hOther = pOther;

		// if this guy has never taken damage, hurt him now
		if ( !m_hurtEntities.HasElement( hOther ) )
		{
			HurtEntity( pOther, m_flDamage * 0.5 );
		}
	}
	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: called from RadiationThink() as well as HurtThink()
//			This function applies damage to any entities currently touching the
//			trigger
// Input  : dt - time since last call
// Output : int - number of entities actually hurt
//-----------------------------------------------------------------------------
#define TRIGGER_HURT_FORGIVE_TIME	3.0f	// time in seconds
int CTriggerHurt::HurtAllTouchers( float dt )
{
	int hurtCount = 0;
	// half second worth of damage
	float fldmg = m_flDamage * dt;
	m_flLastDmgTime = gpGlobals->curtime;

	m_hurtEntities.RemoveAll();

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch )
			{
				if ( HurtEntity( pTouch, fldmg ) )
				{
					hurtCount++;
				}
			}
		}
	}

	if( m_damageModel == DAMAGEMODEL_DOUBLE_FORGIVENESS )
	{
		if( hurtCount == 0 )
		{
			if( gpGlobals->curtime > m_flDmgResetTime  )
			{
				// Didn't hurt anyone. Reset the damage if it's time. (hence, the forgiveness)
				m_flDamage = m_flOriginalDamage;
			}
		}
		else
		{
			// Hurt someone! double the damage
			m_flDamage *= 2.0f;

			if( m_flDamage > m_flDamageCap )
			{
				// Clamp
				m_flDamage = m_flDamageCap;
			}

			// Now, put the damage reset time into the future. The forgive time is how long the trigger
			// must go without harming anyone in order that its accumulated damage be reset to the amount
			// set by the level designer. This is a stop-gap for an exploit where players could hop through
			// slime and barely take any damage because the trigger would reset damage anytime there was no
			// one in the trigger when this function was called. (sjb)
			m_flDmgResetTime = gpGlobals->curtime + TRIGGER_HURT_FORGIVE_TIME;
		}
	}

	return hurtCount;
}

void CTriggerHurt::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CTriggerHurt::HurtThink );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if this point is in any trigger_hurt zones with positive damage
//-----------------------------------------------------------------------------
bool IsTakingTriggerHurtDamageAtPoint( const Vector &vecPoint )
{
	for ( int i = 0; i < ITriggerHurtAutoList::AutoList().Count(); i++ )
	{
		// Some maps use trigger_hurt with negative values as healing triggers; don't consider those
		CTriggerHurt *pTrigger = static_cast<CTriggerHurt*>( ITriggerHurtAutoList::AutoList()[i] );
		if ( !pTrigger->m_bDisabled && pTrigger->PointIsWithin( vecPoint ) && pTrigger->m_flDamage > 0.f )
		{
			return true;
		}
	}

	return false;
}

// ##################################################################################
//	>> TriggerMultiple
// ##################################################################################
LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


BEGIN_DATADESC( CTriggerMultiple )

	// Function Pointers
	DEFINE_FUNCTION(MultiTouch),
	DEFINE_FUNCTION(MultiWaitOver ),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger")

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerMultiple::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if (m_flWait == 0)
	{
		m_flWait = 0.2;
	}

	ASSERTSZ(m_iHealth == 0, "trigger_multiple with health");
	SetTouch( &CTriggerMultiple::MultiTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Touch function. Activates the trigger.
// Input  : pOther - The thing that touched us.
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther))
	{
		ActivateMultiTrigger( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pActivator - 
//-----------------------------------------------------------------------------
void CTriggerMultiple::ActivateMultiTrigger(CBaseEntity *pActivator)
{
	if (GetNextThink() > gpGlobals->curtime)
		return;         // still waiting for reset time

	m_hActivator = pActivator;

	m_OnTrigger.FireOutput(m_hActivator, this);

	if (m_flWait > 0)
	{
		SetThink( &CTriggerMultiple::MultiWaitOver );
		SetNextThink( gpGlobals->curtime + m_flWait );
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink(  &CTriggerMultiple::SUB_Remove );
	}
}


//-----------------------------------------------------------------------------
// Purpose: The wait time has passed, so set back up for another activation
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiWaitOver( void )
{
	SetThink( NULL );
}

// ##################################################################################
//	>> TriggerOnce
// ##################################################################################
class CTriggerOnce : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerOnce, CTriggerMultiple );
public:

	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );

void CTriggerOnce::Spawn( void )
{
	BaseClass::Spawn();

	m_flWait = -1;
}


// ##################################################################################
//	>> TriggerLook
//
//  Triggers once when player is looking at m_target
//
// ##################################################################################
#define SF_TRIGGERLOOK_FIREONCE		128
#define SF_TRIGGERLOOK_USEVELOCITY	256

class CTriggerLook : public CTriggerOnce
{
	DECLARE_CLASS( CTriggerLook, CTriggerOnce );
public:

	EHANDLE m_hLookTarget;
	float m_flFieldOfView;
	float m_flLookTime;			// How long must I look for
	float m_flLookTimeTotal;	// How long have I looked
	float m_flLookTimeLast;		// When did I last look
	float m_flTimeoutDuration;	// Number of seconds after start touch to fire anyway
	bool m_bTimeoutFired;		// True if the OnTimeout output fired since the last StartTouch.
	EHANDLE m_hActivator;		// The entity that triggered us.

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void StartTouch(CBaseEntity *pOther);
	void EndTouch( CBaseEntity *pOther );
	int	 DrawDebugTextOverlays(void);

	DECLARE_DATADESC();

private:

	void Trigger(CBaseEntity *pActivator, bool bTimeout);
	void TimeoutThink();

	COutputEvent m_OnTimeout;
};

LINK_ENTITY_TO_CLASS( trigger_look, CTriggerLook );
BEGIN_DATADESC( CTriggerLook )

	DEFINE_KEYFIELD( m_flTimeoutDuration, FIELD_FLOAT, "timeout" ),

	DEFINE_OUTPUT( m_OnTimeout, "OnTimeout" ),

	DEFINE_FUNCTION( TimeoutThink ),

	// Inputs
	DEFINE_INPUT( m_flFieldOfView,		FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( m_flLookTime,			FIELD_FLOAT,	"LookTime" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::Spawn( void )
{
	m_hLookTarget = NULL;
	m_flLookTimeTotal = -1;
	m_bTimeoutFired = false;

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerLook::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if (pOther->IsPlayer() && m_flTimeoutDuration)
	{
		m_bTimeoutFired = false;
		m_hActivator = pOther;
		SetThink(&CTriggerLook::TimeoutThink);
		SetNextThink(gpGlobals->curtime + m_flTimeoutDuration);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerLook::TimeoutThink(void)
{
	Trigger(m_hActivator, true);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch(pOther);

	if (pOther->IsPlayer())
	{
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );

		m_flLookTimeTotal = -1;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::Touch(CBaseEntity *pOther)
{
	// Don't fire the OnTrigger if we've already fired the OnTimeout. This will be
	// reset in OnEndTouch.
	if (m_bTimeoutFired)
		return;

	// --------------------------------
	// Make sure we have a look target
	// --------------------------------
	if (m_hLookTarget == NULL)
	{
		m_hLookTarget = GetNextTarget();
		if (m_hLookTarget == NULL)
		{
			return;
		}
	}

	// This is designed for single player only
	// so we'll always have the same player
	if (pOther->IsPlayer())
	{
		// ----------------------------------------
		// Check that toucher is facing the target
		// ----------------------------------------
		Vector vLookDir;
		if ( HasSpawnFlags( SF_TRIGGERLOOK_USEVELOCITY ) )
		{
			vLookDir = pOther->GetAbsVelocity();
			VectorNormalize( vLookDir );
		}
		else
		{
			vLookDir = ((CBaseCombatCharacter*)pOther)->EyeDirection3D( );
		}

		Vector vTargetDir = m_hLookTarget->GetAbsOrigin() - pOther->EyePosition();
		VectorNormalize(vTargetDir);

		float fDotPr = DotProduct(vLookDir,vTargetDir);
		if (fDotPr > m_flFieldOfView)
		{
			// Is it the first time I'm looking?
			if (m_flLookTimeTotal == -1)
			{
				m_flLookTimeLast	= gpGlobals->curtime;
				m_flLookTimeTotal	= 0;
			}
			else
			{
				m_flLookTimeTotal	+= gpGlobals->curtime - m_flLookTimeLast;
				m_flLookTimeLast	=  gpGlobals->curtime;
			}

			if (m_flLookTimeTotal >= m_flLookTime)
			{
				Trigger(pOther, false);
			}
		}
		else
		{
			m_flLookTimeTotal	= -1;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the trigger is fired by look logic or timeout.
//-----------------------------------------------------------------------------
void CTriggerLook::Trigger(CBaseEntity *pActivator, bool bTimeout)
{
	if (bTimeout)
	{
		// Fired due to timeout (player never looked at the target).
		m_OnTimeout.FireOutput(pActivator, this);

		// Don't fire the OnTrigger for this toucher.
		m_bTimeoutFired = true;
	}
	else
	{
		// Fire because the player looked at the target.
		m_OnTrigger.FireOutput(pActivator, this);
		m_flLookTimeTotal = -1;

		// Cancel the timeout think.
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );
	}

	if (HasSpawnFlags(SF_TRIGGERLOOK_FIREONCE))
	{
		SetThink(&CTriggerLook::SUB_Remove);
		SetNextThink(gpGlobals->curtime);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerLook::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// ----------------
		// Print Look time
		// ----------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Time:   %3.2f",m_flLookTime - MAX(0,m_flLookTimeTotal));
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: A trigger that pushes the player, NPCs, or objects.
//-----------------------------------------------------------------------------
class CTriggerPush : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerPush, CBaseTrigger );

	void Spawn( void );
	void Activate( void );
	void Touch( CBaseEntity *pOther );
	void Untouch( CBaseEntity *pOther );

	Vector m_vecPushDir;

	DECLARE_DATADESC();
	
	float m_flAlternateTicksFix; // Scale factor to apply to the push speed when running with alternate ticks
	float m_flPushSpeed;
};

BEGIN_DATADESC( CTriggerPush )
	DEFINE_KEYFIELD( m_vecPushDir, FIELD_VECTOR, "pushdir" ),
	DEFINE_KEYFIELD( m_flAlternateTicksFix, FIELD_FLOAT, "alternateticksfix" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPush::Spawn()
{
	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle(m_vecPushDir.x, m_vecPushDir.y, m_vecPushDir.z);
	AngleVectors(angPushDir, &vecAbsDir);

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vecPushDir );

	BaseClass::Spawn();

	InitTrigger();

	if (m_flSpeed == 0)
	{
		m_flSpeed = 100;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTriggerPush::Activate()
{
	// Fix problems with triggers pushing too hard under sv_alternateticks.
	// This is somewhat hacky, but it's simple and we're really close to shipping.
	ConVarRef sv_alternateticks( "sv_alternateticks" );
	if ( ( m_flAlternateTicksFix != 0 ) && sv_alternateticks.GetBool() )
	{
		m_flPushSpeed = m_flSpeed * m_flAlternateTicksFix;
	}
	else
	{
		m_flPushSpeed = m_flSpeed;
	}
	
	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPush::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || (pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE ) )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	// FIXME: If something is hierarchically attached, should we try to push the parent?
	if (pOther->GetMoveParent())
		return;

	// Transform the push dir into global space
	Vector vecAbsDir;
	VectorRotate( m_vecPushDir, EntityToWorldTransform(), vecAbsDir );

	// Instant trigger, just transfer velocity and remove
	if (HasSpawnFlags(SF_TRIG_PUSH_ONCE))
	{
		pOther->ApplyAbsVelocityImpulse( m_flPushSpeed * vecAbsDir );

		if ( vecAbsDir.z > 0 )
		{
			pOther->SetGroundEntity( NULL );
		}
		UTIL_Remove( this );
		return;
	}

	switch( pOther->GetMoveType() )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
		break;

	case MOVETYPE_VPHYSICS:
		{
			IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
			if ( pPhys )
			{
				// UNDONE: Assume the velocity is for a 100kg object, scale with mass
				pPhys->ApplyForceCenter( m_flPushSpeed * vecAbsDir * 100.0f * gpGlobals->frametime );
				return;
			}
		}
		break;

	default:
		{
#if defined( HL2_DLL )
			// HACK HACK  HL2 players on ladders will only be disengaged if the sf is set, otherwise no push occurs.
			if ( pOther->IsPlayer() && 
				 pOther->GetMoveType() == MOVETYPE_LADDER )
			{
				if ( !HasSpawnFlags(SF_TRIG_PUSH_AFFECT_PLAYER_ON_LADDER) )
				{
					// Ignore the push
					return;
				}
			}
#endif

			Vector vecPush = (m_flPushSpeed * vecAbsDir);
			if (pOther->GetFlags() & FL_BASEVELOCITY)
			{
				vecPush = vecPush + pOther->GetBaseVelocity();
			}

			if ( vecPush.z > 0 && (pOther->GetFlags() & FL_ONGROUND) )
			{
				pOther->SetGroundEntity( NULL );
				Vector origin = pOther->GetAbsOrigin();
				origin.z += 1.0f;
				pOther->SetAbsOrigin( origin );
			}

			pOther->SetBaseVelocity( vecPush );
			pOther->AddFlag( FL_BASEVELOCITY );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Teleport trigger
//-----------------------------------------------------------------------------
const int SF_TELEPORT_PRESERVE_ANGLES = 0x20;	// Preserve angles even when a local landmark is not specified

class CTriggerTeleport : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerTeleport, CBaseTrigger );

	virtual void Spawn( void ) OVERRIDE;
	virtual void Touch( CBaseEntity *pOther ) OVERRIDE;

	string_t m_iLandmark;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

BEGIN_DATADESC( CTriggerTeleport )

	DEFINE_KEYFIELD( m_iLandmark, FIELD_STRING, "landmark" ),

END_DATADESC()



void CTriggerTeleport::Spawn( void )
{
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Teleports the entity that touched us to the location of our target,
//			setting the toucher's angles to our target's angles if they are a
//			player.
//
//			If a landmark was specified, the toucher is offset from the target
//			by their initial offset from the landmark and their angles are
//			left alone.
//
// Input  : pOther - The entity that touched us.
//-----------------------------------------------------------------------------
void CTriggerTeleport::Touch( CBaseEntity *pOther )
{
	CBaseEntity	*pentTarget = NULL;

	if (!PassesTriggerFilters(pOther))
	{
		return;
	}

	// The activator and caller are the same
	pentTarget = gEntList.FindEntityByName( pentTarget, m_target, NULL, pOther, pOther );
	if (!pentTarget)
	{
	   return;
	}
	
	//
	// If a landmark was specified, offset the player relative to the landmark.
	//
	CBaseEntity	*pentLandmark = NULL;
	Vector vecLandmarkOffset(0, 0, 0);
	if (m_iLandmark != NULL_STRING)
	{
		// The activator and caller are the same
		pentLandmark = gEntList.FindEntityByName(pentLandmark, m_iLandmark, NULL, pOther, pOther );
		if (pentLandmark)
		{
			vecLandmarkOffset = pOther->GetAbsOrigin() - pentLandmark->GetAbsOrigin();
		}
	}

	pOther->SetGroundEntity( NULL );
	
	Vector tmp = pentTarget->GetAbsOrigin();

	if (!pentLandmark && pOther->IsPlayer())
	{
		// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
		tmp.z -= pOther->WorldAlignMins().z;
	}

	//
	// Only modify the toucher's angles and zero their velocity if no landmark was specified.
	//
	const QAngle *pAngles = NULL;
	Vector *pVelocity = NULL;

	if (!pentLandmark && !HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES) )
	{
		pAngles = &pentTarget->GetAbsAngles();
		pVelocity = NULL;	//BUGBUG - This does not set the player's velocity to zero!!!
	}

	tmp += vecLandmarkOffset;
	pOther->Teleport( &tmp, pAngles, pVelocity );
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );


class CTriggerGravity : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerGravity, CBaseTrigger );
	DECLARE_DATADESC();

	void Spawn( void );
	void GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

BEGIN_DATADESC( CTriggerGravity )

	// Function Pointers
	DEFINE_FUNCTION(GravityTouch),

END_DATADESC()

void CTriggerGravity::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->SetGravity( GetGravity() );
}


// this is a really bad idea.
class CAI_ChangeTarget : public CBaseEntity
{
public:
	DECLARE_CLASS( CAI_ChangeTarget, CBaseEntity );

	// Input handlers.
	void InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	string_t	m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS( ai_changetarget, CAI_ChangeTarget );

BEGIN_DATADESC( CAI_ChangeTarget )

	DEFINE_KEYFIELD( m_iszNewTarget, FIELD_STRING, "m_iszNewTarget" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

END_DATADESC()

void CAI_ChangeTarget::InputActivate( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = NULL;

	while ((pTarget = gEntList.FindEntityByName( pTarget, m_target, NULL, inputdata.pActivator, inputdata.pCaller )) != NULL)
	{
		pTarget->m_target = m_iszNewTarget;
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer( );
		if (pNPC)
		{
			pNPC->SetGoalEnt( NULL );
		}
	}
}

#define SF_CAMERA_PLAYER_POSITION		1
#define SF_CAMERA_PLAYER_TARGET			2
#define SF_CAMERA_PLAYER_TAKECONTROL	4
#define SF_CAMERA_PLAYER_INFINITE_WAIT	8
#define SF_CAMERA_PLAYER_SNAP_TO		16
#define SF_CAMERA_PLAYER_NOT_SOLID		32
#define SF_CAMERA_PLAYER_INTERRUPT		64

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerCamera, CBaseEntity );

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Enable( void );
	void Disable( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FollowTarget( void );
	void Move(void);

	// Always transmit to clients so they know where to move the view to
	virtual int UpdateTransmitState();
	
	DECLARE_DATADESC();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:
	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;

	// used for moving the camera along a path (rail rides)
	CBaseEntity *m_pPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	Vector m_vecMoveDir;


	string_t m_iszTargetAttachment;
	int	  m_iAttachmentIndex;
	bool  m_bSnapToGoal;

	int   m_nPlayerButtons;
	int m_nOldTakeDamage;

private:
	COutputEvent m_OnEndFollow;
};

LINK_ENTITY_TO_CLASS( point_viewcontrol, CTriggerCamera );

BEGIN_DATADESC( CTriggerCamera )

	DEFINE_KEYFIELD( m_iszTargetAttachment, FIELD_STRING, "targetattachment" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Function Pointers
	DEFINE_FUNCTION( FollowTarget ),
	DEFINE_OUTPUT( m_OnEndFollow, "OnEndFollow" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Spawn( void )
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NOCLIP );
	SetSolid( SOLID_NONE );								// Remove model & collisions
	SetRenderColorA( 0 );								// The engine won't draw this model if this is set to 0 and blending is on
	m_nRenderMode = kRenderTransTexture;

	m_state = USE_OFF;
	
	m_initialSpeed = m_flSpeed;

	if ( m_acceleration == 0 )
		m_acceleration = 500;

	if ( m_deceleration == 0 )
		m_deceleration = 500;

	DispatchUpdateTransmitState();
}

int CTriggerCamera::UpdateTransmitState()
{
	// always tranmit if currently used by a monitor
	if ( m_state == USE_ON )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerCamera::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "wait"))
	{
		m_flWait = atof(szValue);
	}
	else if (FStrEq(szKeyName, "moveto"))
	{
		m_sPath = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "acceleration"))
	{
		m_acceleration = atof( szValue );
	}
	else if (FStrEq(szKeyName, "deceleration"))
	{
		m_deceleration = atof( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CTriggerCamera::InputEnable( inputdata_t &inputdata )
{ 
	m_hPlayer = inputdata.pActivator;
	Enable();
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CTriggerCamera::InputDisable( inputdata_t &inputdata )
{ 
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Enable( void )
{
	m_state = USE_ON;

	if ( !m_hPlayer || !m_hPlayer->IsPlayer() )
	{
		m_hPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	}

	if ( !m_hPlayer )
	{
		DispatchUpdateTransmitState();
		return;
	}

	Assert( m_hPlayer->IsPlayer() );
	CBasePlayer *pPlayer = NULL;

	if ( m_hPlayer->IsPlayer() )
	{
		pPlayer = ((CBasePlayer*)m_hPlayer.Get());
	}
	else
	{
		Warning("CTriggerCamera could not find a player!\n");
		return;
	}

	// if the player was already under control of a similar trigger, disable the previous trigger.
	{
		CBaseEntity *pPrevViewControl = pPlayer->GetViewEntity();
		if (pPrevViewControl && pPrevViewControl != pPlayer)
		{
			CTriggerCamera *pOtherCamera = dynamic_cast<CTriggerCamera *>(pPrevViewControl);
			if ( pOtherCamera )
			{
				if ( pOtherCamera == this )
				{
					// what the hell do you think you are doing?
					Warning("Viewcontrol %s was enabled twice in a row!\n", GetDebugName());
					return;
				}
				else
				{
					pOtherCamera->Disable();
				}
			}
		}
	}


	m_nPlayerButtons = pPlayer->m_nButtons;

	
	// Make the player invulnerable while under control of the camera.  This will prevent situations where the player dies while under camera control but cannot restart their game due to disabled player inputs.
	m_nOldTakeDamage = m_hPlayer->m_takedamage;
	m_hPlayer->m_takedamage = DAMAGE_NO;
	
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_NOT_SOLID ) )
	{
		m_hPlayer->AddSolidFlags( FSOLID_NOT_SOLID );
	}
	
	m_flReturnTime = gpGlobals->curtime + m_flWait;
	m_flSpeed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	// this pertains to view angles, not translation.
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_SNAP_TO ) )
	{
		m_bSnapToGoal = true;
	}

	if ( HasSpawnFlags(SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// If we don't have a target, ignore the attachment / etc
	if ( m_hTarget )
	{
		m_iAttachmentIndex = 0;
		if ( m_iszTargetAttachment != NULL_STRING )
		{
			if ( !m_hTarget->GetBaseAnimating() )
			{
				Warning("%s tried to target an attachment (%s) on target %s, which has no model.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
			}
			else
			{
				m_iAttachmentIndex = m_hTarget->GetBaseAnimating()->LookupAttachment( STRING(m_iszTargetAttachment) );
				if ( m_iAttachmentIndex <= 0 )
				{
					Warning("%s could not find attachment %s on target %s.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
				}
			}
		}
	}

	if (HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		((CBasePlayer*)m_hPlayer.Get())->EnableControl(FALSE);
	}

	if ( m_sPath != NULL_STRING )
	{
		m_pPath = gEntList.FindEntityByName( NULL, m_sPath, NULL, m_hPlayer );
	}
	else
	{
		m_pPath = NULL;
	}

	m_flStopTime = gpGlobals->curtime;
	if ( m_pPath )
	{
		if ( m_pPath->m_flSpeed != 0 )
			m_targetSpeed = m_pPath->m_flSpeed;
		
		m_flStopTime += m_pPath->GetDelay();
	}


	// copy over player information. If we're interpolating from
	// the player position, do something more elaborate.
	if (HasSpawnFlags(SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( this, m_hPlayer->EyePosition() );
		SetLocalAngles( QAngle( m_hPlayer->GetLocalAngles().x, m_hPlayer->GetLocalAngles().y, 0 ) );
		SetAbsVelocity( m_hPlayer->GetAbsVelocity() );
	}
	else
	{
		SetAbsVelocity( vec3_origin );
	}


	pPlayer->SetViewEntity( this );

	// Hide the player's viewmodel
	if ( pPlayer->GetActiveWeapon() )
	{
		pPlayer->GetActiveWeapon()->AddEffects( EF_NODRAW );
	}

	// Only track if we have a target
	if ( m_hTarget )
	{
		// follow the player down
		SetThink( &CTriggerCamera::FollowTarget );
		SetNextThink( gpGlobals->curtime );
	}

	m_moveDistance = 0;
	Move();

	DispatchUpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Disable( void )
{
	if ( m_hPlayer && m_hPlayer->IsAlive() )
	{
		if ( HasSpawnFlags( SF_CAMERA_PLAYER_NOT_SOLID ) )
		{
			m_hPlayer->RemoveSolidFlags( FSOLID_NOT_SOLID );
		}

		((CBasePlayer*)m_hPlayer.Get())->SetViewEntity( m_hPlayer );
		((CBasePlayer*)m_hPlayer.Get())->EnableControl(TRUE);

		// Restore the player's viewmodel
		if ( ((CBasePlayer*)m_hPlayer.Get())->GetActiveWeapon() )
		{
			((CBasePlayer*)m_hPlayer.Get())->GetActiveWeapon()->RemoveEffects( EF_NODRAW );
		}
		//return the player to previous takedamage state
		m_hPlayer->m_takedamage = m_nOldTakeDamage;
	}

	CBasePlayer *m_hPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	// return the player to previous takedamage state
	if (m_hPlayer)
		m_hPlayer->m_takedamage = m_nOldTakeDamage;

	m_state = USE_OFF;
	m_flReturnTime = gpGlobals->curtime;
	SetThink( NULL );

	m_OnEndFollow.FireOutput(this, this); // dvsents2: what is the best name for this output?
	SetLocalAngularVelocity( vec3_angle );

	DispatchUpdateTransmitState();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	if ( m_state != USE_OFF )
	{
		Disable();
	}
	else
	{
		m_hPlayer = pActivator;
		Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if ( m_hTarget == NULL )
	{
		Disable();
		return;
	}

	if ( !HasSpawnFlags(SF_CAMERA_PLAYER_INFINITE_WAIT) && (!m_hTarget || m_flReturnTime < gpGlobals->curtime) )
	{
		Disable();
		return;
	}

	QAngle vecGoal;
	if ( m_iAttachmentIndex )
	{
		Vector vecOrigin;
		m_hTarget->GetBaseAnimating()->GetAttachment( m_iAttachmentIndex, vecOrigin );
		VectorAngles( vecOrigin - GetAbsOrigin(), vecGoal );
	}
	else
	{
		if ( m_hTarget )
		{
			VectorAngles( m_hTarget->GetAbsOrigin() - GetAbsOrigin(), vecGoal );
		}
		else
		{
			// Use the viewcontroller's angles
			vecGoal = GetAbsAngles();
		}
	}

	// Should we just snap to the goal angles?
	if ( m_bSnapToGoal ) 
	{
		SetAbsAngles( vecGoal );
		m_bSnapToGoal = false;
	}
	else
	{
		// UNDONE: Can't we just use UTIL_AngleDiff here?
		QAngle angles = GetLocalAngles();

		if (angles.y > 360)
			angles.y -= 360;

		if (angles.y < 0)
			angles.y += 360;

		SetLocalAngles( angles );

		float dx = vecGoal.x - GetLocalAngles().x;
		float dy = vecGoal.y - GetLocalAngles().y;

		if (dx < -180) 
			dx += 360;
		if (dx > 180) 
			dx = dx - 360;
		
		if (dy < -180) 
			dy += 360;
		if (dy > 180) 
			dy = dy - 360;

		QAngle vecAngVel;
		vecAngVel.Init( dx * 40 * gpGlobals->frametime, dy * 40 * gpGlobals->frametime, GetLocalAngularVelocity().z );
		SetLocalAngularVelocity(vecAngVel);
	}

	if (!HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL))	
	{
		SetAbsVelocity( GetAbsVelocity() * 0.8 );
		if (GetAbsVelocity().Length( ) < 10.0)
		{
			SetAbsVelocity( vec3_origin );
		}
	}

	SetNextThink( gpGlobals->curtime );

	Move();
}

void CTriggerCamera::Move()
{
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_INTERRUPT ) )
	{
		if ( m_hPlayer )
		{
			CBasePlayer *pPlayer = ToBasePlayer( m_hPlayer );

			if ( pPlayer  )
			{
				int buttonsChanged = m_nPlayerButtons ^ pPlayer->m_nButtons;

				if ( buttonsChanged && pPlayer->m_nButtons )
				{
					 Disable();
					 return;
				}

				m_nPlayerButtons = pPlayer->m_nButtons;
			}
		}
	}

	// Not moving on a path, return
	if (!m_pPath)
		return;

	{
		// Subtract movement from the previous frame
		m_moveDistance -= m_flSpeed * gpGlobals->frametime;

		// Have we moved enough to reach the target?
		if ( m_moveDistance <= 0 )
		{
			variant_t emptyVariant;
			m_pPath->AcceptInput( "InPass", this, this, emptyVariant, 0 );
			// Time to go to the next target
			m_pPath = m_pPath->GetNextTarget();

			// Set up next corner
			if ( !m_pPath )
			{
				SetAbsVelocity( vec3_origin );
			}
			else 
			{
				if ( m_pPath->m_flSpeed != 0 )
					m_targetSpeed = m_pPath->m_flSpeed;

				m_vecMoveDir = m_pPath->GetLocalOrigin() - GetLocalOrigin();
				m_moveDistance = VectorNormalize( m_vecMoveDir );
				m_flStopTime = gpGlobals->curtime + m_pPath->GetDelay();
			}
		}

		if ( m_flStopTime > gpGlobals->curtime )
			m_flSpeed = UTIL_Approach( 0, m_flSpeed, m_deceleration * gpGlobals->frametime );
		else
			m_flSpeed = UTIL_Approach( m_targetSpeed, m_flSpeed, m_acceleration * gpGlobals->frametime );

		float fraction = 2 * gpGlobals->frametime;
		SetAbsVelocity( ((m_vecMoveDir * m_flSpeed) * fraction) + (GetAbsVelocity() * (1-fraction)) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Measures the proximity to a specified entity of any entities within
//			the trigger, provided they are within a given radius of the specified
//			entity. The nearest entity distance is output as a number from [0 - 1].
//-----------------------------------------------------------------------------
class CTriggerProximity : public CBaseTrigger
{
public:

	DECLARE_CLASS( CTriggerProximity, CBaseTrigger );

	virtual void Spawn(void);
	virtual void Activate(void);
	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	void MeasureThink(void);

protected:

	EHANDLE m_hMeasureTarget;
	string_t m_iszMeasureTarget;		// The entity from which we measure proximities.
	float m_fRadius;			// The radius around the measure target that we measure within.
	int m_nTouchers;			// Number of entities touching us.

	// Outputs
	COutputFloat m_NearestEntityDistance;

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CTriggerProximity )

	// Functions
	DEFINE_FUNCTION(MeasureThink),

	// Keys
	DEFINE_KEYFIELD(m_iszMeasureTarget, FIELD_STRING, "measuretarget"),
	DEFINE_KEYFIELD(m_fRadius, FIELD_FLOAT, "radius"),

	// Outputs
	DEFINE_OUTPUT(m_NearestEntityDistance, "NearestEntityDistance"),

END_DATADESC()



LINK_ENTITY_TO_CLASS(trigger_proximity, CTriggerProximity);
LINK_ENTITY_TO_CLASS(logic_proximity, CPointEntity);


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerProximity::Spawn(void)
{
	// Avoid divide by zero in MeasureThink!
	if (m_fRadius == 0)
	{
		m_fRadius = 32;
	}

	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//			Finds the reference point from which to measure.
//-----------------------------------------------------------------------------
void CTriggerProximity::Activate(void)
{
	BaseClass::Activate();
	m_hMeasureTarget = gEntList.FindEntityByName(NULL, m_iszMeasureTarget );

	//
	// Disable our Touch function if we were given a bad measure target.
	//
	if ((m_hMeasureTarget == NULL) || (m_hMeasureTarget->edict() == NULL))
	{
		Warning( "TriggerProximity - Missing measure target or measure target with no origin!\n");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers++;	

		SetThink( &CTriggerProximity::MeasureThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers--;

		if ( m_nTouchers == 0 )
		{
			SetThink( NULL );
			SetNextThink( TICK_NEVER_THINK );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function called every frame as long as we have entities touching
//			us that we care about. Finds the closest entity to the measure
//			target and outputs the distance as a normalized value from [0..1].
//-----------------------------------------------------------------------------
void CTriggerProximity::MeasureThink( void )
{
	if ( ( m_hMeasureTarget == NULL ) || ( m_hMeasureTarget->edict() == NULL ) )
	{
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );
		return;
	}

	//
	// Traverse our list of touchers and find the entity that is closest to the
	// measure target.
	//
	float fMinDistance = m_fRadius + 100;
	CBaseEntity *pNearestEntity = NULL;

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		touchlink_t *pLink = root->nextLink;
		while ( pLink != root )
		{
			CBaseEntity *pEntity = pLink->entityTouched;

			// If this is an entity that we care about, check its distance.
			if ( ( pEntity != NULL ) && PassesTriggerFilters( pEntity ) )
			{
				float flDistance = (pEntity->GetLocalOrigin() - m_hMeasureTarget->GetLocalOrigin()).Length();
				if (flDistance < fMinDistance)
				{
					fMinDistance = flDistance;
					pNearestEntity = pEntity;
				}
			}

			pLink = pLink->nextLink;
		}
	}

	// Update our output with the nearest entity distance, normalized to [0..1].
	if ( fMinDistance <= m_fRadius )
	{
		fMinDistance /= m_fRadius;
		if ( fMinDistance != m_NearestEntityDistance.Get() )
		{
			m_NearestEntityDistance.Set( fMinDistance, pNearestEntity, this );
		}
	}

	SetNextThink( gpGlobals->curtime );
}


// ##################################################################################
//	>> TriggerWind
//
//  Blows physics objects in the trigger
//
// ##################################################################################

#define MAX_WIND_CHANGE		5.0f

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
class CPhysicsWind : public IMotionEvent
{
public:
	simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
	{
		// If we have no windspeed, we're not doing anything
		if ( !m_flWindSpeed )
			return IMotionEvent::SIM_NOTHING;

		// Get a cosine modulated noise between 5 and 20 that is object specific
		int nNoiseMod = 5+(int)pObject%15; // 

		// Turn wind yaw direction into a vector and add noise
		QAngle vWindAngle = vec3_angle;	
		vWindAngle[1] = m_nWindYaw+(30*cos(nNoiseMod * gpGlobals->curtime + nNoiseMod));
		Vector vWind;
		AngleVectors(vWindAngle,&vWind);

		// Add lift with noise
		vWind.z = 1.1 + (1.0 * sin(nNoiseMod * gpGlobals->curtime + nNoiseMod));

		linear = 3*vWind*m_flWindSpeed;
		angular = vec3_origin;
		return IMotionEvent::SIM_GLOBAL_FORCE;	
	}

	int		m_nWindYaw;
	float	m_flWindSpeed;
};

extern short g_sModelIndexSmoke;
#define WIND_THINK_CONTEXT		"WindThinkContext"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerWind : public CBaseVPhysicsTrigger
{
	DECLARE_CLASS( CTriggerWind, CBaseVPhysicsTrigger );
public:
	DECLARE_DATADESC();

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	UpdateOnRemove();
	bool	CreateVPhysics();
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );
	void	WindThink( void );
	int		DrawDebugTextOverlays( void );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputSetSpeed( inputdata_t &inputdata );

private:
	int 	m_nSpeedBase;	// base line for how hard the wind blows
	int		m_nSpeedNoise;	// noise added to wind speed +/-
	int		m_nSpeedCurrent;// current wind speed
	int		m_nSpeedTarget;	// wind speed I'm approaching

	int		m_nDirBase;		// base line for direction the wind blows (yaw)
	int		m_nDirNoise;	// noise added to wind direction
	int		m_nDirCurrent;	// the current wind direction
	int		m_nDirTarget;	// wind direction I'm approaching

	int		m_nHoldBase;	// base line for how long to wait before changing wind
	int		m_nHoldNoise;	// noise added to how long to wait before changing wind

	bool	m_bSwitch;		// when does wind change

	IPhysicsMotionController*	m_pWindController;
	CPhysicsWind				m_WindCallback;

};

LINK_ENTITY_TO_CLASS( trigger_wind, CTriggerWind );

BEGIN_DATADESC( CTriggerWind )

	DEFINE_KEYFIELD( m_nSpeedNoise,	FIELD_INTEGER, "SpeedNoise"),
	DEFINE_KEYFIELD( m_nDirNoise,	FIELD_INTEGER, "DirectionNoise"),
	DEFINE_KEYFIELD( m_nHoldBase,	FIELD_INTEGER, "HoldTime"),
	DEFINE_KEYFIELD( m_nHoldNoise,	FIELD_INTEGER, "HoldNoise"),

	DEFINE_FUNCTION( WindThink ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSpeed", InputSetSpeed ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::Spawn( void )
{
	m_bSwitch = true;
	m_nDirBase = GetLocalAngles().y;

	BaseClass::Spawn();

	m_nSpeedCurrent = m_nSpeedBase;
	m_nDirCurrent = m_nDirBase;

	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime, WIND_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerWind::KeyValue( const char *szKeyName, const char *szValue )
{
	// Done here to avoid collision with CBaseEntity's speed key
	if ( FStrEq(szKeyName, "Speed") )
	{
		m_nSpeedBase = atoi( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CTriggerWind::CreateVPhysics()
{
	BaseClass::CreateVPhysics();

	m_pWindController = physenv->CreateMotionController( &m_WindCallback );
	return true;
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CTriggerWind::UpdateOnRemove()
{
	if ( m_pWindController )
	{
		physenv->DestroyMotionController( m_pWindController );
		m_pWindController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::StartTouch(CBaseEntity *pOther)
{
	if ( !PassesTriggerFilters(pOther) )
		return;
	if ( pOther->IsPlayer() )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys)
	{
		m_pWindController->AttachObject( pPhys, false );
		pPhys->Wake();
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::EndTouch(CBaseEntity *pOther)
{
	if ( !PassesTriggerFilters(pOther) )
		return;
	if ( pOther->IsPlayer() )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys && m_pWindController )
	{
		m_pWindController->DetachObject( pPhys );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputEnable( inputdata_t &inputdata )
{
	BaseClass::InputEnable( inputdata );
	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime + 0.1f, WIND_THINK_CONTEXT );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::WindThink( void )
{
	// By default...
	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime + 0.1, WIND_THINK_CONTEXT );

	// Is it time to change the wind?
	if (m_bSwitch)
	{
		m_bSwitch = false;

		// Set new target direction and speed
		m_nSpeedTarget = m_nSpeedBase + random->RandomInt( -m_nSpeedNoise, m_nSpeedNoise );
		m_nDirTarget = UTIL_AngleMod( m_nDirBase + random->RandomInt(-m_nDirNoise, m_nDirNoise) );
	}
	else
	{
		bool bDone = true;
		// either ramp up, or sleep till change
		if (abs(m_nSpeedTarget - m_nSpeedCurrent) > MAX_WIND_CHANGE)
		{
			m_nSpeedCurrent += (m_nSpeedTarget > m_nSpeedCurrent) ? MAX_WIND_CHANGE : -MAX_WIND_CHANGE;
			bDone = false;
		}

		if (abs(m_nDirTarget - m_nDirCurrent) > MAX_WIND_CHANGE)
		{

			m_nDirCurrent = UTIL_ApproachAngle( m_nDirTarget, m_nDirCurrent, MAX_WIND_CHANGE );
			bDone = false;
		}
		
		if (bDone)
		{
			m_nSpeedCurrent = m_nSpeedTarget;
			SetContextThink( &CTriggerWind::WindThink, m_nHoldBase + random->RandomFloat(-m_nHoldNoise,m_nHoldNoise), WIND_THINK_CONTEXT );
			m_bSwitch = true;
		}
	}

	// If we're starting to blow, where we weren't before, wake up all our objects
	if ( m_nSpeedCurrent )
	{
		m_pWindController->WakeObjects();
	}

	// store the wind data in the controller callback
	m_WindCallback.m_nWindYaw = m_nDirCurrent;
	if ( m_bDisabled )
	{
		m_WindCallback.m_flWindSpeed = 0;
	}
	else
	{
		m_WindCallback.m_flWindSpeed = m_nSpeedCurrent;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputSetSpeed( inputdata_t &inputdata )
{
	// Set new speed and mark to switch
	m_nSpeedBase = inputdata.value.Int();
	m_bSwitch = true;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerWind::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Dir: %i (%i)",m_nDirCurrent,m_nDirTarget);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Speed: %i (%i)",m_nSpeedCurrent,m_nSpeedTarget);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


// ##################################################################################
//	>> TriggerImpact
//
//  Blows physics objects in the trigger
//
// ##################################################################################
#define TRIGGERIMPACT_VIEWKICK_SCALE 0.1

class CTriggerImpact : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerImpact, CTriggerMultiple );
public:
	DECLARE_DATADESC();

	float	m_flMagnitude;
	float	m_flNoise;
	float	m_flViewkick;

	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );

	// Inputs
	void InputSetMagnitude( inputdata_t &inputdata );
	void InputImpact( inputdata_t &inputdata );

	// Outputs
	COutputVector	m_pOutputForce;		// Output force in case anyone else wants to use it

	// Debug
	int		DrawDebugTextOverlays(void);
};

LINK_ENTITY_TO_CLASS( trigger_impact, CTriggerImpact );

BEGIN_DATADESC( CTriggerImpact )

	DEFINE_KEYFIELD( m_flMagnitude,	FIELD_FLOAT, "Magnitude"),
	DEFINE_KEYFIELD( m_flNoise,		FIELD_FLOAT, "Noise"),
	DEFINE_KEYFIELD( m_flViewkick,	FIELD_FLOAT, "Viewkick"),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,  "Impact", InputImpact ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMagnitude", InputSetMagnitude ),

	// Outputs
	DEFINE_OUTPUT(m_pOutputForce, "ImpactForce"),

	// Function Pointers
	DEFINE_FUNCTION( Disable ),


END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::Spawn( void )
{	
	// Clamp date in case user made an error
	m_flNoise = clamp(m_flNoise,0.f,1.f);
	m_flViewkick = clamp(m_flViewkick,0.f,1.f);

	// Always start disabled
	m_bDisabled = true;
	BaseClass::Spawn();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputImpact( inputdata_t &inputdata )
{
	// Output the force vector in case anyone else wants to use it
	Vector vDir;
	AngleVectors( GetLocalAngles(),&vDir );
	m_pOutputForce.Set( m_flMagnitude * vDir, inputdata.pActivator, inputdata.pCaller);

	// Enable long enough to throw objects inside me
	Enable();
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink(&CTriggerImpact::Disable);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::StartTouch(CBaseEntity *pOther)
{
	//If the entity is valid and has physics, hit it
	if ( ( pOther != NULL  ) && ( pOther->VPhysicsGetObject() != NULL ) )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );
		vDir += RandomVector(-m_flNoise,m_flNoise);
		pOther->VPhysicsGetObject()->ApplyForceCenter( m_flMagnitude * vDir );
	}

	// If the player, so a view kick
	if (pOther->IsPlayer() && fabs(m_flMagnitude)>0 )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );

		float flPunch = -m_flViewkick*m_flMagnitude*TRIGGERIMPACT_VIEWKICK_SCALE;
		pOther->ViewPunch( QAngle( vDir.y * flPunch, 0, vDir.x * flPunch ) );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputSetMagnitude( inputdata_t &inputdata )
{
	m_flMagnitude = inputdata.value.Float();
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerImpact::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Magnitude: %3.2f",m_flMagnitude);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Disables auto movement on players that touch it
//-----------------------------------------------------------------------------

const int SF_TRIGGER_MOVE_AUTODISABLE				= 0x80;		// disable auto movement
const int SF_TRIGGER_AUTO_DUCK						= 0x800;	// Duck automatically

class CTriggerPlayerMovement : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerPlayerMovement, CBaseTrigger );
public:

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_playermovement, CTriggerPlayerMovement );

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPlayerMovement::Spawn( void )
{
	if( HasSpawnFlags( SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS ) )
	{
		// @Note (toml 01-07-04): fix up spawn flag collision coding error. Remove at some point once all maps fixed up please!
		DevMsg("*** trigger_playermovement using obsolete spawnflag. Remove and reset with new value for \"Disable auto player movement\"\n" );
		RemoveSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS);
		AddSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE);
	}
	BaseClass::Spawn();

	InitTrigger();
}


// UNDONE: This will not support a player touching more than one of these
// UNDONE: Do we care?  If so, ref count automovement in the player?
void CTriggerPlayerMovement::StartTouch( CBaseEntity *pOther )
{	
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_DUCK ) )
	{
		pPlayer->ForceButtons( IN_DUCK );
	}

	// UNDONE: Currently this is the only operation this trigger can do
	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = false;
	}
}

void CTriggerPlayerMovement::EndTouch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_DUCK ) )
	{
		pPlayer->UnforceButtons( IN_DUCK );
	}

	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = true;
	}
}

//------------------------------------------------------------------------------
// Base VPhysics trigger implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Save/load
//------------------------------------------------------------------------------
BEGIN_DATADESC( CBaseVPhysicsTrigger )
	DEFINE_KEYFIELD( m_bDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
END_DATADESC()

//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::Spawn()
{
	Precache();

	SetSolid( SOLID_VPHYSICS );	
	AddSolidFlags( FSOLID_NOT_SOLID );

	// NOTE: Don't make yourself FSOLID_TRIGGER here or you'll get game 
	// collisions AND vphysics collisions.  You don't want any game collisions
	// so just use FSOLID_NOT_SOLID

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		AddEffects( EF_NODRAW );
	}

	CreateVPhysics();
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CBaseVPhysicsTrigger::CreateVPhysics()
{
	IPhysicsObject *pPhysics;
	if ( !HasSpawnFlags( SF_VPHYSICS_MOTION_MOVEABLE ) )
	{
		pPhysics = VPhysicsInitStatic();
	}
	else
	{
		pPhysics = VPhysicsInitShadow( false, false );
	}

	pPhysics->BecomeTrigger();
	return true;
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::UpdateOnRemove()
{
	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->RemoveTrigger();
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Activate
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}

//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		m_bDisabled = false;
		if ( VPhysicsGetObject())
		{
			VPhysicsGetObject()->EnableCollisions( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		m_bDisabled = true;
		if ( VPhysicsGetObject())
		{
			VPhysicsGetObject()->EnableCollisions( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::StartTouch( CBaseEntity *pOther )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::EndTouch( CBaseEntity *pOther )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseVPhysicsTrigger::PassesTriggerFilters( CBaseEntity *pOther )
{
	if ( pOther->GetMoveType() != MOVETYPE_VPHYSICS && !pOther->IsPlayer() )
		return false;

	// First test spawn flag filters
	if ( HasSpawnFlags(SF_TRIGGER_ALLOW_ALL) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS))
	{
		bool bOtherIsPlayer = pOther->IsPlayer();
		if( HasSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS) && !bOtherIsPlayer )
		{
			CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

			if( !pNPC || !pNPC->IsPlayerAlly() )
			{
				return false;
			}
		}

		CBaseFilter *pFilter = m_hFilter.Get();
		return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
	}
	return false;
}

//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: VPhysics trigger that changes the motion of vphysics objects that touch it
//-----------------------------------------------------------------------------
class CTriggerVPhysicsMotion : public CBaseVPhysicsTrigger, public IMotionEvent
{
	DECLARE_CLASS( CTriggerVPhysicsMotion, CBaseVPhysicsTrigger );

public:
	void Spawn();
	void Precache();
	virtual void UpdateOnRemove();
	bool CreateVPhysics();

	// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
	// BUGBUG: If a player touches two of these, his movement will screw up.
	// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );

	void InputSetVelocityLimitTime( inputdata_t &inputdata );

	float LinearLimit();

	inline bool HasGravityScale() { return m_gravityScale != 1.0 ? true : false; }
	inline bool HasAirDensity() { return m_addAirDensity != 0 ? true : false; }
	inline bool HasLinearLimit() { return LinearLimit() != 0.0f; }
	inline bool HasLinearScale() { return m_linearScale != 1.0 ? true : false; }
	inline bool HasAngularLimit() { return m_angularLimit != 0 ? true : false; }
	inline bool HasAngularScale() { return m_angularScale != 1.0 ? true : false; }
	inline bool HasLinearForce() { return m_linearForce != 0.0 ? true : false; }

	DECLARE_DATADESC();

	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	IPhysicsMotionController	*m_pController;

#ifndef _XBOX
	EntityParticleTrailInfo_t	m_ParticleTrail;
#endif //!_XBOX

	float						m_gravityScale;
	float						m_addAirDensity;
	float						m_linearLimit;
	float						m_linearLimitDelta;
	float						m_linearLimitTime;
	float						m_linearLimitStart;
	float						m_linearLimitStartTime;
	float						m_linearScale;
	float						m_angularLimit;
	float						m_angularScale;
	float						m_linearForce;
	QAngle						m_linearForceAngles;
};


//------------------------------------------------------------------------------
// Save/load
//------------------------------------------------------------------------------
BEGIN_DATADESC( CTriggerVPhysicsMotion )
#ifndef _XBOX
	DEFINE_EMBEDDED( m_ParticleTrail ),
#endif //!_XBOX
	DEFINE_INPUT( m_gravityScale, FIELD_FLOAT, "SetGravityScale" ),
	DEFINE_INPUT( m_addAirDensity, FIELD_FLOAT, "SetAdditionalAirDensity" ),
	DEFINE_INPUT( m_linearLimit, FIELD_FLOAT, "SetVelocityLimit" ),
	DEFINE_INPUT( m_linearLimitDelta, FIELD_FLOAT, "SetVelocityLimitDelta" ),
	DEFINE_INPUT( m_linearScale, FIELD_FLOAT, "SetVelocityScale" ),
	DEFINE_INPUT( m_angularLimit, FIELD_FLOAT, "SetAngVelocityLimit" ),
	DEFINE_INPUT( m_angularScale, FIELD_FLOAT, "SetAngVelocityScale" ),
	DEFINE_INPUT( m_linearForce, FIELD_FLOAT, "SetLinearForce" ),
	DEFINE_INPUT( m_linearForceAngles, FIELD_VECTOR, "SetLinearForceAngles" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetVelocityLimitTime", InputSetVelocityLimitTime ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_vphysics_motion, CTriggerVPhysicsMotion );


//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::Spawn()
{
	Precache();

	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Precache
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::Precache()
{
#ifndef _XBOX
	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		PrecacheMaterial( STRING(m_ParticleTrail.m_strMaterialName) ); 
	}
#endif //!_XBOX
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
float CTriggerVPhysicsMotion::LinearLimit()
{
	if ( m_linearLimitTime == 0.0f )
		return m_linearLimit;

	float dt = gpGlobals->curtime - m_linearLimitStartTime;
	if ( dt >= m_linearLimitTime )
	{
		m_linearLimitTime = 0.0;
		return m_linearLimit;
	}

	dt /= m_linearLimitTime;
	float flLimit = RemapVal( dt, 0.0f, 1.0f, m_linearLimitStart, m_linearLimit );
	return flLimit;
}

	
//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CTriggerVPhysicsMotion::CreateVPhysics()
{
	m_pController = physenv->CreateMotionController( this );
	BaseClass::CreateVPhysics();

	return true;
}


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::UpdateOnRemove()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Start/End Touch
//------------------------------------------------------------------------------
// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
// BUGBUG: If a player touches two of these, his movement will screw up.
// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
void CTriggerVPhysicsMotion::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( !PassesTriggerFilters(pOther) )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( pPlayer )
	{
		pPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, true );
		pPlayer->m_Local.m_bSlowMovement = true;
	}

	triggerevent_t event;
	PhysGetTriggerEvent( &event, this );
	if ( event.pObject )
	{
		// these all get done again on save/load, so check
		m_pController->AttachObject( event.pObject, true );
	}

	// Don't show these particles on the XBox
#ifndef _XBOX
	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		CEntityParticleTrail::Create( pOther, m_ParticleTrail, this ); 
	}
#endif

	if ( pOther->GetBaseAnimating() && pOther->GetBaseAnimating()->IsRagdoll() )
	{
		CRagdollBoogie::IncrementSuppressionCount( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerVPhysicsMotion::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( !PassesTriggerFilters(pOther) )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( pPlayer )
	{
		pPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, false );
		pPlayer->m_Local.m_bSlowMovement = false;
	}
	triggerevent_t event;
	PhysGetTriggerEvent( &event, this );
	if ( event.pObject && m_pController )
	{
		m_pController->DetachObject( event.pObject );
	}

#ifndef _XBOX
	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		CEntityParticleTrail::Destroy( pOther, m_ParticleTrail ); 
	}
#endif //!_XBOX

	if ( pOther->GetBaseAnimating() && pOther->GetBaseAnimating()->IsRagdoll() )
	{
		CRagdollBoogie::DecrementSuppressionCount( pOther );
	}
}


//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::InputSetVelocityLimitTime( inputdata_t &inputdata )
{
	m_linearLimitStart = LinearLimit();
	m_linearLimitStartTime = gpGlobals->curtime;

	float args[2];
	UTIL_StringToFloatArray( args, 2, inputdata.value.String() );
	m_linearLimit = args[0];
	m_linearLimitTime = args[1];
}

//------------------------------------------------------------------------------
// Apply the forces to the entity
//------------------------------------------------------------------------------
IMotionEvent::simresult_e CTriggerVPhysicsMotion::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( m_bDisabled )
		return SIM_NOTHING;

	linear.Init();
	angular.Init();

	if ( HasGravityScale() )
	{
		// assume object already has 1.0 gravities applied to it, so apply the additional amount
		linear.z -= (m_gravityScale-1) * GetCurrentGravity();
	}

	if ( HasLinearForce() )
	{
		Vector vecForceDir;
		AngleVectors( m_linearForceAngles, &vecForceDir );
		VectorMA( linear, m_linearForce, vecForceDir, linear );
	}

	if ( HasAirDensity() || HasLinearLimit() || HasLinearScale() || HasAngularLimit() || HasAngularScale() )
	{
		Vector vel;
		AngularImpulse angVel;
		pObject->GetVelocity( &vel, &angVel );
		vel += linear * deltaTime; // account for gravity scale

		Vector unitVel = vel;
		Vector unitAngVel = angVel;

		float speed = VectorNormalize( unitVel );
		float angSpeed = VectorNormalize( unitAngVel );

		float speedScale = 0.0;
		float angSpeedScale = 0.0;

		if ( HasAirDensity() )
		{
			float linearDrag = -0.5 * m_addAirDensity * pObject->CalculateLinearDrag( unitVel ) * deltaTime;
			if ( linearDrag < -1 )
			{
				linearDrag = -1;
			}
			speedScale += linearDrag / deltaTime;
			float angDrag = -0.5 * m_addAirDensity * pObject->CalculateAngularDrag( unitAngVel ) * deltaTime;
			if ( angDrag < -1 )
			{
				angDrag = -1;
			}
			angSpeedScale += angDrag / deltaTime;
		}

		if ( HasLinearLimit() && speed > m_linearLimit )
		{
			float flDeltaVel = (LinearLimit() - speed) / deltaTime;
			if ( m_linearLimitDelta != 0.0f )
			{
				float flMaxDeltaVel = -m_linearLimitDelta / deltaTime;
				if ( flDeltaVel < flMaxDeltaVel )
				{
					flDeltaVel = flMaxDeltaVel;
				}
			}
			VectorMA( linear, flDeltaVel, unitVel, linear );
		}
		if ( HasAngularLimit() && angSpeed > m_angularLimit )
		{
			angular += ((m_angularLimit - angSpeed)/deltaTime) * unitAngVel;
		}
		if ( HasLinearScale() )
		{
			speedScale = ( (speedScale+1) * m_linearScale ) - 1;
		}
		if ( HasAngularScale() )
		{
			angSpeedScale = ( (angSpeedScale+1) * m_angularScale ) - 1;
		}
		linear += vel * speedScale;
		angular += angVel * angSpeedScale;
	}

	return SIM_GLOBAL_ACCELERATION;
}

class CServerRagdollTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CServerRagdollTrigger, CBaseTrigger );

public:

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
	virtual void Spawn( void );

};

LINK_ENTITY_TO_CLASS( trigger_serverragdoll, CServerRagdollTrigger );

void CServerRagdollTrigger::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

void CServerRagdollTrigger::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( pOther->IsPlayer() )
		return;

	CBaseCombatCharacter *pCombatChar = pOther->MyCombatCharacterPointer();

	if ( pCombatChar )
	{
		pCombatChar->m_bForceServerRagdoll = true;
	}
}

void CServerRagdollTrigger::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( pOther->IsPlayer() )
		return;

	CBaseCombatCharacter *pCombatChar = pOther->MyCombatCharacterPointer();

	if ( pCombatChar )
	{
		pCombatChar->m_bForceServerRagdoll = false;
	}
}

bool IsTriggerClass( CBaseEntity *pEntity )
{
	if ( NULL != dynamic_cast<CBaseTrigger *>(pEntity) )
		return true;

	if ( NULL != dynamic_cast<CTriggerVPhysicsMotion *>(pEntity) )
		return true;
	
	return false;
}
