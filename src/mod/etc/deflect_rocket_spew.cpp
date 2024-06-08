#include "mod.h"
#include "stub/projectiles.h"
#include "stub/usermessages_sv.h"


namespace Mod::Etc::Deflect_Rocket_Spew
{
	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Destroy, bool bGlow, bool bDud)
	{
		static int msg_type = usermessages->LookupUserMessage("BreakModelRocketDud");
		
		if (bDud && msg_type != -1) {
			auto rocket = reinterpret_cast<CTFBaseRocket *>(this);
			
			extern ConVar cvar_enable;
			int count = Clamp(cvar_enable.GetInt(), 0, 1000);
			
			const Vector& origin = rocket->GetAbsOrigin();
			const QAngle& angles = rocket->GetAbsAngles();
			int model_index      = rocket->GetModelIndex();
			
			CPVSFilter filter(origin);
			
			for (int i = 0; i < count; ++i) {
				bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
				if (msg == nullptr) break;
				
				msg->WriteShort(model_index);
				msg->WriteBitVec3Coord(origin);
				msg->WriteBitAngles(angles);
				
				engine->MessageEnd();
			}
			
			bDud = false;
		}
		
		DETOUR_MEMBER_CALL(bGlow, bDud);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Deflect_Rocket_Spew")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Destroy, "CTFBaseRocket::Destroy");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_deflect_rocket_spew", "0", FCVAR_NOTIFY,
		"Mod: intentionally cause multiple rocket breakmodels to spew forth when projectile deflection occurs",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
