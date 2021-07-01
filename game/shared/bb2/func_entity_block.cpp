//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Player Block Clip : Choose which entity you want to allow to pass through the volume, good for protecting spawns. This also blocks npcs. (you can choose to let them pass)
//
//========================================================================================//

#include "cbase.h"
#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#define CFuncEntityBlock C_FuncEntityBlock
#endif
#include "hl2mp_gamerules.h"

enum DynamicBlockerStates
{
	DYN_BLOCK_ALL = 0, // By default the brush blocks all entities when disabled.
	DYN_BLOCK_NOTHING, // Block nothing when brush is disabled.
};

class CFuncEntityBlock : public CBaseEntity
{
public:

	DECLARE_CLASS(CFuncEntityBlock, CBaseEntity);
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
	void Spawn();
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputSetState(inputdata_t &inputdata);
#endif

	CFuncEntityBlock();
	bool ShouldCollide(int collisionGroup, int contentsMask) const;

private:

	CNetworkVar(int, m_iCollisionGroup);
	CNetworkVar(int, m_iBlockState);
	CNetworkVar(bool, m_bDisabled);
};

IMPLEMENT_NETWORKCLASS_ALIASED(FuncEntityBlock, DT_FuncEntityBlock)

BEGIN_NETWORK_TABLE(CFuncEntityBlock, DT_FuncEntityBlock)
#if !defined( CLIENT_DLL )
SendPropInt(SENDINFO(m_iCollisionGroup), 6, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iBlockState), 2, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bDisabled)),
#else
RecvPropInt(RECVINFO(m_iCollisionGroup)),
RecvPropInt(RECVINFO(m_iBlockState)),
RecvPropBool(RECVINFO(m_bDisabled)),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC(CFuncEntityBlock)
DEFINE_KEYFIELD(m_iCollisionGroup, FIELD_INTEGER, "CollisionGroup"),
DEFINE_KEYFIELD(m_iBlockState, FIELD_INTEGER, "BlockState"),
DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetState", InputSetState),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS(trigger_player_block, CFuncEntityBlock)

CFuncEntityBlock::CFuncEntityBlock()
{
	m_iCollisionGroup = 0;
	m_iBlockState = 0;
	m_bDisabled = false;
}

#ifdef GAME_DLL
void CFuncEntityBlock::Spawn()
{
	BaseClass::Spawn();
	SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_NONE);
	SetModel(STRING(GetModelName()));
	VPhysicsInitShadow(false, false);
	SetBlocksLOS(false);
}

void CFuncEntityBlock::InputEnable(inputdata_t &inputdata)
{
	m_bDisabled.Set(false);
}

void CFuncEntityBlock::InputDisable(inputdata_t &inputdata)
{
	m_bDisabled.Set(true);
}

void CFuncEntityBlock::InputSetState(inputdata_t &inputdata)
{
	m_iBlockState.Set(inputdata.value.Int());
}
#endif

//-----------------------------------------------------------------------------
// Purpose: We check if the desired collision group or player team is within this brush, if so we let them pass. 
// Notice: We now use unique collision groups for the players & npcs.
//-----------------------------------------------------------------------------
bool CFuncEntityBlock::ShouldCollide(int collisionGroup, int contentsMask) const
{
	if (m_bDisabled.Get()) // We're disabled, by default block all ents, block nothing otherwise.
		return (m_iBlockState.Get() == DYN_BLOCK_ALL);

	if ((m_iCollisionGroup == 30) && (collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_REALITY_PHASE || collisionGroup == COLLISION_GROUP_NPC || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_BOSS || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_CRAWLER || collisionGroup == COLLISION_GROUP_NPC_MILITARY || collisionGroup == COLLISION_GROUP_NPC_MERCENARY))
		return false;

	if ((m_iCollisionGroup == 31) && (collisionGroup == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_BOSS || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_CRAWLER || collisionGroup == COLLISION_GROUP_NPC_MILITARY || collisionGroup == COLLISION_GROUP_NPC_MERCENARY))
		return false;

	if ((m_iCollisionGroup == 32) && (collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_REALITY_PHASE || collisionGroup == COLLISION_GROUP_NPC_MILITARY))
		return false;

	if ((m_iCollisionGroup == 33) && (collisionGroup == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_BOSS || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_CRAWLER))
		return false;

	// We have more types of npcs so we do an in-depth check:
	if (m_iCollisionGroup == COLLISION_GROUP_NPC)
	{
		if ((collisionGroup == COLLISION_GROUP_NPC_ZOMBIE) || (collisionGroup == COLLISION_GROUP_NPC_MILITARY) || (collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_BOSS) || (collisionGroup == COLLISION_GROUP_NPC_ZOMBIE_CRAWLER)
			|| (collisionGroup == COLLISION_GROUP_NPC_ACTOR) || (collisionGroup == COLLISION_GROUP_NPC_SCRIPTED) || (collisionGroup == COLLISION_GROUP_NPC_MERCENARY) ||
			(collisionGroup == COLLISION_GROUP_NPC))
			return false;
	}

	if (collisionGroup == m_iCollisionGroup)
		return false;

	// If we want to allow human players only we need to check if the collision group is COLLISION_GROUP_PLAYER_REALITY_PHASE or COLLISION_GROUP_PLAYER. The above check will handle COLLISION_GROUP_PLAYER.
	if ((m_iCollisionGroup == COLLISION_GROUP_PLAYER) && (collisionGroup == COLLISION_GROUP_PLAYER_REALITY_PHASE))
		return false;

	return true;
}