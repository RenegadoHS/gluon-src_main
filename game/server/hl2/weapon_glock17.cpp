//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:     Glock 17 - hand gun
//
// $NoKeywords: $
//=============================================================================//
 
#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"
#include "particle_parse.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define GLOCK17_FASTEST_REFIRE_TIME     0.02f
#define GLOCK17_FASTEST_DRY_REFIRE_TIME 0.04f
 
#define GLOCK17_ACCURACY_SHOT_PENALTY_TIME      0.2f    // Applied amount of time each shot adds to the time we must recover from
#define GLOCK17_ACCURACY_MAXIMUM_PENALTY_TIME   1.5f    // Maximum penalty to deal out
 
ConVar  glock17_use_new_accuracy( "glock17_use_new_accuracy", "1" );
 
//-----------------------------------------------------------------------------
// CWeaponGlock17
//-----------------------------------------------------------------------------
 
class CWeaponGlock17 : public CBaseHLCombatWeapon
{
    DECLARE_DATADESC();
 
public:
    DECLARE_CLASS( CWeaponGlock17, CBaseHLCombatWeapon );
 
    CWeaponGlock17(void);
 
    DECLARE_SERVERCLASS();
 
    void    Precache( void );
    void    ItemPostFrame( void );
    void    ItemPreFrame( void );
    void    ItemBusyFrame( void );
    void    PrimaryAttack( void );
    void    AddViewKick( void );
    void    DryFire( void );
    void    Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
 
    void    UpdatePenaltyTime( void );
 
    int     CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
    Activity    GetPrimaryAttackActivity( void );
 
    virtual bool Reload( void );
 
    virtual const Vector& GetBulletSpread( void )
    {      
        // Handle NPCs first
        static Vector npcCone = VECTOR_CONE_5DEGREES;
        if ( GetOwner() && GetOwner()->IsNPC() )
            return npcCone;
           
        static Vector cone;
 
        if ( glock17_use_new_accuracy.GetBool() )
        {
            float ramp = RemapValClamped(   m_flAccuracyPenalty,
                                            0.0f,
                                            GLOCK17_ACCURACY_MAXIMUM_PENALTY_TIME,
                                            0.0f,
                                            1.0f );
 
            // We lerp from very accurate to inaccurate over time
            VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );
        }
        else
        {
            // Old value
            cone = VECTOR_CONE_4DEGREES;
        }
 
        return cone;
    }
   
    virtual int GetMinBurst()
    {
        return 1;
    }
 
    virtual int GetMaxBurst()
    {
        return 3;
    }
 
    virtual float GetFireRate( void )
    {
        return 0.2f;
    }
 
    DECLARE_ACTTABLE();
 
private:
    float   m_flSoonestPrimaryAttack;
    float   m_flLastAttackTime;
    float   m_flAccuracyPenalty;
    int     m_nNumShotsFired;
};
 
 
IMPLEMENT_SERVERCLASS_ST(CWeaponGlock17, DT_WeaponGlock17)
END_SEND_TABLE()
 
LINK_ENTITY_TO_CLASS( weapon_glock17, CWeaponGlock17 );
PRECACHE_WEAPON_REGISTER( weapon_glock17 );
 
BEGIN_DATADESC( CWeaponGlock17 )
 
    DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
    DEFINE_FIELD( m_flLastAttackTime,       FIELD_TIME ),
    DEFINE_FIELD( m_flAccuracyPenalty,      FIELD_FLOAT ), //NOTENOTE: This is NOT tracking game time
    DEFINE_FIELD( m_nNumShotsFired,         FIELD_INTEGER ),
 
END_DATADESC()
 
acttable_t  CWeaponGlock17::m_acttable[] =
{
    { ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_PISTOL,                    false },
    { ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_PISTOL,                    false },
    { ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
    { ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,    false },
    { ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_PISTOL,                    false },
    { ACT_RANGE_ATTACK1,                ACT_RANGE_ATTACK_PISTOL,                false },
};
 
IMPLEMENT_ACTTABLE( CWeaponGlock17 );
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGlock17::CWeaponGlock17( void )
{
    m_flSoonestPrimaryAttack = gpGlobals->curtime;
    m_flAccuracyPenalty = 0.0f;
 
    m_fMinRange1        = 24;
    m_fMaxRange1        = 1500;
    m_fMinRange2        = 24;
    m_fMaxRange2        = 200;
 
    m_bFiresUnderwater  = true;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::Precache( void )
{
	PrecacheParticleSystem( "weapon_glock_muzzleflash" );
    BaseClass::Precache();
}
 
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponGlock17::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
    switch( pEvent->event )
    {
        case EVENT_WEAPON_PISTOL_FIRE:
        {
            Vector vecShootOrigin, vecShootDir;
            vecShootOrigin = pOperator->Weapon_ShootPosition();
 
            CAI_BaseNPC *npc = pOperator->MyNPCPointer();
            ASSERT( npc != NULL );
 
            vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
 
            CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
 
            WeaponSound( SINGLE_NPC );
            pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
            pOperator->DoMuzzleFlash();
            m_iClip1 = m_iClip1 - 1;
        }
        break;
        default:
            BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
            break;
    }
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::DryFire( void )
{
    WeaponSound( EMPTY );
    SendWeaponAnim( ACT_VM_DRYFIRE );
   
    m_flSoonestPrimaryAttack    = gpGlobals->curtime + GLOCK17_FASTEST_DRY_REFIRE_TIME;
    m_flNextPrimaryAttack       = gpGlobals->curtime + SequenceDuration();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::PrimaryAttack( void )
{
    if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
    {
        m_nNumShotsFired = 0;
    }
    else
    {
        m_nNumShotsFired++;
    }
 
    m_flLastAttackTime = gpGlobals->curtime;
    m_flSoonestPrimaryAttack = gpGlobals->curtime + GLOCK17_FASTEST_REFIRE_TIME;
    CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner() );
 
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
    if( pOwner )
    {
        // Each time the player fires the pistol, reset the view punch. This prevents
        // the aim from 'drifting off' when the player fires very quickly. This may
        // not be the ideal way to achieve this, but it's cheap and it works, which is
        // great for a feature we're evaluating. (sjb)
        pOwner->ViewPunchReset();
    }
	DispatchParticleEffect( "weapon_glock_muzzleflash", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "muzzle", true);
 
    BaseClass::PrimaryAttack();
 
    // Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
    m_flAccuracyPenalty += GLOCK17_ACCURACY_SHOT_PENALTY_TIME;
 
    m_iPrimaryAttacks++;
    gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::UpdatePenaltyTime( void )
{
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
    if ( pOwner == NULL )
        return;
 
    // Check our penalty time decay
    if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
    {
        m_flAccuracyPenalty -= gpGlobals->frametime;
        m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, GLOCK17_ACCURACY_MAXIMUM_PENALTY_TIME );
    }
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::ItemPreFrame( void )
{
    UpdatePenaltyTime();
 
    BaseClass::ItemPreFrame();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::ItemBusyFrame( void )
{
    UpdatePenaltyTime();
 
    BaseClass::ItemBusyFrame();
}
 
//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponGlock17::ItemPostFrame( void )
{
    BaseClass::ItemPostFrame();
 
    if ( m_bInReload )
        return;
   
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
    if ( pOwner == NULL )
        return;
 
    //Allow a refire as fast as the player can click
    if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
    {
        m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
    }
    else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
    {
        DryFire();
    }
}
 
//-----------------------------------------------------------------------------
// Purpose:
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponGlock17::GetPrimaryAttackActivity( void )
{
    if ( m_nNumShotsFired < 1 )
        return ACT_VM_PRIMARYATTACK;
 
    if ( m_nNumShotsFired < 2 )
        return ACT_VM_PRIMARYATTACK;
 
    if ( m_nNumShotsFired < 3 )
        return ACT_VM_PRIMARYATTACK;
 
    return ACT_VM_PRIMARYATTACK;
}
 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponGlock17::Reload( void )
{
    bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
    if ( fRet )
    {
        WeaponSound( RELOAD );
        m_flAccuracyPenalty = 0.0f;
    }
    return fRet;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGlock17::AddViewKick( void )
{
    CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
   
    if ( pPlayer == NULL )
        return;
 
    QAngle  viewPunch;
 
    viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
    viewPunch.y = random->RandomFloat( -.6f, .6f );
    viewPunch.z = 0.0f;
 
    //Add it to the view punch
    pPlayer->ViewPunch( viewPunch );
}