#include "mod.h"
#include "stub/projectiles.h"
#include "stub/baseplayer.h"


class CTFWeaponInfo;


namespace Mod::Etc::Limp_Projectiles
{
	DETOUR_DECL_STATIC(CTFGrenadePipebombProjectile *, CTFGrenadePipebombProjectile_Create, const Vector& position, const QAngle& angles, const Vector& velocity, const AngularImpulse& angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo& weaponInfo, int i1, float f1)
	{
		DevMsg("[ %.0f %.0f %.0f ]\n",
			velocity.x, velocity.y, velocity.z);
		
		Vector vel_mod(velocity.x, 0.0f, velocity.z);
		return DETOUR_STATIC_CALL(position, angles, vel_mod, angVelocity, pOwner, weaponInfo, i1, f1);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Limp_Projectiles")
		{
			MOD_ADD_DETOUR_STATIC(CTFGrenadePipebombProjectile_Create, "CTFGrenadePipebombProjectile::Create");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_limp_projectiles", "0", FCVAR_NOTIFY,
		"Etc: make projectiles come out in a disappointingly limp fashion",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
