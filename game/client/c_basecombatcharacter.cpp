//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnPreDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnPreDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon* pWeapon = GetActiveWeapon();
	if (pWeapon)
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate IK for client-anims.
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::CalculateIKLocks(float currentTime)
{
	if (!m_pIk)
		return;

	int targetCount = m_pIk->m_target.Count();
	if (targetCount == 0)
		return;

	// In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
	// get in here during the view setup code, and it's not normally supposed to be able to access the spatial
	// partition that early in the rendering loop. So we allow access right here for that special case.
	SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
	partition->SuppressLists(PARTITION_ALL_CLIENT_EDICTS, false);
	CBaseEntity::PushEnableAbsRecomputations(false);

	for (int i = 0; i < targetCount; i++)
	{
		trace_t trace;
		CIKTarget* pTarget = &m_pIk->m_target[i];

		if (!pTarget->IsActive())
			continue;

		switch (pTarget->type)
		{
		case IK_GROUND:
		{
			pTarget->SetPos(Vector(pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z));
			pTarget->SetAngles(GetRenderAngles());
		}
		break;

		case IK_ATTACHMENT:
		{
			C_BaseEntity* pEntity = NULL;
			float flDist = pTarget->est.radius;

			// FIXME: make entity finding sticky!
			// FIXME: what should the radius check be?
			for (CEntitySphereQuery sphere(pTarget->est.pos, 64); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
			{
				C_BaseAnimating* pAnim = pEntity->GetBaseAnimating();
				if (!pAnim)
					continue;

				int iAttachment = pAnim->LookupAttachment(pTarget->offset.pAttachmentName);
				if (iAttachment <= 0)
					continue;

				Vector origin;
				QAngle angles;
				pAnim->GetAttachment(iAttachment, origin, angles);

				// debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

				float d = (pTarget->est.pos - origin).Length();

				if (d >= flDist)
					continue;

				flDist = d;
				pTarget->SetPos(origin);
				pTarget->SetAngles(angles);
				// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
			}

			if (flDist >= pTarget->est.radius)
			{
				// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
				// no solution, disable ik rule
				pTarget->IKFailed();
			}
		}
		break;
		}
	}

	CBaseEntity::PopEnableAbsRecomputations();
	partition->SuppressLists(curSuppressed, true);
}

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE(C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive)
RecvPropTime(RECVINFO(m_flNextAttack)),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
RecvPropDataTable("bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive)),
RecvPropEHandle(RECVINFO(m_hActiveWeapon)),
RecvPropArray3(RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle(RECVINFO(m_hMyWeapons[0]))),
RecvPropInt(RECVINFO(m_nGibFlags)),
RecvPropInt(RECVINFO(m_nMaterialOverlayFlags)),
END_RECV_TABLE()


BEGIN_PREDICTION_DATA(C_BaseCombatCharacter)

DEFINE_PRED_FIELD(m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_ARRAY(m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE),

END_PREDICTION_DATA()
