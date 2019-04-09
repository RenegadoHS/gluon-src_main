//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "basehlcombatweapon.h"
#include "decals.h"
#include "beam_shared.h"
#include "AmmoDef.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "soundenvelope.h"
#include "soundent.h"
#include "shake.h"
#include "explode.h"
#include "particle_parse.h"

#include "weapon_gauss.h"

int isParticleInit = 0;
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
//			&angles - 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Gauss gun
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST( CWeaponGaussGun, DT_WeaponGaussGun )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_gauss, CWeaponGaussGun );
PRECACHE_WEAPON_REGISTER( weapon_gauss );

acttable_t	CWeaponGaussGun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_RPG,                    false },
    { ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_RPG,                    false },
    { ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_RPG,            false },
    { ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_RPG,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG,    false },
    { ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_RPG,        false },
    { ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_RPG,                    false },
    { ACT_RANGE_ATTACK1,                ACT_RANGE_ATTACK_RPG,                false },
};

IMPLEMENT_ACTTABLE( CWeaponGaussGun );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponGaussGun )

	DEFINE_FIELD( m_hViewModel,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextChargeTime,	FIELD_TIME ),
	DEFINE_SOUNDPATCH( m_sndCharge ),
	DEFINE_FIELD( m_flChargeStartTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bCharging,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bChargeIndicated,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flCoilMaxVelocity,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flCoilVelocity,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flCoilAngle,		FIELD_FLOAT ),

END_DATADESC()

ConVar sk_plr_dmg_gauss( "sk_plr_dmg_gauss", "0" );
ConVar sk_plr_max_dmg_gauss( "sk_plr_max_dmg_gauss", "0" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGaussGun::CWeaponGaussGun( void )
{
	m_hViewModel = NULL;
	m_flNextChargeTime	= 0;
	m_flChargeStartTime = 0;
	m_sndCharge			= NULL;
	m_bCharging			= false;
	m_bChargeIndicated	= false;
	isParticleInit = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::Precache( void )
{
	enginesound->PrecacheSound( "weapons/gauss/gauss_spinup.wav" );
	enginesound->PrecacheSound( "weapons/gauss/gauss_overcharging.wav" );
	PrecacheParticleSystem( "tau_fire" );
	PrecacheParticleSystem( "tau_charge_fire" );
	PrecacheParticleSystem( "tau_charge" );
	PrecacheParticleSystem( "tau_charge_explode" );
	
	//Temporary till fix!
	PrecacheParticleSystem( "muzzle_glow_mp5" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::Fire( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	StopParticleEffects(this);
	
	if ( pOwner == NULL )
		return;

	m_bCharging = false;

	if ( m_hViewModel == NULL )
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if ( vm )
		{
			m_hViewModel.Set( vm );
		}
	}

	Vector	startPos= pOwner->Weapon_ShootPosition();
	Vector	aimDir	= pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecUp, vecRight;
	VectorVectors( aimDir, vecRight, vecUp );

	float x, y, z;

	//Gassian spread
	do {
		x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);

	aimDir = aimDir + x * GetBulletSpread().x * vecRight + y * GetBulletSpread().y * vecUp;

	Vector	endPos	= startPos + ( aimDir * MAX_TRACE_LENGTH );
	
	//Shoot a shot straight out
	trace_t	tr;
	UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	
	ClearMultiDamage();

	CBaseEntity *pHit = tr.m_pEnt;
	
	CTakeDamageInfo dmgInfo( this, pOwner, sk_plr_dmg_gauss.GetFloat(), DMG_SHOCK );

	if ( pHit != NULL )
	{
		CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}
	
	if ( tr.DidHitWorld() )
	{
		float hitAngle = -DotProduct( tr.plane.normal, aimDir );

		if ( hitAngle < 0.5f )
		{
			Vector vReflection;
		
			vReflection = 2.0 * tr.plane.normal * hitAngle + aimDir;
			
			startPos	= tr.endpos;
			endPos		= startPos + ( vReflection * MAX_TRACE_LENGTH );
			
			//Draw beam to reflection point
			DrawBeam( tr.startpos, tr.endpos, 1.6, true );

			CPVSFilter filter( tr.endpos );
			te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

			UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );

			//Find new reflection end position
			UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

			if ( tr.m_pEnt != NULL )
			{
				dmgInfo.SetDamageForce( GetAmmoDef()->DamageForce(m_iPrimaryAmmoType) * vReflection );
				dmgInfo.SetDamagePosition( tr.endpos );
				tr.m_pEnt->DispatchTraceAttack( dmgInfo, vReflection, &tr );
			}

			//Connect reflection point to end
			DrawBeam( tr.startpos, tr.endpos, 0.4 );
		}
		else
		{
			DrawBeam( tr.startpos, tr.endpos, 1.6, true );
		}
	}
	else
	{
		DrawBeam( tr.startpos, tr.endpos, 1.6, true );
	}
	
	ApplyMultiDamage();

	UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );

	CPVSFilter filter( tr.endpos );
	te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

	AddViewKick();

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::ChargedFire( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	StopParticleEffects(this);
	
	if ( pOwner == NULL )
		return;

	bool penetrated = false;
	
	//FIX FIRE ROTATION (using ball for now)
	//DispatchParticleEffect( "tau_charge_fire", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "fire01", true);
	DispatchParticleEffect( "muzzle_glow_mp5", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "fire01", true);
	
	//Play shock sounds
	WeaponSound( SPECIAL2 );
	//WeaponSound( SPECIAL2 );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	StopChargeSound();

	m_bCharging = false;
	m_bChargeIndicated = false;

	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.2f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

	//Shoot a shot straight out
	Vector	startPos= pOwner->Weapon_ShootPosition();
	Vector	aimDir	= pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector	endPos	= startPos + ( aimDir * MAX_TRACE_LENGTH );
	
	trace_t	tr;
	UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	
	ClearMultiDamage();

	//Find how much damage to do
	float flChargeAmount = ( gpGlobals->curtime - m_flChargeStartTime ) / MAX_GAUSS_CHARGE_TIME;

	//Clamp this
	if ( flChargeAmount > 1.0f )
	{
		flChargeAmount = 1.0f;
	}

	//Determine the damage amount
	float flDamage = sk_plr_dmg_gauss.GetFloat() + ( ( sk_plr_max_dmg_gauss.GetFloat() - sk_plr_dmg_gauss.GetFloat() ) * flChargeAmount );

	CBaseEntity *pHit = tr.m_pEnt;
	if ( tr.DidHitWorld() )
	{
		//Try wall penetration
		UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );
		UTIL_DecalTrace( &tr, "RedGlowFade" );

		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		
		Vector	testPos = tr.endpos + ( aimDir * 48.0f );

		UTIL_TraceLine( testPos, tr.endpos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
			
		if ( tr.allsolid == false )
		{
			UTIL_DecalTrace( &tr, "RedGlowFade" );

			penetrated = true;
		}
	}
	else if ( pHit != NULL )
	{
		CTakeDamageInfo dmgInfo( this, pOwner, flDamage, DMG_SHOCK );
		CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );

		//Do direct damage to anything in our path
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}

	ApplyMultiDamage();

	UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -4.0f, -8.0f );
	viewPunch.y = random->RandomFloat( -0.25f,  0.25f );
	viewPunch.z = 0;

	pOwner->ViewPunch( viewPunch );

	DrawBeam( startPos, tr.endpos, 9.6, true );

	Vector	recoilForce = pOwner->BodyDirection2D() * -( flDamage * 10.0f );
	recoilForce[2] += 128.0f;

	pOwner->ApplyAbsVelocityImpulse( recoilForce );

	CPVSFilter filter( tr.endpos );
	te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

	if ( penetrated == true )
	{
		RadiusDamage( CTakeDamageInfo( this, this, flDamage, DMG_SHOCK ), tr.endpos, 200.0f, CLASS_NONE, NULL);
	}

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::DrawBeam( const Vector &startPos, const Vector &endPos, float width, bool useMuzzle )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	StopParticleEffects(this);
	
	if ( pOwner == NULL )
		return;

	//Check to store off our view model index
	if ( m_hViewModel == NULL )
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if ( vm )
		{
			m_hViewModel.Set( vm );
		}
	}

	//Draw the main beam shaft
	CBeam *pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, width );
	
	if ( useMuzzle )
	{
		pBeam->PointEntInit( endPos, m_hViewModel );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( width / 4.0f );
		pBeam->SetEndWidth( width );
	}
	else
	{
		pBeam->SetStartPos( startPos );
		pBeam->SetEndPos( endPos );
		pBeam->SetWidth( width );
		pBeam->SetEndWidth( width / 4.0f );
	}

	pBeam->SetBrightness( 255 );
	pBeam->SetColor( 255, 145+random->RandomInt( -16, 16 ), 0 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );

	//Draw electric bolts along shaft
	for ( int i = 0; i < 3; i++ )
	{
		pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, (width/2.0f) + i );
		
		if ( useMuzzle )
		{
			pBeam->PointEntInit( endPos, m_hViewModel );
			pBeam->SetEndAttachment( 1 );
		}
		else
		{
			pBeam->SetStartPos( startPos );
			pBeam->SetEndPos( endPos );
		}
		
		pBeam->SetBrightness( random->RandomInt( 64, 255 ) );
		pBeam->SetColor( 255, 255, 150+random->RandomInt( 0, 64 ) );
		pBeam->RelinkBeam();
		pBeam->LiveForTime( 0.1f );
		//pBeam->SetNoise( 1.6f * i );
		pBeam->SetEndWidth( 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	StopParticleEffects(this);
	
	if ( pOwner == NULL )
		return;
	
	//FIX FIRE ROTATION (using ball for now)
	//DispatchParticleEffect( "tau_fire", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "fire01", true);
	DispatchParticleEffect( "muzzle_glow_mp5", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "fire01", true);
	
	WeaponSound( SINGLE );
	//WeaponSound( SPECIAL2 );
	
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	
	pOwner->DoMuzzleFlash();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	Fire();

	m_flCoilMaxVelocity = 0.0f;
	m_flCoilVelocity = 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::IncreaseCharge( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if(isParticleInit == 0)
	{
		DispatchParticleEffect( "tau_charge", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "fire01", true);
	}
	isParticleInit = 1;
	if ( m_flNextChargeTime > gpGlobals->curtime )
		return;
	
	if ( pOwner == NULL )
		return;

	//Check our charge time
	if ( ( gpGlobals->curtime - m_flChargeStartTime ) > MAX_GAUSS_CHARGE_TIME )
	{
		//Notify the player they're at maximum charge
		if ( m_bChargeIndicated == false )
		{
			//WeaponSound( SPECIAL2 );
			//insert beep sound (inserted)
			CPASAttenuationFilter filter( this );
			m_sndBeep	= (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "weapons/gauss/gauss_overcharging.wav", ATTN_NORM );
			(CSoundEnvelopeController::GetController()).Play( m_sndBeep, 1.0f, 100 );
			m_bChargeIndicated = true;
		}

		if ( ( gpGlobals->curtime - m_flChargeStartTime ) > DANGER_GAUSS_CHARGE_TIME )
		{
			StopParticleEffects(this);
			//Damage the player
			//WeaponSound( SPECIAL2 );
			WeaponSound( SPECIAL1 );
			
			// Add DMG_CRUSH because we don't want any physics force
			pOwner->TakeDamage( CTakeDamageInfo( this, this, 25, DMG_SHOCK | DMG_CRUSH ) );
			
			DispatchParticleEffect( "tau_charge_explode", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), 0, true);
			
			color32 gaussDamage = {255,128,0,128};
			UTIL_ScreenFade( pOwner, gaussDamage, 0.2f, 0.2f, FFADE_IN );

			//m_flNextChargeTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 2.5f );
			ChargedFire();
			isParticleInit = 0;
			return;
		}

		return;
	}

	//Decrement power
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	//Make sure we can draw power
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		ChargedFire();
		return;
	}

	m_flNextChargeTime = gpGlobals->curtime + GAUSS_CHARGE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::SecondaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	if ( m_bCharging == false )
	{
		//Start looping animation
		SendWeaponAnim( ACT_GAUSS_SPINCYCLE );
		
		//Start looping sound
		if ( m_sndCharge == NULL )
		{
			CPASAttenuationFilter filter( this );
			m_sndCharge	= (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "weapons/gauss/gauss_spinup.wav", ATTN_NORM );
		}

		assert(m_sndCharge!=NULL);
		if ( m_sndCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCharge, 1.0f, 100 );
			//(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCharge, 250, 3.0f );
		}

		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		m_bChargeIndicated = false;
		isParticleInit = 0;

		//Decrement power
		pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	}

	IncreaseCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::AddViewKick( void )
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -0.5f, -0.2f );
	viewPunch.y = random->RandomFloat( -0.5f,  0.5f );
	viewPunch.z = 0;

	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::ItemPostFrame( void )
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	if ( pPlayer->m_afButtonReleased & IN_ATTACK2 )
	{
		if ( m_bCharging )
		{
			ChargedFire();
		}
	}

	m_flCoilVelocity = UTIL_Approach( m_flCoilMaxVelocity, m_flCoilVelocity, 10.0f );
	m_flCoilAngle = UTIL_AngleMod( m_flCoilAngle + ( m_flCoilVelocity * gpGlobals->frametime ) );

	static float fanAngle = 0.0f;

	fanAngle = UTIL_AngleMod( fanAngle + 2 );

	//Update spinning bits
	//studiohdr_t *pModel = modelinfo->GetStudiomodel( GetModel() );
	SetBoneController( 0, fanAngle );
	SetBoneController( 1, m_flCoilAngle );
	//Studio_SetController(pModel, 0, fanAngle, controllers[0]);
	//Studio_SetController(pModel, 1, m_flCoilAngle, controllers[1]);
	
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::StopChargeSound( void )
{
	if ( m_sndCharge != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCharge, 0.1f );
	}
	if ( m_sndBeep != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndBeep, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGaussGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopParticleEffects(this);
	StopChargeSound();
	m_bCharging = false;
	m_bChargeIndicated = false;
	isParticleInit = 0;

	return BaseClass::Holster( pSwitchingTo );
}
