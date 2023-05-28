#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_WEAPON_SHOOT_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_WEAPON_SHOOT_H_


namespace Mod::Common::Weapon_Shoot
{
    CBaseEntity *FireWeapon(CBaseEntity *shooter, CTFWeaponBaseGun *weapon, const Vector &vecOrigin, const QAngle &vecAngles, bool isCrit, bool removeAmmo, int teamDefault = 0);
}
#endif