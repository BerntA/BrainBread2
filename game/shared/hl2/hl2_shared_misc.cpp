//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared HL2 Stuff...
//
//========================================================================================//

#include "cbase.h"
#include "hl2_shared_misc.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "view.h"
#else
#include "soundent.h"
#include "explode.h"
#include "smoke_trail.h"
#include "hl2_shareddefs.h"
#include "collisionutils.h"
#include "te_effect_dispatch.h"
#endif

#include "random_extended.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vphysics/friction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar player_throwforce("player_throwforce", "1000", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar player_pickup_maxmass("player_pickup_maxmass", "250", FCVAR_REPLICATED | FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Purpose: Computes a local matrix for the player clamped to valid carry ranges
//-----------------------------------------------------------------------------
// when looking level, hold bottom of object 8 inches below eye level
#define PLAYER_HOLD_LEVEL_EYES	-8

// when looking down, hold bottom of object 0 inches from feet
#define PLAYER_HOLD_DOWN_FEET	2

// when looking up, hold bottom of object 24 inches above eye level
#define PLAYER_HOLD_UP_EYES		24

// use a +/-30 degree range for the entire range of motion of pitch
#define PLAYER_LOOK_PITCH_RANGE	30

// player can reach down 2ft below his feet (otherwise he'll hold the object above the bottom)
#define PLAYER_REACH_DOWN_DISTANCE	24

static void ComputePlayerMatrix(CBasePlayer *pPlayer, matrix3x4_t &out)
{
	if (!pPlayer)
		return;

	QAngle angles = pPlayer->EyeAngles();
	Vector origin = pPlayer->EyePosition();

	// 0-360 / -180-180
	//angles.x = init ? 0 : AngleDistance( angles.x, 0 );
	//angles.x = clamp( angles.x, -PLAYER_LOOK_PITCH_RANGE, PLAYER_LOOK_PITCH_RANGE );
	angles.x = 0;

	float feet = pPlayer->GetAbsOrigin().z + pPlayer->WorldAlignMins().z;
	float eyes = origin.z;
	float zoffset = 0;
	// moving up (negative pitch is up)
	if (angles.x < 0)
	{
		zoffset = RemapVal(angles.x, 0, -PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_UP_EYES);
	}
	else
	{
		zoffset = RemapVal(angles.x, 0, PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_DOWN_FEET + (feet - eyes));
	}
	origin.z += zoffset;
	angles.x = 0;
	AngleMatrix(angles, origin, out);
}

static void MatrixOrthogonalize(matrix3x4_t &matrix, int column)
{
	Vector columns[3];
	int i;

	for (i = 0; i < 3; i++)
	{
		MatrixGetColumn(matrix, i, columns[i]);
	}

	int index0 = column;
	int index1 = (column + 1) % 3;
	int index2 = (column + 2) % 3;

	columns[index2] = CrossProduct(columns[index0], columns[index1]);
	columns[index1] = CrossProduct(columns[index2], columns[index0]);
	VectorNormalize(columns[index2]);
	VectorNormalize(columns[index1]);
	MatrixSetColumn(columns[index1], index1, matrix);
	MatrixSetColumn(columns[index2], index2, matrix);
}

#define SIGN(x) ( (x) < 0 ? -1 : 1 )

static QAngle AlignAngles(const QAngle &angles, float cosineAlignAngle)
{
	matrix3x4_t alignMatrix;
	AngleMatrix(angles, alignMatrix);

	// NOTE: Must align z first
	for (int j = 3; --j >= 0;)
	{
		Vector vec;
		MatrixGetColumn(alignMatrix, j, vec);
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vec[i]) > cosineAlignAngle)
			{
				vec[i] = SIGN(vec[i]);
				vec[(i + 1) % 3] = 0;
				vec[(i + 2) % 3] = 0;
				MatrixSetColumn(vec, j, alignMatrix);
				MatrixOrthogonalize(alignMatrix, j);
				break;
			}
		}
	}

	QAngle out;
	MatrixAngles(alignMatrix, out);
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Grab Controller
//-----------------------------------------------------------------------------

// derive from this so we can add save/load data to it
struct game_shadowcontrol_params_t : public hlshadowcontrol_params_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC(game_shadowcontrol_params_t)

DEFINE_FIELD(targetPosition, FIELD_POSITION_VECTOR),
DEFINE_FIELD(targetRotation, FIELD_VECTOR),
DEFINE_FIELD(maxAngular, FIELD_FLOAT),
DEFINE_FIELD(maxDampAngular, FIELD_FLOAT),
DEFINE_FIELD(maxSpeed, FIELD_FLOAT),
DEFINE_FIELD(maxDampSpeed, FIELD_FLOAT),
DEFINE_FIELD(dampFactor, FIELD_FLOAT),
DEFINE_FIELD(teleportDistance, FIELD_FLOAT),

END_DATADESC()

//-----------------------------------------------------------------------------
class CGrabController : public IMotionEvent
{
public:

	CGrabController(void);
	~CGrabController(void);
	void AttachEntity(CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon, const Vector &vGrabPosition, bool bUseGrabPosition);
	void DetachEntity(bool bClearVelocity);
	void OnRestore();

	bool UpdateObject(CBasePlayer *pPlayer, float flError);

	void SetTargetPosition(const Vector &target, const QAngle &targetOrientation);
	float ComputeError();
	float GetLoadWeight(void) const { return m_flLoadWeight; }
	void SetAngleAlignment(float alignAngleCosine) { m_angleAlignment = alignAngleCosine; }
	void SetIgnorePitch(bool bIgnore) { m_bIgnoreRelativePitch = bIgnore; }
	QAngle TransformAnglesToPlayerSpace(const QAngle &anglesIn, CBasePlayer *pPlayer);
	QAngle TransformAnglesFromPlayerSpace(const QAngle &anglesIn, CBasePlayer *pPlayer);

	CBaseEntity *GetAttached() { return (CBaseEntity *)m_attachedEntity; }

	IMotionEvent::simresult_e Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular);
	float GetSavedMass(IPhysicsObject *pObject);

	QAngle			m_attachedAnglesPlayerSpace;
	Vector			m_attachedPositionObjectSpace;

private:
	// Compute the max speed for an attached object
	void ComputeMaxSpeed(CBaseEntity *pEntity, IPhysicsObject *pPhysics);

	game_shadowcontrol_params_t	m_shadow;
	float			m_timeToArrive;
	float			m_errorTime;
	float			m_error;
	float			m_contactAmount;
	float			m_angleAlignment;
	bool			m_bCarriedEntityBlocksLOS;
	bool			m_bIgnoreRelativePitch;

	float			m_flLoadWeight;
	float			m_savedRotDamping[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	float			m_savedMass[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	EHANDLE			m_attachedEntity;
	QAngle			m_vecPreferredCarryAngles;
	bool			m_bHasPreferredCarryAngles;


	IPhysicsMotionController *m_controller;
	int				m_frameCount;
	friend class CWeaponPhysCannon;
};

const float DEFAULT_MAX_ANGULAR = 360.0f * 10.0f;
const float REDUCED_CARRY_MASS = 1.0f;

CGrabController::CGrabController(void)
{
	m_shadow.dampFactor = 1.0;
	m_shadow.teleportDistance = 0;
	m_errorTime = 0;
	m_error = 0;
	// make this controller really stiff!
	m_shadow.maxSpeed = 1000;
	m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;
	m_shadow.maxDampSpeed = m_shadow.maxSpeed * 2;
	m_shadow.maxDampAngular = m_shadow.maxAngular;
	m_attachedEntity = NULL;
	m_vecPreferredCarryAngles = vec3_angle;
	m_bHasPreferredCarryAngles = false;
}

CGrabController::~CGrabController(void)
{
	DetachEntity(false);
}

void CGrabController::OnRestore()
{
	if (m_controller)
	{
		m_controller->SetEventHandler(this);
	}
}

void CGrabController::SetTargetPosition(const Vector &target, const QAngle &targetOrientation)
{
	m_shadow.targetPosition = target;
	m_shadow.targetRotation = targetOrientation;

	m_timeToArrive = gpGlobals->frametime;

	CBaseEntity *pAttached = GetAttached();
	if (pAttached)
	{
		IPhysicsObject *pObj = pAttached->VPhysicsGetObject();

		if (pObj != NULL)
		{
			pObj->Wake();
		}
		else
		{
			DetachEntity(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CGrabController::ComputeError()
{
	if (m_errorTime <= 0)
		return 0;

	CBaseEntity *pAttached = GetAttached();
	if (pAttached)
	{
		Vector pos;
		IPhysicsObject *pObj = pAttached->VPhysicsGetObject();

		if (pObj)
		{
			pObj->GetShadowPosition(&pos, NULL);

			float error = (m_shadow.targetPosition - pos).Length();
			if (m_errorTime > 0)
			{
				if (m_errorTime > 1)
				{
					m_errorTime = 1;
				}
				float speed = error / m_errorTime;
				if (speed > m_shadow.maxSpeed)
				{
					error *= 0.5;
				}
				m_error = (1 - m_errorTime) * m_error + error * m_errorTime;
			}
		}
		else
		{
			DevMsg("Object attached to Physcannon has no physics object\n");
			DetachEntity(false);
			return 9999; // force detach
		}
	}

	if (pAttached->IsEFlagSet(EFL_IS_BEING_LIFTED_BY_BARNACLE))
	{
		m_error *= 3.0f;
	}

	m_errorTime = 0;

	return m_error;
}

#define MASS_SPEED_SCALE	60
#define MAX_MASS			40

void CGrabController::ComputeMaxSpeed(CBaseEntity *pEntity, IPhysicsObject *pPhysics)
{
#ifndef CLIENT_DLL
	m_shadow.maxSpeed = 1000;
	m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;

	// Compute total mass...
	float flMass = PhysGetEntityMass(pEntity);
	float flMaxMass = player_pickup_maxmass.GetFloat();
	if (flMass <= flMaxMass)
		return;

	float flLerpFactor = clamp(flMass, flMaxMass, 500.0f);
	flLerpFactor = SimpleSplineRemapVal(flLerpFactor, flMaxMass, 500.0f, 0.0f, 1.0f);

	float invMass = pPhysics->GetInvMass();
	float invInertia = pPhysics->GetInvInertia().Length();

	float invMaxMass = 1.0f / MAX_MASS;
	float ratio = invMaxMass / invMass;
	invMass = invMaxMass;
	invInertia *= ratio;

	float maxSpeed = invMass * MASS_SPEED_SCALE * 200;
	float maxAngular = invInertia * MASS_SPEED_SCALE * 360;

	m_shadow.maxSpeed = Lerp(flLerpFactor, m_shadow.maxSpeed, maxSpeed);
	m_shadow.maxAngular = Lerp(flLerpFactor, m_shadow.maxAngular, maxAngular);
#endif
}

QAngle CGrabController::TransformAnglesToPlayerSpace(const QAngle &anglesIn, CBasePlayer *pPlayer)
{
	if (m_bIgnoreRelativePitch)
	{
		matrix3x4_t test;
		QAngle angleTest = pPlayer->EyeAngles();
		angleTest.x = 0;
		AngleMatrix(angleTest, test);
		return TransformAnglesToLocalSpace(anglesIn, test);
	}
	return TransformAnglesToLocalSpace(anglesIn, pPlayer->EntityToWorldTransform());
}

QAngle CGrabController::TransformAnglesFromPlayerSpace(const QAngle &anglesIn, CBasePlayer *pPlayer)
{
	if (m_bIgnoreRelativePitch)
	{
		matrix3x4_t test;
		QAngle angleTest = pPlayer->EyeAngles();
		angleTest.x = 0;
		AngleMatrix(angleTest, test);
		return TransformAnglesToWorldSpace(anglesIn, test);
	}
	return TransformAnglesToWorldSpace(anglesIn, pPlayer->EntityToWorldTransform());
}

void CGrabController::AttachEntity(CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon, const Vector &vGrabPosition, bool bUseGrabPosition)
{
	// play the impact sound of the object hitting the player
	// used as feedback to let the player know he picked up the object
#ifndef CLIENT_DLL
	PhysicsImpactSound(pPlayer, pPhys, CHAN_STATIC, pPhys->GetMaterialIndex(), pPlayer->VPhysicsGetObject() ? pPlayer->VPhysicsGetObject()->GetMaterialIndex() : pPhys->GetMaterialIndex(), 1.0, 64);
#endif
	Vector position;
	QAngle angles;
	pPhys->GetPosition(&position, &angles);
	// If it has a preferred orientation, use that instead.
#ifndef CLIENT_DLL
	Pickup_GetPreferredCarryAngles(pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles);
#endif

	//	ComputeMaxSpeed( pEntity, pPhys );

	// Carried entities can never block LOS
	m_bCarriedEntityBlocksLOS = pEntity->BlocksLOS();
	pEntity->SetBlocksLOS(false);
	m_controller = physenv->CreateMotionController(this);
	m_controller->AttachObject(pPhys, true);
	// Don't do this, it's causing trouble with constraint solvers.
	//m_controller->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );

	pPhys->Wake();
	PhysSetGameFlags(pPhys, FVPHYSICS_PLAYER_HELD);
	SetTargetPosition(position, angles);
	m_attachedEntity = pEntity;
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pEntity->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
	m_flLoadWeight = 0;
	float damping = 10;
	float flFactor = count / 7.5f;
	if (flFactor < 1.0f)
	{
		flFactor = 1.0f;
	}
	for (int i = 0; i < count; i++)
	{
		float mass = pList[i]->GetMass();
		pList[i]->GetDamping(NULL, &m_savedRotDamping[i]);
		m_flLoadWeight += mass;
		m_savedMass[i] = mass;

		// reduce the mass to prevent the player from adding crazy amounts of energy to the system
		pList[i]->SetMass(REDUCED_CARRY_MASS / flFactor);
		pList[i]->SetDamping(NULL, &damping);
	}

	// Give extra mass to the phys object we're actually picking up
	pPhys->SetMass(REDUCED_CARRY_MASS);
	pPhys->EnableDrag(false);

	m_errorTime = -1.0f; // 1 seconds until error starts accumulating
	m_error = 0;
	m_contactAmount = 0;

	m_attachedAnglesPlayerSpace = TransformAnglesToPlayerSpace(angles, pPlayer);
	if (m_angleAlignment != 0)
	{
		m_attachedAnglesPlayerSpace = AlignAngles(m_attachedAnglesPlayerSpace, m_angleAlignment);
	}

	VectorITransform(pEntity->WorldSpaceCenter(), pEntity->EntityToWorldTransform(), m_attachedPositionObjectSpace);

#ifndef CLIENT_DLL
	// If it's a prop, see if it has desired carry angles
	CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pEntity);
	if (pProp)
	{
		m_bHasPreferredCarryAngles = pProp->GetPropDataAngles("preferred_carryangles", m_vecPreferredCarryAngles);
	}
	else
	{
		m_bHasPreferredCarryAngles = false;
	}
#else

	m_bHasPreferredCarryAngles = false;
#endif
}

static void ClampPhysicsVelocity(IPhysicsObject *pPhys, float linearLimit, float angularLimit)
{
	Vector vel;
	AngularImpulse angVel;
	pPhys->GetVelocity(&vel, &angVel);
	float speed = VectorNormalize(vel) - linearLimit;
	float angSpeed = VectorNormalize(angVel) - angularLimit;
	speed = speed < 0 ? 0 : -speed;
	angSpeed = angSpeed < 0 ? 0 : -angSpeed;
	vel *= speed;
	angVel *= angSpeed;
	pPhys->AddVelocity(&vel, &angVel);
}

void CGrabController::DetachEntity(bool bClearVelocity)
{
	CBaseEntity *pEntity = GetAttached();
	if (pEntity)
	{
		// Restore the LS blocking state
		pEntity->SetBlocksLOS(m_bCarriedEntityBlocksLOS);
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = pEntity->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));

		for (int i = 0; i < count; i++)
		{
			IPhysicsObject *pPhys = pList[i];
			if (!pPhys)
				continue;

			// on the odd chance that it's gone to sleep while under anti-gravity
			pPhys->EnableDrag(true);
			pPhys->Wake();
			pPhys->SetMass(m_savedMass[i]);
			pPhys->SetDamping(NULL, &m_savedRotDamping[i]);
			PhysClearGameFlags(pPhys, FVPHYSICS_PLAYER_HELD);
			if (bClearVelocity)
			{
				PhysForceClearVelocity(pPhys);
			}
			else
			{
#ifndef CLIENT_DLL
				ClampPhysicsVelocity(pPhys, 150 * 1.5f, 2.0f * 360.0f);
#endif
			}

		}
	}

	m_attachedEntity = NULL;
	if (physenv)
	{
		physenv->DestroyMotionController(m_controller);
	}
	m_controller = NULL;
}

static bool InContactWithHeavyObject(IPhysicsObject *pObject, float heavyMass)
{
	bool contact = false;
	IPhysicsFrictionSnapshot *pSnapshot = pObject->CreateFrictionSnapshot();
	while (pSnapshot->IsValid())
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		if (!pOther->IsMoveable() || pOther->GetMass() > heavyMass)
		{
			contact = true;
			break;
		}
		pSnapshot->NextFrictionData();
	}
	pObject->DestroyFrictionSnapshot(pSnapshot);
	return contact;
}

IMotionEvent::simresult_e CGrabController::Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular)
{
	game_shadowcontrol_params_t shadowParams = m_shadow;
	if (InContactWithHeavyObject(pObject, GetLoadWeight()))
	{
		m_contactAmount = Approach(0.1f, m_contactAmount, deltaTime*2.0f);
	}
	else
	{
		m_contactAmount = Approach(1.0f, m_contactAmount, deltaTime*2.0f);
	}
	shadowParams.maxAngular = m_shadow.maxAngular * m_contactAmount * m_contactAmount * m_contactAmount;
#ifndef CLIENT_DLL
	m_timeToArrive = pObject->ComputeShadowControl(shadowParams, m_timeToArrive, deltaTime);
#else
	m_timeToArrive = pObject->ComputeShadowControl(shadowParams, (TICK_INTERVAL * 2), deltaTime);
#endif

	// Slide along the current contact points to fix bouncing problems
	Vector velocity;
	AngularImpulse angVel;
	pObject->GetVelocity(&velocity, &angVel);
	PhysComputeSlideDirection(pObject, velocity, angVel, &velocity, &angVel, GetLoadWeight());
	pObject->SetVelocityInstantaneous(&velocity, NULL);

	linear.Init();
	angular.Init();
	m_errorTime += deltaTime;

	return SIM_LOCAL_ACCELERATION;
}

float CGrabController::GetSavedMass(IPhysicsObject *pObject)
{
	CBaseEntity *pHeld = m_attachedEntity;
	if (pHeld)
	{
		if (pObject->GetGameData() == (void*)pHeld)
		{
			IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
			int count = pHeld->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
			for (int i = 0; i < count; i++)
			{
				if (pList[i] == pObject)
					return m_savedMass[i];
			}
		}
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CGrabController::UpdateObject(CBasePlayer *pPlayer, float flError)
{
	CBaseEntity *pEntity = GetAttached();
	if (!pEntity)
		return false;
	if (ComputeError() > flError)
		return false;
	if (pPlayer->GetGroundEntity() == pEntity)
		return false;
	if (!pEntity->VPhysicsGetObject())
		return false;

	//Adrian: Oops, our object became motion disabled, let go!
	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if (pPhys && pPhys->IsMoveable() == false)
	{
		return false;
	}

	if (m_frameCount == gpGlobals->framecount)
	{
		return true;
	}
	m_frameCount = gpGlobals->framecount;
	Vector forward, right, up;
	QAngle playerAngles = pPlayer->EyeAngles();

	float pitch = AngleDistance(playerAngles.x, 0);
	playerAngles.x = clamp(pitch, -75, 75);
	AngleVectors(playerAngles, &forward, &right, &up);

	// Now clamp a sphere of object radius at end to the player's bbox
	Vector radial = physcollision->CollideGetExtent(pPhys->GetCollide(), vec3_origin, pEntity->GetAbsAngles(), -forward);
	Vector player2d = pPlayer->CollisionProp()->OBBMaxs();
	float playerRadius = player2d.Length2D();
	float flDot = DotProduct(forward, radial);

	float radius = playerRadius + fabs(flDot);

	float distance = 24 + (radius * 2.0f);

	Vector start = pPlayer->Weapon_ShootPosition();
	Vector end = start + (forward * distance);

	trace_t	tr;
	CTraceFilterSkipTwoEntities traceFilter(pPlayer, pEntity, COLLISION_GROUP_NONE);
	Ray_t ray;
	ray.Init(start, end);
	enginetrace->TraceRay(ray, MASK_SOLID_BRUSHONLY, &traceFilter, &tr);

	if (tr.fraction < 0.5)
	{
		end = start + forward * (radius*0.5f);
	}
	else if (tr.fraction <= 1.0f)
	{
		end = start + forward * (distance - radius);
	}

	Vector playerMins, playerMaxs, nearest;
	pPlayer->CollisionProp()->WorldSpaceAABB(&playerMins, &playerMaxs);
	Vector playerLine = pPlayer->CollisionProp()->WorldSpaceCenter();
	CalcClosestPointOnLine(end, playerLine + Vector(0, 0, playerMins.z), playerLine + Vector(0, 0, playerMaxs.z), nearest, NULL);

	Vector delta = end - nearest;
	float len = VectorNormalize(delta);
	if (len < radius)
	{
		end = nearest + radius * delta;
	}

	QAngle angles = TransformAnglesFromPlayerSpace(m_attachedAnglesPlayerSpace, pPlayer);

#ifndef CLIENT_DLL
	// If it has a preferred orientation, update to ensure we're still oriented correctly.
	Pickup_GetPreferredCarryAngles(pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles);

	// We may be holding a prop that has preferred carry angles
	if (m_bHasPreferredCarryAngles)
	{
		matrix3x4_t tmp;
		ComputePlayerMatrix(pPlayer, tmp);
		angles = TransformAnglesToWorldSpace(m_vecPreferredCarryAngles, tmp);
	}
#endif

	matrix3x4_t attachedToWorld;
	Vector offset;
	AngleMatrix(angles, attachedToWorld);
	VectorRotate(m_attachedPositionObjectSpace, attachedToWorld, offset);

	SetTargetPosition(end - offset, angles);

	return true;
}

//-----------------------------------------------------------------------------
// Player pickup controller
//-----------------------------------------------------------------------------

class CPlayerPickupController : public CBaseEntity
{
	DECLARE_CLASS(CPlayerPickupController, CBaseEntity);
public:
	void Init(CBasePlayer *pPlayer, CBaseEntity *pObject);
	void Shutdown(bool bThrown = false);
	bool OnControls(CBaseEntity *pControls) { return true; }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void OnRestore()
	{
		m_grabController.OnRestore();
	}
	void VPhysicsUpdate(IPhysicsObject *pPhysics){}
	void VPhysicsShadowUpdate(IPhysicsObject *pPhysics) {}

	bool IsHoldingEntity(CBaseEntity *pEnt);
	CGrabController &GetGrabController() { return m_grabController; }

private:
	CGrabController		m_grabController;
	CBasePlayer			*m_pPlayer;
};

LINK_ENTITY_TO_CLASS(player_pickup, CPlayerPickupController);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pObject - 
//-----------------------------------------------------------------------------
void CPlayerPickupController::Init(CBasePlayer *pPlayer, CBaseEntity *pObject)
{
#ifndef CLIENT_DLL
	// Holster player's weapon
	if (pPlayer->GetActiveWeapon())
	{
		if (!pPlayer->GetActiveWeapon()->Holster())
		{
			Shutdown();
			return;
		}
	}

	// If the target is debris, convert it to non-debris
	if (pObject->GetCollisionGroup() == COLLISION_GROUP_DEBRIS)
	{
		// Interactive debris converts back to debris when it comes to rest
		pObject->SetCollisionGroup(COLLISION_GROUP_INTERACTIVE_DEBRIS);
	}

	// done so I'll go across level transitions with the player
	SetParent(pPlayer);
	m_grabController.SetIgnorePitch(true);
	m_grabController.SetAngleAlignment(DOT_30DEGREE);
	m_pPlayer = pPlayer;
	IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();
	Pickup_OnPhysGunPickup(pObject, m_pPlayer);

	m_grabController.AttachEntity(pPlayer, pObject, pPhysics, false, vec3_origin, false);

	m_pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
	m_pPlayer->SetUseEntity(this);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void CPlayerPickupController::Shutdown(bool bThrown)
{
#ifndef CLIENT_DLL
	CBaseEntity *pObject = m_grabController.GetAttached();

	bool bClearVelocity = false;
	if (!bThrown && pObject && pObject->VPhysicsGetObject() && pObject->VPhysicsGetObject()->GetContactPoint(NULL, NULL))
	{
		bClearVelocity = true;
	}

	m_grabController.DetachEntity(bClearVelocity);

	if (pObject != NULL)
	{
		Pickup_OnPhysGunDrop(pObject, m_pPlayer, bThrown ? THROWN_BY_PLAYER : DROPPED_BY_PLAYER);
	}

	if (m_pPlayer)
	{
		m_pPlayer->SetUseEntity(NULL);
		if (m_pPlayer->GetActiveWeapon())
		{
			if (!m_pPlayer->GetActiveWeapon()->Deploy())
			{
				// We tried to restore the player's weapon, but we couldn't.
				// This usually happens when they're holding an empty weapon that doesn't
				// autoswitch away when out of ammo. Switch to next best weapon.
				m_pPlayer->SwitchToNextBestWeapon(NULL);
			}
		}

		m_pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	}
	Remove();

#endif

}


void CPlayerPickupController::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ToBasePlayer(pActivator) == m_pPlayer)
	{
		CBaseEntity *pAttached = m_grabController.GetAttached();

		// UNDONE: Use vphysics stress to decide to drop objects
		// UNDONE: Must fix case of forcing objects into the ground you're standing on (causes stress) before that will work
		if (!pAttached || useType == USE_OFF || (m_pPlayer->m_nButtons & IN_ATTACK2) || m_grabController.ComputeError() > 12)
		{
			Shutdown();
			return;
		}

		//Adrian: Oops, our object became motion disabled, let go!
		IPhysicsObject *pPhys = pAttached->VPhysicsGetObject();
		if (pPhys && pPhys->IsMoveable() == false)
		{
			Shutdown();
			return;
		}

#if STRESS_TEST
		vphysics_objectstress_t stress;
		CalculateObjectStress(pPhys, pAttached, &stress);
		if (stress.exertedStress > 250)
		{
			Shutdown();
			return;
		}
#endif
		// +ATTACK will throw phys objects
		if (m_pPlayer->m_nButtons & IN_ATTACK)
		{
			Shutdown(true);
			Vector vecLaunch;
			m_pPlayer->EyeVectors(&vecLaunch);
			// JAY: Scale this with mass because some small objects really go flying
			if (!pPhys)
				return;

			float flNewMass = pPhys->GetMass();
			if (flNewMass < 0.5 || flNewMass > 15)
				flNewMass = 10;

			float massFactor = clamp(flNewMass, 0.5, 15);
			massFactor = RemapVal(massFactor, 0.5, 15, 0.5, 4);
			vecLaunch *= player_throwforce.GetFloat() * massFactor;

			pPhys->ApplyForceCenter(vecLaunch);
			AngularImpulse aVel = RandomAngularImpulse(-10, 10) * massFactor;
			pPhys->ApplyTorqueCenter(aVel);
			return;
		}

		if (useType == USE_SET)
		{
			// update position
			m_grabController.UpdateObject(m_pPlayer, 12);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPlayerPickupController::IsHoldingEntity(CBaseEntity *pEnt)
{
	return (m_grabController.GetAttached() == pEnt);
}

bool PlayerPickupControllerIsHoldingEntity(CBaseEntity *pPickupControllerEntity, CBaseEntity *pHeldEntity)
{
	CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);
	return pController ? pController->IsHoldingEntity(pHeldEntity) : false;
}

float PlayerPickupGetHeldObjectMass(CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject)
{
	float mass = 0.0f;
	CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);
	if (pController)
	{
		CGrabController &grab = pController->GetGrabController();
		mass = grab.GetSavedMass(pHeldObject);
	}
	return mass;
}

void PlayerPickupObject(CBasePlayer *pPlayer, CBaseEntity *pObject)
{
#ifndef CLIENT_DLL
	//Don't pick up if we don't have a phys object.
	if (pObject->VPhysicsGetObject() == NULL)
		return;

	CPlayerPickupController *pController = (CPlayerPickupController *)CBaseEntity::Create("player_pickup", pObject->GetAbsOrigin(), vec3_angle, pPlayer);
	if (!pController)
		return;

	pController->Init(pPlayer, pObject);
#endif
}

#define	RPG_SPEED	1500
#define	RPG_LASER_SPRITE	"sprites/redglow1.vmt"

#ifndef CLIENT_DLL
const char *g_pLaserDotThink = "LaserThinkContext";

static ConVar sk_apc_missile_damage("sk_apc_missile_damage", "15");
#define APC_MISSILE_DAMAGE	sk_apc_missile_damage.GetFloat()

#endif

#ifdef CLIENT_DLL
#define CLaserDot C_LaserDot
#endif

//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------
class CLaserDot : public CBaseEntity
{
	DECLARE_CLASS(CLaserDot, CBaseEntity);
public:

	CLaserDot(void);
	~CLaserDot(void);

	static CLaserDot *Create(const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true);

	void	SetTargetEntity(CBaseEntity *pTarget) { m_hTargetEnt = pTarget; }
	CBaseEntity *GetTargetEntity(void) { return m_hTargetEnt; }

	void	SetLaserPosition(const Vector &origin, const Vector &normal);
	Vector	GetChasePosition();
	void	TurnOn(void);
	void	TurnOff(void);
	bool	IsOn() const { return m_bIsOn; }

	void	Toggle(void);

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	void	MakeInvisible(void);

#ifdef CLIENT_DLL

	virtual bool			IsTransparent(void) { return true; }
	virtual RenderGroup_t	GetRenderGroup(void) { return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	virtual int				DrawModel(int flags);
	virtual void			OnDataChanged(DataUpdateType_t updateType);
	virtual bool			ShouldDraw(void) { return (IsEffectActive(EF_NODRAW) == false); }

	CMaterialReference	m_hSpriteMaterial;
#endif

protected:
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;
	bool				m_bVisibleLaserDot;
	bool				m_bIsOn;

	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
public:
	CLaserDot			*m_pNext;
};

IMPLEMENT_NETWORKCLASS_ALIASED(LaserDot, DT_LaserDot)

BEGIN_NETWORK_TABLE(CLaserDot, DT_LaserDot)
END_NETWORK_TABLE()

#ifndef CLIENT_DLL

// a list of laser dots to search quickly
CEntityClassList<CLaserDot> g_LaserDotList;
template <> CLaserDot *CEntityClassList<CLaserDot>::m_pClassList = NULL;
CLaserDot *GetLaserDotList()
{
	return g_LaserDotList.m_pClassList;
}

BEGIN_DATADESC(CMissile)

DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
DEFINE_FIELD(m_hRocketTrail, FIELD_EHANDLE),
DEFINE_FIELD(m_flAugerTime, FIELD_TIME),
DEFINE_FIELD(m_flMarkDeadTime, FIELD_TIME),
DEFINE_FIELD(m_flGracePeriodEndsAt, FIELD_TIME),
DEFINE_FIELD(m_flDamage, FIELD_FLOAT),
DEFINE_FIELD(m_flRadius, FIELD_FLOAT),

#ifdef BB2_AI
DEFINE_FIELD(m_bCreateDangerSounds, FIELD_BOOLEAN),
#endif //BB2_AI

// Function Pointers
DEFINE_FUNCTION(MissileTouch),
DEFINE_FUNCTION(AccelerateThink),
DEFINE_FUNCTION(AugerThink),
DEFINE_FUNCTION(IgniteThink),
DEFINE_FUNCTION(SeekThink),

END_DATADESC()

LINK_ENTITY_TO_CLASS(rpg_missile, CMissile);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CMissile::CMissile()
{
	m_hRocketTrail = NULL;

#ifdef BB2_AI
	m_bCreateDangerSounds = false; //
#endif //BB2_AI
}

CMissile::~CMissile()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CMissile::Precache(void)
{
	PrecacheModel("models/weapons/w_panzerschreck_rocket.mdl");
	PrecacheModel("models/weapons/w_missile.mdl");
	PrecacheModel("models/weapons/w_missile_launch.mdl");
	PrecacheModel("models/weapons/w_missile_closed.mdl");

	PrecacheScriptSound("Missile.Ignite");
	PrecacheScriptSound("Missile.Accelerate");

	// Laser dot...
	PrecacheModel("sprites/redglow1.vmt");
	PrecacheModel(RPG_LASER_SPRITE);

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("rpg_missile");
#endif
}

Class_T CMissile::Classify(void)
{
	if (m_bShouldBeFriendly)
		return CLASS_COMBINE;

	return CLASS_MISSILE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CMissile::Spawn(void)
{
	Precache();

	SetSolid(SOLID_BBOX);
	SetModel("models/weapons/w_panzerschreck_rocket.mdl");
	UTIL_SetSize(this, -Vector(4, 4, 4), Vector(4, 4, 4));

	SetTouch(&CMissile::MissileTouch);

	SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	SetThink(&CMissile::IgniteThink);

	SetNextThink(gpGlobals->curtime + 0.3f);

#ifdef BB2_AI
	SetDamage(EXPLOSION_DAMAGE);
	SetRadius(EXPLOSION_RADIUS);
#endif //BB2_AI

	m_takedamage = DAMAGE_YES;
	m_iHealth = m_iMaxHealth = 100;
	m_bloodColor = DONT_BLEED;
	m_flGracePeriodEndsAt = 0;

	AddFlag(FL_OBJECT);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::Event_Killed(const CTakeDamageInfo &info)
{
	m_takedamage = DAMAGE_NO;

	ShotDown();
}

unsigned int CMissile::PhysicsSolidMaskForEntity(void) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CMissile::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	if ((info.GetDamageType() & (DMG_MISSILEDEFENSE | DMG_AIRBOAT)) == false)
		return 0;

	bool bIsDamaged;
	if (m_iHealth <= AugerHealth())
	{
		// This missile is already damaged (i.e., already running AugerThink)
		bIsDamaged = true;
	}
	else
	{
		// This missile isn't damaged enough to wobble in flight yet
		bIsDamaged = false;
	}

	int nRetVal = BaseClass::OnTakeDamage_Alive(info);

	if (!bIsDamaged)
	{
		if (m_iHealth <= AugerHealth())
		{
			ShotDown();
		}
	}

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Stops any kind of tracking and shoots dumb
//-----------------------------------------------------------------------------
void CMissile::DumbFire(void)
{
	SetThink(NULL);
	SetMoveType(MOVETYPE_FLY);

	SetModel("models/weapons/w_panzerschreck_rocket.mdl");
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	EmitSound("Missile.Ignite");

	// Smoke trail.
	CreateSmokeTrail();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::SetGracePeriod(float flGracePeriod)
{
	m_flGracePeriodEndsAt = gpGlobals->curtime + flGracePeriod;

	// Go non-solid until the grace period ends
	AddSolidFlags(FSOLID_NOT_SOLID);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AccelerateThink(void)
{
	Vector vecForward;

	// !!!UNDONE - make this work exactly the same as HL1 RPG, lest we have looping sound bugs again!
	EmitSound("Missile.Accelerate");

	// SetEffects( EF_LIGHT );

	AngleVectors(GetLocalAngles(), &vecForward);
	SetAbsVelocity(vecForward * RPG_SPEED);

	SetThink(&CMissile::SeekThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

#define AUGER_YDEVIANCE 20.0f
#define AUGER_XDEVIANCEUP 8.0f
#define AUGER_XDEVIANCEDOWN 1.0f

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AugerThink(void)
{
	// If we've augered long enough, then just explode
	if (m_flAugerTime < gpGlobals->curtime)
	{
		Explode();
		return;
	}

	if (m_flMarkDeadTime < gpGlobals->curtime)
	{
		m_lifeState = LIFE_DYING;
	}

	QAngle angles = GetLocalAngles();

	angles.y += random->RandomFloat(-AUGER_YDEVIANCE, AUGER_YDEVIANCE);
	angles.x += random->RandomFloat(-AUGER_XDEVIANCEDOWN, AUGER_XDEVIANCEUP);

	SetLocalAngles(angles);

	Vector vecForward;

	AngleVectors(GetLocalAngles(), &vecForward);

	SetAbsVelocity(vecForward * 1000.0f);

	SetNextThink(gpGlobals->curtime + 0.05f);
}

//-----------------------------------------------------------------------------
// Purpose: Causes the missile to spiral to the ground and explode, due to damage
//-----------------------------------------------------------------------------
void CMissile::ShotDown(void)
{
	CEffectData	data;
	data.m_vOrigin = GetAbsOrigin();

	DispatchEffect("RPGShotDown", data);

	if (m_hRocketTrail != NULL)
	{
		m_hRocketTrail->m_bDamaged = true;
	}

	SetThink(&CMissile::AugerThink);
	SetNextThink(gpGlobals->curtime);
	m_flAugerTime = gpGlobals->curtime + 1.5f;
	m_flMarkDeadTime = gpGlobals->curtime + 0.75;

	if (m_hOwner != NULL)
	{
		m_hOwner = NULL;
	}
}

//-----------------------------------------------------------------------------
// The actual explosion 
//-----------------------------------------------------------------------------
void CMissile::DoExplosion(void)
{
	int iDamageType = (TryTheLuck(0.15) ? DMG_BURN : -1); // 15 % chance to spawn fire on gibs etc...
	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(), GetDamage(), GetRadius(), 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE,
		0.0f, this, iDamageType, NULL, m_bShouldBeFriendly ? CLASS_PLAYER : CLASS_NONE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::Explode(void)
{
	// Don't explode against the skybox. Just pretend that 
	// the missile flies off into the distance.
	Vector forward;

	GetVectors(&forward, NULL, NULL);

	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + forward * 16, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	m_takedamage = DAMAGE_NO;
	SetSolid(SOLID_NONE);
	if (tr.fraction == 1.0 || !(tr.surface.flags & SURF_SKY))
	{
		DoExplosion();
	}

	if (m_hRocketTrail)
	{
		m_hRocketTrail->SetLifetime(0.1f);
		m_hRocketTrail = NULL;
	}

	if (m_hOwner != NULL)
	{
		m_hOwner = NULL;
	}

	StopSound("Missile.Ignite");
	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CMissile::MissileTouch(CBaseEntity *pOther)
{
	Assert(pOther);

	// Don't touch triggers (but DO hit weapons)
	if (pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) && pOther->GetCollisionGroup() != COLLISION_GROUP_WEAPON)
#ifdef BB2_AI
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ((pOther->m_takedamage == DAMAGE_NO) || (pOther->m_takedamage == DAMAGE_EVENTS_ONLY))
			return;
	}
#else
		return;
#endif //BB2_AI

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::CreateSmokeTrail(void)
{
	if (m_hRocketTrail)
		return;

	// Smoke trail.
	if ((m_hRocketTrail = RocketTrail::CreateRocketTrail()) != NULL)
	{
		m_hRocketTrail->m_Opacity = 0.2f;
		m_hRocketTrail->m_SpawnRate = 100;
		m_hRocketTrail->m_ParticleLifetime = 0.5f;
		m_hRocketTrail->m_StartColor.Init(0.65f, 0.65f, 0.65f);
		m_hRocketTrail->m_EndColor.Init(0.0, 0.0, 0.0);
		m_hRocketTrail->m_StartSize = 8;
		m_hRocketTrail->m_EndSize = 32;
		m_hRocketTrail->m_SpawnRadius = 4;
		m_hRocketTrail->m_MinSpeed = 2;
		m_hRocketTrail->m_MaxSpeed = 16;

		m_hRocketTrail->SetLifetime(999);
		m_hRocketTrail->FollowEntity(this, "0");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::IgniteThink(void)
{
	SetMoveType(MOVETYPE_FLY);
	SetModel("models/weapons/w_panzerschreck_rocket.mdl");
	UTIL_SetSize(this, vec3_origin, vec3_origin);
	RemoveSolidFlags(FSOLID_NOT_SOLID);

	//TODO: Play opening sound

	Vector vecForward;

	EmitSound("Missile.Ignite");

	AngleVectors(GetLocalAngles(), &vecForward);
	SetAbsVelocity(vecForward * RPG_SPEED);

	SetThink(&CMissile::SeekThink);
	SetNextThink(gpGlobals->curtime);

	if (m_hOwner && m_hOwner->GetOwnerEntity())
	{
		CBasePlayer *pPlayer = ToBasePlayer(m_hOwner->GetOwnerEntity());
#ifdef BB2_AI
		if (pPlayer)
		{
			color32 white = { 255, 225, 205, 64 };
			UTIL_ScreenFade(pPlayer, white, 0.1f, 0.0f, FFADE_IN);
		}
#else
		color32 white = { 255, 225, 205, 64 };
		UTIL_ScreenFade(pPlayer, white, 0.1f, 0.0f, FFADE_IN);
#endif //BB2_AI
	}

	CreateSmokeTrail();
}

//-----------------------------------------------------------------------------
// Gets the shooting position 
//-----------------------------------------------------------------------------
void CMissile::GetShootPosition(CLaserDot *pLaserDot, Vector *pShootPosition)
{
	if (pLaserDot->GetOwnerEntity() != NULL)
	{
		//FIXME: Do we care this isn't exactly the muzzle position?
		*pShootPosition = pLaserDot->GetOwnerEntity()->WorldSpaceCenter();
	}
	else
	{
		*pShootPosition = pLaserDot->GetChasePosition();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define	RPG_HOMING_SPEED	0.125f

void CMissile::ComputeActualDotPosition(CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed)
{
	*pHomingSpeed = RPG_HOMING_SPEED;
	if (pLaserDot->GetTargetEntity())
	{
		*pActualDotPosition = pLaserDot->GetChasePosition();
		return;
	}

	Vector vLaserStart;
	GetShootPosition(pLaserDot, &vLaserStart);

	//Get the laser's vector
	Vector vLaserDir;
	VectorSubtract(pLaserDot->GetChasePosition(), vLaserStart, vLaserDir);

	//Find the length of the current laser
	float flLaserLength = VectorNormalize(vLaserDir);

	//Find the length from the missile to the laser's owner
	float flMissileLength = GetAbsOrigin().DistTo(vLaserStart);

	//Find the length from the missile to the laser's position
	Vector vecTargetToMissile;
	VectorSubtract(GetAbsOrigin(), pLaserDot->GetChasePosition(), vecTargetToMissile);
	float flTargetLength = VectorNormalize(vecTargetToMissile);

	// See if we should chase the line segment nearest us
	if ((flMissileLength < flLaserLength) || (flTargetLength <= 512.0f))
	{
		*pActualDotPosition = UTIL_PointOnLineNearestPoint(vLaserStart, pLaserDot->GetChasePosition(), GetAbsOrigin());
		*pActualDotPosition += (vLaserDir * 256.0f);
	}
	else
	{
		// Otherwise chase the dot
		*pActualDotPosition = pLaserDot->GetChasePosition();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::SeekThink(void)
{
	CBaseEntity	*pBestDot = NULL;
	float		flBestDist = MAX_TRACE_LENGTH;
	float		dotDist;

	// If we have a grace period, go solid when it ends
	if (m_flGracePeriodEndsAt)
	{
		if (m_flGracePeriodEndsAt < gpGlobals->curtime)
		{
			RemoveSolidFlags(FSOLID_NOT_SOLID);
			m_flGracePeriodEndsAt = 0;
		}
	}

	//Search for all dots relevant to us
	for (CLaserDot *pEnt = GetLaserDotList(); pEnt != NULL; pEnt = pEnt->m_pNext)
	{
		if (!pEnt->IsOn())
			continue;

		if (pEnt->GetOwnerEntity() != GetOwnerEntity())
			continue;

		dotDist = (GetAbsOrigin() - pEnt->GetAbsOrigin()).Length();

		//Find closest
		if (dotDist < flBestDist)
		{
			pBestDot = pEnt;
			flBestDist = dotDist;
		}
	}
#ifdef BB2_AI
	if (flBestDist <= (GetAbsVelocity().Length() * 2.5f) && FVisible(pBestDot->GetAbsOrigin()))
	{
		// Scare targets
		CSoundEnt::InsertSound(SOUND_DANGER, pBestDot->GetAbsOrigin(), GetRadius(), 0.2f, pBestDot, SOUNDENT_CHANNEL_REPEATED_DANGER, NULL); //
	}
#endif //BB2_AI

	//If we have a dot target
	if (pBestDot == NULL)
	{
		//Think as soon as possible
		SetNextThink(gpGlobals->curtime);
		return;
	}

	CLaserDot *pLaserDot = (CLaserDot *)pBestDot;
	Vector	targetPos;

	float flHomingSpeed;
	Vector vecLaserDotPosition;
	ComputeActualDotPosition(pLaserDot, &targetPos, &flHomingSpeed);

	if (IsSimulatingOnAlternateTicks())
		flHomingSpeed *= 2;

	Vector	vTargetDir;
	VectorSubtract(targetPos, GetAbsOrigin(), vTargetDir);
	float flDist = VectorNormalize(vTargetDir);

#ifdef BB2_AI
	if (pLaserDot->GetTargetEntity() != NULL && flDist <= 240.0f) //
	{
		// Prevent the missile circling the Strider like a Halo in ep1_c17_06. If the missile gets within 20
		// feet of a Strider, tighten up the turn speed of the missile so it can break the halo and strike. (sjb 4/27/2006)
		if (pLaserDot->GetTargetEntity()->ClassMatches("npc_strider")) //
		{
			flHomingSpeed *= 1.75f;
		}
	}
#endif //BB2_AI

	Vector	vDir = GetAbsVelocity();
	float	flSpeed = VectorNormalize(vDir);
	Vector	vNewVelocity = vDir;
	if (gpGlobals->frametime > 0.0f)
	{
		if (flSpeed != 0)
		{
			vNewVelocity = (flHomingSpeed * vTargetDir) + ((1 - flHomingSpeed) * vDir);

			// This computation may happen to cancel itself out exactly. If so, slam to targetdir.
			if (VectorNormalize(vNewVelocity) < 1e-3)
			{
				vNewVelocity = (flDist != 0) ? vTargetDir : vDir;
			}
		}
		else
		{
			vNewVelocity = vTargetDir;
		}
	}

	QAngle	finalAngles;
	VectorAngles(vNewVelocity, finalAngles);
	SetAbsAngles(finalAngles);

	vNewVelocity *= flSpeed;
	SetAbsVelocity(vNewVelocity);

	if (GetAbsVelocity() == vec3_origin)
	{
		// Strange circumstances have brought this missile to halt. Just blow it up.
		Explode();
		return;
	}

	// Think as soon as possible
	SetNextThink(gpGlobals->curtime);

#ifdef BB2_AI
	if (m_bCreateDangerSounds == true)
	{
		trace_t tr;
		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 0.5, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

		CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 100, 0.2, this, SOUNDENT_CHANNEL_REPEATED_DANGER);
	}
#endif //BB2_AI
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : &vecOrigin - 
//			&vecAngles - 
//			NULL - 
//
// Output : CMissile
//-----------------------------------------------------------------------------
CMissile *CMissile::Create(const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner = NULL, bool bFriendly)
{
	//CMissile *pMissile = (CMissile *)CreateEntityByName("rpg_missile" );
	CMissile *pMissile = (CMissile *)CBaseEntity::Create("rpg_missile", vecOrigin, vecAngles, CBaseEntity::Instance(pentOwner));
	pMissile->SetFriendly(bFriendly);
	pMissile->SetOwnerEntity(Instance(pentOwner));
	pMissile->Spawn();
	pMissile->AddEffects(EF_NOSHADOW);

	Vector vecForward;
	AngleVectors(vecAngles, &vecForward);

	pMissile->SetAbsVelocity(vecForward * 300 + Vector(0, 0, 128));

	return pMissile;
}

#ifdef BB2_AI
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CUtlVector<CMissile::CustomDetonator_t> CMissile::gm_CustomDetonators;

void CMissile::AddCustomDetonator(CBaseEntity *pEntity, float radius, float height)
{
	int i = gm_CustomDetonators.AddToTail();
	gm_CustomDetonators[i].hEntity = pEntity;
	gm_CustomDetonators[i].radiusSq = Square(radius);
	gm_CustomDetonators[i].halfHeight = height * 0.5f;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMissile::RemoveCustomDetonator(CBaseEntity *pEntity)
{
	for (int i = 0; i < gm_CustomDetonators.Count(); i++)
	{
		if (gm_CustomDetonators[i].hEntity == pEntity)
		{
			gm_CustomDetonators.FastRemove(i);
			break;
		}
	}
}
#endif //BB2_AI

//-----------------------------------------------------------------------------
// This entity is used to create little force boxes that the helicopter
// should avoid. 
//-----------------------------------------------------------------------------
class CInfoAPCMissileHint : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CInfoAPCMissileHint, CBaseEntity);

	virtual void Spawn();
	virtual void Activate();
	virtual void UpdateOnRemove();

	static CBaseEntity *FindAimTarget(CBaseEntity *pMissile, const char *pTargetName,
		const Vector &vecCurrentTargetPos, const Vector &vecCurrentTargetVel);

private:
	EHANDLE	m_hTarget;

	typedef CHandle<CInfoAPCMissileHint> APCMissileHintHandle_t;
	static CUtlVector< APCMissileHintHandle_t > s_APCMissileHints;
};

//-----------------------------------------------------------------------------
//
// This entity is used to create little force boxes that the helicopters should avoid. 
//
//-----------------------------------------------------------------------------
CUtlVector< CInfoAPCMissileHint::APCMissileHintHandle_t > CInfoAPCMissileHint::s_APCMissileHints;

LINK_ENTITY_TO_CLASS(info_apc_missile_hint, CInfoAPCMissileHint);

BEGIN_DATADESC(CInfoAPCMissileHint)
DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),
END_DATADESC()


//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CInfoAPCMissileHint::Spawn()
{
	SetModel(STRING(GetModelName()));
	SetSolid(SOLID_BSP);
	AddSolidFlags(FSOLID_NOT_SOLID);
	AddEffects(EF_NODRAW);
}

void CInfoAPCMissileHint::Activate()
{
	BaseClass::Activate();

	m_hTarget = gEntList.FindEntityByName(NULL, m_target);
	if (m_hTarget == NULL)
	{
		DevWarning("%s: Could not find target '%s'!\n", GetClassname(), STRING(m_target));
	}
	else
	{
		s_APCMissileHints.AddToTail(this);
	}
}

void CInfoAPCMissileHint::UpdateOnRemove()
{
	s_APCMissileHints.FindAndRemove(this);
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
#define HINT_PREDICTION_TIME 3.0f

CBaseEntity *CInfoAPCMissileHint::FindAimTarget(CBaseEntity *pMissile, const char *pTargetName,
	const Vector &vecCurrentEnemyPos, const Vector &vecCurrentEnemyVel)
{
	if (!pTargetName)
		return NULL;

	float flOOSpeed = pMissile->GetAbsVelocity().Length();
	if (flOOSpeed != 0.0f)
	{
		flOOSpeed = 1.0f / flOOSpeed;
	}

	for (int i = s_APCMissileHints.Count(); --i >= 0;)
	{
		CInfoAPCMissileHint *pHint = s_APCMissileHints[i];
		if (!pHint->NameMatches(pTargetName))
			continue;

		if (!pHint->m_hTarget)
			continue;

		Vector vecMissileToHint, vecMissileToEnemy;
		VectorSubtract(pHint->m_hTarget->WorldSpaceCenter(), pMissile->GetAbsOrigin(), vecMissileToHint);
		VectorSubtract(vecCurrentEnemyPos, pMissile->GetAbsOrigin(), vecMissileToEnemy);
		float flDistMissileToHint = VectorNormalize(vecMissileToHint);
		VectorNormalize(vecMissileToEnemy);
		if (DotProduct(vecMissileToHint, vecMissileToEnemy) < 0.866f)
			continue;

		// Determine when the target will be inside the volume.
		// Project at most 3 seconds in advance
		Vector vecRayDelta;
		VectorMultiply(vecCurrentEnemyVel, HINT_PREDICTION_TIME, vecRayDelta);

		BoxTraceInfo_t trace;
		if (!IntersectRayWithOBB(vecCurrentEnemyPos, vecRayDelta, pHint->CollisionProp()->CollisionToWorldTransform(),
			pHint->CollisionProp()->OBBMins(), pHint->CollisionProp()->OBBMaxs(), 0.0f, &trace))
		{
			continue;
		}

		// Determine the amount of time it would take the missile to reach the target
		// If we can reach the target within the time it takes for the enemy to reach the 
		float tSqr = flDistMissileToHint * flOOSpeed / HINT_PREDICTION_TIME;
		if ((tSqr < (trace.t1 * trace.t1)) || (tSqr >(trace.t2 * trace.t2)))
			continue;

		return pHint->m_hTarget;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// a list of missiles to search quickly
//-----------------------------------------------------------------------------
CEntityClassList<CAPCMissile> g_APCMissileList;
template <> CAPCMissile *CEntityClassList<CAPCMissile>::m_pClassList = NULL;
CAPCMissile *GetAPCMissileList()
{
	return g_APCMissileList.m_pClassList;
}

//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCMissile *FindAPCMissileInCone(const Vector &vecOrigin, const Vector &vecDirection, float flAngle)
{
	float flCosAngle = cos(DEG2RAD(flAngle));
	for (CAPCMissile *pEnt = GetAPCMissileList(); pEnt != NULL; pEnt = pEnt->m_pNext)
	{
		if (!pEnt->IsSolid())
			continue;

		Vector vecDelta;
		VectorSubtract(pEnt->GetAbsOrigin(), vecOrigin, vecDelta);
		VectorNormalize(vecDelta);
		float flDot = DotProduct(vecDelta, vecDirection);
		if (flDot > flCosAngle)
			return pEnt;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//
// Specialized version of the missile
//
//-----------------------------------------------------------------------------
#define MAX_HOMING_DISTANCE 2250.0f
#define MIN_HOMING_DISTANCE 1250.0f
#define MAX_NEAR_HOMING_DISTANCE 1750.0f
#define MIN_NEAR_HOMING_DISTANCE 1000.0f
#define DOWNWARD_BLEND_TIME_START 0.2f
#define MIN_HEIGHT_DIFFERENCE	250.0f
#define MAX_HEIGHT_DIFFERENCE	550.0f
#define CORRECTION_TIME		0.2f
#define	APC_LAUNCH_HOMING_SPEED	0.1f
#define	APC_HOMING_SPEED	0.025f
#define HOMING_SPEED_ACCEL	0.01f

BEGIN_DATADESC(CAPCMissile)

DEFINE_FIELD(m_flReachedTargetTime, FIELD_TIME),
DEFINE_FIELD(m_flIgnitionTime, FIELD_TIME),
DEFINE_FIELD(m_bGuidingDisabled, FIELD_BOOLEAN),
DEFINE_FIELD(m_hSpecificTarget, FIELD_EHANDLE),
DEFINE_FIELD(m_strHint, FIELD_STRING),
DEFINE_FIELD(m_flLastHomingSpeed, FIELD_FLOAT),

DEFINE_THINKFUNC(BeginSeekThink),
DEFINE_THINKFUNC(AugerStartThink),
DEFINE_THINKFUNC(ExplodeThink),
#ifdef BB2_AI
DEFINE_THINKFUNC(APCSeekThink),
#endif //BB2_AI

DEFINE_FUNCTION(APCMissileTouch),

END_DATADESC()

LINK_ENTITY_TO_CLASS(apc_missile, CAPCMissile);

CAPCMissile *CAPCMissile::Create(const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner)
{
	CAPCMissile *pMissile = (CAPCMissile *)CBaseEntity::Create("apc_missile", vecOrigin, vecAngles, pOwner);
	pMissile->SetOwnerEntity(pOwner);
	pMissile->Spawn();
	pMissile->SetAbsVelocity(vecVelocity);
	pMissile->AddFlag(FL_NOTARGET);
	pMissile->AddEffects(EF_NOSHADOW);
	return pMissile;
}

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CAPCMissile::CAPCMissile()
{
	g_APCMissileList.Insert(this);
}

CAPCMissile::~CAPCMissile()
{
	g_APCMissileList.Remove(this);
}

//-----------------------------------------------------------------------------
// Shared initialization code
//-----------------------------------------------------------------------------
void CAPCMissile::Init()
{
	SetMoveType(MOVETYPE_FLY);
	SetModel("models/weapons/w_missile.mdl");
	UTIL_SetSize(this, vec3_origin, vec3_origin);
	CreateSmokeTrail();
	SetTouch(&CAPCMissile::APCMissileTouch);
	m_flLastHomingSpeed = APC_HOMING_SPEED;

#ifdef BB2_AI
	CreateDangerSounds(true);
#endif //BB2_AI
}

//-----------------------------------------------------------------------------
// For hitting a specific target
//-----------------------------------------------------------------------------
void CAPCMissile::AimAtSpecificTarget(CBaseEntity *pTarget)
{
	m_hSpecificTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CAPCMissile::APCMissileTouch(CBaseEntity *pOther)
{
	Assert(pOther);
	if (!pOther->IsSolid() && !pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
		return;

	Explode();
}

//-----------------------------------------------------------------------------
// Specialized version of the missile
//-----------------------------------------------------------------------------
void CAPCMissile::IgniteDelay(void)
{
	m_flIgnitionTime = gpGlobals->curtime + 0.3f;

	SetThink(&CAPCMissile::BeginSeekThink);
	SetNextThink(m_flIgnitionTime);
	Init();
	AddSolidFlags(FSOLID_NOT_SOLID);
}

void CAPCMissile::AugerDelay(float flDelay)
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink(&CAPCMissile::AugerStartThink);
	SetNextThink(gpGlobals->curtime + flDelay);
	Init();
	DisableGuiding();
}

void CAPCMissile::AugerStartThink()
{
	if (m_hRocketTrail != NULL)
	{
		m_hRocketTrail->m_bDamaged = true;
	}
	m_flAugerTime = gpGlobals->curtime + random->RandomFloat(1.0f, 2.0f);
	SetThink(&CAPCMissile::AugerThink);
	SetNextThink(gpGlobals->curtime);
}

void CAPCMissile::ExplodeDelay(float flDelay)
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink(&CAPCMissile::ExplodeThink);
	SetNextThink(gpGlobals->curtime + flDelay);
	Init();
	DisableGuiding();
}

void CAPCMissile::BeginSeekThink(void)
{
	RemoveSolidFlags(FSOLID_NOT_SOLID);

#ifdef BB2_AI
	SetThink(&CAPCMissile::APCSeekThink);
#else
	SetThink(&CAPCMissile::SeekThink);
#endif //BB2_AI	

	SetNextThink(gpGlobals->curtime);
}

#ifdef BB2_AI
void CAPCMissile::APCSeekThink(void)
{
	BaseClass::SeekThink();

	bool bFoundDot = false;

	//If we can't find a dot to follow around then just send me wherever I'm facing so I can blow up in peace.
	for (CLaserDot *pEnt = GetLaserDotList(); pEnt != NULL; pEnt = pEnt->m_pNext)
	{
		if (!pEnt->IsOn())
			continue;

		if (pEnt->GetOwnerEntity() != GetOwnerEntity())
			continue;

		bFoundDot = true;
	}

	if (bFoundDot == false)
	{
		Vector	vDir = GetAbsVelocity();
		VectorNormalize(vDir);

		SetAbsVelocity(vDir * 800);

		SetThink(NULL);
	}
}
#endif //BB2_AI

void CAPCMissile::ExplodeThink()
{
	DoExplosion();
}

//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
int CAPCMissile::AugerHealth()
{
	return m_iMaxHealth - 25;
}

//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
void CAPCMissile::DisableGuiding()
{
	m_bGuidingDisabled = true;
}

//-----------------------------------------------------------------------------
// Guidance hints
//-----------------------------------------------------------------------------
void CAPCMissile::SetGuidanceHint(const char *pHintName)
{
	m_strHint = MAKE_STRING(pHintName);
}

//-----------------------------------------------------------------------------
// The actual explosion 
//-----------------------------------------------------------------------------
void CAPCMissile::DoExplosion(void)
{
	if (GetWaterLevel() != 0)
	{
		CEffectData data;
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = 128;
		data.m_flScale = 128;
		data.m_fFlags = 0;
		DispatchEffect("WaterSurfaceExplosion", data);
	}
	else
	{
		ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(),
			APC_MISSILE_DAMAGE, 100, true, 20000);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCMissile::ComputeLeadingPosition(const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition)
{
	Vector vecTarget = pTarget->BodyTarget(vecShootPosition, false);
	float flShotSpeed = GetAbsVelocity().Length();
	if (flShotSpeed == 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	Vector vecVelocity = pTarget->GetSmoothedVelocity();
	vecVelocity.z = 0.0f;
	float flTargetSpeed = VectorNormalize(vecVelocity);
	Vector vecDelta;
	VectorSubtract(vecShootPosition, vecTarget, vecDelta);
	float flTargetToShooter = VectorNormalize(vecDelta);
	float flCosTheta = DotProduct(vecDelta, vecVelocity);

	// Law of cosines... z^2 = x^2 + y^2 - 2xy cos Theta
	// where z = flShooterToPredictedTargetPosition = flShotSpeed * predicted time
	// x = flTargetSpeed * predicted time
	// y = flTargetToShooter
	// solve for predicted time using at^2 + bt + c = 0, t = (-b +/- sqrt( b^2 - 4ac )) / 2a
	float a = flTargetSpeed * flTargetSpeed - flShotSpeed * flShotSpeed;
	float b = -2.0f * flTargetToShooter * flCosTheta * flTargetSpeed;
	float c = flTargetToShooter * flTargetToShooter;

	float flDiscrim = b*b - 4 * a*c;
	if (flDiscrim < 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	flDiscrim = sqrt(flDiscrim);
	float t = (-b + flDiscrim) / (2.0f * a);
	float t2 = (-b - flDiscrim) / (2.0f * a);
	if (t < t2)
	{
		t = t2;
	}

	if (t <= 0.0f)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	VectorMA(vecTarget, flTargetSpeed * t, vecVelocity, *pLeadPosition);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCMissile::ComputeActualDotPosition(CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed)
{
	if (m_bGuidingDisabled)
	{
		*pActualDotPosition = GetAbsOrigin();
		*pHomingSpeed = 0.0f;
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}

	if ((m_strHint != NULL_STRING) && (!m_hSpecificTarget))
	{
		Vector vecOrigin, vecVelocity;
		CBaseEntity *pTarget = pLaserDot->GetTargetEntity();
		if (pTarget)
		{
			vecOrigin = pTarget->BodyTarget(GetAbsOrigin(), false);
			vecVelocity = pTarget->GetSmoothedVelocity();
		}
		else
		{
			vecOrigin = pLaserDot->GetChasePosition();
			vecVelocity = vec3_origin;
		}

		m_hSpecificTarget = CInfoAPCMissileHint::FindAimTarget(this, STRING(m_strHint), vecOrigin, vecVelocity);
	}

	CBaseEntity *pLaserTarget = m_hSpecificTarget ? m_hSpecificTarget.Get() : pLaserDot->GetTargetEntity();
	if (!pLaserTarget)
	{
		BaseClass::ComputeActualDotPosition(pLaserDot, pActualDotPosition, pHomingSpeed);
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}

	if (pLaserTarget->ClassMatches("npc_bullseye"))
	{
		if (m_flLastHomingSpeed != RPG_HOMING_SPEED)
		{
			if (m_flLastHomingSpeed > RPG_HOMING_SPEED)
			{
				m_flLastHomingSpeed -= HOMING_SPEED_ACCEL * UTIL_GetSimulationInterval();
				if (m_flLastHomingSpeed < RPG_HOMING_SPEED)
				{
					m_flLastHomingSpeed = RPG_HOMING_SPEED;
				}
			}
			else
			{
				m_flLastHomingSpeed += HOMING_SPEED_ACCEL * UTIL_GetSimulationInterval();
				if (m_flLastHomingSpeed > RPG_HOMING_SPEED)
				{
					m_flLastHomingSpeed = RPG_HOMING_SPEED;
				}
			}
		}
		*pHomingSpeed = m_flLastHomingSpeed;
		*pActualDotPosition = pLaserTarget->WorldSpaceCenter();
		return;
	}

	Vector vLaserStart;
	GetShootPosition(pLaserDot, &vLaserStart);
	*pHomingSpeed = APC_LAUNCH_HOMING_SPEED;

	//Get the laser's vector
	Vector vecTargetPosition = pLaserTarget->BodyTarget(GetAbsOrigin(), false);

	// Compute leading position
	Vector vecLeadPosition;
	ComputeLeadingPosition(GetAbsOrigin(), pLaserTarget, &vecLeadPosition);

	Vector vecTargetToMissile, vecTargetToShooter;
	VectorSubtract(GetAbsOrigin(), vecTargetPosition, vecTargetToMissile);
	VectorSubtract(vLaserStart, vecTargetPosition, vecTargetToShooter);

	*pActualDotPosition = vecLeadPosition;

	float flMinHomingDistance = MIN_HOMING_DISTANCE;
	float flMaxHomingDistance = MAX_HOMING_DISTANCE;
	float flBlendTime = gpGlobals->curtime - m_flIgnitionTime;
	if (flBlendTime > DOWNWARD_BLEND_TIME_START)
	{
		if (m_flReachedTargetTime != 0.0f)
		{
			*pHomingSpeed = APC_HOMING_SPEED;
			float flDeltaTime = clamp(gpGlobals->curtime - m_flReachedTargetTime, 0.0f, CORRECTION_TIME);
			*pHomingSpeed = SimpleSplineRemapVal(flDeltaTime, 0.0f, CORRECTION_TIME, 0.2f, *pHomingSpeed);
			flMinHomingDistance = SimpleSplineRemapVal(flDeltaTime, 0.0f, CORRECTION_TIME, MIN_NEAR_HOMING_DISTANCE, flMinHomingDistance);
			flMaxHomingDistance = SimpleSplineRemapVal(flDeltaTime, 0.0f, CORRECTION_TIME, MAX_NEAR_HOMING_DISTANCE, flMaxHomingDistance);
		}
		else
		{
			flMinHomingDistance = MIN_NEAR_HOMING_DISTANCE;
			flMaxHomingDistance = MAX_NEAR_HOMING_DISTANCE;
			Vector vecDelta;
			VectorSubtract(GetAbsOrigin(), *pActualDotPosition, vecDelta);
			if (vecDelta.z > MIN_HEIGHT_DIFFERENCE)
			{
				float flClampedHeight = clamp(vecDelta.z, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE);
				float flHeightAdjustFactor = SimpleSplineRemapVal(flClampedHeight, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE, 0.0f, 1.0f);

				vecDelta.z = 0.0f;
				float flDist = VectorNormalize(vecDelta);

				float flForwardOffset = 2000.0f;
				if (flDist > flForwardOffset)
				{
					Vector vecNewPosition;
					VectorMA(GetAbsOrigin(), -flForwardOffset, vecDelta, vecNewPosition);
					vecNewPosition.z = pActualDotPosition->z;

					VectorLerp(*pActualDotPosition, vecNewPosition, flHeightAdjustFactor, *pActualDotPosition);
				}
			}
			else
			{
				m_flReachedTargetTime = gpGlobals->curtime;
			}
		}

		// Allows for players right at the edge of rocket range to be threatened
		if (flBlendTime > 0.6f)
		{
			float flTargetLength = GetAbsOrigin().DistTo(pLaserTarget->WorldSpaceCenter());
			flTargetLength = clamp(flTargetLength, flMinHomingDistance, flMaxHomingDistance);
			*pHomingSpeed = SimpleSplineRemapVal(flTargetLength, flMaxHomingDistance, flMinHomingDistance, *pHomingSpeed, 0.01f);
		}
	}

	float flDot = DotProduct2D(vecTargetToShooter.AsVector2D(), vecTargetToMissile.AsVector2D());
	if ((flDot < 0) || m_bGuidingDisabled)
	{
		*pHomingSpeed = 0.0f;
	}

	m_flLastHomingSpeed = *pHomingSpeed;
}

#endif

//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CLASS(env_laserdot, CLaserDot);

BEGIN_DATADESC(CLaserDot)
DEFINE_FIELD(m_vecSurfaceNormal, FIELD_VECTOR),
DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
DEFINE_FIELD(m_bVisibleLaserDot, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsOn, FIELD_BOOLEAN),

//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),	// don't save - regenerated by constructor
END_DATADESC()

//-----------------------------------------------------------------------------
// Finds missiles in cone
//-----------------------------------------------------------------------------
CBaseEntity *CreateLaserDot(const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot)
{
	return CLaserDot::Create(origin, pOwner, bVisibleDot);
}

void SetLaserDotTarget(CBaseEntity *pLaserDot, CBaseEntity *pTarget)
{
	CLaserDot *pDot = assert_cast<CLaserDot*>(pLaserDot);
	pDot->SetTargetEntity(pTarget);
}

void EnableLaserDot(CBaseEntity *pLaserDot, bool bEnable)
{
	CLaserDot *pDot = assert_cast<CLaserDot*>(pLaserDot);
	if (bEnable)
	{
		pDot->TurnOn();
	}
	else
	{
		pDot->TurnOff();
	}
}

CLaserDot::CLaserDot(void)
{
	m_hTargetEnt = NULL;
	m_bIsOn = true;
#ifndef CLIENT_DLL
	g_LaserDotList.Insert(this);
#endif
}

CLaserDot::~CLaserDot(void)
{
#ifndef CLIENT_DLL
	g_LaserDotList.Remove(this);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CLaserDot
//-----------------------------------------------------------------------------
CLaserDot *CLaserDot::Create(const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot)
{
#ifndef CLIENT_DLL
	CLaserDot *pLaserDot = (CLaserDot *)CBaseEntity::Create("env_laserdot", origin, QAngle(0, 0, 0));

	if (pLaserDot == NULL)
		return NULL;

	pLaserDot->m_bVisibleLaserDot = bVisibleDot;
	pLaserDot->SetMoveType(MOVETYPE_NONE);
	pLaserDot->AddSolidFlags(FSOLID_NOT_SOLID);
	pLaserDot->AddEffects(EF_NOSHADOW);
	UTIL_SetSize(pLaserDot, -Vector(4, 4, 4), Vector(4, 4, 4));

	pLaserDot->SetOwnerEntity(pOwner);

	pLaserDot->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

	if (!bVisibleDot)
	{
		pLaserDot->MakeInvisible();
	}

	return pLaserDot;
#else
	return NULL;
#endif
}

void CLaserDot::SetLaserPosition(const Vector &origin, const Vector &normal)
{
	SetAbsOrigin(origin);
	m_vecSurfaceNormal = normal;
}

Vector CLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOn(void)
{
	m_bIsOn = true;
	if (m_bVisibleLaserDot)
	{
		//BaseClass::TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOff(void)
{
	m_bIsOn = false;
	if (m_bVisibleLaserDot)
	{
		//BaseClass::TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::MakeInvisible(void)
{
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Draw our sprite
//-----------------------------------------------------------------------------
int CLaserDot::DrawModel(int flags)
{
	color32 color = { 255, 255, 255, 255 };
	Vector	vecAttachment, vecDir;
	QAngle	angles;

	float	scale;
	Vector	endPos;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(GetOwnerEntity());

	if (pOwner != NULL && pOwner->IsDormant() == false)
	{
		// Always draw the dot in front of our faces when in first-person
		if (pOwner->IsLocalPlayer())
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();
		}
		else
		{
			// Take the eye position and direction
			vecAttachment = pOwner->EyePosition();
			QAngle angles = pOwner->EyeAngles();
			AngleVectors(angles, &vecDir);
		}

		trace_t tr;
		UTIL_TraceLine(vecAttachment, vecAttachment + (vecDir * MAX_TRACE_LENGTH), MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

		// Backup off the hit plane
		endPos = tr.endpos + (tr.plane.normal * 4.0f);
	}
	else
	{
		// Just use our position if we can't predict it otherwise
		endPos = GetAbsOrigin();
	}

	// Randomly flutter
	scale = 16.0f + random->RandomFloat(-4.0f, 4.0f);

	// Draw our laser dot in space
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(m_hSpriteMaterial, this);
	DrawSprite(endPos, scale, scale, color);

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Setup our sprite reference
//-----------------------------------------------------------------------------
void CLaserDot::OnDataChanged(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		m_hSpriteMaterial.Init(RPG_LASER_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS);
	}
}

#endif	//CLIENT_DLL