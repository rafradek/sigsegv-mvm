#include "mod.h"
#include "stub/projectiles.h"
#include "stub/baseplayer.h"


namespace Mod::Debug::Grenade_Logger
{
	// col: grenade #
	// col: server time
	// col: pos x
	// col: pos y
	// col: pos z
	// col: angles pitch
	// col: angles yaw
	// col: angles roll
	// col: vel x
	// col: vel y
	// col: vel z
	// col: avel pitch
	// col: avel yaw
	// col: avel roll
	// col: speed
	// col: delta speed
	// col: acceleration
	// col: mass
	// col: linear drag
	// col: estimated area
	
	
	struct GrenadeData
	{
		int id;
		float curtime;
		
		Vector pos;
		QAngle angles;
		
		Vector vel;
		AngularImpulse avel;
		
		float speed;
		float dspeed;
		float accel;
		
		float mass;
		float linear_drag;
		float est_area;
		
		bool operator<(const GrenadeData& that) const
		{
			if (this->id != that.id) {
				return (this->id < that.id);
			}
			
			return (this->curtime < that.curtime);
		}
	};
	
	
	std::map<CHandle<CTFWeaponBaseGrenadeProj>, int> idmap;
	std::vector<GrenadeData> data;
	
	
	int GetProjectileID(CTFWeaponBaseGrenadeProj *proj)
	{
		static int counter = 0;
		
		auto it = idmap.find(proj);
		if (it == idmap.end()) {
			it = (idmap.insert(std::make_pair(proj, counter++))).first;
		}
		
		return (*it).second;
	}
	
	
	CON_COMMAND(sig_debug_grenade_logger_reset, "")
	{
		Msg("Cleared %u grenade data records\n", data.size());
		data.clear();
	}
	
	CON_COMMAND(sig_debug_grenade_logger_write, "")
	{
		if (args.ArgC() != 2) {
			Msg("Usage: %s <path>", args[0]);
			return;
		}
		
		const char *path = args[1];
		
		FILE *file = fopen(path, "w");
		if (file == nullptr) {
			Warning("fopen(\"%s\") failed with errno %d\n", path, errno);
			return;
		}
		
		std::stable_sort(data.begin(), data.end());
		
		fprintf(file, "id,time,"
			"pos_x,pos_y,pos_z,"
			"angles_p,angles_y,angles_r,"
			"vel_x,vel_y,vel_z,"
			"angvel_p,angvel_y,angvel_r,"
			"speed,delta_speed,accel,"
			"mass,linear_drag,est_area\n");
		
		for (const auto& datum : data) {
			fprintf(file, "%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
				datum.id, datum.curtime,
				datum.pos.x, datum.pos.y, datum.pos.z,
				datum.angles.x, datum.angles.y, datum.angles.z,
				datum.vel.x, datum.vel.y, datum.vel.z,
				datum.avel.x, datum.avel.y, datum.avel.z,
				datum.speed, datum.dspeed, datum.accel,
				datum.mass, datum.linear_drag, datum.est_area);
		}
		
		fclose(file);
		
		Msg("Wrote %u grenade data records to CSV file \"%s\"\n", data.size(), path);
		data.clear();
	}
	
	
	void RecordData(CTFWeaponBaseGrenadeProj *proj)
	{
		IPhysicsObject *physobj = proj->VPhysicsGetObject();
		if (physobj == nullptr) return;
		
		
		// FIXME: this structure will grow over time unnecessarily
		static std::map<CHandle<CTFWeaponBaseGrenadeProj>, float> lastspeed;
		
		
		GrenadeData datum;
		
		datum.id = GetProjectileID(proj);
		datum.curtime = gpGlobals->curtime;
		
		
		datum.pos = proj->GetAbsOrigin();
		datum.angles = proj->GetAbsAngles();
		
		
		proj->GetVelocity(&datum.vel, &datum.avel);
		
		
		datum.speed = datum.vel.Length();
		
		float speed_prev = 0.0f;
		auto it = lastspeed.find(proj);
		if (it != lastspeed.end()) {
			speed_prev = (*it).second;
		}
		lastspeed[proj] = datum.speed;
		
		datum.dspeed = datum.speed - speed_prev;
		datum.accel = datum.dspeed / gpGlobals->curtime;
		
		
		datum.mass = physobj->GetMass();
		
		Vector vel_unit = datum.vel;
		VectorNormalize(vel_unit);
		datum.linear_drag = physobj->CalculateLinearDrag(vel_unit);
		
		constexpr float coeff = 1.0f;
		datum.est_area = (datum.linear_drag * datum.mass / coeff) / (0.0254f * 0.0254f);
		
		
		data.push_back(datum);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Grenade_Logger") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CTFWeaponBaseGrenadeProj *>(IBaseProjectileAutoList::AutoList()[i]);
				if (proj == nullptr) continue;
				
				RecordData(proj);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_grenade_logger", "0", FCVAR_NOTIFY,
		"Debug: log grenade data to a CSV file",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
