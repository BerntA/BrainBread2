//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom ally/enemy/villain actor NPC.
//
//========================================================================================//

#ifndef	NPC_CUSTOM_ACTOR_H
#define	NPC_CUSTOM_ACTOR_H

#include "npc_base_soldier.h"

//-------------------------------------

class CNPC_CustomActor : public CNPC_BaseSoldier
{
	DECLARE_CLASS(CNPC_CustomActor, CNPC_BaseSoldier);
public:
	CNPC_CustomActor();

	//---------------------------------
	void			Precache();
	void			Spawn();
	bool			ParseNPC(CBaseEntity *pEntity);
	Class_T 		Classify();
	bool            GetGender() { return m_bGender; }
	bool            UsesNavMesh(void) { return true; }
	int				SelectSchedule(void);

	Activity		NPC_TranslateActivity(Activity eNewActivity);
	void 			HandleAnimEvent(animevent_t *pEvent);
	void			TaskFail(AI_TaskFailureCode_t code);

	void 			SimpleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	//---------------------------------
	// Combat
	//---------------------------------
	bool 			OnBeginMoveAndShoot();

	bool	        UseAttackSquadSlots()	{ return false; }
	void 			LocateEnemySound();

	void 			OnChangeActiveWeapon(CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon);

	//---------------------------------
	// Damage handling
	//---------------------------------
	int				OnTakeDamage(const CTakeDamageInfo &info);
	void			FireBullets(const FireBulletsInfo_t &info);

	//---------------------------------
	//	Sounds & speech
	//---------------------------------
	void			FearSound(void);
	void			DeathSound(const CTakeDamageInfo &info);
	void			PlaySound(const char *sound, float eventtime = -1.0f);

	void			AnnounceEnemyKill(CBaseEntity *pEnemy);

	BB2_SoundTypes GetNPCType() { return TYPE_CUSTOM; }
	const char *GetNPCName()
	{
		const char *name = STRING(m_iszNPCName);
		return ((name && name[0]) ? name : "N/A");
	}
	bool IsBoss() { return m_bBossState; }

protected:
	void UpdateNPCScaling();

private:

	// Customization
	string_t		m_iszNPCName;
	bool			m_bBossState;
	bool            m_bIsAlly;

	float m_flDamageScaleFactor;
	float m_flHealthScaleFactor;

	// Sound Logic
	int m_iTimesGreeted;

	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent		m_OnPlayerUse;
	COutputEvent		m_OnNavFailBlocked;

	bool					m_bNotifyNavFailBlocked;

	DECLARE_DATADESC();
};

#endif // NPC_CUSTOM_ACTOR_H