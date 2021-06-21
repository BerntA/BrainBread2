//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "filesystem.h"
#include "sentence.h"
#include "hud_closecaption.h"
#include "engine/ivmodelinfo.h"
#include "engine/ivdebugoverlay.h"
#include "bone_setup.h"
#include "soundinfo.h"
#include "tools/bonelist.h"
#include "KeyValues.h"
#include "tier0/vprof.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar g_CV_FlexRules("flex_rules", "1", 0, "Allow flex animation rules to run." );
ConVar g_CV_BlinkDuration("blink_duration", "0.2", 0, "How many seconds an eye blink will last." );
ConVar g_CV_FlexSmooth("flex_smooth", "1", 0, "Applies smoothing/decay curve to flex animation controller changes." );

#if defined( CBaseFlex )
#undef CBaseFlex
#endif

IMPLEMENT_CLIENTCLASS_DT(C_BaseFlex, DT_BaseFlex, CBaseFlex)
	RecvPropVector(RECVINFO(m_viewtarget)),

#ifdef HL2_CLIENT_DLL
	RecvPropFloat( RECVINFO(m_vecViewOffset[0]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[1]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[2]) ),

	RecvPropVector(RECVINFO(m_vecLean)),
	RecvPropVector(RECVINFO(m_vecShift)),
#endif
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseFlex )
END_PREDICTION_DATA()

C_BaseFlex::C_BaseFlex() : 
	m_iv_viewtarget( "C_BaseFlex::m_iv_viewtarget" ), 
	m_iv_flexWeight("C_BaseFlex:m_iv_flexWeight" ),
#ifdef HL2_CLIENT_DLL
	m_iv_vecLean("C_BaseFlex:m_iv_vecLean" ),
	m_iv_vecShift("C_BaseFlex:m_iv_vecShift" )
#endif
{
#ifdef _DEBUG
	((Vector&)m_viewtarget).Init();
#endif

	AddVar( &m_viewtarget, &m_iv_viewtarget, LATCH_ANIMATION_VAR | INTERPOLATE_LINEAR_ONLY );
	AddVar( m_flexWeight, &m_iv_flexWeight, LATCH_ANIMATION_VAR );

	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;

	/// Make sure size is correct
	Assert( PHONEME_CLASS_STRONG + 1 == NUM_PHONEME_CLASSES );

#ifdef HL2_CLIENT_DLL
	// Get general lean vector
	AddVar( &m_vecLean, &m_iv_vecLean, LATCH_ANIMATION_VAR );
	AddVar( &m_vecShift, &m_iv_vecShift, LATCH_ANIMATION_VAR );
#endif
}

C_BaseFlex::~C_BaseFlex()
{
	delete[] m_flFlexDelayedWeight;
}

//-----------------------------------------------------------------------------
// Purpose: initialize fast lookups when model changes
//-----------------------------------------------------------------------------
CStudioHdr *C_BaseFlex::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	
	// init to invalid setting
	m_iBlink = -1;
	m_iEyeUpdown = LocalFlexController_t(-1);
	m_iEyeRightleft = LocalFlexController_t(-1);
	m_bSearchedForEyeFlexes = false;
	m_iMouthAttachment = 0;

	delete[] m_flFlexDelayedWeight;
	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;

	if (hdr)
	{
		if (hdr->numflexdesc())
		{
			m_cFlexDelayedWeight = hdr->numflexdesc();
			m_flFlexDelayedWeight = new float[ m_cFlexDelayedWeight ];
			memset( m_flFlexDelayedWeight, 0, sizeof( float ) * m_cFlexDelayedWeight );
		}

		m_iv_flexWeight.SetMaxCount( hdr->numflexcontrollers() );

		m_iMouthAttachment = LookupAttachment( "mouth" );

		LinkToGlobalFlexControllers( hdr );
	}

	return hdr;
}

void C_BaseFlex::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

#ifdef HL2_CLIENT_DLL
	// shift pelvis, rotate body
	if (hdr->GetNumIKChains() != 0 && (m_vecShift.x != 0.0 || m_vecShift.y != 0.0))
	{
		//CIKContext auto_ik;
		//auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), currentTime, gpGlobals->framecount, boneMask );
		//auto_ik.AddAllLocks( pos, q );

		matrix3x4_t rootxform;
		AngleMatrix( GetRenderAngles(), GetRenderOrigin(), rootxform );

		Vector localShift;
		VectorIRotate( m_vecShift, rootxform, localShift );
		Vector localLean;
		VectorIRotate( m_vecLean, rootxform, localLean );

		Vector p0 = pos[0];
		float length = VectorNormalize( p0 );

		// shift the root bone, but keep the height off the origin the same
		Vector shiftPos = pos[0] + localShift;
		VectorNormalize( shiftPos );
		Vector leanPos = pos[0] + localLean;
		VectorNormalize( leanPos );
		pos[0] = shiftPos * length;

		// rotate the root bone based on how much it was "leaned"
		Vector p1;
		CrossProduct( p0, leanPos, p1 );
		float sinAngle = VectorNormalize( p1 );
		float cosAngle = DotProduct( p0, leanPos );
		float angle = atan2( sinAngle, cosAngle ) * 180 / M_PI;
		Quaternion q1;
		angle = clamp( angle, -45, 45 );
		AxisAngleQuaternion( p1, angle, q1 );
		QuaternionMult( q1, q[0], q[0] );
		QuaternionNormalize( q[0] );

		// DevMsgRT( "   (%.2f) %.2f %.2f %.2f\n", angle, p1.x, p1.y, p1.z );
		// auto_ik.SolveAllLocks( pos, q );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: place "voice" sounds on mouth
//-----------------------------------------------------------------------------
bool C_BaseFlex::GetSoundSpatialization( SpatializationInfo_t& info )
{
	bool bret = BaseClass::GetSoundSpatialization( info );
	// Default things it's audible, put it at a better spot?
	if ( bret )
	{
		if ((info.info.nChannel == CHAN_VOICE || info.info.nChannel == CHAN_VOICE2) && m_iMouthAttachment > 0)
		{
			Vector origin;
			QAngle angles;
			
			C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false );

			if (GetAttachment( m_iMouthAttachment, origin, angles ))
			{
				if (info.pOrigin)
				{
					*info.pOrigin = origin;
				}

				if (info.pAngles)
				{
					*info.pAngles = angles;
				}
			}
		}
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: run the interpreted FAC's expressions, converting global flex_controller 
//			values into FAC weights
//-----------------------------------------------------------------------------
void C_BaseFlex::RunFlexRules( CStudioHdr *hdr, float *dest )
{
	if ( !g_CV_FlexRules.GetInt() )
		return;

	if ( !hdr )
		return;

	hdr->RunFlexRules( g_flexweight, dest );
}

//-----------------------------------------------------------------------------
// Purpose: make sure the eyes are within 30 degrees of forward
//-----------------------------------------------------------------------------
Vector C_BaseFlex::SetViewTarget( CStudioHdr *pStudioHdr )
{
  	if ( !pStudioHdr )
  		return Vector( 0, 0, 0);

	// aim the eyes
	Vector tmp = m_viewtarget;

	if ( !m_bSearchedForEyeFlexes )
	{
		m_bSearchedForEyeFlexes = true;

		m_iEyeUpdown = FindFlexController( "eyes_updown" );
		m_iEyeRightleft = FindFlexController( "eyes_rightleft" );

		if ( m_iEyeUpdown != LocalFlexController_t(-1) )
		{
			pStudioHdr->pFlexcontroller( m_iEyeUpdown )->localToGlobal = AddGlobalFlexController( "eyes_updown" );
		}
		if ( m_iEyeRightleft != LocalFlexController_t(-1) )
		{
			pStudioHdr->pFlexcontroller( m_iEyeRightleft )->localToGlobal = AddGlobalFlexController( "eyes_rightleft" );
		}
	}

	if (m_iEyeAttachment > 0)
	{
		matrix3x4_t attToWorld;
		if (!GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			return Vector( 0, 0, 0);
		}

		Vector local;
		VectorITransform( tmp, attToWorld, local );
		
		// FIXME: clamp distance to something based on eyeball distance
		if (local.x < 6)
		{
			local.x = 6;
		}
		float flDist = local.Length();
		VectorNormalize( local );

		// calculate animated eye deflection
		Vector eyeDeflect;
		QAngle eyeAng( 0, 0, 0 );
		if ( m_iEyeUpdown != LocalFlexController_t(-1) )
		{
			mstudioflexcontroller_t *pflex = pStudioHdr->pFlexcontroller( m_iEyeUpdown );
			eyeAng.x = g_flexweight[ pflex->localToGlobal ];
		}
		
		if ( m_iEyeRightleft != LocalFlexController_t(-1) )
		{
			mstudioflexcontroller_t *pflex = pStudioHdr->pFlexcontroller( m_iEyeRightleft );
			eyeAng.y = g_flexweight[ pflex->localToGlobal ];
		}

		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%5.3f %5.3f", eyeAng.x, eyeAng.y );

		AngleVectors( eyeAng, &eyeDeflect );
		eyeDeflect.x = 0;

		// reduce deflection the more the eye is off center
		// FIXME: this angles make no damn sense
		eyeDeflect = eyeDeflect * (local.x * local.x);
		local = local + eyeDeflect;
		VectorNormalize( local );

		// check to see if the eye is aiming outside the max eye deflection
		float flMaxEyeDeflection = pStudioHdr->MaxEyeDeflection();
		if ( local.x < flMaxEyeDeflection )
		{
			// if so, clamp it to 30 degrees offset
			// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 1, 0, "%5.3f %5.3f %5.3f", local.x, local.y, local.z );
			local.x = 0;
			float d = local.LengthSqr();
			if ( d > 0.0f )
			{
				d = sqrtf( ( 1.0f - flMaxEyeDeflection * flMaxEyeDeflection ) / ( local.y*local.y + local.z*local.z ) );
				local.x = flMaxEyeDeflection;
				local.y = local.y * d;
				local.z = local.z * d;
			}
			else
			{
				local.x = 1.0;
			}
		}
		local = local * flDist;
		VectorTransform( local, attToWorld, tmp );
	}

	modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );

	/*
	debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f  : %.2f %.2f %.2f", 
		m_viewtarget.x, m_viewtarget.y, m_viewtarget.z, 
		m_prevviewtarget.x, m_prevviewtarget.y, m_prevviewtarget.z );
	*/

	return tmp;
}

//-----------------------------------------------------------------------------
// Purpose: fill keyvalues message with flex state
// Input  :
//-----------------------------------------------------------------------------
void C_BaseFlex::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_BaseFlex::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	BaseClass::GetToolRecordingState( msg );

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	memset( g_flexweight, 0, sizeof( g_flexweight ) );

	if ( hdr->numflexcontrollers() == 0 )
		return;

	LocalFlexController_t i;

	//ProcessSceneEvents( true );

	// FIXME: shouldn't this happen at runtime?
	// initialize the models local to global flex controller mappings
	if (hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1)
	{
		for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
		{
			int j = AddGlobalFlexController( hdr->pFlexcontroller( i )->pszName() );
			hdr->pFlexcontroller( i )->localToGlobal = j;
		}
	}

	// blend weights from server
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		g_flexweight[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		g_flexweight[pflex->localToGlobal] = g_flexweight[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	//ProcessSceneEvents( false );

	// check for blinking
	if (m_blinktoggle != m_prevblinktoggle)
	{
		m_prevblinktoggle = m_blinktoggle;
		m_blinktime = gpGlobals->curtime + g_CV_BlinkDuration.GetFloat();
	}

	if (m_iBlink == -1)
		m_iBlink = AddGlobalFlexController( "blink" );
	g_flexweight[m_iBlink] = 0;

	// FIXME: this needs a better algorithm
	// blink the eyes
	float t = (m_blinktime - gpGlobals->curtime) * M_PI * 0.5 * (1.0/g_CV_BlinkDuration.GetFloat());
	if (t > 0)
	{
		// do eyeblink falloff curve
		t = cos(t);
		if (t > 0)
		{
			g_flexweight[m_iBlink] = sqrtf( t ) * 2;
			if (g_flexweight[m_iBlink] > 1)
				g_flexweight[m_iBlink] = 2.0 - g_flexweight[m_iBlink];
		}
	}

	// Necessary???
	SetViewTarget( hdr );

	Vector viewtarget = m_viewtarget; // Use the unfiltered value

	// HACK HACK: Unmap eyes right/left amounts
	if (m_iEyeUpdown != LocalFlexController_t(-1) && m_iEyeRightleft != LocalFlexController_t(-1))
	{
		mstudioflexcontroller_t *flexupdown = hdr->pFlexcontroller( m_iEyeUpdown );
		mstudioflexcontroller_t *flexrightleft = hdr->pFlexcontroller( m_iEyeRightleft );

		if ( flexupdown->localToGlobal != -1 && flexrightleft->localToGlobal != -1 )
		{
			float updown = g_flexweight[ flexupdown->localToGlobal ];
			float rightleft = g_flexweight[ flexrightleft->localToGlobal ];

			if ( flexupdown->min != flexupdown->max )
			{
				updown = RemapVal( updown, flexupdown->min, flexupdown->max, 0.0f, 1.0f );
			}
			if ( flexrightleft->min != flexrightleft->max )
			{
				rightleft = RemapVal( rightleft, flexrightleft->min, flexrightleft->max, 0.0f, 1.0f );
			}
	
			g_flexweight[ flexupdown->localToGlobal ] = updown;
			g_flexweight[ flexrightleft->localToGlobal ] = rightleft;
		}
	}

	// Convert back to normalized weights
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		// rescale
		if ( pflex->max != pflex->min )
		{
			g_flexweight[pflex->localToGlobal] = ( g_flexweight[pflex->localToGlobal] - pflex->min ) / ( pflex->max - pflex->min );
		}
	}

	static BaseFlexRecordingState_t state;
	state.m_nFlexCount = MAXSTUDIOFLEXCTRL;
	state.m_pDestWeight = g_flexweight;
	state.m_vecViewTarget = viewtarget;
	msg->SetPtr( "baseflex", &state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::OnThreadedDrawSetup()
{
	if (m_iEyeAttachment < 0)
		return;

	CStudioHdr *hdr = GetModelPtr();
	if (!hdr)
		return;

	CalcAttachments();
}

//-----------------------------------------------------------------------------
// Should we use delayed flex weights?
//-----------------------------------------------------------------------------
bool C_BaseFlex::UsesFlexDelayedWeights()
{
	return ( m_flFlexDelayedWeight && g_CV_FlexSmooth.GetBool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::LinkToGlobalFlexControllers( CStudioHdr *hdr )
{
	if ( hdr && hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1 )
	{
		for (LocalFlexController_t i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
		{
			int j = AddGlobalFlexController( hdr->pFlexcontroller( i )->pszName() );
			hdr->pFlexcontroller( i )->localToGlobal = j;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Rendering callback to allow the client to set up all the model specific flex weights
//-----------------------------------------------------------------------------
void C_BaseFlex::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// hack in an initialization
	LinkToGlobalFlexControllers( GetModelPtr() );
	m_iBlink = AddGlobalFlexController( "blink" );

	if ( SetupGlobalWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights ) )
	{
		SetupLocalWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use the local bone positions to set flex control weights
//          via boneflexdrivers specified in the model
//-----------------------------------------------------------------------------
void C_BaseFlex::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	const int nBoneFlexDriverCount = pStudioHdr->BoneFlexDriverCount();

	for ( int i = 0; i < nBoneFlexDriverCount; ++i )
	{
		const mstudioboneflexdriver_t *pBoneFlexDriver = pStudioHdr->BoneFlexDriver( i );
		const Vector &position = pos[ pBoneFlexDriver->m_nBoneIndex ];

		const int nControllerCount = pBoneFlexDriver->m_nControlCount;
		for ( int j = 0; j < nControllerCount; ++j )
		{
			const mstudioboneflexdrivercontrol_t *pController = pBoneFlexDriver->pBoneFlexDriverControl( j );
			Assert( pController->m_nFlexControllerIndex >= 0 && pController->m_nFlexControllerIndex < pStudioHdr->numflexcontrollers() );
			Assert( pController->m_nBoneComponent >= 0 && pController->m_nBoneComponent <= 2 );
			SetFlexWeight( static_cast< LocalFlexController_t >( pController->m_nFlexControllerIndex ), RemapValClamped( position[pController->m_nBoneComponent], pController->m_flMin, pController->m_flMax, 0.0f, 1.0f ) );
		}
	}

	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );
}

//-----------------------------------------------------------------------------
// Purpose: process the entities networked state, vcd playback, wav file visemes, and blinks into a global shared flex controller array
//-----------------------------------------------------------------------------
bool C_BaseFlex::SetupGlobalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return false;

	memset( g_flexweight, 0, sizeof(g_flexweight) );

	// FIXME: this should assert then, it's too complex a class for the model
	if ( hdr->numflexcontrollers() == 0 )
	{
		int nSizeInBytes = nFlexWeightCount * sizeof( float );
		memset( pFlexWeights, 0, nSizeInBytes );
		if ( pFlexDelayedWeights )
		{
			memset( pFlexDelayedWeights, 0, nSizeInBytes );
		}
		return false;
	}

	LocalFlexController_t i;

	//ProcessSceneEvents( true );

	Assert( hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal != -1 );

	// get the networked flexweights and convert them from 0..1 to real dynamic range
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		g_flexweight[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		g_flexweight[pflex->localToGlobal] = g_flexweight[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	//ProcessSceneEvents( false );

	// check for blinking
	if (m_blinktoggle != m_prevblinktoggle)
	{
		m_prevblinktoggle = m_blinktoggle;
		m_blinktime = gpGlobals->curtime + g_CV_BlinkDuration.GetFloat();
	}

	if (m_iBlink == -1)
	{
		m_iBlink = AddGlobalFlexController( "blink" );
	}

	// FIXME: this needs a better algorithm
	// blink the eyes
	float flBlinkDuration = g_CV_BlinkDuration.GetFloat();
	float flOOBlinkDuration = ( flBlinkDuration > 0 ) ? 1.0f / flBlinkDuration : 0.0f;
	float t = ( m_blinktime - gpGlobals->curtime ) * M_PI * 0.5 * flOOBlinkDuration;
	if (t > 0)
	{
		// do eyeblink falloff curve
		t = cos(t);
		if (t > 0.0f && t < 1.0f)
		{
			t = sqrtf( t ) * 2.0f;
			if (t > 1.0f)
				t = 2.0f - t;
			t = clamp( t, 0.0f, 1.0f );
			// add it to whatever the blink track is doing
			g_flexweight[m_iBlink] = clamp( g_flexweight[m_iBlink] + t, 0.0f, 1.0f );
		}
	}

	// Drive the mouth from .wav file playback...
	//ProcessVisemes( m_PhonemeClasses );

	return true;
}

void C_BaseFlex::RunFlexDelay( int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights, float &flFlexDelayTime )
{
	// process the delayed version of the flexweights
	if ( flFlexDelayTime > 0.0f && flFlexDelayTime < gpGlobals->curtime )
	{
		float d = clamp( gpGlobals->curtime - flFlexDelayTime, 0.0, gpGlobals->frametime );
		d = ExponentialDecay( 0.8, 0.033, d );

		for ( int i = 0; i < nFlexWeightCount; i++)
		{
			pFlexDelayedWeights[i] = pFlexDelayedWeights[i] * d + pFlexWeights[i] * (1.0f - d);
		}
	}
	flFlexDelayTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: convert the global flex controllers into model specific flex weights
//-----------------------------------------------------------------------------
void C_BaseFlex::SetupLocalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	// BUGBUG: We have a bug with SetCustomModel that causes a disagreement between the studio header here and the one used in l_studio.cpp CModelRender::DrawModelExecute
	// So when we hit that case, let's not do any work because otherwise we'd crash since the array sizes (m_flFlexDelayedWeight vs pFlexWeights) don't match.
	// Note that this check is duplicated in CEconEntity::SetupWeights.
	AssertMsg( nFlexWeightCount == m_cFlexDelayedWeight, "Disagreement between the number of flex weights. Do the studio headers match?" );
	if ( nFlexWeightCount != m_cFlexDelayedWeight )
	{
		return;
	}

	// convert the flex controllers into actual flex values
	RunFlexRules( hdr, pFlexWeights );

	// aim the eyes
	SetViewTarget( hdr );

	AssertOnce( hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal != -1 );

	if ( pFlexDelayedWeights )
	{
		RunFlexDelay( nFlexWeightCount, pFlexWeights, m_flFlexDelayedWeight, m_flFlexDelayTime );
		memcpy( pFlexDelayedWeights, m_flFlexDelayedWeight, sizeof( float ) * nFlexWeightCount );
	}

	/*
	LocalFlexController_t i;

	for (i = 0; i < hdr->numflexdesc; i++)
	{
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i-hdr->numflexcontrollers, 0, "%2d:%s : %3.2f", i, hdr->pFlexdesc( i )->pszFACS(), pFlexWeights[i] );
	}
	*/

	/*
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		int j = hdr->pFlexcontroller( i )->link;
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -i, 0, "%s %3.2f", g_flexcontroller[i], g_flexweight[j] );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Unified set of flex controller entries that all systems can talk to
//-----------------------------------------------------------------------------

int C_BaseFlex::g_numflexcontrollers;
char * C_BaseFlex::g_flexcontroller[MAXSTUDIOFLEXCTRL*4];
float C_BaseFlex::g_flexweight[MAXSTUDIOFLEXDESC];

int C_BaseFlex::AddGlobalFlexController( const char *szName )
{
	int i;
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		if (Q_stricmp( g_flexcontroller[i], szName ) == 0)
		{
			return i;
		}
	}

	if ( g_numflexcontrollers < MAXSTUDIOFLEXCTRL * 4 )
	{
		g_flexcontroller[g_numflexcontrollers++] = strdup( szName );
	}
	else
	{
		// FIXME: missing runtime error condition
	}
	return i;
}

char const *C_BaseFlex::GetGlobalFlexControllerName( int idx )
{
	if ( idx < 0 || idx >= g_numflexcontrollers )
	{
		return "";
	}

	return g_flexcontroller[ idx ];
}

void C_BaseFlex::SetFlexWeight( LocalFlexController_t index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0f, 1.0f );
		}

		m_flexWeight[ index ] = value;
	}
}

float C_BaseFlex::GetFlexWeight( LocalFlexController_t index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return 0;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return m_flexWeight[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}
				
		return m_flexWeight[index];
	}
	return 0.0;
}

LocalFlexController_t C_BaseFlex::FindFlexController( const char *szName )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t(-1);
}