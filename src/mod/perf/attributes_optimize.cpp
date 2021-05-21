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

    struct AttribCache
    {
        const char *name;
        float in;
        float in_alt;
        float out;
        float out_alt;
        unsigned int call_ctr = 0;
    };

    struct EntityAttribCache
    {
        CBaseEntity *ent;
        CAttributeManager *mgr;
        CUtlVector<AttribCache> attribs;
        CUtlVector<AttribCache> attribs_int;
    };
    
    //CUtlVector<EntityAttribCache> attrib_manager_cache;
    std::vector<EntityAttribCache*> attrib_manager_cache;
    std::unordered_map<CAttributeManager *, EntityAttribCache*> attrib_manager_mgr_cache;
    CBaseEntity *last_entity;
    CBaseEntity *last_entity_2;
    CBaseEntity *next_entity;
    EntityAttribCache *last_manager;
    EntityAttribCache *last_manager_2;
    
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
    float GetAttribValue(float value, const char *attr, CBaseEntity *ent, bool isint) {

        /*calls++;
        if (tick_check + 660 < gpGlobals->tickcount) {
            tick_check = gpGlobals->tickcount;
            DevMsg("calls: %d time spent: %.9f %.9f %.9f entity cache miss: %d, average pos: %f , total reads: %d, moves: %d\n", calls, timespent_entity.GetSeconds() - timespent_test.GetSeconds(), timespent_attr.GetSeconds() - timespent_test.GetSeconds(), timespent_test.GetSeconds(), entity_cache_miss, (float)(average_pos_add)/(float)(average_pos_count), average_pos_count, moves);
            calls = 0;
            entity_cache_miss = 0;
            average_pos_count = 0;
            average_pos_add = 0;
            moves = 0;
            timespent_entity.Init();
            timespent_attr.Init();
            timespent_test.Init();
        }*/
        
        //CTimeAdder timer_test(&timespent_test);
        //timer_test.End();

        EntityAttribCache *entity_cache = nullptr;
        if (last_entity == ent) {
            entity_cache = last_manager;
        }
        else if (last_entity_2 == ent) {
            entity_cache = last_manager_2;
            last_entity_2 = last_entity;
            last_manager_2 = last_manager;
            last_entity = ent;
            last_manager = entity_cache;
        }
        else {
            //CTimeAdder timer(&timespent_entity);
            int count = attrib_manager_cache.size();
            for (int i = 0; i < count; i++) {
                if (ent == attrib_manager_cache[i]->ent) {
                    entity_cache = attrib_manager_cache[i];
                    if (i > 0) {
                        EntityAttribCache *mover = attrib_manager_cache[i-1];
                        attrib_manager_cache[i-1] = entity_cache;
                        attrib_manager_cache[i] = mover;
                    }
                    break;
                }
            }
            //entity_cache_miss++;
            if (entity_cache == nullptr) {
                CAttributeManager *mgr = nullptr;

                if (ent->IsPlayer()) {
                    mgr = reinterpret_cast<CTFPlayer *>(ent)->GetAttributeManager();
                }
                else if (ent->IsBaseCombatWeapon()) {
                    CEconEntity *econent = rtti_cast<CEconEntity *>(ent);
                    if (econent != nullptr) {
                        mgr = econent->GetAttributeManager();
                    }
                }
                else if (ent->IsWearable()) {
                    mgr = reinterpret_cast<CEconEntity *>(ent)->GetAttributeManager();
                }
                
                if (mgr == nullptr) {
                    //timer.End();
                    return value;
                }

                entity_cache = new EntityAttribCache();
                entity_cache->mgr = mgr;
                entity_cache->ent = ent;

                attrib_manager_cache.push_back(entity_cache);
                attrib_manager_mgr_cache[entity_cache->mgr] = entity_cache;

                //DevMsg("Count %d %d\n",attrib_manager_cache.size(), entity_cache->mgr);
            }
            last_entity_2 = last_entity;
            last_manager_2 = last_manager;
            last_entity = ent;
            last_manager = entity_cache;
            //timer.End();
        }


        if (entity_cache == nullptr || entity_cache->mgr == nullptr)
            return value;

        //CTimeAdder timerattr(&timespent_attr);

        auto &attribs = entity_cache->attribs;
        int count = attribs.Count();
        int index_insert = -1;
        //int lastattribcalls = 0;
        for ( int i = 0; i < count; i++ )
        {
            auto &attrib = attribs[i];
            if (attrib.name == attr) {

                float result;
                if (attrib.in == value) {
                    result = attrib.out;
                }
                else if (attrib.in_alt == value) {
                    result = attrib.out_alt;
                }
                else
                    index_insert = i;
                //DevMsg("Incorrect attribute %s value, expected %f = %f, got %f \n", attr, attribs[i].in, attribs[i].out, value);
                //attribs.Remove(i);
                //average_pos_add +=i;
                //average_pos_count++;
                attrib.call_ctr++;
                if (i > 0) {
                    AttribCache &prev = *((&attrib)-1);
                    if (prev.call_ctr < attrib.call_ctr) {
                    //moves++;
                        AttribCache mover = prev;
                        prev = attrib;
                        attrib = mover;
                        index_insert--;
                    }
                }
                if (index_insert >= 0)
                    break;
                else{
                    //timerattr.End();
                    return result;
                }
            }
            //lastattribcalls = attrib.call_ctr;
        }
        float result = entity_cache->mgr->ApplyAttributeFloat(value, ent, AllocPooledString_StaticConstantStringPointer(attr));

        if (index_insert == -1)
            index_insert = attribs.AddToTail();
        else {
            attribs[index_insert].in_alt = attribs[index_insert].in;
            attribs[index_insert].out_alt = attribs[index_insert].out;
        }
        attribs[index_insert].in = value;
        attribs[index_insert].out = result;
        attribs[index_insert].name = attr;

        //timerattr.End();
        return result;
    }

    int callshookfloat = 0;
	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{

        callshookfloat++;
        if (attr == nullptr || ent == nullptr)
            return value;
        
        if (vec != nullptr || !b1){
            DevMsg("Attribute float not static %s %d\n", attr, b1);
            return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_float)(value, attr, ent, vec, b1);
        }

        //DevMsg("attrib %s\n",attr);
        VPROF_BUDGET("AttribHookValueFloatModded", "Attributes"); \
        return GetAttribValue(value, attr, const_cast<CBaseEntity *>(ent), false);
	}
    int callshookint = 0;
    DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
        callshookint++;
        if (attr == nullptr || ent == nullptr)
            return value;
        
        if (vec != nullptr || !b1) {
            DevMsg("Attribute int not static %s %d\n", attr, b1);
            return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_int)(value, attr, ent, vec, b1);
        }

        VPROF_BUDGET("AttribHookValueIntModded", "Attributes"); \
        float result = GetAttribValue(static_cast<float>(value), attr, const_cast<CBaseEntity *>(ent), true);


        return RoundFloatToInt(result);
	}

    std::vector<std::pair<const char *, string_t>> text_cache;
    const char *last_text;
    string_t last_text_string;
    DETOUR_DECL_STATIC(string_t, AllocPooledString_StaticConstantStringPointer, const char *text)
	{
        VPROF_BUDGET("AllocStaticStringPointer", "Attributes");

        if (text == last_text)
            return last_text_string;

        int count = text_cache.size();
        for (int i = 0; i < count; i++) {
            if (text == text_cache[i].first) {
                string_t result = text_cache[i].second;
                if (i > 0) {
                    text_cache[i] = text_cache[i-1];
                    text_cache[i-1] = {text, result};
                }
                return result;
            }
        }
        string_t result = DETOUR_STATIC_CALL(AllocPooledString_StaticConstantStringPointer)(text);
        text_cache.push_back( {text, result} );
        return result;
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

        auto it = attrib_manager_mgr_cache.find(mgr);

        if (it != attrib_manager_mgr_cache.end()) {
            it->second->attribs.Purge();
            it->second->attribs_int.Purge();
        }
	}

    void RemoveAttributeManager(CAttributeManager *mgr) {

        auto it = attrib_manager_mgr_cache.find(mgr);

        if (it != attrib_manager_mgr_cache.end()) {
            last_entity_pos = 1;
            CBaseEntity *toremove = it->second->ent;
            for (auto it2 = attrib_manager_cache.begin(); it2 != attrib_manager_cache.end(); it2++) {
                if ((*it2)->ent == toremove) {
                    attrib_manager_cache.erase(it2);
                    break;
                }
            }
            attrib_manager_mgr_cache.erase(it);
            delete it->second;
        }
    }

    DETOUR_DECL_MEMBER(void, CEconEntity_UpdateOnRemove)
	{
        DETOUR_MEMBER_CALL(CEconEntity_UpdateOnRemove)();

        if (reinterpret_cast<CBaseEntity *>(this) == last_entity) {
            last_entity = nullptr;
        }
        if (reinterpret_cast<CBaseEntity *>(this) == last_entity_2) {
            last_entity_2 = nullptr;
        }
        RemoveAttributeManager(reinterpret_cast<CEconEntity *>(this)->GetAttributeManager());
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
        DETOUR_MEMBER_CALL(CTFPlayer_UpdateOnRemove)();
        
        if (reinterpret_cast<CBaseEntity *>(this) == last_entity) {
            last_entity = nullptr;
        }
        if (reinterpret_cast<CBaseEntity *>(this) == last_entity_2) {
            last_entity_2 = nullptr;
        }
        RemoveAttributeManager(reinterpret_cast<CTFPlayer *>(this)->GetAttributeManager());
    }
    

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Perf:Attributes_Optimize")
		{
			MOD_ADD_DETOUR_MEMBER(CAttributeManager_ClearCache,            "CAttributeManager::ClearCache");
			MOD_ADD_DETOUR_MEMBER(CEconEntity_UpdateOnRemove,              "CEconEntity::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateOnRemove,                "CTFPlayer::UpdateOnRemove");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int,   "CAttributeManager::AttribHookValue<int>");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
        
		}
        
		virtual void OnUnload() override
		{
			for (auto pair : attrib_manager_cache) {
                delete pair;
            }
            attrib_manager_cache.clear();
            attrib_manager_mgr_cache.clear();

            last_entity_pos = 1;
		}
		
		virtual void OnDisable() override
		{
			for (auto pair : attrib_manager_cache) {
                delete pair;
            }
            attrib_manager_cache.clear();
            attrib_manager_mgr_cache.clear();
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

            if (gpGlobals->tickcount % 66 == 0) {
                DevMsg("calls hook: %d %d apply: %d\n", callshookfloat, callshookint, callsapply);
                callshookint = 0;
                callshookfloat = 0;
                callsapply = 0;
            }
            /*auto it_squad = bot_squad_map.begin();
            while (it_squad != bot_squad_map.end()) {
                if (bot.IsAlive)
            }*/
            if (!attrib_manager_mgr_cache.empty()) {
                auto it = attrib_manager_mgr_cache.begin();
                int cache_version = it->first->GetGlobalCacheVersion();
                if(last_cache_version != cache_version) {
                    for (auto pair : attrib_manager_mgr_cache) {
                        it->second->attribs.Purge();
                        it->second->attribs_int.Purge();
                    }
                    last_cache_version = cache_version;
                }
            }
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_attributes_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve attributes performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}