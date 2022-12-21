#include "mod.h"

namespace Mod::Etc::Static_Prop_Handle_Fix
{
	constexpr uint8_t s_Buf_CStaticPropMgr_GetStaticProp[] = {
        /*0:  */0x55,                      //push   ebp
		/*1:  */0x89, 0xe5,                   //mov    ebp,esp
		/*3:  */0x8b, 0x45, 0x0c,                //mov    eax,DWORD PTR [ebp+0xc]
		/*6:  */0x5d,                      //pop    ebp
		/*7:  */0x66, 0x81, 0x78, 0x02, 0x00, 0x80,//       cmp    WORD PTR [eax+0x2],0x8000
		/*d:  */0x0f, 0x94, 0xc0,                //sete   al
		/*10: */0xc3                      //ret
	};
        
    struct CPatch_CStaticPropMgr_GetStaticProp : public CPatch
	{
		CPatch_CStaticPropMgr_GetStaticProp() : CPatch(sizeof(s_Buf_CStaticPropMgr_GetStaticProp)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticPropMgr::GetStaticProp (CBaseHandle)"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CStaticPropMgr_GetStaticProp);
			
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

	DETOUR_DECL_MEMBER(bool, CStaticProp_Init, int index, void *lump, model_t *pModel)
	{
		auto ret = DETOUR_MEMBER_CALL(CStaticProp_Init)(index, lump, pModel);
		auto prop = reinterpret_cast<IHandleEntity *>(this);
		CBaseHandle handle = prop->GetRefEHandle();
		handle.Init(handle.GetEntryIndex(), 0x8000);
		prop->SetRefEHandle(handle);
		return ret;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Static_Prop_Handle_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CStaticProp_Init,       "CStaticProp::Init");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_static_prop_handle_fix", "0", FCVAR_NOTIFY,
		"Etc: Fix static prop bug introduced in vscript TF2 update. Enabling this also enables virtual optimize mod",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			bool enabled = static_cast<ConVar *>(pConVar)->GetBool();
			s_Mod.Toggle(enabled);
			if (enabled) {
				ConVarRef sig_perf_virtual_call_optimize("sig_perf_virtual_call_optimize");
				sig_perf_virtual_call_optimize.SetValue(true);
			}
		});
}
