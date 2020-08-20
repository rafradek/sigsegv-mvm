#include "mod.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/tfplayer.h"


namespace Mod::Attr::Custom_Attributes
{

	GlobalThunk<void *> g_pFullFileSystem("g_pFullFileSystem");
	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanAirDash)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFPlayer_CanAirDash)();
		if (!ret) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (!player->IsPlayerClass(TF_CLASS_SCOUT)) {
				int dash = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( player, dash, air_dash_count );
				if (dash > player->m_Shared->m_iAirDash)
					return true;
			}
		}
		return ret;
	}

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Attr:Custom_Attributes")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanAirDash, "CTFPlayer::CanAirDash");
			this->Toggle(true);
		}

		virtual void LevelInitPostEntity() override
		{

			
		}
		
		virtual void LevelInitPreEntity() override
		{
			DevMsg("BINITATTRIBUTES %d\n", GetItemSchema() == nullptr);
			KeyValues *kv = new KeyValues("attributes");
			char path[PLATFORM_MAX_PATH];
			g_pSM->BuildPath(Path_SM,path,sizeof(path),"gamedata/sigsegv/custom_attributes.txt");
			if (kv->LoadFromFile(filesystem, path)) {
				DevMsg("Loaded attrs\n");
				CUtlVector<CUtlString> err;
				GetItemSchema()->BInitAttributes(kv, &err);
			}
		}
		virtual bool ShouldReceiveCallbacks() const override { return true; }

		virtual void OnUnload() override
		{
		}
		
		virtual void OnDisable() override
		{
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_custom", "0", FCVAR_NOTIFY,
		"Mod: enable custom attributes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
