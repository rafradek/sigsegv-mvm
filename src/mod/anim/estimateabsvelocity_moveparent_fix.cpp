#include "mod.h"
#include "stub/baseentity_cl.h"
#include "stub/baseentity.h"


namespace Mod::Anim::EstimateAbsVelocity_MoveParent_Fix
{
	DETOUR_DECL_MEMBER(void, C_BaseEntity_EstimateAbsVelocity, Vector& vel)
	{
		auto ent = reinterpret_cast<C_BaseEntity *>(this);
		
		DETOUR_MEMBER_CALL(vel);
		
		CBaseEntity *ent_sv = UTIL_EntityByIndex(ent->entindex());
		if (ent_sv != nullptr) {
			// 
			// All of these hardcoded offsets are from Windows version 20170616a
			// 
			
			// client
			unsigned int m_pMoveParent        = (*(unsigned int *)((uintptr_t)this + 0x1ac) & 0xfff);
			unsigned int m_pMoveChild         = (*(unsigned int *)((uintptr_t)this + 0x1b0) & 0xfff);
			unsigned int m_pMovePeer          = (*(unsigned int *)((uintptr_t)this + 0x1b4) & 0xfff);
			unsigned int m_pMovePrevPeer      = (*(unsigned int *)((uintptr_t)this + 0x1b8) & 0xfff);
			unsigned int m_hNetworkMoveParent = (*(unsigned int *)((uintptr_t)this + 0x1bc) & 0xfff);
			unsigned int m_hOldMoveParent     = (*(unsigned int *)((uintptr_t)this + 0x1c0) & 0xfff);
			unsigned int m_hGroundEntity      = (*(unsigned int *)((uintptr_t)this + 0x280) & 0xfff);
			
			(void)ent->GetAbsVelocity();
			ent->CalcAbsolutePosition();
			
			// client
			const Vector& m_vecVelocity     = *(const Vector *)((uintptr_t)this + 0x120);
			const Vector& m_vecAbsVelocity  = *(const Vector *)((uintptr_t)this + 0x15c);
			const Vector& m_vecBaseVelocity = *(const Vector *)((uintptr_t)this + 0x184);
			
			// client
			unsigned int m_fFlags = *(unsigned int *)((uintptr_t)this + 0x37c);
			// 0x301: bits 0 and 8 and 9
			// 0: FL_ONGROUND
			// 8: FL_CLIENT
			// 9: FL_FAKECLIENT
			
			// server
			static int off1 = Prop::FindOffsetAssert("CBaseEntity", "m_hMoveParent");
			static int off3 = Prop::FindOffsetAssert("CBaseEntity", "m_hMovePeer");
			static int off2 = Prop::FindOffsetAssert("CBaseEntity", "m_hMoveChild");
			unsigned int sv_m_hMoveParent = (*(unsigned int *)((uintptr_t)ent_sv + off1) & 0xfff);
			unsigned int sv_m_hMoveChild  = (*(unsigned int *)((uintptr_t)ent_sv + off2) & 0xfff);
			unsigned int sv_m_hMovePeer   = (*(unsigned int *)((uintptr_t)ent_sv + off3) & 0xfff);
			
			// TODO: m_vecBaseVelocity
			// TODO: ground entity!!!
			// TODO: MOVETYPE_PUSH
			// TODO: FL_CONVEYOR
			// TODO: FL_BASEVELOCITY
			
			// CPlayerMove::CheckMovingGround is fundamentally important to how conveyors work
			// ALSO: CBaseEntity::UpdateBaseVelocity, WHICH IS SHARED BUT ONLY DOES ANYTHING SERVER-SIDE!
			
			auto l = [](unsigned int h) -> int { return (h != 0xfff ? h : -1); };
			
		//	DevMsg("Ent %4d: [m_pMoveParent: %4d] [m_pMoveChild: %4d] [m_pMovePeer: %4d (old: %4d)] [m_hNetworkMoveParent: %4d (old: %4d)]\n"
		//		"  Server: [m_hMoveParent: %4d] [m_hMoveChild: %4d] [m_hMovePeer: %4d]\n"
		//		"Velocity: [m_vecVelocity: % 7.1f] [m_vecAbsVelocity: % 7.1f] [m_vecBaseVelocity: % 7.1f]\n"
		//		"  Ground: [m_hGroundEntity: %4d] [m_fFlags: %08x]\n\n",
		//		ent->entindex(), l(m_pMoveParent), l(m_pMoveChild), l(m_pMovePeer), l(m_pMovePrevPeer), l(m_hNetworkMoveParent), l(m_hOldMoveParent),
		//		l(sv_m_hMoveParent), l(sv_m_hMoveChild), l(sv_m_hMovePeer),
		//		m_vecVelocity.Length(), m_vecAbsVelocity.Length(), m_vecBaseVelocity.Length(),
		//		l(m_hGroundEntity), m_fFlags);
		}
		
		if (ent->entindex() != engineclient->GetLocalPlayer()) {
			Vector old_vel = vel;
			
#if 1
			// METHOD #1: just use GetAbsVelocity instead (non-ideal but easy)
			
			vel = ent->GetAbsVelocity();
			DevMsg("C_BaseEntity::EstimateAbsVelocity [#%d %08x]: was % 7.1f, now % 7.1f\n",
				ent->entindex(), (uintptr_t)ent, old_vel.Length(), vel.Length());
#elif 0
			// METHOD #2: directly compensate for the move-parent's velocity (better but harder)
			// (ideally: use the parent's EstimateAbsVelocity, rather than their GetAbsVelocity)
			// (even more ideal: also do the attachment velocity stuff like C_BaseEntity::CalculateAbsoluteVelocity does)
			
			C_BaseEntity *parent = ent->GetMoveParent();
			if (parent != nullptr) {
				/* doing the call in this weird way is mandatory */
				matrix3x4_t xform; xform = ent->GetParentToWorldTransform(xform);
				
				Vector new_vel;
				VectorRotate(vel, xform, new_vel);
				
				Vector parent_vel = parent->GetAbsVelocity(); // TODO: try EstimateAbsVelocity
				vel -= parent_vel;
				
				DevMsg("C_BaseEntity::EstimateAbsVelocity [#%d %08x]: was % 7.1f, parent % 7.1f, now % 7.1f\n",
					ent->entindex(), (uintptr_t)ent, old_vel.Length(), parent_vel.Length(), vel.Length());
			}
#else
			// METHOD #3: subtract the entity's base velocity
			// (only works if the client actually HAS the base velocity, which isn't the case for players other than the
			// local player, due to m_vecBaseVelocity being in DT_LocalPlayerExclusive)
			
			Vector sub = *(const Vector *)((uintptr_t)this + 0x184);
			vel -= sub;
			
			DevMsg("C_BaseEntity::EstimateAbsVelocity [#%d %08x]: was % 7.1f, sub % 7.1f, now % 7.1f\n",
				ent->entindex(), (uintptr_t)ent, old_vel.Length(), sub.Length(), vel.Length());
#endif
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Anim:EstimateAbsVelocity_MoveParent_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(C_BaseEntity_EstimateAbsVelocity, "[client] C_BaseEntity::EstimateAbsVelocity");
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_anim_estimateabsvelocity_moveparent_fix", "0", FCVAR_NOTIFY,
		"Mod: fix client/server animation de-sync (move speed, and therefore sequence) when players stand on moving entities (e.g. func_conveyor)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
