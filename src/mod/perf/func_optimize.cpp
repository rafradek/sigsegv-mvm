#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#ifdef SE_IS_TF2
#include "stub/tfplayer.h"
#endif
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#ifdef SE_IS_TF2
#include "stub/tfweaponbase.h"
#include "stub/tfbot.h"
#include "stub/strings.h"
#endif
#include "stub/extraentitydata.h"
#include "stub/nav.h"
#include "sdk2013/mempool.h"
#include "mem/protect.h"
#include "util/prop_helper.h"
#include <utlsymbol.h>
#include "stub/trace.h"
#include "util/iterate.h"
#include "../mvm-reversed/server/NextBot/NextBotKnownEntity.h"
#include "collisionutils.h"
#include "collisionutils.cpp"
#include "stub/particles.h"

#ifdef SE_IS_TF2
REPLACE_FUNC_MEMBER(bool, CTFPlayerShared_InCond, int index) {
    auto me = reinterpret_cast<CTFPlayerShared *>(this);
    return me->GetCondData().InCond(index);
}
REPLACE_FUNC_MEMBER(CEconItemDefinition *, CEconItemView_GetStaticData) {
    auto me = reinterpret_cast<CEconItemView *>(this);
    return GetItemSchema()->GetItemDefinition(me->m_iItemDefinitionIndex);
}
#endif

REPLACE_FUNC_MEMBER(bool, CKnownEntity_OperatorEquals, const CKnownEntity &other) {
    auto me = reinterpret_cast<CKnownEntity *>(this);
    return me->IsEqualInline(other);
}

namespace Mod::Perf::Func_Optimize
{   

    constexpr uint8_t s_Buf_CMDLCache_BeginEndLock[] = {
        0xc3, 0x90,                      //ret
	};

    struct CPatch_CMDLCache_BeginEndLock : public CPatch
	{
		CPatch_CMDLCache_BeginEndLock(std::string name) : CPatch(sizeof(s_Buf_CMDLCache_BeginEndLock)), name(name)  {}
		
		virtual const char *GetFuncName() const override { return name.c_str(); }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CMDLCache_BeginEndLock);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CMDLCache_BeginEndLock);
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			buf.CopyFrom(s_Buf_CMDLCache_BeginEndLock);
			return true;
		}
        std::string name;
	};

#ifdef SE_IS_TF2

	constexpr uint8_t s_Buf_CTFPlayerShared_ConditionGameRulesThink[] = {
        0x81, 0xfb, 0x82, 0x00, 0x00, 0x00,  // +0x0000 cmp     ebx, TF_COND_COUNT - 1
        0x0f, 0x84, 0xd3, 0x00, 0x00, 0x00,  // +0x0006 jz      loc_8C9ABF
        0x83, 0xc3, 0x01,                    // +0x000c add     ebx, 1
    };

	struct CPatch_CTFPlayerShared_ConditionGameRulesThink: public CPatch
	{
		CPatch_CTFPlayerShared_ConditionGameRulesThink() : CPatch(sizeof(s_Buf_CTFPlayerShared_ConditionGameRulesThink)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::ConditionGameRulesThink"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0060; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0140; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPlayerShared_ConditionGameRulesThink);
            // buf[0x3 + 2] = sizeof(condition_source_t);
            buf.SetDword(0x0 + 2, GetNumberOfTFConds() - 1);
			
			/* allow any 3-bit destination register code */
			mask[0x0 + 1] = 0b11111000;
			mask[0xc + 1] = 0b11111000;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetDword(0x00 + 2, 0x01);
			
			mask.SetDword(0x0 + 2, 0xffffffff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			buf.SetDword(0x00 + 2, 0x01);
			
			return true;
		}
	};
    
    constexpr uint8_t s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot[] = {
#ifdef PLATFORM_64BITS
        0x48, 0x8d, 0x15, 0x49, 0xd3, 0xdb, 0x00,  // +0x0000 lea     rdx, _ZTI11CTFWearable; lpdtype
        0x31, 0xc9,                                // +0x0007 xor     ecx, ecx; s2d
        0x48, 0x8d, 0x35, 0x68, 0xb3, 0xd9, 0x00,  // +0x0009 lea     rsi, _ZTI13CEconWearable; lpstype
        0xe8, 0xb3, 0x37, 0xb9, 0xff,              // +0x0010 call    ___dynamic_cast
#else
        0x6A, 0x00, // 0
        0x68, 0xBC, 0x08, 0x21, 0x01, // 2
        0x68, 0xB8, 0x11, 0x1E, 0x01, // 7
        0x50, // C
        0xE8, 0x5D, 0x0D, 0x0C, 0x00 // d
#endif
	};
	struct CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot : public CPatch
	{
		CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot() : CPatch(sizeof(s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::GetEquippedWearableForLoadoutSlot"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot);
			
#ifdef PLATFORM_64BITS
			mask.SetDword(0x00 + 3, 0);
			mask.SetDword(0x07 + 1, 0);
			mask.SetDword(0x09 + 3, 0);
			mask.SetDword(0x10 + 1, 0);
#else
			mask.SetDword(0x02 + 1, 0);
			mask.SetDword(0x07 + 1, 0);
			mask.SetDword(0x0d + 1, 0);
#endif
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
            // change to mov rax, rdi , nop
			buf[0x10] = 0x48;
            buf[0x11] = 0x89;
            buf[0x12] = 0xf8;
            buf[0x13] = 0x90;
            buf[0x14] = 0x90;
			mask.SetRange(0x10, 5, 0xFF);
#else
			buf.SetRange(0x0d, 5, 0x90);
			mask.SetRange(0x0d, 5, 0xFF);
#endif
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
#ifndef PLATFORM_64BITS
			buf.SetRange(0x0d, 5, 0x90);
#endif
			return true;
		}
	};

	DETOUR_DECL_MEMBER(int, CTFPlayerShared_GetCarryingRuneType)
	{
		return reinterpret_cast<CTFPlayerShared *>(this)->GetCarryingRuneType();
		//return DETOUR_MEMBER_CALL();
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_ConditionGameRulesThink)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
        {
            int maxCond = GetNumberOfTFConds();
            auto condData = shared->GetCondData();
            for (int i = 0; i < maxCond; i++) {
                if (condData.InCond(i) && !(i < 32 && (shared->m_ConditionList->_condition_bits & (1 << i)) )) {
                    auto &data = shared->m_ConditionData.Get()[i];
                    float duration = data.m_flExpireTime;
                    if (duration != -1) {
                        data.m_flExpireTime -= gpGlobals->frametime;

                        if ( data.m_flExpireTime <= 0 )
                        {
                            shared->RemoveCond((ETFCond)i);
                        }
                    }
                }
            }
        }
		DETOUR_MEMBER_CALL();
	}

    DETOUR_DECL_MEMBER(bool, CTFConditionList_Add, ETFCond cond, float duration, CTFPlayer *player, CBaseEntity *provider)
	{
		bool success = DETOUR_MEMBER_CALL(cond, duration, player, provider);
        if (success) {
            player->m_Shared->GetCondData().AddCondBit(cond);
        }
        return success;
	}

    DETOUR_DECL_MEMBER(bool, CTFConditionList_Remove, ETFCond cond, bool ignore)
	{
		bool success = DETOUR_MEMBER_CALL(cond, ignore);
        if (success) {
            auto outer = (CTFPlayerShared *)((char *)this - CTFPlayerShared::s_prop_m_ConditionList.GetOffsetDirect());
            outer->GetCondData().RemoveCondBit(cond);
        }
        return success;
	}


    CEconItemSchema *schema = nullptr;
    void OptimizeGetSchema(void *schemaptr, uint8_t* callerAddr, uintptr_t funcAddr)
    {
        uintptr_t offset = funcAddr - (uintptr_t)callerAddr;
        if (offset == *((uintptr_t*)callerAddr - 1)) {
            ByteBuf buf(5);
            buf[0] = 0xB8;
            buf.SetDword(1, (uintptr_t)schemaptr);
            MemProtModifier_RX_RWX(callerAddr-5, 5);
            memcpy(callerAddr-5, buf.CPtr(), 5);
        }
    }

    uintptr_t GEconItemSchema_addr = 0;
    DETOUR_DECL_STATIC(CEconItemSchema *, GEconItemSchema)
	{
		schema = DETOUR_STATIC_CALL();
        auto addr = (uint8_t*)__builtin_return_address(0);
        OptimizeGetSchema(schema, addr, GEconItemSchema_addr);
        // Msg("addr %d %d %d\n", *(addr2-1), AddrManager::GetAddr("GEconItemSchema"), *(addr2-1) + LibMgr::GetInfo(Library::SERVER).GetSeg(Segment::TEXT).AddrBegin());
        // static bool f = [addr](){
        //     Msg("\nFrame 0a:");
        //     for (int i = -16; i<16;i++) {
        //         Msg(" %02hhX", *(addr+i));
        //     }
        //     Msg("\n");
        //     raise(SIGTRAP);
        // return true;
        // }();
        return schema;
	}

    uintptr_t GetItemSchema_addr = 0;
    DETOUR_DECL_STATIC(CEconItemSchema *, GetItemSchema)
	{
		schema = DETOUR_STATIC_CALL();
        auto addr = (uint8_t*)__builtin_return_address(0);
        OptimizeGetSchema(schema, addr, GetItemSchema_addr);
        return schema;
	}

    DETOUR_DECL_MEMBER(CTFItemDefinition *, CEconItemView_GetStaticData)
	{
		auto me = reinterpret_cast<CEconItemView *>(this);
        return static_cast<CTFItemDefinition *>(schema->GetItemDefinition(me->m_iItemDefinitionIndex));
	}

    bool useOrig = false;
    DETOUR_DECL_MEMBER(void, CBasePlayer_EquipWearable, CEconWearable *wearable)
	{
		DETOUR_MEMBER_CALL(wearable);
        auto item = wearable->GetItem() ;
        if (item == nullptr) return;

        auto itemDef = item->GetItemDefinition() ;
        if (itemDef == nullptr) return;

        auto player = reinterpret_cast<CTFPlayer *>(this);
        auto data = GetExtraPlayerData(player);
        auto slot = itemDef->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
        if (slot == -1 && itemDef->m_iItemDefIndex != 0) {
            slot = itemDef->GetLoadoutSlot(TF_CLASS_UNDEFINED);
        }
        if (slot >= 0 && slot < LOADOUT_POSITION_COUNT)
            data->quickItemInLoadoutSlot[slot] = wearable;
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_Weapon_Equip, CBaseCombatWeapon *weapon)
	{
		DETOUR_MEMBER_CALL(weapon);
        auto item = weapon->GetItem() ;
        if (item == nullptr) return;

        auto itemDef = item->GetItemDefinition();
        if (itemDef == nullptr) return;

        auto player = reinterpret_cast<CTFPlayer *>(this);
        auto data = GetExtraPlayerData(player);
        auto slot = itemDef->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
        if (slot == -1 && itemDef->m_iItemDefIndex != 0) {
            slot = itemDef->GetLoadoutSlot(TF_CLASS_UNDEFINED);
        }
        if (slot >= 0 && slot < LOADOUT_POSITION_COUNT && (data->quickItemInLoadoutSlot[slot] == nullptr || data->quickItemInLoadoutSlot[slot]->GetOwnerEntity() != player) ) {
            data->quickItemInLoadoutSlot[slot] = weapon;
        }
	}

    DETOUR_DECL_MEMBER(CBaseEntity *, CTFPlayer_GetEntityForLoadoutSlot, int slot)
	{
        if (useOrig) return DETOUR_MEMBER_CALL(slot);
        
        auto player = reinterpret_cast<CTFPlayer *>(this);
        auto data = GetExtraPlayerData(player, false);
        if (data != nullptr) {
            auto handle = data->quickItemInLoadoutSlot[slot];
            auto ent = handle.Get();
            if (handle.IsValid() && (ent == nullptr || ent->GetOwnerEntity() != player)) {
                ent = static_cast<CEconEntity *>(DETOUR_MEMBER_CALL(slot));
                data->quickItemInLoadoutSlot[slot] = ent;
            }
            return ent;
        }
        return nullptr;
	}
#endif

    edict_t *world_edict = nullptr;
    DETOUR_DECL_STATIC(CBasePlayer *, UTIL_PlayerByIndex, int slot)
	{
        if (slot <= 0 || slot > gpGlobals->maxClients) return nullptr;
        auto edict = world_edict + slot;
        if (edict == nullptr || edict->IsFree()) return nullptr;

        return (CBasePlayer *)edict->GetUnknown();
    }

    RefCount rc_CBaseEntity_PrecacheModel;
    DETOUR_DECL_STATIC(int, CBaseEntity_PrecacheModel, const char *model)
	{
        int index = modelinfo->GetModelIndex(model);
        if (index != -1 && !IsDynamicModelIndex(index)) {
            return index;
        }
        return DETOUR_STATIC_CALL(model);
    }

    class CModelLoader {};
    GlobalThunk<CModelLoader *> modelloader("modelloader");
    MemberFuncThunk<CModelLoader *, void> CModelLoader_FlushDynamicModels("CModelLoader::FlushDynamicModels");
    DETOUR_DECL_MEMBER(int, CModelInfoServer_RegisterDynamicModel, const char *model, bool isClient)
	{
        int index = modelinfo->GetModelIndex(model);
        if (index != -1) {
            return index;
        }
        auto ret = DETOUR_MEMBER_CALL(model, isClient);
        CModelLoader_FlushDynamicModels(modelloader.GetRef());
        return ret;
        //return CBaseEntity::PrecacheModel(model);
        // if (useOrig) return DETOUR_MEMBER_CALL(model, isClient);
        
        // auto player = reinterpret_cast<CTFPlayer *>(this);
        // auto data = GetExtraPlayerData(player, false);
        // if (data != nullptr) {
        //     auto handle = data->quickItemInLoadoutSlot[slot];
        //     auto ent = handle.Get();
        //     if (handle.IsValid() && (ent == nullptr || ent->GetOwnerEntity() != player)) {
        //         ent = static_cast<CEconEntity *>(DETOUR_MEMBER_CALL(slot));
        //         data->quickItemInLoadoutSlot[slot] = ent;
        //     }
        //     return ent;
        // }
        // return nullptr;
	}
    DETOUR_DECL_MEMBER(void, CMDLCache_BeginLock)
	{
        AVERAGE_TIME(begin);
        DETOUR_MEMBER_CALL();
    }
    DETOUR_DECL_MEMBER(void, CMDLCache_EndLock)
	{
        AVERAGE_TIME(end);
        DETOUR_MEMBER_CALL();
        
    }
    
#ifdef SE_IS_TF2
    DETOUR_DECL_STATIC(void, CTFPlayer_PrecacheMvM)
	{
        DETOUR_STATIC_CALL();
        for (int i = 1; i < TF_CLASS_COUNT; i++) {
            auto hatDef = GetItemSchema()->GetItemDefinitionByName(g_szRomePromoItems_Hat[i]);
            auto hatModel = hatDef != nullptr ? hatDef->GetKeyValues()->GetString("model_player") : nullptr;
            if (hatModel != nullptr) {
                CBaseEntity::PrecacheModel(hatModel);
            }
            auto miscDef = GetItemSchema()->GetItemDefinitionByName(g_szRomePromoItems_Misc[i]);
            auto miscModel = miscDef != nullptr ? miscDef->GetKeyValues()->GetString("model_player") : nullptr;
            if (miscModel != nullptr) {
                CBaseEntity::PrecacheModel(miscModel);
            }
        }
    }
    DETOUR_DECL_MEMBER(bool, CTFPlayer_SpeakConceptIfAllowed, int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
	{
        if (iConcept == MP_CONCEPT_HURT) {
            auto player = reinterpret_cast<CTFPlayer *>(this);
            auto data = GetExtraPlayerData(player);
            if (data->lastHurtSpeakTime + 0.25f > gpGlobals->curtime) {
                return false;
            }
            data->lastHurtSpeakTime = gpGlobals->curtime;
        }
		bool ret;
        {
            //AVERAGE_TIME2(SpeakConcept);
            ret = DETOUR_MEMBER_CALL(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
        }
        return ret;
	}

    uintptr_t CThreadLocalBase_Get_addr = 0;
    DETOUR_DECL_MEMBER(void *, CThreadLocalBase_Get)
	{
        auto addrA = &pthread_getspecific;
        uintptr_t caller = (uintptr_t)__builtin_return_address(0);
        uint8_t *caller2 = (uint8_t *)__builtin_return_address(0);
        
            printf("Pre-Fix:");
            for (int i = -16; i<16;i++) {
                printf(" %02hhX", *(caller2+i));
            }
            printf("\n");
        // uintptr_t offset = CThreadLocalBase_Get_addr - caller;
        // Msg("Addr %d caller addr %d", CThreadLocalBase_Get_addr, *((uintptr_t*)caller - 1) + caller);
        // if (offset == *((uintptr_t*)caller - 1)) {

            // ByteBuf buf(5);
            // buf[0] = 0x9A;
            // buf.SetDword(1, (uintptr_t)addrA);
            // MemProtModifier_RX_RWX((void *)(caller-5), 5);
            // memcpy((void *)(caller-5), buf.CPtr(), 5);
        //}
		return DETOUR_MEMBER_CALL();
	}

    DETOUR_DECL_MEMBER(void, CTFBot_AvoidPlayers, CUserCmd *cmd)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		if ((bot->entindex() + gpGlobals->tickcount) % 8 == 0) {
            float forwardPre = cmd->forwardmove;
            float sidePre = cmd->sidemove;
            DETOUR_MEMBER_CALL(cmd);
            auto data = GetExtraBotData(bot);
            data->lastForwardMove = cmd->forwardmove - forwardPre;
            data->lastSideMove = cmd->sidemove - sidePre;
        }
        else {
            auto data = GetExtraBotData(bot);
            cmd->forwardmove += data->lastForwardMove;
            cmd->sidemove += data->lastSideMove;
        }
		
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_OnNavAreaChanged, CNavArea *enteredArea, CNavArea *leftArea)
	{
        VPROF_BUDGET("CTFPlayer::OnNavAreaChanged", "NextBot")
        auto player = reinterpret_cast<CTFPlayer *>(this);
		if (!player->IsAlive())
        {
            return;
        }

        std::vector<CNavArea *> potentiallylVisibleAreas;
        potentiallylVisibleAreas.reserve(enteredArea ? enteredArea->m_potentiallyVisibleAreas.Get().Count() + (enteredArea->m_inheritVisibilityFrom->area ? enteredArea->m_inheritVisibilityFrom->area->m_potentiallyVisibleAreas->Count() : 0) : 0);
        //timer4.End();
        
        //CCycleCount time1;
        //CCycleCount time2;
        //CCycleCount time3;
        
        //CTimeAdder timer1(&time1);
        if ( enteredArea )
        {
            ForAllPotentiallyVisibleAreasReverse([&](CNavArea *area){
                potentiallylVisibleAreas.push_back(area);

            }, enteredArea);
        }
        //timer1.End();

        //CTimeAdder timer2(&time2);
        if ( leftArea )
        {
            ForAllPotentiallyVisibleAreas([&](CNavArea *area)__attribute__((always_inline)){
                bool found = false;
                
                for (auto it = potentiallylVisibleAreas.end() - 1; it != potentiallylVisibleAreas.begin() - 1; it--) {
                    if (*it == area) {
                        potentiallylVisibleAreas.erase(it);
                        found = true;
                        break;
                    }
                    //if (it == potentiallylVisibleAreas.begin()) break;
                }
                if (!found) {
                    auto &actors = ((CTFNavArea *)area)->m_potentiallyVisibleActor.Get();
                    for(int i = 0; i < 4; i++) {
                        actors[i].FindAndFastRemove(player);
                    }
                }
                
            }, leftArea);
        }
        
        //timer2.End();
        //CTimeAdder timer3(&time3);
        for (auto area : potentiallylVisibleAreas) {
            ((CTFNavArea *)area)->AddPotentiallyVisibleActor(player);
        }
        //timer3.End();
       // Msg("New area time %.9f %.9f %.9f %.9f\n", time1.GetSeconds(), time2.GetSeconds(), time3.GetSeconds(), time4.GetSeconds());
	}

	CTFBot *bot_additem;
	int bot_classnum = TF_CLASS_UNDEFINED;
	const char *item_name;
    RefCount rc_CTFBot_AddItem;
	std::unordered_map<std::string, CTFItemDefinition*> item_defs;
    DETOUR_DECL_MEMBER(void, CTFBot_AddItem, const char *item)
	{
		SCOPED_INCREMENT(rc_CTFBot_AddItem);
		//clock_t start = clock();
		item_name = item;
		bot_additem = reinterpret_cast<CTFBot *>(this);
		bot_classnum = bot_additem->GetPlayerClass()->GetClassIndex();
		DETOUR_MEMBER_CALL(item);
	}

    DETOUR_DECL_MEMBER(void *, CItemGeneration_GenerateRandomItem, void *criteria, const Vector &vec, const QAngle &ang, const char *name)
	{
		if (rc_CTFBot_AddItem > 0) {
            CTFItemDefinition *item_def = nullptr;
            auto find = item_defs.find(item_name);
            if (find != item_defs.end()) {
                item_def = find->second;
            }
            else {
                item_def = static_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(item_name));
                item_defs[item_name] = item_def;
            }
            if (item_def != nullptr) {
                return ItemGeneration()->SpawnItem(item_def->m_iItemDefIndex,vec, ang, 6, 9999, item_def->GetItemClass());
            }
        }
        return DETOUR_MEMBER_CALL(criteria,vec,ang, name);
    }
	std::unordered_map<std::string, CEconItemAttributeDefinition *> attributeCache;

    DETOUR_DECL_MEMBER(CEconItemAttributeDefinition *, CEconItemSchema_GetAttributeDefinitionByName, const char *name)
	{
        if (name == nullptr) return nullptr;

        std::string str {name};
        StrLowerASCII(str.data());
        auto find = attributeCache.find(str);
        if (find != attributeCache.end()) {
            return find->second;
        }
		auto attr = DETOUR_MEMBER_CALL(name);
        attributeCache[str] = attr;
        return attr;
    }

    DETOUR_DECL_MEMBER(void, CEconItemSchema_Reset)
	{
        attributeCache.clear();
        DETOUR_MEMBER_CALL();
    }

    DETOUR_DECL_MEMBER(bool, CEconItemSchema_BInitAttributes, KeyValues *kv, CUtlVector<CUtlString> *errors)
	{
        auto ret = DETOUR_MEMBER_CALL(kv, errors);
        attributeCache.clear();
        return ret;
    }

    bool IsOnNav(CBaseEntity *entity, float maxZDistance, float maxXYDistance)
	{
		auto subjectCombat = entity->MyCombatCharacterPointer();
		if (subjectCombat != nullptr) {
			auto area = subjectCombat->GetLastKnownArea();
			if (area != nullptr) {
				
				auto &vec = entity->GetAbsOrigin();
				bool contains = area->IsOverlapping(vec,maxXYDistance);
				float z = area->GetZ(vec.x, vec.y);
				if (!contains || z - vec.z > maxZDistance) {
					return false;
				}
			}
		}
		return true;
	}
#endif

    int player_move_entindex = 0;
	
	ConVar sig_perf_lagcomp_bench("sig_perf_lagcomp_bench", "0", FCVAR_NOTIFY,
		"Mod: Optimize common calls");

    bool restorebot = false;
    float coneOfAttack = -1.0f;
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_StartLagCompensation, CBasePlayer *player, CUserCmd *cmd)
	{
        if (player->IsBot() && sig_perf_lagcomp_bench.GetBool()) {
            player->m_fFlags &= ~FL_FAKECLIENT;
            player->m_fLerpTime = fmodf(player->entindex() * 0.015f, 0.9f);
            restorebot = true;
        }
		int preMaxPlayers = gpGlobals->maxClients;
		DETOUR_MEMBER_CALL(player, cmd);
        coneOfAttack = -1.0f;
        if (restorebot) {
            player->m_fFlags |= FL_FAKECLIENT;
            restorebot = false;
        }
	}
	
    DETOUR_DECL_MEMBER(void, CLagCompensationManager_BacktrackPlayer, CBasePlayer *player, float flTargetTime)
	{
        if (restorebot) {
            Msg("Backtracking amount %f\n", gpGlobals->curtime - flTargetTime);
        }
		DETOUR_MEMBER_CALL(player, flTargetTime);
    }

	DETOUR_DECL_MEMBER(void, CLagCompensationManager_FinishLagCompensation, CBasePlayer *player)
	{
		int preMaxPlayers = gpGlobals->maxClients;
		DETOUR_MEMBER_CALL(player);
	}

    DETOUR_DECL_MEMBER(void, NextBotManager_OnWeaponFired, CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon)
	{
        if (whoFired != nullptr) {
            // Do not trigger onweaponfired too often
            auto data = GetExtraCombatCharacterData(whoFired);
            if (gpGlobals->curtime - data->lastShootTime >= 0.6f) {
                return;
            }
            data->lastShootTime = gpGlobals->curtime;
        }

		DETOUR_MEMBER_CALL(whoFired, weapon);
	}

    DETOUR_DECL_MEMBER(bool, CTFPlayer_WantsLagCompensationOnEntity, const CBasePlayer *target, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		auto result = DETOUR_MEMBER_CALL(target, pCmd, pEntityTransmitBits);
		if (result && coneOfAttack != -1.0f) {
            Vector start = player->EyePosition();
            const Vector &ourPos = player->GetAbsOrigin();
            Vector forward;
            AngleVectors(pCmd->viewangles, &forward);
            
            auto cprop = target->CollisionProp(); 
            const Vector &entityPos = target->GetAbsOrigin();
            const Vector &velocity = target->GetAbsVelocity();
            float backTime = (gpGlobals->tickcount - pCmd->tick_count + 1) * gpGlobals->interval_per_tick + player->m_fLerpTime;
            const float speed = Max(target->MaxSpeed(), Max(fabs(velocity.x), fabs(velocity.y))* 1.4f);
            Vector range(speed, speed, Max(50.0f, fabs(velocity.z) * 2.0f));
            range *= backTime;
            range.x += 25.0f;
            range.y += 25.0f;
            float cone = coneOfAttack != -1.0f ? coneOfAttack : 0.15f;
            if (cone > 0.0f) {
                float dist = (entityPos - ourPos).Length();
                range += Vector(dist * cone, dist * cone, dist * cone);
            }

            if (IsBoxIntersectingRay(entityPos+cprop->OBBMins() - range, entityPos+cprop->OBBMaxs() + range, start, forward * 8192, 0.0f)) {
                return true;
            }
            return false;
        }
        return result;
	}

    DETOUR_DECL_STATIC(void, FX_FireBullets, CTFWeaponBase *pWpn, int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
	{
        coneOfAttack = flSpread;
		DETOUR_STATIC_CALL(pWpn, iPlayer, vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
        coneOfAttack = -1.0f;
	}

    DETOUR_DECL_MEMBER(void, CTFFlameThrower_PrimaryAttack)
	{
        coneOfAttack = 0;
		DETOUR_MEMBER_CALL();
        coneOfAttack = -1.0f;
	}

    DETOUR_DECL_MEMBER(void, CWeaponMedigun_PrimaryAttack)
	{
        coneOfAttack = 0;
		DETOUR_MEMBER_CALL();
        coneOfAttack = -1.0f;
	}

    class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf::Func_Optimize")
		{
            // Rewrite CKnownEntity::OperatorEquals to compare handle indexes instead of entity pointers
            MOD_ADD_REPLACE_FUNC_MEMBER(CKnownEntity_OperatorEquals, "CKnownEntity::operator==");
            
#ifdef SE_IS_TF2
            // Not enough speed up from condition optimization after update
            // // Modify CTFPlayerShared::ConditionGameRulesThink so that conditions are not updated here, they are updated in a detour instead (needed for another patch)
			// this->AddPatch(new CPatch_CTFPlayerShared_ConditionGameRulesThink());
            // // Rewrite CTFPlayerShared::InCond to remove extra check for conditions < 32
            // MOD_ADD_REPLACE_FUNC_MEMBER(CTFPlayerShared_InCond, "CTFPlayerShared::InCond");
            
            // MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_GetCarryingRuneType, "CTFPlayerShared::GetCarryingRuneType");
            // MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_ConditionGameRulesThink, "CTFPlayerShared::ConditionGameRulesThink", LOW);
            // MOD_ADD_DETOUR_MEMBER(CTFConditionList_Add, "CTFConditionList::_Add");
            // MOD_ADD_DETOUR_MEMBER(CTFConditionList_Remove, "CTFConditionList::_Remove");

            // Rewrite CEconItemView::GetStaticData to not use dynamic_cast (always assume CEconItemDefintion is CTFItemDefintion)
            MOD_ADD_REPLACE_FUNC_MEMBER(CEconItemView_GetStaticData, "CEconItemView::GetStaticData");
            // Modify CTFPlayer::GetEquippedWearableForLoadoutSlot to not use dynamic_cast (always assume CEconWearable is CTFWearable)
			this->AddPatch(new CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot());
            
            //MOD_ADD_DETOUR_STATIC(GEconItemSchema, "GEconItemSchema");
            //MOD_ADD_DETOUR_STATIC(GetItemSchema, "GetItemSchema");
            
            // Optimize CTFPlayer::GetEntityForLoadoutSlot by using an array that contains items for every slot
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_EquipWearable, "CBasePlayer::EquipWearable");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Weapon_Equip, "CBasePlayer::Weapon_Equip");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_GetEntityForLoadoutSlot, "CTFPlayer::GetEntityForLoadoutSlot", HIGHEST);
            
            // Optimize UTIL_PlayerByIndex so that it reads player pointer from (edict + offset) directly
            MOD_ADD_DETOUR_STATIC(UTIL_PlayerByIndex, "UTIL_PlayerByIndex");
#endif

            // Those detours disable multithreading for mdl load to get rid of performance affecting locks
            MOD_ADD_DETOUR_STATIC(CBaseEntity_PrecacheModel, "CBaseEntity::PrecacheModel");
            MOD_ADD_DETOUR_MEMBER(CModelInfoServer_RegisterDynamicModel, "CModelInfoServer::RegisterDynamicModel");
            // MOD_ADD_DETOUR_MEMBER(CMDLCache_BeginLock, "CMDLCache::BeginLock");
            // MOD_ADD_DETOUR_MEMBER(CMDLCache_EndLock, "CMDLCache::EndLock");
			this->AddPatch(new CPatch_CMDLCache_BeginEndLock("CMDLCache::BeginLock"));
			this->AddPatch(new CPatch_CMDLCache_BeginEndLock("CMDLCache::EndLock"));
#ifdef SE_IS_TF2
            MOD_ADD_DETOUR_STATIC(CTFPlayer_PrecacheMvM, "CTFPlayer::PrecacheMvM");
            
            // Prevent player hurt from triggering speak too often
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed, "CTFPlayer::SpeakConceptIfAllowed");
            // Update bot push vector at smaller frequency
            MOD_ADD_DETOUR_MEMBER(CTFBot_AvoidPlayers, "CTFBot::AvoidPlayers");
            
            //MOD_ADD_DETOUR_MEMBER(CThreadLocalBase_Get, "CThreadLocalBase::Get");
            
            //MOD_ADD_DETOUR_MEMBER(CEconItemView_GetStaticData, "CEconItemView::GetStaticData");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnNavAreaChanged, "CTFPlayer::OnNavAreaChanged");

            // Fix lag when spawning items on bots
            MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem, "CTFBot::AddItem");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CItemGeneration_GenerateRandomItem, "CItemGeneration::GenerateRandomItem", LOWEST);

            // Cache for getting attribute definition by name
            MOD_ADD_DETOUR_MEMBER(CEconItemSchema_GetAttributeDefinitionByName, "CEconItemSchema::GetAttributeDefinitionByName [non-const]");
            MOD_ADD_DETOUR_MEMBER(CEconItemSchema_Reset, "CEconItemSchema::Reset");
            MOD_ADD_DETOUR_MEMBER(CEconItemSchema_BInitAttributes, "CEconItemSchema::BInitAttributes");
#endif
            // Reduce the amount of onweaponfired being fired
            MOD_ADD_DETOUR_MEMBER(NextBotManager_OnWeaponFired, "NextBotManager::OnWeaponFired");
            // Optimize lag compensation by reusing previous bone caches
            // Not enough difference in performance
            
            // MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_StartLagCompensation, "CLagCompensationManager::StartLagCompensation");

#ifdef SE_IS_TF2
            // Better culling for WantsLagCompensationOnEntity
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_WantsLagCompensationOnEntity, "CTFPlayer::WantsLagCompensationOnEntity");
            MOD_ADD_DETOUR_STATIC(FX_FireBullets, "FX_FireBullets");
            MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_PrimaryAttack, "CTFFlameThrower::PrimaryAttack");
            MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_PrimaryAttack, "CWeaponMedigun::PrimaryAttack");
#endif
            //MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_BacktrackPlayer, "CLagCompensationManager::BacktrackPlayer");
            
            
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

#ifdef SE_IS_TF2
        virtual void LevelInitPreEntity() override 
        {
            item_defs.clear();
        }

        virtual bool OnLoad() override 
        {
            GEconItemSchema_addr = (uintptr_t)AddrManager::GetAddr("GEconItemSchema");
            GetItemSchema_addr = (uintptr_t)AddrManager::GetAddr("GetItemSchema");
            CThreadLocalBase_Get_addr = (uintptr_t)AddrManager::GetAddr("CThreadLocalBase::Get");
            return true;
        }
#endif


        virtual void OnEnablePost() override 
        {
            world_edict = INDEXENT(0);
#ifdef SE_IS_TF2
            schema = GetItemSchema();

            // In case the mod was toggled on/off, restore the quick item in loadout slot optimizations
            ForEachTFPlayer([](CTFPlayer *player){
                auto data = GetExtraPlayerData(player);
                ForEachTFPlayerEconEntity(player, [data, player](CEconEntity *entity){

                    auto item = entity->GetItem() ;
                    if (item == nullptr) return;

                    auto itemDef = item->GetItemDefinition();
                    if (itemDef == nullptr) return;

                    auto slot = itemDef->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
                    if (slot == -1 && itemDef->m_iItemDefIndex != 0) {
                        slot = itemDef->GetLoadoutSlot(TF_CLASS_UNDEFINED);
                    }
                    if (slot >= 0 && slot < LOADOUT_POSITION_COUNT && (data->quickItemInLoadoutSlot[slot] == nullptr || data->quickItemInLoadoutSlot[slot]->GetOwnerEntity() != player) ) {
                        data->quickItemInLoadoutSlot[slot] = entity;
                    }
                });
            });
#endif
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Post enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }l
            
            // Msg("\n");
        }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_func_optimize", "0", FCVAR_NOTIFY,
		"Mod: Optimize common calls",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}