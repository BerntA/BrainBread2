//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Console commands for debugging and manipulating NPCs.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void SetDebugBits( CBasePlayer* pPlayer, const char *name, int bit );
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

//------------------------------------------------------------------------------
// Purpose: Disables all NPCs
//------------------------------------------------------------------------------
void CC_AI_Disable( void )
{
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		CAI_BaseNPC::m_nDebugBits &= ~bits_debugDisableAI;
		DevMsg("AI Enabled.\n");
	}
	else
	{
		CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
		DevMsg("AI Disabled.\n");
	}

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_disable("ai_disable", CC_AI_Disable, "Bi-passes all AI logic routines and puts all NPCs into their idle animations.  Can be used to get NPCs out of your way and to test effect of AI logic routines on frame rate", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: NPC step trough AI
//------------------------------------------------------------------------------
void CC_AI_Step( void )
{
	DevMsg("AI Stepping...\n");

	// Start NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits |= bits_debugStepAI;
	CAI_BaseNPC::m_nDebugPauseIndex++;
}
static ConCommand ai_step("ai_step", CC_AI_Step, "NPCs will freeze after completing their current task.  To complete the next task, use 'ai_step' again.  To resume processing normally use 'ai_resume'", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Resume normal AI processing after stepping
//------------------------------------------------------------------------------
void CC_AI_Resume( void )
{
	DevMsg("AI Resume...\n");

	// End NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits &= ~bits_debugStepAI;
}
static ConCommand ai_resume("ai_resume", CC_AI_Resume, "If NPC is stepping through tasks (see ai_step ) will resume normal processing.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show route triangulation attempts
//------------------------------------------------------------------------------
void CC_NPC_Bipass( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_TRIANGULATE_BIT);
}
static ConCommand npc_bipass("npc_bipass", CC_NPC_Bipass, "Displays the local movement attempts by the given NPC(s) (triangulation detours).  Failed bypass routes are displayed in red, successful bypasses are shown in green.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at.", FCVAR_CHEAT);
	
//------------------------------------------------------------------------------
// Purpose: Destroy selected NPC
//------------------------------------------------------------------------------
void CC_NPC_Destroy( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_ZAP_BIT);
}
static ConCommand npc_destroy("npc_destroy", CC_NPC_Destroy, "Removes the given NPC(s) from the universe\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_NPC_Kill( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_KILL_BIT);
}
static ConCommand npc_kill("npc_kill", CC_NPC_Kill, "Kills the given NPC(s)\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show selected NPC's enemies
//------------------------------------------------------------------------------
void CC_NPC_Enemies( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_ENEMIES_BIT);
}
static ConCommand npc_enemies("npc_enemies", CC_NPC_Enemies, "Shows memory of NPC.  Draws an X on top of each memory.\n\tEluded entities drawn in blue (don't know where it went)\n\tUnreachable entities drawn in green (can't get to it)\n\tCurrent enemy drawn in red\n\tCurrent target entity drawn in magenta\n\tAll other entities drawn in pink\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show seletected NPC's current enemy and target entity
//------------------------------------------------------------------------------
void CC_NPC_Focus( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_FOCUS_BIT);
}
static ConCommand npc_focus("npc_focus", CC_NPC_Focus, "Displays red line to NPC's enemy (if has one) and blue line to NPC's target entity (if has one)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

ConVar npc_create_equipment("npc_create_equipment", "");
//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_NPC_Create( const CCommand &args )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(args[1]) );
	if (baseNPC)
	{
		baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();

		if ( args.ArgC() == 3 )
		{
			baseNPC->SetName( AllocPooledString( args[2] ) );
		}

		DispatchSpawn(baseNPC);
		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		AI_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}

		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand npc_create("npc_create", CC_NPC_Create, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_NPC_Create_Aimed( const CCommand &args )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(args[1]) );
	if (baseNPC)
	{
		baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();
		DispatchSpawn( baseNPC );

		// Now attempt to drop into the world
		QAngle angles;
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		VectorAngles( forward, angles );
		angles.x = 0; 
		angles.z = 0;
		AI_TraceLine( pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, &angles, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, &angles, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}
		else
		{
			baseNPC->Teleport( NULL, &angles, NULL );
		}

		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand npc_create_aimed("npc_create_aimed", CC_NPC_Create_Aimed, "Creates an NPC aimed away from the player of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Destroy unselected NPCs
//------------------------------------------------------------------------------
void CC_NPC_DestroyUnselected( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) && !pNPC->ClassMatches("npc_bullseye"))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_ZAP_BIT;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_destroy_unselected("npc_destroy_unselected", CC_NPC_DestroyUnselected, "Removes all NPCs from the universe that aren't currently selected", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Freeze or unfreeze the selected NPCs. If no NPCs are selected, the
//			NPC under the crosshair is frozen/unfrozen.
//------------------------------------------------------------------------------
void CC_NPC_Freeze( const CCommand &args )
{
	if (FStrEq(args[1], "")) 
	{
		//	
		// No NPC was specified, try to freeze selected NPCs.
		//
		bool bFound = false;
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );
		while (npc)
		{
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
				bFound = true;
				npc->ToggleFreeze();
			}
			npc = gEntList.NextEntByClass(npc);
		}

		if (!bFound)
		{
			//	
			// No selected NPCs, look for the NPC under the crosshair.
			//
			CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
			if ( pEntity )
			{
				CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
				if (pNPC)
				{
					pNPC->ToggleFreeze();
				}
			}
		}
	}
	else
	{
		// TODO: look for NPCs by name, classname.
	}
}
static ConCommand npc_freeze("npc_freeze", CC_NPC_Freeze, "Selected NPC(s) will freeze in place (or unfreeze). If there are no selected NPCs, uses the NPC under the crosshair.\n\tArguments:	-none-", FCVAR_CHEAT);


CON_COMMAND( npc_freeze_unselected, "Freeze all NPCs not selected" )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			pNPC->ToggleFreeze();
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}

//------------------------------------------------------------------------------
CON_COMMAND(npc_thinknow, "Trigger NPC to think")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetThink( &CAI_BaseNPC::CallNPCThink );
			pNPC->SetNextThink( gpGlobals->curtime );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Tell selected NPC to go to a where player is looking
//------------------------------------------------------------------------------

void CC_NPC_Teleport( void )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	AI_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0)
	{
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

		while (npc)
		{
			//Only Teleport one NPC if more than one is selected.
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
                npc->Teleport( &tr.endpos, NULL, NULL );
				break;
			}

			npc = gEntList.NextEntByClass(npc);
		}
	}
}

static ConCommand npc_teleport("npc_teleport", CC_NPC_Teleport, "Selected NPC will teleport to the location that the player is looking (shown with a purple box)\n\tArguments:	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: ?Does this work?
//------------------------------------------------------------------------------
void CC_NPC_Reset( void )
{
	CAI_BaseNPC::ClearAllSchedules();
	g_AI_SchedulesManager.LoadAllSchedules();
}
static ConCommand npc_reset("npc_reset", CC_NPC_Reset, "Reloads schedules for all NPC's from their script files\n\tArguments:	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show the selected NPC's route
//------------------------------------------------------------------------------
void CC_NPC_Route( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_ROUTE_BIT);
}
static ConCommand npc_route("npc_route", CC_NPC_Route, "Displays the current route of the given NPC as a line on the screen.  Waypoints along the route are drawn as small cyan rectangles.  Line is color coded in the following manner:\n\tBlue	- path to a node\n\tCyan	- detour around an object (triangulation)\n\tRed	- jump\n\tMaroon - path to final target position\n\tArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Select an NPC
//------------------------------------------------------------------------------
void CC_NPC_Select( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_SELECTED_BIT);
}
static ConCommand npc_select("npc_select", CC_NPC_Select, "Select or deselects the given NPC(s) for later manipulation.  Selected NPC's are shown surrounded by a red translucent box\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show combat related data for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Combat( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_SQUAD_BIT);
}
static ConCommand npc_combat("npc_combat", CC_NPC_Combat, "Displays text debugging information about the squad and enemy of the selected NPC  (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);
// For backwards compatibility
static ConCommand npc_squads("npc_squads", CC_NPC_Combat, "Obsolete.  Replaced by npc_combat", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show tasks for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Tasks( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_TASK_BIT);
}
static ConCommand npc_tasks("npc_tasks", CC_NPC_Tasks, "Displays detailed text debugging information about the all the tasks of the selected NPC current schedule (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show tasks (on the console) for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Task_Text( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(), args[1], OVERLAY_TASK_TEXT_BIT);
}
static ConCommand npc_task_text("npc_task_text", CC_NPC_Task_Text, "Outputs text debugging information to the console about the all the tasks + break conditions of the selected NPC current schedule\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Shows all current conditions for an NPC.
//------------------------------------------------------------------------------
void CC_NPC_Conditions( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(), args[1], OVERLAY_NPC_CONDITIONS_BIT);
}
static ConCommand npc_conditions("npc_conditions", CC_NPC_Conditions, "Displays all the current AI conditions that an NPC has in the overlay text.\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show an NPC's viewcone
//------------------------------------------------------------------------------
void CC_NPC_Viewcone( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_VIEWCONE_BIT);
}
static ConCommand npc_viewcone("npc_viewcone", CC_NPC_Viewcone, "Displays the viewcone of the NPC (where they are currently looking and what the extents of there vision is)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show an NPC's relationships to other NPCs
//------------------------------------------------------------------------------
void CC_NPC_Relationships( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1], OVERLAY_NPC_RELATION_BIT );
}
static ConCommand npc_relationships("npc_relationships", CC_NPC_Relationships, "Displays the relationships between this NPC and all others.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: Show an NPC's steering regulations
//------------------------------------------------------------------------------
void CC_NPC_ViewSteeringRegulations( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(), args[1], OVERLAY_NPC_STEERING_REGULATIONS);
}
static ConCommand npc_steering("npc_steering", CC_NPC_ViewSteeringRegulations, "Displays the steering obstructions of the NPC (used to perform local avoidance)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

void CC_NPC_ViewSteeringRegulationsAll( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_STEERING_REGULATIONS;
		}
		else
		{
			pNPC->m_debugOverlays &= ~OVERLAY_NPC_STEERING_REGULATIONS;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_steering_all("npc_steering_all", CC_NPC_ViewSteeringRegulationsAll, "Displays the steering obstructions of all NPCs (used to perform local avoidance)\n", FCVAR_CHEAT);

//------------------------------------------------------------------------------

CON_COMMAND( npc_heal, "Heals the target back to full health" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetHealth( pNPC->GetMaxHealth() );
		}
	}
}

CON_COMMAND(npc_ammo_deplete, "Subtracts half of the target's ammo")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CBaseEntity *pEntity = FindPickerEntity(UTIL_GetCommandClient());
	if (pEntity)
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC && pNPC->GetActiveWeapon())
			pNPC->GetActiveWeapon()->m_iClip *= 0.5;
	}
}

CON_COMMAND( ai_test_los, "Test AI LOS from the player's POV" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	trace_t tr;
	// Use the custom LOS trace filter
	CTraceFilterLOS traceFilter( UTIL_GetLocalPlayer(), COLLISION_GROUP_NONE );
	UTIL_TraceLine( UTIL_GetLocalPlayer()->EyePosition(), UTIL_GetLocalPlayer()->EyePosition() + UTIL_GetLocalPlayer()->EyeDirection3D() * MAX_COORD_RANGE, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr );
	NDebugOverlay::Line( UTIL_GetLocalPlayer()->EyePosition(), tr.endpos, 127, 127, 127, true, 5 );
	NDebugOverlay::Cross3D( tr.endpos, 24, 255, 255, 255, true, 5 );
}

#ifdef VPROF_ENABLED

CON_COMMAND(ainet_generate_report, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet" );
}

CON_COMMAND(ainet_generate_report_only, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet", g_VProfCurrentProfile.BudgetGroupNameToBudgetGroupID( "AINet" ) );
}

#endif
