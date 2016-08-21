//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Player Block Clip : Choose which entity you want to allow to pass through the volume, good for protecting spawns. This also blocks npcs. (you can choose to let them pass)
//
//========================================================================================//

#include "cbase.h"
#include "baseplayer_shared.h"
#include "trigger_player_block.h"
#include "hl2mp_gamerules.h"
#include "baseentity.h"

BEGIN_DATADESC(CTriggerPlayerBlock)
DEFINE_KEYFIELD(m_iCollisionGroup, FIELD_INTEGER, "CollisionGroup"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(trigger_player_block, CTriggerPlayerBlock)

CTriggerPlayerBlock::CTriggerPlayerBlock()
{
	m_iCollisionGroup = 0;
}

void CTriggerPlayerBlock::Spawn()
{
	BaseClass::Spawn();
	SetSolid(SOLID_VPHYSICS);
	SetModel(STRING(GetModelName()));
	VPhysicsInitShadow(false, false);
}

//-----------------------------------------------------------------------------
// Purpose: We check if the desired collision group or player team is within this trigger, if so we let them pass. 
// Notice: We now use unique collision groups for the players & npcs.
//-----------------------------------------------------------------------------
bool CTriggerPlayerBlock::ShouldCollide(int collisionGroup, int contentsMask) const
{
	// If this trigger is disabled we're always solid!
	if (m_bDisabled)
		return true;

	if ((m_iCollisionGroup == 30) && (collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_REALITY_PHASE || collisionGroup == COLLISION_GROUP_NPC || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_MILITARY || collisionGroup == COLLISION_GROUP_NPC_MERCENARY))
		return false;

	if ((m_iCollisionGroup == 31) && (collisionGroup == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_MILITARY || collisionGroup == COLLISION_GROUP_NPC_MERCENARY))
		return false;

	if ((m_iCollisionGroup == 32) && (collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_REALITY_PHASE || collisionGroup == COLLISION_GROUP_NPC_MILITARY))
		return false;

	if ((m_iCollisionGroup == 33) && (collisionGroup == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup == COLLISION_GROUP_NPC_ZOMBIE))
		return false;

	// We have more types of npcs so we do an in-depth check:
	if (m_iCollisionGroup == COLLISION_GROUP_NPC)
	{
		if ((collisionGroup == COLLISION_GROUP_NPC_ZOMBIE) || (collisionGroup == COLLISION_GROUP_NPC_MILITARY)
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