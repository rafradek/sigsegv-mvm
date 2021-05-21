#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
//#include "util/misc.h"


namespace Mod::AI::Giant_Pyro
{
	ConVar cvar_attack("sig_ai_giant_pyro_attack", "0", FCVAR_NOTIFY, "");
	
	ConVar cvar_eye_x("sig_ai_giant_pyro_eye_x", "0", FCVAR_NOTIFY, "");
	ConVar cvar_eye_y("sig_ai_giant_pyro_eye_y", "0", FCVAR_NOTIFY, "");
	ConVar cvar_eye_z("sig_ai_giant_pyro_eye_z", "0", FCVAR_NOTIFY, "");
	
	
	class CTFBotGiantPyro : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotGiantPyro() {}
		virtual ~CTFBotGiantPyro() {}
		
		virtual const char *GetName() const override { return "GiantPyro"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
	};
	
	
	ActionResult<CTFBot> CTFBotGiantPyro::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotGiantPyro::Update(CTFBot *actor, float dt)
	{
		if (cvar_attack.GetBool()) {
			actor->PressFireButton();
		} else {
			actor->ReleaseFireButton();
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	
	EventDesiredResult<CTFBot> CTFBotGiantPyro::OnCommandString(CTFBot *actor, const char *cmd)
	{
		if (FStrEq(cmd, "teleport")) {
			Vector vecOrigin  (0.0f, 0.0f, 0.0f);
			QAngle vecAngles  (cvar_eye_x.GetFloat(), cvar_eye_y.GetFloat(), cvar_eye_z.GetFloat());
			Vector vecVelocity(0.0f, 0.0f, 0.0f);
			
			if (FStrEq(actor->GetPlayerName(), "Normal Pyro")) vecOrigin = Vector(  0.0f, 4096.0f, -1700.0f);
			if (FStrEq(actor->GetPlayerName(), "Giant Pyro"))  vecOrigin = Vector(128.0f, 4096.0f, -1700.0f);
			
			if (FStrEq(actor->GetPlayerName(), "Normal Flamethrower"))    vecOrigin = Vector(  0.0f, 4096.0f, -1700.0f);
			if (FStrEq(actor->GetPlayerName(), "MvM Giant Flamethrower")) vecOrigin = Vector(128.0f, 4096.0f, -1700.0f);
			
			actor->Teleport(&vecOrigin, &vecAngles, &vecVelocity);
		}
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		/* do nothing */
		return ActionResult<CTFBot>::Continue();
	}
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotMainAction_InitialContainedAction, CTFBot *actor)
	{
		return new CTFBotGiantPyro();
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFBot_DrawDebugTextOverlays)
	{
		int line = 1;
		
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		bot->EntityText(line++, CFmtStr("%s", bot->GetPlayerName()), 0.0f, 0xff, 0xff, 0xff, 0xff);
		
		return line;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:Giant_Pyro")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_InitialContainedAction, "CTFBotMainAction::InitialContainedAction");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_DrawDebugTextOverlays, "CTFBot::DrawDebugTextOverlays");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_giant_pyro", "0", FCVAR_NOTIFY,
		"Mod: TFBot AI action for MvM Giant Flamethrower video purposes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
