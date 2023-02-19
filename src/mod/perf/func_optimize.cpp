#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#ifdef SE_TF2
#include "stub/tfplayer.h"
#endif
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#ifdef SE_TF2
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
 

namespace Mod::Perf::Func_Optimize
{   
    constexpr uint8_t s_Buf_CKnownEntity_OperatorEquals[] = {
        /*0:*/  0x52,                      //push   edx
        /*1:*/  0x8b, 0x44, 0x24, 0x08,    //         mov    eax,DWORD PTR [esp+0x8]
        /*5:*/  0x8b, 0x50, 0x04,          //      mov    edx,DWORD PTR [eax+0x4]
        /*8:*/  0x31, 0xc0,                //   xor    eax,eax
        /*a:*/  0x83, 0xfa, 0xff,          //      cmp    edx,0xffffffff
        /*d:*/  0x74, 0x0a,                //   je     19 <L125>
        /*f:*/  0x8b, 0x44, 0x24, 0x0c,    //         mov    eax,DWORD PTR [esp+0xc]
        /*13:*/ 0x39, 0x50, 0x04,          //      cmp    DWORD PTR [eax+0x4],edx
        /*16:*/ 0x0f, 0x94, 0xc0,          //      sete   a
        //00000019 <L125>:
        /*19:*/ 0x5a,                      //pop edx
        /*1a:*/ 0xc3,                      //ret
	};

    struct CPatch_CKnownEntity_OperatorEquals : public CPatch
	{
		CPatch_CKnownEntity_OperatorEquals() : CPatch(sizeof(s_Buf_CKnownEntity_OperatorEquals)) {}
		
		virtual const char *GetFuncName() const override { return "CKnownEntity::operator=="; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CKnownEntity_OperatorEquals);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
	};

    constexpr uint8_t s_Buf_CMDLCache_BeginEndLock[] = {
        0xc3,                      //ret
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
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
        std::string name;
	};

#ifdef SE_TF2
    constexpr uint8_t s_Buf_CEconItemView_GetStaticData[] = {
        0x55,                      //push   ebp 0
        0x89, 0xe5,                   //mov    ebp,esp 1
        0x53,                      //push   ecx 3
        0x83, 0xec, 0x14,                      //push   edx 4
        0x8b, 0x45, 0x08,                //mov    ecx,DWORD PTR [ebp+0xc] 7
        0x0f, 0xb7, 0x58, 0x04,               // mov    edx,DWORD PTR [ebp+0x8] a
        0xE8, 0x6D, 0x8D, 0xFF, 0xFF,       //add    edx,0xcc b e
        0x89, 0x04, 0x24,                   //test   ecx,ecx 13
        0x89, 0x5c, 0x24, 0x04, // 16
        0xe8, 0x61, 0x51, 0xfc, 0xff, //1a
        0x83, 0xc4, 0x14,
        0x5b,
        0x5d,
        0xc3,                      //ret
	};
        
    struct CPatch_CEconItemView_GetStaticData : public CPatch
	{
		CPatch_CEconItemView_GetStaticData() : CPatch(sizeof(s_Buf_CEconItemView_GetStaticData)) {}
		
		virtual const char *GetFuncName() const override { return "CEconItemView::GetStaticData"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CEconItemView_GetStaticData);
			
			mask.SetAll(0x00);
            mask[0] = 0xFF;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            buf[0x0a + 3] = (uint8_t)CEconItemView::s_prop_m_iItemDefinitionIndex.GetOffsetDirect();
			mask.SetAll(0xFF);
            
            mask.SetDword(0x0e + 1, 0);
            mask.SetDword(0x1a + 1, 0);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
            buf[0x0a + 3] = (uint8_t)CEconItemView::s_prop_m_iItemDefinitionIndex.GetOffsetDirect();
			return true;
		}
	};
    
    
    constexpr uint8_t s_Buf_CTFPlayerShared_InCond[] = {
        //0x55,                      //push   ebp 0
        //0x89, 0xe5,                   //mov    ebp,esp 1
        0x51,                      //push   ecx 0
        0x52,                      //push   edx 1
        0x8b, 0x4c, 0x24, 0x10,                //mov    ecx,DWORD PTR [esp+0x8 + 0x8] 2
        0x8b, 0x54, 0x24, 0x0C,               // mov    edx,DWORD PTR [esp+0x4 + 0x8] 6
        0x81, 0xc2, 0xcc, 0x00, 0x00, 0x00,       //add    edx,0xcc a
        0x85, 0xc9,                   //test   ecx,ecx
        0x8d, 0x41, 0x1f,                //lea    eax,[ecx+0x1f]
        0x0f, 0x49, 0xc1,                //cmovns eax,ecx
        0xc1, 0xf8, 0x05,                //sar    eax,0x5
        0x8b, 0x04, 0x82,                //mov    eax,DWORD PTR [edx+eax*4]
        0xd3, 0xf8,                   //sar    eax,cl
        0x83, 0xe0, 0x01,                //and    eax,0x1
        0x5a,                      //pop    edx
        0x59,                      //pop    ecx
        //0x5d,                      //pop    ebp
        0xc3,                      //ret
	};
        
    struct CPatch_CTFPlayerShared_InCond: public CPatch
	{
		CPatch_CTFPlayerShared_InCond() : CPatch(sizeof(s_Buf_CTFPlayerShared_InCond)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::InCond"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            
			buf.CopyFrom(s_Buf_CTFPlayerShared_InCond);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            buf.SetDword(0x0a + 2, CTFPlayerShared::s_prop_m_nPlayerCond.GetOffsetDirect());
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
            buf.SetDword(0x0a + 2, CTFPlayerShared::s_prop_m_nPlayerCond.GetOffsetDirect());
			return true;
		}
	};
	constexpr uint8_t s_Buf_CTFPlayerShared_ConditionGameRulesThink[] = {
        0x83, 0xC7, 0x01, //add     edi, 1
        0x83, 0xC3, 0x14, //add     ebx, sizeof(condition_source_t)
        0x81, 0xFF, 0x82, 0x00, 0x00, 0x00 //cmp     edi, TF_COND_COUNT
	};
	struct CPatch_CTFPlayerShared_ConditionGameRulesThink: public CPatch
	{
		CPatch_CTFPlayerShared_ConditionGameRulesThink() : CPatch(sizeof(s_Buf_CTFPlayerShared_ConditionGameRulesThink)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::ConditionGameRulesThink"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPlayerShared_ConditionGameRulesThink);
            buf[0x3 + 2] = sizeof(condition_source_t);
            buf.SetDword(0x6 + 2, GetNumberOfTFConds());
			
			/* allow any 3-bit destination register code */
			mask[0x0 + 1] = 0b11111000;
			mask[0x3 + 1] = 0b11111000;
			mask[0x6 + 1] = 0b11111000;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf.SetDword(0x06 + 2, 0x01);
			
			mask.SetDword(0x6 + 2, 0xffffffff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf.SetDword(0x06 + 2, 0x01);
			buf[0x6] = 0x83;
			
			return true;
		}
	};
    
    constexpr uint8_t s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot[] = {
        0xC7, 0x44, 0x24, 0x0C, 0x00, 0x00, 0x00, 0x00, // 0
        0xC7, 0x44, 0x24, 0x08, 0x00, 0xCC, 0x1E, 0x01, // 8
        0xC7, 0x44, 0x24, 0x04, 0xD0, 0xE2, 0x1B, 0x01, // 10
        0x89, 0x04, 0x24, // 18
        0xE8, 0x9F, 0x41, 0xFF, 0x00 //1b
	};
	struct CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot: public CPatch
	{
		CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot() : CPatch(sizeof(s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::GetEquippedWearableForLoadoutSlot"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPlayer_GetEquippedWearableForLoadoutSlot);
			
			mask.SetDword(0x00 + 4, 0);
			mask.SetDword(0x08 + 4, 0);
			mask.SetDword(0x10 + 4, 0);
			mask.SetDword(0x1b + 1, 0);
			/* allow any 3-bit source register code */
			mask[0x18 + 1] = 0b11000111;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf.SetAll(0x90);
			
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf.SetAll(0x90);
			
			return true;
		}
	};

	DETOUR_DECL_MEMBER(int, CTFPlayerShared_GetCarryingRuneType)
	{
		return reinterpret_cast<CTFPlayerShared *>(this)->GetCarryingRuneType();
		//return DETOUR_MEMBER_CALL(CTFPlayerShared_GetCarryingRuneType)();
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
		DETOUR_MEMBER_CALL(CTFPlayerShared_ConditionGameRulesThink)();
	}

    DETOUR_DECL_MEMBER(bool, CTFConditionList_Add, ETFCond cond, float duration, CTFPlayer *player, CBaseEntity *provider)
	{
		bool success = DETOUR_MEMBER_CALL(CTFConditionList_Add)(cond, duration, player, provider);
        if (success) {
            player->m_Shared->GetCondData().AddCondBit(cond);
        }
        return success;
	}

    DETOUR_DECL_MEMBER(bool, CTFConditionList_Remove, ETFCond cond, bool ignore)
	{
		bool success = DETOUR_MEMBER_CALL(CTFConditionList_Remove)(cond, ignore);
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
		schema = DETOUR_STATIC_CALL(GEconItemSchema)();
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
		schema = DETOUR_STATIC_CALL(GetItemSchema)();
        auto addr = (uint8_t*)__builtin_return_address(0);
        OptimizeGetSchema(schema, addr, GetItemSchema_addr);
        return schema;
	}

    
#define AVERAGE_TIME2(name) \
	static int tickLast_##name = 0; \
	static int counter_##name = 0; \
    static CCycleCount cycle_##name; \
	class CTimeScopeMsg_##name \
	{ \
	public: \
		CTimeScopeMsg_##name() : m_Timer(&cycle_##name) { } \
		~CTimeScopeMsg_##name() \
		{ \
			m_Timer.End(); \
		} \
	private:	\
		CTimeAdder m_Timer;\
	} name##_TSM; \
	counter_##name++;\
	if (tickLast_##name + 66 < gpGlobals->tickcount ) {\
		Msg( #name "calls: %d total: %.9fs avg: %.9fs\n", counter_##name, cycle_##name.GetSeconds(), cycle_##name.GetSeconds()/counter_##name );\
        cycle_##name.Init(); \
		counter_##name = 0;\
		tickLast_##name = gpGlobals->tickcount;\
	}\

    DETOUR_DECL_MEMBER(CTFItemDefinition *, CEconItemView_GetStaticData)
	{
		auto me = reinterpret_cast<CEconItemView *>(this);
        return static_cast<CTFItemDefinition *>(schema->GetItemDefinition(me->m_iItemDefinitionIndex));
	}

    bool useOrig = false;
    DETOUR_DECL_MEMBER(void, CBasePlayer_EquipWearable, CEconWearable *wearable)
	{
		DETOUR_MEMBER_CALL(CBasePlayer_EquipWearable)(wearable);
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
		DETOUR_MEMBER_CALL(CTFPlayer_Weapon_Equip)(weapon);
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
        if (useOrig) return DETOUR_MEMBER_CALL(CTFPlayer_GetEntityForLoadoutSlot)(slot);
        
        auto player = reinterpret_cast<CTFPlayer *>(this);
        auto data = GetExtraPlayerData(player, false);
        if (data != nullptr) {
            auto handle = data->quickItemInLoadoutSlot[slot];
            auto ent = handle.Get();
            if (handle.IsValid() && (ent == nullptr || ent->GetOwnerEntity() != player)) {
                ent = static_cast<CEconEntity *>(DETOUR_MEMBER_CALL(CTFPlayer_GetEntityForLoadoutSlot)(slot));
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
        return DETOUR_STATIC_CALL(CBaseEntity_PrecacheModel)(model);
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
        auto ret = DETOUR_MEMBER_CALL(CModelInfoServer_RegisterDynamicModel)(model, isClient);
        CModelLoader_FlushDynamicModels(modelloader.GetRef());
        return ret;
        //return CBaseEntity::PrecacheModel(model);
        // if (useOrig) return DETOUR_MEMBER_CALL(CModelInfoServer_RegisterDynamicModel)(model, isClient);
        
        // auto player = reinterpret_cast<CTFPlayer *>(this);
        // auto data = GetExtraPlayerData(player, false);
        // if (data != nullptr) {
        //     auto handle = data->quickItemInLoadoutSlot[slot];
        //     auto ent = handle.Get();
        //     if (handle.IsValid() && (ent == nullptr || ent->GetOwnerEntity() != player)) {
        //         ent = static_cast<CEconEntity *>(DETOUR_MEMBER_CALL(CTFPlayer_GetEntityForLoadoutSlot)(slot));
        //         data->quickItemInLoadoutSlot[slot] = ent;
        //     }
        //     return ent;
        // }
        // return nullptr;
	}
    DETOUR_DECL_MEMBER(void, CMDLCache_BeginLock)
	{
        
    }
    DETOUR_DECL_MEMBER(void, CMDLCache_EndLock)
	{
        
    }
    
#ifdef SE_TF2
    DETOUR_DECL_STATIC(void, CTFPlayer_PrecacheMvM)
	{
        DETOUR_STATIC_CALL(CTFPlayer_PrecacheMvM)();
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
            ret = DETOUR_MEMBER_CALL(CTFPlayer_SpeakConceptIfAllowed)(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
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
		return DETOUR_MEMBER_CALL(CThreadLocalBase_Get)();
	}

    DETOUR_DECL_MEMBER(void, CTFBot_AvoidPlayers, CUserCmd *cmd)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		if ((bot->entindex() + gpGlobals->tickcount) % 8 == 0) {
            float forwardPre = cmd->forwardmove;
            float sidePre = cmd->sidemove;
            DETOUR_MEMBER_CALL(CTFBot_AvoidPlayers)(cmd);
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

    template<typename Functor>
	inline void ForAllPotentiallyVisibleAreas(Functor func, CNavArea *curArea)
	{
		int i;
        auto &potentiallyVisible = curArea->m_potentiallyVisibleAreas.Get();
        for ( i=0; i < potentiallyVisible.Count(); ++i )
		{
			CNavArea *area = potentiallyVisible[i].area;
			if ( !area )
				continue;

			if ( potentiallyVisible[i].attributes == 0 )
				continue;

			func(area);
		}

		// for each inherited area
		if ( !curArea->m_inheritVisibilityFrom->area )
			return;

		CUtlVectorConservative<AreaBindInfo> &inherited = curArea->m_inheritVisibilityFrom->area->m_potentiallyVisibleAreas;

		for ( i=0; i<inherited.Count(); ++i )
		{
			if ( !inherited[i].area )
				continue;

			if ( inherited[i].attributes == 0 )
				continue;

            func(inherited[i].area);
		}
	}

    
    template<typename Functor>
	inline void ForAllPotentiallyVisibleAreasReverse(Functor func, CNavArea *curArea)
	{
		int i;
        auto &potentiallyVisible = curArea->m_potentiallyVisibleAreas.Get();
        for ( i=potentiallyVisible.Count()-1; i >= 0 ; --i )
		{
			CNavArea *area = potentiallyVisible[i].area;
			if ( !area )
				continue;

			if ( potentiallyVisible[i].attributes == 0 )
				continue;

			func(area);
		}

		// for each inherited area
		if ( !curArea->m_inheritVisibilityFrom->area )
			return;

		CUtlVectorConservative<AreaBindInfo> &inherited = curArea->m_inheritVisibilityFrom->area->m_potentiallyVisibleAreas;

		for ( i=inherited.Count()-1; i >= 0; --i )
		{
			if ( !inherited[i].area )
				continue;

			if ( inherited[i].attributes == 0 )
				continue;

            func(inherited[i].area);
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
        //CCycleCount time4;
        //CTimeAdder timer4(&time4);
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
		DETOUR_MEMBER_CALL(CTFBot_AddItem)(item);
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
        return DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang, name);
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

    class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf::Func_Optimize")
		{
			this->AddPatch(new CPatch_CKnownEntity_OperatorEquals());
#ifdef SE_TF2
			this->AddPatch(new CPatch_CTFPlayerShared_ConditionGameRulesThink());
			this->AddPatch(new CPatch_CTFPlayerShared_InCond());
			this->AddPatch(new CPatch_CEconItemView_GetStaticData());
			this->AddPatch(new CPatch_CTFPlayer_GetEquippedWearableForLoadoutSlot());
            
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_GetCarryingRuneType, "CTFPlayerShared::GetCarryingRuneType");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_ConditionGameRulesThink, "CTFPlayerShared::ConditionGameRulesThink", LOW);
            MOD_ADD_DETOUR_MEMBER(CTFConditionList_Add, "CTFConditionList::_Add");
            MOD_ADD_DETOUR_MEMBER(CTFConditionList_Remove, "CTFConditionList::_Remove");
            //MOD_ADD_DETOUR_STATIC(GEconItemSchema, "GEconItemSchema");
            //MOD_ADD_DETOUR_STATIC(GetItemSchema, "GetItemSchema");
            
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_EquipWearable, "CBasePlayer::EquipWearable");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Weapon_Equip, "CBasePlayer::Weapon_Equip");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_GetEntityForLoadoutSlot, "CTFPlayer::GetEntityForLoadoutSlot", HIGHEST);
            MOD_ADD_DETOUR_STATIC(UTIL_PlayerByIndex, "UTIL_PlayerByIndex");
#endif

            // Those detours disable multithreading for mdl load to get rid of performance affecting locks
            MOD_ADD_DETOUR_STATIC(CBaseEntity_PrecacheModel, "CBaseEntity::PrecacheModel");
            MOD_ADD_DETOUR_MEMBER(CModelInfoServer_RegisterDynamicModel, "CModelInfoServer::RegisterDynamicModel");
			this->AddPatch(new CPatch_CMDLCache_BeginEndLock("CMDLCache::BeginLock"));
			this->AddPatch(new CPatch_CMDLCache_BeginEndLock("CMDLCache::EndLock"));
#ifdef SE_TF2
            MOD_ADD_DETOUR_STATIC(CTFPlayer_PrecacheMvM, "CTFPlayer::PrecacheMvM");
            
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed, "CTFPlayer::SpeakConceptIfAllowed");
            MOD_ADD_DETOUR_MEMBER(CTFBot_AvoidPlayers, "CTFBot::AvoidPlayers");
            
            //MOD_ADD_DETOUR_MEMBER(CThreadLocalBase_Get, "CThreadLocalBase::Get");
            
            //MOD_ADD_DETOUR_MEMBER(CEconItemView_GetStaticData, "CEconItemView::GetStaticData");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnNavAreaChanged, "CTFPlayer::OnNavAreaChanged");

            // Fix lag when spawning items on bots
            MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem, "CTFBot::AddItem");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CItemGeneration_GenerateRandomItem, "CItemGeneration::GenerateRandomItem", LOWEST);
#endif
            
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

#ifdef SE_TF2
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
#ifdef SE_TF2
            schema = GetItemSchema();
#endif
            world_edict = INDEXENT(0);
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