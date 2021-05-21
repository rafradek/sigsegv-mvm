#include "mod.h"
#include "stub/projectiles.h"
#include "stub/baseplayer.h"


namespace Mod::Visualize::Projectile_Speed
{
	IPhysicsEnvironment *GetPhysicsEnvironment()
	{
		assert(physics != nullptr);
		static IPhysicsEnvironment *physenv = physics->GetActiveEnvironmentByIndex(0);
		assert(physenv != nullptr);
		return physenv;
	}
	
	
	ConVar cvar_interval("sig_visualize_projectile_speed_interval", "2", FCVAR_NOTIFY,
		"Visualization: frame interval");
	
	
	// IPhysicsObject::CalculateLinearDrag returns the following quantity:
	// coefficient_of_drag * projected_area / mass
	// in units of: m^2 kg^-1
	
	
	void SpeedOverlay(CBaseProjectile *proj)
	{
		// FIXME: this structure will grow over time unnecessarily
		static std::map<CHandle<CBaseProjectile>, float> lastspeed;
		
		Vector v_now;
		proj->GetVelocity(&v_now);
		float s_now = v_now.Length();
		
		bool have_init = false;
		Vector v_init = vec3_origin;
		float s_init = 0.0f;
		
		CTFBaseRocket *rocket             = nullptr;
		CTFWeaponBaseGrenadeProj *grenade = nullptr;
		
		if ((rocket = rtti_cast<CTFBaseRocket *>(proj)) != nullptr) {
			v_init = rocket->m_vInitialVelocity;
			s_init = v_init.Length();
			have_init = true;
		} else if ((grenade = rtti_cast<CTFWeaponBaseGrenadeProj *>(proj)) != nullptr) {
			v_init = grenade->m_vInitialVelocity;
			s_init = v_init.Length();
			have_init = true;
		}
		
		float s_last = 0.0f;
		
		auto it = lastspeed.find(proj);
		if (it != lastspeed.end()) {
			s_last = (*it).second;
		}
		lastspeed[proj] = s_now;
		
		float a_tangent = (s_now - s_last) / (cvar_interval.GetInt() * gpGlobals->interval_per_tick);
		
		float duration = cvar_interval.GetInt() * gpGlobals->interval_per_tick;
		
		char buf[128];
		
		CBaseEntity *launcher = proj->GetOriginalLauncher();
		if (launcher != nullptr) {
			CBasePlayer *owner = ToBasePlayer(launcher->GetOwnerEntity());
			if (owner != nullptr) {
				NDebugOverlay::EntityText(ENTINDEX(proj), 0, owner->GetPlayerName(), duration, 0xff, 0xff, 0xff, 0xff);
			}
		}
		
//		snprintf(buf, sizeof(buf), "Speed: %5.0f HU/s", s_now);
//		NDebugOverlay::EntityText(ENTINDEX(proj), 1, buf, duration, 0xff, 0xff, 0xff, 0xff);
		
		snprintf(buf, sizeof(buf), "Accel: %+5.0f HU/s^2", a_tangent);
		NDebugOverlay::EntityText(ENTINDEX(proj), 2, buf, duration, 0xff, 0xff, 0xff, 0xff);
		
		IPhysicsObject *physobj = proj->VPhysicsGetObject();
		if (physobj != nullptr) {
			Vector vel;
			physobj->GetVelocity(&vel, nullptr);
			VectorNormalize(vel);
			float drag = physobj->CalculateLinearDrag(vel);
			
			float mass  = physobj->GetMass();
		//	float rho   = GetPhysicsEnvironment()->GetAirDensity() * CUBIC_METERS_PER_CUBIC_INCH;
		//	float v_sqr = Square(s_now);
			float coeff = 1.0f;
			
			float area = (drag * mass / coeff) / (0.0254f * 0.0254f);
			
			snprintf(buf, sizeof(buf), " Area: %5.0f HU^2", area);
			NDebugOverlay::EntityText(ENTINDEX(proj), 3, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
//			snprintf(buf, sizeof(buf), " Mass: %5.2f kg", mass);
//			NDebugOverlay::EntityText(ENTINDEX(proj), 4, buf, duration, 0xff, 0xff, 0xff, 0xff);
			
#if 0
			
			/* assumptions */
			float mass = 5.0f;
			float force = a_tangent * mass;
			float coeff = 1.0f;
			float density = 2.0f * (0.0254f * 0.0254f * 0.0254f);
			
			float c_drag = -force * duration * Square(mass) / Square(s_now);
			float area = -2.0f * force / (coeff * density * Square(s_now));
			
			DevMsg("#%d: [vel: %.5e] [vel^2: %.5e] [accel: %.5e] [drag: %.5e] [a/v^2: %.5e] [c_drag: %.5e] [area: %.5e] [c_drag / drag: %.5e]\n",
				ENTINDEX(proj),
				s_now,
				Square(s_now),
				a_tangent,
				drag,
				a_tangent / Square(s_now),
				c_drag,
				area,
				c_drag / drag);
#endif
		}
		
		// TODO:
		// draw the vphysics bounding box
		// (get the entity's bbox, rotate it based on angles from physobj->GetPosition)
		
		// physobj->GetCollide?
		
		// check out how vcollide_wireframe works and try to replicate it if possible
		
		
	//	if (have_init) {
	//		snprintf(buf, sizeof(buf), " INIT: %.0f HU/s", s_init);
	//		NDebugOverlay::EntityText(ENTINDEX(proj), 4, buf, duration, 0xff, 0xff, 0xff, 0xff);
	//	}
		
		NDebugOverlay::EntityBounds(proj, 0xff, 0xff, 0xff, 0xff, cvar_interval.GetInt() * gpGlobals->interval_per_tick);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Projectile_Speed") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % cvar_interval.GetInt() != 0) return;
			
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
				assert(proj != nullptr);
				
				SpeedOverlay(proj);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_projectile_speed", "0", FCVAR_NOTIFY,
		"Visualization: projectile speed overlay",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
