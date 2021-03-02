#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfplayer.h"
#include "stub/objects.h"
#include "stub/gamerules.h"
#include "util/backtrace.h"
#include "util/iterate.h"

namespace Mod::Etc::Player_Bullet_Bounding_Fix
{

	CCycleCount timespent;
	DETOUR_DECL_STATIC(void, FX_FireBullets, CTFWeaponBase *pWpn, int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
	{
        
        CTimeAdder timer(&timespent);
		std::vector<CBasePlayer *> player_vec;
		ForEachPlayer([&](CBasePlayer *playerl){
			
			if (playerl->IsAlive()) {
                Vector min = VEC_HULL_MIN * 4;
                Vector max = VEC_HULL_MAX * 4;
				UTIL_SetSize(playerl, &min, &max);
				playerl->CollisionProp()->SetSurroundingBoundsType(USE_SPECIFIED_BOUNDS, &min, &max);
				playerl->CollisionProp()->MarkPartitionHandleDirty();
				player_vec.push_back(playerl);
			}
			
		});
		DETOUR_STATIC_CALL(FX_FireBullets)(pWpn, iPlayer, vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
		
		for (CBasePlayer *playerl : player_vec) {
            playerl->CollisionProp()->SetSurroundingBoundsType(USE_SPECIFIED_BOUNDS, &VEC_HULL_MIN, &VEC_HULL_MAX);
			playerl->CollisionProp()->MarkPartitionHandleDirty();
		}
        timer.End();
        DevMsg("Bullet time: %.9f\n", timespent.GetSeconds());
        timespent.Init();
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Player_Bullet_Bounding_Fix")
		{
			MOD_ADD_DETOUR_STATIC(FX_FireBullets, "FX_FireBullets");
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_etc_player_bullet_bounding_fix", "0", FCVAR_NOTIFY,
		"Mod: fix player bounding issues with bullets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}