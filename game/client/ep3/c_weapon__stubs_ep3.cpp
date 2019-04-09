//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "c_basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

STUB_WEAPON_CLASS( cycler_weapon, WeaponCycler, C_BaseCombatWeapon );

STUB_WEAPON_CLASS( weapon_binoculars, WeaponBinoculars, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_extinguisher, WeaponExtinguisher, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_bugbait, WeaponBugBait, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_flaregun, Flaregun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_cguard, WeaponCGuard, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_immolator, WeaponImmolator, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_annabelle, WeaponAnnabelle, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_gauss, WeaponGaussGun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap, WeaponCubemap, C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_alyxgun, WeaponAlyxGun, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_citizenpackage, WeaponCitizenPackage, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_citizensuitcase, WeaponCitizenSuitcase, C_WeaponCitizenPackage );
STUB_WEAPON_CLASS( weapon_ar1, WeaponAR1, C_HLMachineGun );
STUB_WEAPON_CLASS( weapon_smg2, WeaponSMG2, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_hmg1, WeaponHMG1, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_sniperrifle, WeaponSniperRifle, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_thumper, WeaponThumper, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_tripwire, Weapon_Tripwire, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_irifle, WeaponIRifle, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_manhack, Weapon_Manhack, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_iceaxe, WeaponIceAxe, C_BaseHLBludgeonWeapon );
STUB_WEAPON_CLASS( weapon_molotov, WeaponMolotov, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_brickbat, WeaponBrickbat, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_hopwire, WeaponHopwire, C_BaseHLCombatWeapon );

#ifndef HL2MP
STUB_WEAPON_CLASS( weapon_ar2, WeaponAR2, C_HLMachineGun );
STUB_WEAPON_CLASS( weapon_frag, WeaponFrag, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_rpg, WeaponRPG, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_pistol, WeaponPistol, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_shotgun, WeaponShotgun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_smg1, WeaponSMG1, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_357, Weapon357, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_crossbow, WeaponCrossbow, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_slam, Weapon_SLAM, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_crowbar, WeaponCrowbar, C_BaseHLBludgeonWeapon );
//BMS Weapons
STUB_WEAPON_CLASS( weapon_glock17, WeaponGlock17, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_mp5, WeaponMP5, C_HLSelectFireMachineGun );
#endif

#ifdef HL2_EPISODIC
STUB_WEAPON_CLASS( weapon_proto1, WeaponProto1, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_ar3, WeaponAR3, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_oldmanharpoon, WeaponOldManHarpoon, C_WeaponCitizenPackage );
#endif