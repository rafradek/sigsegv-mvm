#include "mod.h"
#include "stub/tfplayer.h"
#include "util/clientmsg.h"


namespace Mod::Debug::Rage
{
	// CTFPlayerShared::m_flRageMeter
	// CTFPlayerShared::m_bRageDraining
	
	
//	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int nType, float flAmount, float flModifier)
//	{
//		
//	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_ModifyRage, float delta)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		float rage_pre  = shared->m_flRageMeter;
		DETOUR_MEMBER_CALL(delta);
		float rage_post = shared->m_flRageMeter;
		
		CTFPlayer *player = shared->GetOuter();
		
		ClientMsgAll("RAGE: +%6.2f [%6.2f -> %6.2f] %s\n",
			delta, rage_pre, rage_post, player->GetPlayerName());
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Rage")
		{
		//	#error need addr
		//	MOD_ADD_DETOUR_STATIC(HandleRageGain, "HandleRageGain");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_ModifyRage, "CTFPlayerShared::ModifyRage");
			// CTFPlayerShared::SetRageMeter
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_rage", "0", FCVAR_NOTIFY,
		"Debug: monitor rage events",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
