//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFCLASSDATA_SHARED_H
#define TFCLASSDATA_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"


enum TFClass
{
	TFCLASS_UNDECIDED = 0,

	TFCLASS_RECON,
	TFCLASS_COMMANDO,
	TFCLASS_MEDIC,
	TFCLASS_DEFENDER,
	TFCLASS_SNIPER,
	TFCLASS_SUPPORT,
	TFCLASS_ESCORT,
	TFCLASS_SAPPER,
	TFCLASS_INFILTRATOR,
	TFCLASS_PYRO,

	// TFCLASS_INDIRECT,

	TFCLASS_CLASS_COUNT,
};


//=============================================================================
//
// Class Shared Data
//
#define PLAYERCLASS_HULL_STAND_MIN		Vector( -24.0f, -24.0f,  0.0f )
#define PLAYERCLASS_HULL_STAND_MAX		Vector(  24.0f,  24.0f,  72.0f )
#define PLAYERCLASS_VIEWOFFSET_STAND	Vector(   0.0f,   0.0f,  64.0f )

#define PLAYERCLASS_HULL_DUCK_MIN		Vector( -24.0f, -24.0f,  0.0f )
#define PLAYERCLASS_HULL_DUCK_MAX		Vector(  24.0f,  24.0f,  36.0f )
#define PLAYERCLASS_VIEWOFFSET_DUCK		Vector(   0.0f,   0.0f,  30.0f )

#define PLAYERCLASS_STEPSIZE			18.0f

//=============================================================================
//
// Commando Class Specific Data
//
//#define COMMANDO_TEST

#ifndef COMMANDO_TEST

#define COMMANDOCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define COMMANDOCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define COMMANDOCLASS_VIEWOFFSET_STAND		PLAYERCLASS_VIEWOFFSET_STAND

#define COMMANDOCLASS_HULL_DUCK_MIN			PLAYERCLASS_HULL_DUCK_MIN
#define COMMANDOCLASS_HULL_DUCK_MAX			PLAYERCLASS_HULL_DUCK_MAX
#define COMMANDOCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define COMMANDOCLASS_STEPSIZE				PLAYERCLASS_STEPSIZE

#else

#define COMMANDOCLASS_HULL_STAND_MIN		Vector( -18.0f, -18.0f,   0.0f )
#define COMMANDOCLASS_HULL_STAND_MAX		Vector(  18.0f,  18.0f,  54.0f )
#define COMMANDOCLASS_VIEWOFFSET_STAND		Vector(   0.0f,   0.0f,  51.0f )

#define COMMANDOCLASS_HULL_DUCK_MIN			Vector( -18.0f, -18.0f,   0.0f )
#define COMMANDOCLASS_HULL_DUCK_MAX			Vector(  18.0f,  18.0f,  40.0f )
#define COMMANDOCLASS_VIEWOFFSET_DUCK		Vector(   0.0f,   0.0f,  35.0f )

#define COMMANDOCLASS_STEPSIZE				18.0f

#endif

#define COMMANDO_MOVETYPE_BULLRUSH			( MOVETYPE_LAST + 1 )

#define COMMANDO_TIME_INVALID				-9999.0f
#define COMMANDO_DOUBLETAP_TIME				300.0f
#define COMMANDO_BULLRUSH_TIME				2000.0f
#define COMMANDO_BULLRUSH_VIEWDELTA_TIME	1000.0f
#define COMMANDO_BULLRUSH_VIEWDELTA_TEST	( COMMANDO_BULLRUSH_TIME - COMMANDO_BULLRUSH_VIEWDELTA_TIME )

struct PlayerClassCommandoData_t
{
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( PlayerClassCommandoData_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	enum { PLAYERCLASS_ID = TFCLASS_COMMANDO };

	CNetworkVar( bool, m_bCanBullRush );
	CNetworkVar( bool, m_bBullRush );
	CNetworkVector( m_vecBullRushDir );
	CNetworkQAngle( m_vecBullRushViewDir );
	CNetworkQAngle( m_vecBullRushViewGoalDir );
	CNetworkVar( float, m_flBullRushTime );
	CNetworkVar( float, m_flDoubleTapForwardTime );
};


//=============================================================================
//
// Defender Class Specific Data
//
#if 0
#define DEFENDERCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define DEFENDERCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define DEFENDERCLASS_VIEWOFFSET_STAND		PLAYERCLASS_VIEWOFFSET_STAND

#define DEFENDERCLASS_HULL_DUCK_MIN			PLAYERCLASS_HULL_DUCK_MIN
#define DEFENDERCLASS_HULL_DUCK_MAX			PLAYERCLASS_HULL_DUCK_MAX
#define DEFENDERCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define DEFENDERCLASS_STEPSIZE				PLAYERCLASS_STEPSIZE
#else
#define DEFENDERCLASS_HULL_STAND_MIN		Vector( -18.0f, -18.0f,   0.0f )
#define DEFENDERCLASS_HULL_STAND_MAX		Vector(  18.0f,  18.0f,  55.0f )
#define DEFENDERCLASS_VIEWOFFSET_STAND		Vector(   0.0f,   0.0f,  53.0f )

#define DEFENDERCLASS_HULL_DUCK_MIN			Vector( -18.0f, -18.0f,   0.0f )
#define DEFENDERCLASS_HULL_DUCK_MAX			Vector(  18.0f,  18.0f,  30.0f )
#define DEFENDERCLASS_VIEWOFFSET_DUCK		Vector(   0.0f,   0.0f,  25.0f )

#define DEFENDERCLASS_STEPSIZE				15.0f
#endif

struct PlayerClassDefenderData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_DEFENDER };

};

//=============================================================================
//
// Escort Class Specific Data
//
#if 0
#define ESCORTCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define ESCORTCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define ESCORTCLASS_VIEWOFFSET_STAND	PLAYERCLASS_VIEWOFFSET_STAND

#define ESCORTCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define ESCORTCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define ESCORTCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define ESCORTCLASS_STEPSIZE			PLAYERCLASS_STEPSIZE
#else
#define ESCORTCLASS_HULL_STAND_MIN		Vector( -24.0f, -24.0f,   0.0f )
#define ESCORTCLASS_HULL_STAND_MAX		Vector(  24.0f,  24.0f,  74.0f )
#define ESCORTCLASS_VIEWOFFSET_STAND	Vector(   0.0f,   0.0f,  67.0f )

#define ESCORTCLASS_HULL_DUCK_MIN		Vector( -24.0f, -24.0f,   0.0f )
#define ESCORTCLASS_HULL_DUCK_MAX		Vector(  24.0f,  24.0f,  72.0f )
#define ESCORTCLASS_VIEWOFFSET_DUCK		Vector(   0.0f,   0.0f,  48.0f )

#define ESCORTCLASS_STEPSIZE			18.0f
#endif

struct PlayerClassEscortData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_ESCORT };

};

//=============================================================================
//
// Infiltrator Class Specific Data
//
#define INFILTRATORCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define INFILTRATORCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define INFILTRATORCLASS_VIEWOFFSET_STAND	PLAYERCLASS_VIEWOFFSET_STAND

#define INFILTRATORCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define INFILTRATORCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define INFILTRATORCLASS_VIEWOFFSET_DUCK	PLAYERCLASS_VIEWOFFSET_DUCK

#define INFILTRATORCLASS_STEPSIZE			PLAYERCLASS_STEPSIZE

struct PlayerClassInfiltratorData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_INFILTRATOR };

};


//=============================================================================
//
// Pyro Class Specific Data
//

#define PYROCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define PYROCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define PYROCLASS_VIEWOFFSET_STAND		PLAYERCLASS_VIEWOFFSET_STAND

#define PYROCLASS_HULL_DUCK_MIN			PLAYERCLASS_HULL_DUCK_MIN
#define PYROCLASS_HULL_DUCK_MAX			PLAYERCLASS_HULL_DUCK_MAX
#define PYROCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define PYROCLASS_STEPSIZE				PLAYERCLASS_STEPSIZE

struct PlayerClassPyroData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_PYRO };

};


//=============================================================================
//
// Medic Class Specific Data
//
#define MEDICCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define MEDICCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define MEDICCLASS_VIEWOFFSET_STAND		PLAYERCLASS_VIEWOFFSET_STAND

#define MEDICCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define MEDICCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define MEDICCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define MEDICCLASS_STEPSIZE				PLAYERCLASS_STEPSIZE

struct PlayerClassMedicData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_MEDIC };

};

//=============================================================================
//
// Recon Class Specific Data
//
#define RECONCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define RECONCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define RECONCLASS_VIEWOFFSET_STAND		PLAYERCLASS_VIEWOFFSET_STAND

#define RECONCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define RECONCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define RECONCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define RECONCLASS_STEPSIZE				PLAYERCLASS_STEPSIZE

struct PlayerClassReconData_t
{
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( PlayerClassReconData_t );
	DECLARE_EMBEDDED_NETWORKVAR();


	enum { PLAYERCLASS_ID = TFCLASS_RECON };

	// For in-air jumps
	CNetworkVar( int, m_nJumpCount );

	// For wall jumps
	CNetworkVar( float, m_flSuppressionJumpTime );
	CNetworkVar( float, m_flSuppressionImpactTime );
	CNetworkVar( float, m_flActiveJumpTime );
	CNetworkVar( float, m_flStickTime );
	CNetworkVector(	m_vecImpactNormal );
	CNetworkVar( float, m_flImpactDist );
	CNetworkVector( m_vecUnstickVelocity );

	// Trail
	CNetworkVar( bool, m_bTrailParticles );
};


//=============================================================================
//
// Sniper Class Specific Data
//
#define SNIPERCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define SNIPERCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define SNIPERCLASS_VIEWOFFSET_STAND	PLAYERCLASS_VIEWOFFSET_STAND

#define SNIPERCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define SNIPERCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define SNIPERCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define SNIPERCLASS_STEPSIZE			PLAYERCLASS_STEPSIZE

struct PlayerClassSniperData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_SNIPER };

};

//=============================================================================
//
// Support Class Specific Data
//
#if 0
#define SUPPORTCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define SUPPORTCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define SUPPORTCLASS_VIEWOFFSET_STAND	PLAYERCLASS_VIEWOFFSET_STAND

#define SUPPORTCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define SUPPORTCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define SUPPORTCLASS_VIEWOFFSET_DUCK	PLAYERCLASS_VIEWOFFSET_DUCK

#define SUPPORTCLASS_STEPSIZE			PLAYERCLASS_STEPSIZE
#else
#define SUPPORTCLASS_HULL_STAND_MIN		Vector( -30.0f, -30.0f,   0.0f )
#define SUPPORTCLASS_HULL_STAND_MAX		Vector(  30.0f,  30.0f, 106.0f )
#define SUPPORTCLASS_VIEWOFFSET_STAND	Vector(   0.0f,   0.0f,  120.0f )

#define SUPPORTCLASS_HULL_DUCK_MIN		Vector( -30.0f, -30.0f,   0.0f )
#define SUPPORTCLASS_HULL_DUCK_MAX		Vector(  30.0f,  30.0f,  72.0f )
#define SUPPORTCLASS_VIEWOFFSET_DUCK	Vector(   0.0f,   0.0f,  64.0f )

#define SUPPORTCLASS_STEPSIZE			27.0f
#endif

struct PlayerClassSupportData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_SUPPORT };

};

//=============================================================================
//
// Sapper Class Specific Data
//
#define SAPPERCLASS_HULL_STAND_MIN		PLAYERCLASS_HULL_STAND_MIN
#define SAPPERCLASS_HULL_STAND_MAX		PLAYERCLASS_HULL_STAND_MAX
#define SAPPERCLASS_VIEWOFFSET_STAND	PLAYERCLASS_VIEWOFFSET_STAND

#define SAPPERCLASS_HULL_DUCK_MIN		PLAYERCLASS_HULL_DUCK_MIN
#define SAPPERCLASS_HULL_DUCK_MAX		PLAYERCLASS_HULL_DUCK_MAX
#define SAPPERCLASS_VIEWOFFSET_DUCK		PLAYERCLASS_VIEWOFFSET_DUCK

#define SAPPERCLASS_STEPSIZE			PLAYERCLASS_STEPSIZE

struct PlayerClassSapperData_t
{
	DECLARE_PREDICTABLE();

	enum { PLAYERCLASS_ID = TFCLASS_SAPPER };
};


#include "tf_shareddefs.h"


#endif // TFCLASSDATA_SHARED_H
