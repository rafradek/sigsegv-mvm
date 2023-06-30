#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/econ.h"
#include "stub/extraentitydata.h"
#include "util/misc.h"
#include "mod/etc/mapentity_additions.h"
#include "util/iterate.h"
#include "mod/common/commands.h"


namespace Mod::Etc::Allow_Civilian_Class
{
	constexpr uint8_t s_Buf_HandleCommand_JoinClass[] = {
		0x83, 0xFE, 0x0A,                   // +0000 cmp     esi, 0Ah
        0x0F, 0x84, 0x97, 0xFB, 0xFF, 0xFF  // +0003 jz      loc_F5DEF7
	};
	
	struct CPatch_HandleCommand_JoinClass : public CPatch
	{
		CPatch_HandleCommand_JoinClass() : CPatch(sizeof(s_Buf_HandleCommand_JoinClass)) {}
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_HandleCommand_JoinClass);
			mask[0x01] = 0x00;
            mask.SetDword(0x05, 0);
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf[0x02] = 0x0B;
			mask[0x02] = 0xFF;
			return true;
		}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::HandleCommand_JoinClass"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0400; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0800; } // @ 0x072a
	};

    bool disabling = false;
    DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinClass, const char *pClassName, bool b1)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        if (!disabling && !player->IsBot() && FStrEq(pClassName, "random")) {
            pClassName = "civilian";
        }
        
        bool prevClassCivilian = player->m_Shared->m_iDesiredPlayerClass == TF_CLASS_CIVILIAN || player->GetPlayerClass()->GetClassIndex() == TF_CLASS_CIVILIAN;
        DETOUR_MEMBER_CALL(CTFPlayer_HandleCommand_JoinClass)(pClassName, b1);

        if (player->m_Shared->m_iDesiredPlayerClass == TF_CLASS_CIVILIAN) {
            auto mod = player->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
			mod->props["m_iDesiredPlayerClass"] = {Variant(TF_CLASS_SCOUT), Variant(TF_CLASS_SCOUT)};
        }
		else if (prevClassCivilian) {
            auto mod = player->GetEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
            if (mod != nullptr) {
                mod->props.erase("m_iDesiredPlayerClass");
            }
        }
    }
    
	DETOUR_DECL_MEMBER(CEconItemView *, CTFPlayerInventory_GetItemInLoadout, int pclass, int slot)
	{
		auto result = DETOUR_MEMBER_CALL(CTFPlayerInventory_GetItemInLoadout)(pclass, slot);
        if (pclass == TF_CLASS_CIVILIAN && result == nullptr) {
            return TFInventoryManager()->GetBaseItemForClass(TF_CLASS_CIVILIAN, 999);
        } 
        return result;
    }

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Allow_Civilian_Class") {
            this->AddPatch(new CPatch_HandleCommand_JoinClass());
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_HandleCommand_JoinClass,             "CTFPlayer::HandleCommand_JoinClass", HIGHEST);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerInventory_GetItemInLoadout,           "CTFPlayerInventory::GetItemInLoadout", LOWEST);
        }

		virtual void PreLoad() override
		{

		}
		
		virtual void OnEnable() override
		{

		}
		
		virtual void OnDisable() override
		{
            // Reset civilian players back to normal
            disabling = true;
            ForEachTFPlayer([](CTFPlayer *player){
                if (player->IsRealPlayer() && player->GetPlayerClass()->GetClassIndex() == TF_CLASS_CIVILIAN) {
                    player->HandleCommand_JoinClass("random");
                }
            });
            disabling = false;
		}

	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_allow_civilian_class", "0", FCVAR_NOTIFY,
		"Mod: allow civilian class to be picked",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
