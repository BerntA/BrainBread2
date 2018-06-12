//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

//==================================================
// Definition for all AI interactions
//==================================================

#ifndef	AI_INTERACTIONS_H
#define	AI_INTERACTIONS_H

#ifdef _WIN32
#pragma once
#endif

//Combine
extern int	g_interactionCombineBash;

//ScriptedTarget
extern int  g_interactionScriptedTarget;

// AI Interaction for being hit by a physics object
extern int  g_interactionHitByPlayerThrownPhysObj;

// Alerts vital allies when the player punts a large object (car)
extern int	g_interactionPlayerPuntedHeavyObject;

// Zombie
// Melee attack will land in one second or so.
extern int	g_interactionZombieMeleeWarning;

#endif	//AI_INTERACTIONS_H