#ifdef SE_TF2
#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfentities.h"
#include "util/misc.h"
#include "mod/item/item_common.h"

namespace Mod::Perf::Clientside_Wearable_Optimize
{

    THINK_FUNC_DECL(Unfollow)
    {
        auto entity = reinterpret_cast<CEconEntity *>(this);
        auto model = entity->GetModelPtr();
        if (model != nullptr) {
            Msg("Item %d %s %s bones %d\n", entity->GetItem()->m_iItemDefinitionIndex.Get(), entity->GetItem()->GetItemDefinition()->GetName(), GetItemNameForDisplay(entity->GetItem()), model->numbones());
        }
        else {
            this->SetNextThink(gpGlobals->curtime + 0.1f, "Unfollow");
        }
    }

	DETOUR_DECL_MEMBER(void, CBasePlayer_EquipWearable, CEconWearable *wearable)
	{
		DETOUR_MEMBER_CALL(CBasePlayer_EquipWearable)(wearable);
        auto item = wearable->GetItem() ;
        if (item == nullptr) return;
        THINK_FUNC_SET(wearable, Unfollow, gpGlobals->curtime);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Clientside_Wearable_Optimize")
		{
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_EquipWearable, "CBasePlayer::EquipWearable");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_clientside_wearable_optimize", "0", FCVAR_NOTIFY,
		"Mod: make wearables and weapons bonemerge on server side if they have just a single bone",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
#endif