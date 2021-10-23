#include "mod.h"
#include "stub/extraentitydata.h"

namespace Mod::Etc::ExtraEntityData
{
    DETOUR_DECL_MEMBER(void, CBaseEntity_CBaseEntity, bool flag)
	{
        DETOUR_MEMBER_CALL(CBaseEntity_CBaseEntity)(flag);
        /*auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (entity->IsPlayer()) {
            entity->m_extraEntityData = new ExtraEntityDataPlayer(entity);
        }
        else if (entity->IsBaseCombatWeapon()) {
            entity->m_extraEntityData = new ExtraEntityDataCombatWeapon(entity);
        }*/
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_D2)
	{
        DETOUR_MEMBER_CALL(CBaseEntity_D2)();
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (entity->m_extraEntityData != nullptr) {
            delete entity->m_extraEntityData;
        }
    }

    class CMod : public IMod
    {
    public:
        CMod() : IMod("Etc:ExtraEntityData")
        {
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_CBaseEntity, "CBaseEntity::CBaseEntity");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_D2, "~CBaseEntity [D2]");
        }
        
        virtual bool EnableByDefault() override { return true; }
    };
    CMod s_Mod;
}