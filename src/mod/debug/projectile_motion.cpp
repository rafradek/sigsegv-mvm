#include "mod.h"
#include "stub/projectiles.h"
#include "util/iterate.h"


namespace Mod::Debug::Projectile_Motion
{
	IPhysicsEnvironment *GetPhysicsEnvironment()
	{
		assert(physics != nullptr);
		static IPhysicsEnvironment *physenv = physics->GetActiveEnvironmentByIndex(0);
		assert(physenv != nullptr);
		return physenv;
	}
	
	
	ConVar cvar_drag("sig_debug_projectile_motion_drag", "normal", FCVAR_NOTIFY,
		"Debug: projectile vphysics drag override (normal, forceon, forceoff)");
	ConVar cvar_gravity("sig_debug_projectile_motion_gravity", "normal", FCVAR_NOTIFY,
		"Debug: projectile vphysics gravity override (normal, forceon, forceoff)");
	
	ConVar cvar_dragoverlay("sig_debug_projectile_motion_dragoverlay", "0", FCVAR_NOTIFY,
		"Enable drag coefficient overlay?");
	ConVar cvar_dragoverlay_interval("sig_debug_projectile_motion_dragoverlay_interval", "2", FCVAR_NOTIFY,
		"Frame interval for the drag coefficient debug overlay");
	
	
	void DragCoefficientOverlay()
	{
		ForEachEntity([](CBaseEntity *ent){
			IPhysicsObject *physobj = ent->VPhysicsGetObject();
			if (physobj == nullptr) return;
			
			const char *classname = ent->GetClassname();
			if (strncmp(classname, "prop_", 5) == 0) return;
			if (strncmp(classname, "func_", 5) == 0) return;
			
		//	DevMsg("DragCoefficientOverlay: class \"%s\"\n",
		//		ent->GetClassname());
			
			/* obviously these offsets are not documented publicly; but the
			 * damn interface doesn't expose them, so this is the only way */
			float m_flDragCoefficient        = *(float *)((uintptr_t)physobj + 0x44);
			float m_flAngularDragCoefficient = *(float *)((uintptr_t)physobj + 0x48);
			
			Vector vel;
			Vector avel;
			physobj->GetVelocity(&vel, &avel);
			
			Vector vel_unit = vel;
			VectorNormalize(vel_unit);
			float drag_linear = physobj->CalculateLinearDrag(vel_unit);
			
			/* not sure that this is right... */
			float drag_angular = physobj->CalculateAngularDrag(avel);
			
			float duration = cvar_dragoverlay_interval.GetInt() * gpGlobals->interval_per_tick;
			
			char buf[128];
			
//			snprintf(buf, sizeof(buf), "    DRAG COEF: %5.3f", m_flDragCoefficient);
//			NDebugOverlay::EntityText(ENTINDEX(ent), 0, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
//			snprintf(buf, sizeof(buf), "ANG DRAG COEF: %5.3f", m_flAngularDragCoefficient);
//			NDebugOverlay::EntityText(ENTINDEX(ent), 1, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
			// A_drag = -(0.5 * C_drag * area * rho * v_obj^2) / M_obj
			
			
			snprintf(buf, sizeof(buf), "   LINEAR VEL: %5.3f", vel.Length());
			NDebugOverlay::EntityText(ENTINDEX(ent), 2, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
			snprintf(buf, sizeof(buf), "  ANGULAR VEL: %5.3f", avel.Length());
			NDebugOverlay::EntityText(ENTINDEX(ent), 3, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
			
			snprintf(buf, sizeof(buf), "  LINEAR DRAG: %f", drag_linear);
			NDebugOverlay::EntityText(ENTINDEX(ent), 4, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
			snprintf(buf, sizeof(buf), " ANGULAR DRAG: %f", drag_angular);
			NDebugOverlay::EntityText(ENTINDEX(ent), 5, buf, duration, 0xff, 0xff, 0xff, 0xff);
		});
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Projectile_Motion") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			++frame;
			
			bool drag_change = false;
			bool drag_value  = false;
			
			const char *drag = cvar_drag.GetString();
			if (stricmp(drag, "forceon") == 0) {
				drag_change = true;
				drag_value  = true;
			} else if (stricmp(drag, "forceoff") == 0) {
				drag_change = true;
				drag_value  = false;
			}
			
			bool gravity_change = false;
			bool gravity_value  = false;
			
			const char *gravity = cvar_gravity.GetString();
			if (stricmp(gravity, "forceon") == 0) {
				gravity_change = true;
				gravity_value  = true;
			} else if (stricmp(gravity, "forceoff") == 0) {
				gravity_change = true;
				gravity_value  = false;
			}
			
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
				assert(proj != nullptr);
				
				IPhysicsObject *physobj = proj->VPhysicsGetObject();
				if (physobj != nullptr) {
					if (drag_change) {
						physobj->EnableDrag(drag_value);
					}
					
					if (gravity_change) {
						physobj->EnableGravity(gravity_value);
					}
				}
			}
			
			if (cvar_dragoverlay.GetBool() && frame % cvar_dragoverlay_interval.GetInt() == 0) {
				DragCoefficientOverlay();
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_projectile_motion", "0", FCVAR_NOTIFY,
		"Debug: projectile motion",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_debug_projectile_motion_vphysics_debug, "Call IPhysicsObject::OutputDebugInfo on each projectile")
	{
		for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
			auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
			assert(proj != nullptr);
			
			IPhysicsObject *physobj = proj->VPhysicsGetObject();
			if (physobj != nullptr) {
				physobj->OutputDebugInfo();
			}
		}
	}
	
	
	CON_COMMAND(sig_debug_projectile_motion_airdensity_get, "IPhysicsEnvironment::GetAirDensity()")
	{
		float density = GetPhysicsEnvironment()->GetAirDensity();
		Msg("Air density: %f\n", density);
	}
	
	CON_COMMAND(sig_debug_projectile_motion_airdensity_set, "IPhysicsEnvironment::SetAirDensity()")
	{
		float old_density = GetPhysicsEnvironment()->GetAirDensity();
		float new_density = std::stof(args[1]);
		GetPhysicsEnvironment()->SetAirDensity(new_density);
		Msg("Air density: was %f, now %f\n", old_density, new_density);
	}
}
