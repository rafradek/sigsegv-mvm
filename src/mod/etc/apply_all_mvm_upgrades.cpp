#include "mod.h"


namespace Mod::Etc::Apply_All_MvM_Upgrades
{
	// TODO: look into fixing attributes by perhaps hooking CTFGameRules::GameModeUsesUpgrades
	// 
	// ALL CALLERS:                                  IMPORTANT?
	// =================================================================================================================
	// CBaseEntity::FireBullets                      YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CObjectSentrygun::Fire                        YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CObjectSentrygun::FireRocket                  YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CTFBaseProjectile::Create                     YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CTFPlayer::FireBullet                         YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CTFWeaponBase::GetProjectileFireSetup         YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CTFWeaponBaseGrenadeProj::VPhysicsUpdate      YES  uses CTraceFilterIgnoreFriendlyCombatItems
	// CTFGrenadeLauncher::LaunchGrenade             NO   plays upgraded shoot sound
	// CTFPipebombLauncher::LaunchGrenade            NO   plays upgraded shoot sound
	// CTFPistol_ScoutPrimary::PlayWeaponShootSound  NO   plays upgraded shoot sound
	// CTFRocketLauncher::FireProjectile             NO   plays upgraded shoot sound
	// CTFShotgun::PlayWeaponShootSound              NO   plays upgraded shoot sound
	// CTFSniperRifle::PlayWeaponShootSound          NO   plays upgraded shoot sound
	// CTFWeaponBase::PlayUpgradedShootSound         NO   plays upgraded shoot sound
	// CTFWeaponBase::Precache                       NO   precaches upgraded shoot sound
	// CBaseObject::CheckUpgradeOnHit                NO   does quick-build time stuff
	// CBaseObject::ShouldBeMiniBuilding             YES  makes extra sentries be mini
	// CObjectDispenser::DispenseAmmo                NO   awards bonus points for dispensing ammo during RND_RUNNING
	// CObjectSapper::Killed                         NO   awards bonus points for removing sappers
	// CObjectTeleporter::TeleporterSend             NO   awards bonus points for teleporting players during RND_RUNNING
	// CTFBaseProjectile::ProjectileTouch            ...
	// CTFPlayer::ApplyPushFromDamage                ...
	// CTFPlayer::CanBuild                           YES  allows building disposable sentry
	// CTFPlayer::OnTakeDamage_Alive                 YES  enables EH, also something related to rage gain
	// CTFPlayer::RegenThink                         YES  enables healing mastery regen
	// CTFPlayer::RememberUpgrade                    NO   remembers upgrades (only works with an info_populator anyway)
	// CTFPlayerShared::SetChargeEffect              YES  does something to ubercharge release rate
	// CTFStunBall::ApplyBallImpactEffectOnVictim    NO   modifies stun parameters based on bot stuff
	// CTFWeaponBase::GetSpreadAngles                YES  does something to weapon overload mechanics
	// CTFWeaponBuilder::StartBuilding               YES  resets robo sapper charge bar when used against bots
	// CUpgrades::NotifyItemOnUpgrade                YES  for heavy "generate rage on damage", sets 50% buff duration
	// CUpgrades::NotifyItemOnUpgrade                YES  for medic "generate rage on heal", sets 120% buff duration
	
	
	// TODO: see if we can directly hook into the game code that knows what the available upgrades are
	// (from mvm_upgrades.txt) and what their increment/cap values are (hmmm... might not work in PvP...)
	
	// OH: using CUpgrades::GrantOrRemoveAllUpgrades sure would be convenient... already does what we want!
	// - would need to create a hidden 'func_upgrade' entity instance if none exists on the map
	//   (or actually... none of the CUpgrades:: member funcs seem to use the `this` ptr at all...)
	
	// TODO: use CanUpgradeWithAttrib and other standard upgrade-application game funcs to the extent feasible
	
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_GameModeUsesUpgrades)
	{
		return true;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Apply_All_MvM_Upgrades")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GameModeUsesUpgrades, "CTFGameRules::GameModeUsesUpgrades");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_apply_all_mvm_upgrades", "0", FCVAR_NOTIFY,
		"Mod: automatically apply all possible MvM upgrades to maximum level, and fix some in PvP mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
