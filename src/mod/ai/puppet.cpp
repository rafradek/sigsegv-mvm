#include "mod.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "util/misc.h"

#include <worldsize.h>


namespace Mod::AI::Puppet
{
	class CTFBotPuppet : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotPuppet();
		virtual ~CTFBotPuppet();
		
		virtual const char *GetName() const override { return "Puppet"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
		
		virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override                            { return QueryResponse::NO; }
		virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return QueryResponse::NO; }
		
	private:
		PathFollower *m_PathFollower = nullptr;
	};
	
	
	CTFBotPuppet::CTFBotPuppet()
	{
		this->m_PathFollower = new PathFollower();
	}
	
	CTFBotPuppet::~CTFBotPuppet()
	{
		delete this->m_PathFollower;
	}
	
	
	ActionResult<CTFBot> CTFBotPuppet::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower->SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		this->m_PathFollower->Invalidate();
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotPuppet::Update(CTFBot *actor, float dt)
	{
		auto nextbot = actor->MyNextBotPointer();
		
		Vector vel = actor->GetAbsVelocity();
		nextbot->DisplayDebugText(CFmtStr("|v| = % 7.1f", vel.Length()));
		nextbot->DisplayDebugText(CFmtStr("v.x = % 7.1f", vel.x));
		nextbot->DisplayDebugText(CFmtStr("v.y = % 7.1f", vel.y));
		nextbot->DisplayDebugText(CFmtStr("v.z = % 7.1f", vel.z));
		
		this->m_PathFollower->Update(nextbot);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	
	ConVar cvar_debug("sig_ai_puppet_debug", "0", FCVAR_CHEAT);
	
	#define PuppetMsg(...) do { if (cvar_debug.GetBool()) { \
		ConColorMsg(Color(0xff, 0x00, 0xff, 0xff), ##__VA_ARGS__); } } while (false)
	
	EventDesiredResult<CTFBot> CTFBotPuppet::OnCommandString(CTFBot *actor, const char *cmd)
	{
		PuppetMsg("[OnCommandString] actor #%d \"%s\", cmd \"%s\"\n", ENTINDEX(actor), actor->GetPlayerName(), cmd);
		
		CSplitString split(cmd, " ");
		
		PuppetMsg("[OnCommandString] CSplitString has %d elements:\n", split.Count());
		FOR_EACH_VEC(split, i) {
			PuppetMsg("  - \"%s\"\n", split[i]);
		}
		
		if (split.Count() >= 1 && FStrEq(split[0], "goto")) {
			PuppetMsg("[OnCommandString] got 'goto' instruction\n");
			
			RouteType rtype = DEFAULT_ROUTE;
			if (split.Count() >= 2) {
				if (FStrEq(split[1], "fastest")) rtype = FASTEST_ROUTE;
				if (FStrEq(split[1], "safest"))  rtype =  SAFEST_ROUTE;
				if (FStrEq(split[1], "retreat")) rtype = RETREAT_ROUTE;
			}
			
			PuppetMsg("[OnCommandString] rtype is %d\n", (int)rtype);
			
		//	Assert(UTIL_GetListenServerHost() != nullptr);
			CBasePlayer *host = UTIL_PlayerByIndex(1);
			Vector fwd; host->EyeVectors(&fwd);
			
			PuppetMsg("[OnCommandString] listen server host is #%d \"%s\"\n", ENTINDEX(host), host->GetPlayerName());
			PuppetMsg("[OnCommandString] fwd is     [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(fwd));
			PuppetMsg("[OnCommandString] eye pos is [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(host->EyePosition()));
			
			Vector from = host->EyePosition();
			PuppetMsg("[OnCommandString] trace from [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(from));
			Vector to = host->EyePosition() + (fwd * MAX_TRACE_LENGTH);
			PuppetMsg("[OnCommandString] trace to   [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(to));
			
			trace_t tr;
			UTIL_TraceLine(host->EyePosition(), host->EyePosition() + (fwd * MAX_TRACE_LENGTH), MASK_SOLID, host, COLLISION_GROUP_NONE, &tr);
			
			PuppetMsg("[OnCommandString] tr.startpos:   [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(tr.startpos));
			PuppetMsg("[OnCommandString] tr.endpos:     [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(tr.endpos));
			PuppetMsg("[OnCommandString] tr.fraction:   %.3f\n", tr.fraction);
			PuppetMsg("[OnCommandString] tr.allsolid:   %s\n", (tr.allsolid      ? "true" : "false"));
			PuppetMsg("[OnCommandString] tr.startsolid: %s\n", (tr.startsolid    ? "true" : "false"));
		//	PuppetMsg("[OnCommandString] hit world:     %s\n", (tr.DidHitWorld() ? "true" : "false"));
		//	PuppetMsg("[OnCommandString] hit entity:    #%d\n", tr.GetEntityIndex());
			
			if (cvar_debug.GetBool()) {
				NDebugOverlay::Box(tr.startpos, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), 0x00, 0xff, 0x00, 0xff, 10.0f);
				NDebugOverlay::EntityTextAtPosition(tr.startpos, 1, "tr.startpos", 10.0f);
				
				NDebugOverlay::Line(tr.startpos, tr.endpos, 0xff, 0xff, 0x00, false, 10.0f);
				
				NDebugOverlay::Box(tr.endpos, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), 0xff, 0x00, 0x00, 0xff, 10.0f);
				NDebugOverlay::EntityTextAtPosition(tr.endpos, 1, "tr.endpos", 10.0f);
			}
			
			if (tr.DidHit()) {
				PuppetMsg("[OnCommandString] trace did hit; computing path to tr.endpos\n");
				
				CTFBotPathCost path_cost(actor, rtype);
				if (this->m_PathFollower->Compute(actor->MyNextBotPointer(), tr.endpos, path_cost, 0.0f, true)) {
					PuppetMsg("[OnCommandString] path computation succeeded\n");
					NDebugOverlay::Cross3D(tr.endpos, 5.0f, 0x00, 0xff, 0x00, false, 10.0f);
				} else {
					PuppetMsg("[OnCommandString] path computation failed\n");
					NDebugOverlay::Cross3D(tr.endpos, 5.0f, 0xff, 0x00, 0x00, false, 10.0f);
				}
			}
		}
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotMainAction_InitialContainedAction, CTFBot *actor)
	{
		return new CTFBotPuppet();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:Puppet")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_InitialContainedAction, "CTFBotMainAction::InitialContainedAction");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_puppet", "0", FCVAR_NOTIFY,
		"Mod: identical CTFBotPuppet action from TF2Classic for test comparison purposes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
