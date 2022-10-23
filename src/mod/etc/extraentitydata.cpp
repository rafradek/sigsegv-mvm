#include "mod.h"
#include "stub/extraentitydata.h"
#include "util/iterate.h"


void CBaseEntity::AddCustomOutput(const char *key, const char *value)
{
    std::string namestr = key;
    boost::algorithm::to_lower(namestr);

    auto &list = GetExtraData(this)->GetCustomOutputs();
    
    bool found = false;
    for (auto &var : list) {
        if (STRING(var.key) == namestr.c_str() || stricmp(namestr.c_str(), STRING(var.key)) == 0) {
            var.output.ParseEventAction(value);
            found = true;
            break;
        }
    }
    if (!found) {
        CustomOutput output {AllocPooledString(namestr.c_str())};
        output.output.ParseEventAction(value);
        list.push_back(output);
    }
}

void CBaseEntity::RemoveCustomOutput(const char *key)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        std::string namestr = key;
        boost::algorithm::to_lower(namestr);

        auto &list = data->GetCustomOutputs();
        
        bool found = false;
        for (auto it = list.begin(); it != list.end(); it++) {
            if (STRING(it->key) == namestr.c_str() || stricmp(namestr.c_str(), STRING(it->key)) == 0) {
                list.erase(it);
                return;
            }
        }
    }
}

void CBaseEntity::RemoveAllCustomOutputs()
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        data->GetCustomOutputs().clear();
    }
}

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
        
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (entity->m_extraEntityData != nullptr) {
            delete entity->m_extraEntityData;
            entity->m_extraEntityData = nullptr;
        }
        DETOUR_MEMBER_CALL(CBaseEntity_D2)();
    }

    class CMod : public IMod
    {
    public:
        CMod() : IMod("Etc:ExtraEntityData")
        {
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_CBaseEntity, "CBaseEntity::CBaseEntity");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_D2, "~CBaseEntity [D2]");
        }
        
		virtual void OnUnload() override
		{
			ForEachEntity([](CBaseEntity *entity) {
                if (entity->m_extraEntityData != nullptr) {
                    delete entity->m_extraEntityData;
                    entity->m_extraEntityData = nullptr;
                }
            });
		}
        
        virtual bool EnableByDefault() override { return true; }
    };
    CMod s_Mod;
}