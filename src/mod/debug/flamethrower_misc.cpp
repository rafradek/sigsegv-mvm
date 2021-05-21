#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "util/iterate.h"


namespace Mod::Debug::Flamethrower_Misc
{
	ConVar cvar_off1("sig_debug_flamethrower_misc_off1", "+0.0 +0.0 +0.0", 0);
	ConVar cvar_off2("sig_debug_flamethrower_misc_off2", "+0.0 +0.0 +0.0", 0);
	ConVar cvar_off3("sig_debug_flamethrower_misc_off3", "+0.0 +0.0 +0.0", 0);
	ConVar cvar_off4("sig_debug_flamethrower_misc_off4", "+0.0 +0.0 +0.0", 0);
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Flamethrower_Misc") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			NDebugOverlay::Clear();
			
			ForEachEntityByRTTI<CTFFlameThrower>([](CTFFlameThrower *pFlameThrower){
				CTFPlayer *pOwner = ToTFPlayer(pFlameThrower->GetOwnerEntity());
				if (pOwner == nullptr) return;
				
				constexpr Color cEye   (0x00, 0xff, 0x00, 0xff);
				constexpr Color cShoot (0x00, 0xff, 0xff, 0xff);
				constexpr Color cMuzzle(0xff, 0xff, 0x00, 0xff);
				constexpr Color cFlame (0xff, 0x00, 0x00, 0xff);
				
				auto l_mark_pos = [](const char *name, const Vector& v, const Color& c, const Vector& off){
					CFmtStr coord("[%+3.0f %+3.0f %+3.0f]", v.x, v.y, v.z);
					
					NDebugOverlay::Cross3D(v, 3.0f, c[0], c[1], c[2], true, 1.0f);
					NDebugOverlay::EntityTextAtPosition(v + off, 0, name,  1.0f, c[0], c[1], c[2], c[3]);
					NDebugOverlay::EntityTextAtPosition(v + off, 1, coord, 1.0f, c[0], c[1], c[2], c[3]);
				};
				
				auto l_parse_cvar = [](const ConVar& cvar){
					Vector vec;
					sscanf(cvar.GetString(), " %f %f %f ", &vec.x, &vec.y, &vec.z);
					return vec;
				};
				
				l_mark_pos("EyePosition",          pOwner->EyePosition(),               cEye,    l_parse_cvar(cvar_off1));
			//	l_mark_pos("Weapon_ShootPosition", pOwner->Weapon_ShootPosition(),      cShoot,  l_parse_cvar(cvar_off2));
				l_mark_pos("VisualMuzzlePos",      pFlameThrower->GetVisualMuzzlePos(), cMuzzle, l_parse_cvar(cvar_off3));
				l_mark_pos("FlameOriginPos",       pFlameThrower->GetFlameOriginPos(),  cFlame,  l_parse_cvar(cvar_off4));
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_flamethrower_misc", "0", FCVAR_NOTIFY,
		"Debug: enable some miscellaneous flamethrower debugging stuff",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#if 1 && 0

// 20161221a server_srv.so
class CTFFlameEntity : public CBaseEntity, public ITFFlameEntityAutoList
{
	// 36c vtable for ITFFlameEntityAutoList
	// 370 Vector m_vecInitialPos
	// 37c Vector m_vecPrevPos
	// 388 Vector m_vecBaseVelocity
	// 394 Vector m_vecAttackerVelocity
	// 3a0 float m_flTimeRemove
	// 3a4 int m_iDmgType
	// 3a8 float m_flDmgAmount
	// 3ac CUtlVector<CHandle<CBaseEntity>> m_hEntitiesBurnt
	// 3c0 CHandle<CBaseEntity> m_hAttacker
	// 3c4 int m_iAttackerTeam
	
	// 3c8 ?
	// 3c9 bool ?
	// 3ca ?
	// 3cb ?
	// 3cc CHandle<CTFFlameThrower> m_hFlameThrower 
};
// sizeof: 0x3d0

#endif
