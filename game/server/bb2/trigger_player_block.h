//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Player Block Clip : Choose which entity you want to allow to pass through the volume, good for protecting spawns. This also blocks npcs. (you can choose to let them pass)
//
//========================================================================================//

#ifndef TRIGGER_PLAYER_BLOCK_H
#define TRIGGER_PLAYER_BLOCK_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerPlayerBlock : public CBaseTrigger
{
public:

	DECLARE_CLASS(CTriggerPlayerBlock, CBaseTrigger);
	DECLARE_DATADESC();

	CTriggerPlayerBlock();
	void Spawn();

	virtual	bool ShouldCollide(int collisionGroup, int contentsMask) const;

private:

	int m_iCollisionGroup;
};

#endif // TRIGGER_PLAYER_BLOCK_H