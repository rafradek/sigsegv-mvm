#include "mod.h"
#include "stub/tfbot.h"
#include "stub/objects.h"
#include "util/misc.h"
#include "util/lua.h"
#include "mod/pop/popmgr_extensions.h"
#include "mod/attr/custom_attributes.h"
#include "mod/etc/mapentity_additions.h"


// Keep a single instance of often called detours for better performance
namespace Mod::Common::Common
{
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		IVision *vision = reinterpret_cast<IVision *>(this);
		CTFBot *actor = static_cast<CTFBot *>(vision->GetBot()->GetEntity());
		
		if (ent != nullptr && actor != nullptr) {
			
			auto attrs = actor->ExtAttr();
			if (attrs[CTFBot::ExtendedAttr::IGNORE_BUILDINGS] && ent->IsBaseObject()) {
				return true;
			}
			else if (attrs[CTFBot::ExtendedAttr::IGNORE_PLAYERS] && ent->IsPlayer()) {
				return true;
			}
			else if (attrs[CTFBot::ExtendedAttr::IGNORE_REAL_PLAYERS] && ent->IsPlayer() && ent->MyNextBotPointer() == nullptr) {
				return true;
			}
			else if (attrs[CTFBot::ExtendedAttr::IGNORE_BOTS] && ent->IsPlayer() && ent->MyNextBotPointer() != nullptr) {
				return true;
			}
			else if (attrs[CTFBot::ExtendedAttr::IGNORE_NPC] && !ent->IsPlayer() && ent->MyNextBotPointer() != nullptr) {
				return true;
			}
            else if (attrs[CTFBot::ExtendedAttr::TARGET_STICKIES]) {
                return !FStrEq(ent->GetClassname(), "tf_projectile_pipe_remote");
            }
		}

		
        auto player = ToTFPlayer(ent);
		int restoreTeam = -1;
		if (player != nullptr) {
			if (actor->GetTeamNumber() == TEAM_SPECTATOR && player->m_Shared->InCond( TF_COND_DISGUISED ) && player->m_Shared->GetDisguiseTeam() != player->GetTeamNumber()) {
				restoreTeam = player->m_Shared->m_nDisguiseTeam;
				player->m_Shared->m_nDisguiseTeam = actor->GetTeamNumber();
			}
		}

		bool ignored = DETOUR_MEMBER_CALL(ent);

		if (restoreTeam != -1) {
			player->m_Shared->m_nDisguiseTeam = restoreTeam;
		}
        if (!ignored && ent->IsBaseObject() && actor->IsPlayerClass(TF_CLASS_SPY) && (ent->GetMoveParent() != nullptr || (Mod::Pop::PopMgr_Extensions::SpyNoSapUnownedBuildings() && ToBaseObject(ent)->GetBuilder() == nullptr))) {
            return false;
        }

		return ignored;
	}

    DETOUR_DECL_MEMBER(bool, CTraceFilterObject_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		CTraceFilterSimple *filter = reinterpret_cast<CTraceFilterSimple*>(this);
		
        // Always a player so ok to cast directly
        CBaseEntity *entityme = reinterpret_cast<CBaseEntity *>(const_cast<IHandleEntity *>(filter->GetPassEntity()));
        
        CBaseEntity *entityhit = EntityFromEntityHandle(pServerEntity);
		if (entityhit == nullptr) return true;

        if (entityme->GetTeamNumber() == TEAM_SPECTATOR) {
			return entityme->GetTeamNumber() != entityhit->GetTeamNumber() || !entityhit->IsPlayer();
		}

		bool result = DETOUR_MEMBER_CALL(pServerEntity, contentsMask);

		if (result) {

			bool entityme_player = entityme->IsPlayer();
			bool entityhit_player = entityhit->IsPlayer();

			if (!entityme_player || (!entityhit_player && !entityhit->IsBaseObject()))
				return true;

			bool me_collide = true;
			bool hit_collide = true;

			int not_solid = GetFastAttributeIntExternal(entityme, 0, Mod::Attr::Custom_Attributes::NOT_SOLID_TO_PLAYERS);
			me_collide = not_solid == 0;

			if (!me_collide)
				return false;

			if (entityhit_player) {
				int not_solid = GetFastAttributeIntExternal(entityhit, 0, Mod::Attr::Custom_Attributes::NOT_SOLID_TO_PLAYERS);
				hit_collide = not_solid == 0;
			}
            
			return hit_collide;
		}
		return result;
	}

    DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, bool, PassServerEntityFilter, IHandleEntity *ent1, IHandleEntity *ent2)
	{
        auto ret = DETOUR_STATIC_CALL(ent1, ent2);
        {
            if (ret) {
                auto entity1 = EntityFromEntityHandle(ent1);
                auto entity2 = EntityFromEntityHandle(ent2);
                
                bool result;
                if (entity1 != entity2 && entity1 != nullptr && entity2 != nullptr)
                {
                    auto mod1 = entity1->GetEntityModule<Util::Lua::LuaEntityModule>("luaentity");
                    if (mod1 != nullptr && Util::Lua::DoCollideTestInternal(entity1, entity2, mod1, result)) {
                        return result;
                    }
                    auto mod2 = entity2->GetEntityModule<Util::Lua::LuaEntityModule>("luaentity");
                    if (mod2 != nullptr && Util::Lua::DoCollideTestInternal(entity2, entity1, mod2, result)) {
                        return result;
                    }

                    variant_t val;
                    if (entity1->GetCustomVariableVariant<"colfilter">(val) && Etc::Mapentity_Additions::DoCollideTestInternal(entity1, entity2, result, val)) {
                        return result;
                    }
                    else if (entity2->GetCustomVariableVariant<"colfilter">(val) && Etc::Mapentity_Additions::DoCollideTestInternal(entity2, entity1, result, val)) {
                        return result;
                    }
                }
            }
        }
        return ret;
    }
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Common:Common")
		{
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored", HIGH);
            MOD_ADD_DETOUR_MEMBER(CTraceFilterObject_ShouldHitEntity, "CTraceFilterObject::ShouldHitEntity");
            MOD_ADD_DETOUR_STATIC(PassServerEntityFilter, "PassServerEntityFilter [clone]");
		}
	};
	CMod s_Mod;
	
	void UpdateEnabledState() {
		static ConVarRef sig_pop_popmgr_extensions("sig_pop_popmgr_extensions");
		static ConVarRef sig_etc_mapentity_additions("sig_etc_mapentity_additions");
		static ConVarRef sig_cond_reprogrammed("sig_cond_reprogrammed");
		static ConVarRef sig_attr_custom("sig_attr_custom");
		static ConVarRef sig_pop_extattr_targetstickies("sig_pop_extattr_targetstickies");
		static ConVarRef sig_pop_extattr_ignoretargets("sig_pop_extattr_ignoretargets");
		static ConVarRef sig_mvm_jointeam_blue_allow("sig_mvm_jointeam_blue_allow");
		static ConVarRef sig_mvm_jointeam_blue_allow_force("sig_mvm_jointeam_blue_allow_force");
        

		s_Mod.Toggle(sig_pop_popmgr_extensions.GetBool() || 
            sig_etc_mapentity_additions.GetBool() || 
            sig_cond_reprogrammed.GetBool() || 
            sig_attr_custom.GetBool() || 
            sig_pop_extattr_targetstickies.GetBool() || 
            sig_pop_extattr_ignoretargets.GetBool() ||
            sig_mvm_jointeam_blue_allow.GetBool() ||
            sig_mvm_jointeam_blue_allow_force.GetBool()
            );
	}
}