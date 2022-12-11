#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"
#include "util/iterate.h"
#include "stub/tfbot_behavior.h"
#include "stub/gamerules.h"
#include "stub/misc.h"


namespace Mod::Perf::Attributes_Optimize
{
    inline bool Enabled();

    struct AttribCache
    {
        const char *name;
        float in;
        float out;
    };

    struct EntityAttribCache
    {
        //CBaseEntity *ent;
        CAttributeManager *mgr;
        CUtlVector<AttribCache> attribs;
        const char *last_name = nullptr;
        float last_in;
        float last_out;
        int count = 0;
        //CUtlVector<AttribCache> attribs_int;
    };
    
    EntityAttribCache* attrib_manager_cache_sp[MAX_EDICTS] = {nullptr};
    
    int last_cache_version;

    int tick_check;
    int calls;
    int entity_cache_miss;
    int average_pos_add;
    int average_pos_count;
    int moves;
    CCycleCount timespent_entity;
    CCycleCount timespent_attr;
    CCycleCount timespent_test;
    int last_entity_pos = 1;

    int callscopy = 0;
    int callshookfloat = 0;
    int callshookint = 0;

    EntityAttribCache* CreateNewCacheEntry(CBaseEntity *ent)
    {
        CAttributeManager *mgr = nullptr;

        if (ent->IsPlayer()) {
            mgr = reinterpret_cast<CTFPlayer *>(ent)->GetAttributeManager();
        }
        else if (ent->IsBaseCombatWeapon() || ent->IsWearable()) {
            mgr = reinterpret_cast<CEconEntity *>(ent)->GetAttributeManager();
        }
        
        if (mgr == nullptr) {
            //timer.End();
            return nullptr;
        }

        EntityAttribCache *entity_cache = new EntityAttribCache();
        entity_cache->mgr = mgr;

        //attrib_manager_cache.push_back(entity_cache);
        //attrib_manager_mgr_cache[entity_cache->mgr] = entity_cache;
        return attrib_manager_cache_sp[ENTINDEX(ent)] = entity_cache;
    }

    float CalculateAttribValue(CBaseEntity *ent, float value, const char *attr, int index_insert, EntityAttribCache* entity_cache)
    {
        CAttributeManager *mgr = nullptr;
        if (ent->IsPlayer()) {
            mgr = reinterpret_cast<CTFPlayer *>(ent)->GetAttributeManager();
        }
        else if (ent->IsBaseCombatWeapon() || ent->IsWearable()) {
            mgr = reinterpret_cast<CEconEntity *>(ent)->GetAttributeManager();
        }
        if (mgr == nullptr)
            return value;
            
        float result = mgr->ApplyAttributeFloat(value, ent, AllocPooledString_StaticConstantStringPointer(attr));

        auto &attribs = entity_cache->attribs;

        if (index_insert == -1)
            index_insert = attribs.AddToTail();

        entity_cache->count = attribs.Count();
        
        attribs[index_insert].in = value;
        attribs[index_insert].out = result;
        attribs[index_insert].name = attr;

        entity_cache->last_name = attr;
        entity_cache->last_in = value;
        entity_cache->last_out = result;
       //timerattr.End();
        return result;
    }

    inline float GetAttribValue(float value, const char *attr, CBaseEntity *ent, bool isint) {
        // if (isint) {
        //     callshookint++;
        // }
        // else {
        //     callshookfloat++;
        // }
        // calls++;
        // if (tick_check + 660 < gpGlobals->tickcount) {
        //     tick_check = gpGlobals->tickcount;
        //     DevMsg("calls: %d time spent: %.9f %.9f %.9f, average pos: %f , total reads: %d, moves: %d\n", calls, (timespent_entity.GetSeconds() - timespent_test.GetSeconds()) / calls, (timespent_attr.GetSeconds() - timespent_test.GetSeconds()) / calls, timespent_test.GetSeconds() / calls, (float)(average_pos_add)/(float)(average_pos_count), average_pos_count, moves);
        //     calls = 0;
        //     entity_cache_miss = 0;
        //     average_pos_count = 0;
        //     average_pos_add = 0;
        //     moves = 0;
        //     timespent_entity.Init();
        //     timespent_attr.Init();
        //     timespent_test.Init();
        // }
        
        //CTimeAdder timer_test(&timespent_test);
        //timer_test.End();

        //CTimeAdder timer(&timespent_entity);
        EntityAttribCache *entity_cache = nullptr;
        entity_cache = attrib_manager_cache_sp[ent->entindex()];
        if (entity_cache == nullptr) {
            entity_cache = CreateNewCacheEntry(ent);
            if (entity_cache == nullptr) {
                return value;
            }
            //DevMsg("Count %d %d\n",attrib_manager_cache.size(), entity_cache->mgr);
        }
        //timer.End();


        //if (entity_cache == nullptr)
        //    return value;

        //CTimeAdder timerattr(&timespent_attr);

        if (entity_cache->last_name == attr && entity_cache->last_in == value) {
            //callscopy += 1;
            //timerattr.End();
            return entity_cache->last_out;
        }

        auto &attribs = entity_cache->attribs;

        int count = entity_cache->count;
        int index_insert = -1;
        //int lastattribcalls = 0;
        for ( int i = 0; i < count; i++ )
        {
            auto &attrib = attribs[i];
            if (attrib.name == attr) {

                //
                if (attrib.in == value) {
                    float result = attrib.out;
                    entity_cache->last_name = attr;
                    entity_cache->last_in = value;
                    entity_cache->last_out = result;
                    //timerattr.End();
                    return result;
                }
                else {
                    index_insert = i;
                    break;
                }
                //else if (attrib.in_alt == value) {
                //    result = attrib.out_alt;
                //}
                //else
                //   

                // attrib.call_ctr++;
                // if (i > 0) {
                //     AttribCache &prev = *((&attrib)-1);
                //     if ((prev.call_ctr + 1) < attrib.call_ctr) {
                //     //moves++;
                //         AttribCache mover = prev;
                //         prev = attrib;
                //         attrib = mover;
                //         index_insert--;
                //     }
                // }
                // if (index_insert >= 0)
                //     break;
                // else{
                //     //timerattr.End();
                    

                //     return result;
                // }
            }
            //lastattribcalls = attrib.call_ctr;
        }
        return CalculateAttribValue(ent, value, attr, index_insert, entity_cache);
    }

    float GetAttribValueOptimizedIfEnabled(float value, const char *attr, CBaseEntity *ent, bool isint, bool literalString) {
        if (ent == nullptr) return value;

        if (!Enabled() || !literalString) {
            return CAttributeManager::ft_AttribHookValue_float(value, attr, ent, nullptr, literalString);
        }
        return GetAttribValue(value, attr, ent, isint);
    }

	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
        if (attr == nullptr || ent == nullptr)
            return value;
        
        if (vec != nullptr || !b1){
            return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_float)(value, attr, ent, vec, b1);
        }

        return GetAttribValue(value, attr, const_cast<CBaseEntity *>(ent), false);
	}
    DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
        //callshookint++;
        if (attr == nullptr || ent == nullptr)
            return value;
        
        if (vec != nullptr || !b1) {
            return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_int)(value, attr, ent, vec, b1);
        }

        return RoundFloatToInt(GetAttribValue(static_cast<float>(value), attr, const_cast<CBaseEntity *>(ent), true));
	}

    int callsapply = 0;
    DETOUR_DECL_MEMBER(float, CAttributeManager_ApplyAttributeFloatWrapper, float val, CBaseEntity *ent, string_t name, CUtlVector<CBaseEntity *> *vec)
	{
        callsapply++;
		
		return DETOUR_MEMBER_CALL(CAttributeManager_ApplyAttributeFloatWrapper)(val, ent, name, vec);
	}

    DETOUR_DECL_MEMBER(void, CAttributeManager_ClearCache)
	{
        DETOUR_MEMBER_CALL(CAttributeManager_ClearCache)();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);

        if (mgr->m_hOuter != nullptr) {
            auto cache = attrib_manager_cache_sp[mgr->m_hOuter->entindex()];
            if (cache != nullptr) {
                cache->last_name = nullptr;
                cache->attribs.Purge();
                cache->count = 0;
                //cache->attribs_int.Purge();
            }
        }
	}

    void RemoveAttributeManager(CBaseEntity *entity) {
        
        int index = ENTINDEX(entity);
        if (attrib_manager_cache_sp[index] != nullptr) {
            delete attrib_manager_cache_sp[index];
            attrib_manager_cache_sp[index] = nullptr;
        }
    }

    DETOUR_DECL_MEMBER(void, CEconEntity_UpdateOnRemove)
	{
        DETOUR_MEMBER_CALL(CEconEntity_UpdateOnRemove)();
        RemoveAttributeManager(reinterpret_cast<CBaseEntity *>(this));
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
        DETOUR_MEMBER_CALL(CTFPlayer_UpdateOnRemove)();
        RemoveAttributeManager(reinterpret_cast<CBaseEntity *>(this));
    }
    

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Perf:Attributes_Optimize")
		{
			MOD_ADD_DETOUR_MEMBER(CAttributeManager_ClearCache,            "CAttributeManager::ClearCache");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CEconEntity_UpdateOnRemove,     "CEconEntity::UpdateOnRemove", LOWEST);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_UpdateOnRemove,       "CTFPlayer::UpdateOnRemove", LOWEST);
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int,   "CAttributeManager::AttribHookValue<int>");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
		}
        
		virtual void OnUnload() override
		{
			// for (auto pair : attrib_manager_cache) {
            //     delete pair;
            // }
            //attrib_manager_cache.clear();
            //attrib_manager_mgr_cache.clear();

            last_entity_pos = 1;
		}
		
		virtual void OnDisable() override
		{
			// for (auto pair : attrib_manager_cache) {
            //     delete pair;
            // }
            //attrib_manager_cache.clear();
            //attrib_manager_mgr_cache.clear();
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
        
        int last_cache_version = 0;
        
        virtual void FrameUpdatePostEntityThink() override
		{
            /*auto it_bot_update = update_mark.begin();
            while (it_bot_update != update_mark.end()) {
                auto &bot = it_bot_update->first;
                auto &counter = it_bot_update->second;

                if (bot == nullptr || bot->IsMarkedForDeletion()) {
                    it_bot_update = update_mark.erase(it_bot_update);
                    continue;
                }

                counter--;

                if (counter <= 0) {
                    reinterpret_cast<NextBotData *>(bot->MyNextBotPointer())->m_bScheduledForNextTick = true;
                    it_bot_update = update_mark.erase(it_bot_update);
                }
                else
                {
                    it_bot_update++;
                }
            }*/

            // if (gpGlobals->tickcount % 66 == 0) {
            //     DevMsg("calls hook: %d %d apply: %d total: %d copies: %d\n", callshookfloat, callshookint, callsapply, calls, callscopy);
            //     callshookint = 0;
            //     callshookfloat = 0;
            //     callsapply = 0;
            // }
            /*auto it_squad = bot_squad_map.begin();
            while (it_squad != bot_squad_map.end()) {
                if (bot.IsAlive)
            }*/
            
            // The function does not make use of this pointer, so its safe to convert to CAttributeManager
            int cache_version = reinterpret_cast<CAttributeManager *>(this)->GetGlobalCacheVersion();

            if(last_cache_version != cache_version) {
                for (auto cache : attrib_manager_cache_sp) {
                    if (cache != nullptr) {
                        cache->last_name = nullptr;
                        cache->attribs.Purge();
                        cache->count = 0;
                        //cache->attribs_int.Purge();
                    }
                }
                last_cache_version = cache_version;
            }
        }
	};
	CMod s_Mod;
	
    inline bool Enabled() {
        return s_Mod.IsEnabled();
    }
    
	ConVar cvar_enable("sig_perf_attributes_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve attributes performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}