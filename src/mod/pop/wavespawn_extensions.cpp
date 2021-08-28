#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "mod/pop/pointtemplate.h"
#include "stub/populators.h"
#include "stub/nextbot_cc_behavior.h"
#include "stub/tf_objective_resource.h"
#include "stub/gamerules.h"
#include "stub/misc.h"

namespace Mod::Pop::Wave_Extensions
{
	void ParseColorsAndPrint(const char *line, float gameTextDelay, int &linenum, CTFPlayer* player = nullptr);
}

namespace Mod::Pop::WaveSpawn_Extensions
{
	
	void PointTemplateSpawnerKillCallback(PointTemplateInstance *inst);

	class CPointTemplateSpawner;
	
	std::unordered_set<CPointTemplateSpawner *> template_spawners;

	CHandle<CBaseEntity> spawner_sentinel;
	
	class CHalloweenBossSpawner;

	std::map<CHandle<CBaseEntity>, CHalloweenBossSpawner *> boss_spawners;

	class CPointTemplateSpawner : public IPopulationSpawner
	{
	public:
		CPointTemplateSpawner( IPopulator *populator ) : IPopulationSpawner(populator)
		{
			m_Populator = rtti_cast<CWaveSpawnPopulator *>(populator);
			template_spawners.insert(this);
		};

		virtual ~CPointTemplateSpawner() {
			template_spawners.erase(this);
		}

		virtual bool IsWhereRequired( void ) const
		{
			return m_bUseWhere;
		}
		
		virtual bool IsMiniBoss( int nSpawnNum = -1 ) override { return m_bIsMiniBoss; }
		virtual string_t GetClassIcon( int nSpawnNum = -1 ) { return m_strClassIcon; }
		
		virtual bool HasEventChangeAttributes( const char* pszEventName ) const override { return false; }

		virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 )
		{ 
			if (type == CTFBot::ATTR_ALWAYS_CRIT)
				return m_bIsCrit;
			else
				return false;
		}

		virtual bool Parse( KeyValues *data ) override
		{
			m_info = Parse_SpawnTemplate(data);
			FOR_EACH_SUBKEY(data, subkey) {
				const char *name = subkey->GetName();
				if (FStrEq(name, "IsMiniBoss")) {
					m_bIsMiniBoss = subkey->GetBool();
				}
				else if (FStrEq(name, "IsCrit")) {
					m_bIsCrit = subkey->GetBool();
				}
				else if (FStrEq(name, "SpawnAtEntity")) {
					m_SpawnAtEntity = subkey->GetString();
					if (FStrEq(subkey->GetString(), "Where")) {
						m_bUseWhere = true;
					}
				}
				else if (FStrEq(name, "ClassIcon")) {
					m_strClassIcon = AllocPooledString(subkey->GetString());
				}
				else if (FStrEq(name, "SpawnCurrencyPack")) {
					m_bSpawnCurrencyPack = true;
					m_sSpawnCurrencyPack = subkey->GetString();
				}
				else if (FStrEq(name, "SpreadRadius")) {
					sscanf(subkey->GetString(), "%f %f %f", &m_SpreadRadius.x, &m_SpreadRadius.y, &m_SpreadRadius.z);
					m_bHasSpreadRadius = true;
				}
				else if (FStrEq(name, "StickToGround")) {
					m_fStickToGround = subkey->GetFloat();
				}
			}
			if (m_info.templ == nullptr)
			{
				Warning("PointTemplateSpawner: missing template (%s)\n", m_info.template_name.c_str());
				return false;
			}
			return true;
		}

		virtual bool Spawn( const Vector &here, CUtlVector<CHandle<CBaseEntity>> *ents ) override
		{
			std::shared_ptr<PointTemplateInstance> inst;
			Vector spawnpos = m_info.translation;
			CBaseEntity *entity_target = nullptr;
			if (!m_bUseWhere) {
				if (m_SpawnAtEntity != "") {
					entity_target = servertools->FindEntityByName(nullptr, m_SpawnAtEntity.c_str());
					if (entity_target == nullptr) {
						return false;
					}
				}
			}
			else {
				spawnpos += here;
			}

			// Spread entites and stick them to the ground
			if (m_bHasSpreadRadius) {
				spawnpos.x += RandomFloat(-m_SpreadRadius.x, m_SpreadRadius.x);
				spawnpos.y += RandomFloat(-m_SpreadRadius.y, m_SpreadRadius.y);
				spawnpos.z += RandomFloat(-m_SpreadRadius.z, m_SpreadRadius.z);
			}

			if (m_fStickToGround != 0.0f) {
				trace_t result;
				UTIL_TraceLine(spawnpos, spawnpos + Vector(0,0, -m_fStickToGround), MASK_SOLID_BRUSHONLY, null, COLLISION_GROUP_NONE, &result);
				if (result.DidHit()) {
					spawnpos = result.endpos;
				}
			}

			PointTemplate *templ = m_info.templ;
			if (templ == nullptr)
				return false;

			inst = templ->SpawnTemplate(entity_target, spawnpos, m_info.rotation, false, m_info.attachment.c_str(), false);

			if (inst != nullptr) {
				if (ents) {
					int count = inst->entities.size();
					if (count > 0) {
						if (spawner_sentinel == nullptr) {
							spawner_sentinel = CreateEntityByName("logic_relay");
							spawner_sentinel->Spawn();
							spawner_sentinel->Activate();
						}
						ents->AddToTail(spawner_sentinel);
					}
					//for (int i = 0; i < count; i++) {
					//}
				}
				m_Instances.push_back({inst, spawnpos});
				return true;
			}
			else {
				return false;
			}
		}
		
		CWaveSpawnPopulator *m_Populator = nullptr;
		string_t m_strClassIcon = NULL_STRING;
		bool m_bSpawnCurrencyPack = false;
		std::string m_sSpawnCurrencyPack = "";
		std::vector<std::pair<std::shared_ptr<PointTemplateInstance>, Vector>> m_Instances;

	private:
		bool m_bIsMiniBoss = false;
		bool m_bIsCrit = false;
		bool m_bUseWhere = false;
		bool m_bHasSpreadRadius = false;
		float m_fStickToGround = 0.0f;
		Vector m_SpreadRadius;
		std::string m_SpawnAtEntity = "";
		PointTemplateInfo m_info;
	};

	class CHalloweenBossSpawner : public IPopulationSpawner
	{
	public:
		CHalloweenBossSpawner( IPopulator *populator ) : IPopulationSpawner(populator)
		{
			m_Populator = rtti_cast<CWaveSpawnPopulator *>(populator);
		};

		virtual ~CHalloweenBossSpawner() {
			for (auto it = boss_spawners.begin(); it != boss_spawners.end();) {
				if (it->second == this) {
					it = boss_spawners.erase(it);
				}
				else {
					it++;
				}
			}
		}

		virtual bool IsWhereRequired( void ) const
		{
			return !m_bSetOrigin && m_SpawnAtEntity.empty();
		}
		
		virtual bool IsMiniBoss( int nSpawnNum = -1 ) override { return m_bIsMiniBoss; }
		virtual string_t GetClassIcon( int nSpawnNum = -1 ) { return m_strClassIcon; }
		
		virtual bool HasEventChangeAttributes( const char* pszEventName ) const override { return false; }

		virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 )
		{ 
			if (type == CTFBot::ATTR_ALWAYS_CRIT)
				return m_bIsCrit;
			else
				return false;
		}

		virtual bool Parse( KeyValues *data ) override
		{
			FOR_EACH_SUBKEY(data, subkey) {
				const char *name = subkey->GetName();
				if (FStrEq(name, "IsMiniBoss")) {
					m_bIsMiniBoss = subkey->GetBool();
				}
				else if (FStrEq(name, "IsCrit")) {
					m_bIsCrit = subkey->GetBool();
				}
				else if (FStrEq(name, "ClassIcon")) {
					m_strClassIcon = AllocPooledString(subkey->GetString());
				}
				else if (FStrEq(name, "SpawnCurrencyPack")) {
					m_bSpawnCurrencyPack = subkey->GetString();
				}
				else if (FStrEq(name, "SpawnTemplate")) {
					m_Attachements.push_back(Parse_SpawnTemplate(subkey));
				}
				else if (FStrEq(name, "BossType")) {
					if (FStrEq(subkey->GetString(), "HHH")) {
						m_Type = CHalloweenBaseBoss::HEADLESS_HATMAN;
					} else if (FStrEq(subkey->GetString(), "MONOCULUS")) {
						m_Type = CHalloweenBaseBoss::EYEBALL_BOSS;
					} else if (FStrEq(subkey->GetString(), "Merasmus")) {
						m_Type = CHalloweenBaseBoss::MERASMUS;
					} else if (FStrEq(subkey->GetString(), "SkeletonNormal")) {
						m_Type = CZombie::SKELETON_NORMAL + 32;
					} else if (FStrEq(subkey->GetString(), "SkeletonKing")) {
						m_Type = CZombie::SKELETON_KING + 32;
					} else if (FStrEq(subkey->GetString(), "SkeletonSmall")) {
						m_Type = CZombie::SKELETON_SMALL + 32;
					} else {
						Warning("Invalid value \'%s\' for BossType key in HalloweenBoss block.\n", subkey->GetString());
					}
				} else if (FStrEq(name, "TeamNum")) {
					m_iTeamNum = subkey->GetInt();
				} else if (FStrEq(name, "Health")) {
					m_iHealth = Max(1, subkey->GetInt());
				} else if (FStrEq(name, "Lifetime")) {
					m_fLifetime = Max(0.0f, subkey->GetFloat());
				} else if (FStrEq(name, "Speed")) {
					m_fSpeed = Max(0.0f, subkey->GetFloat());
				} else if (FStrEq(name, "DamageMultiplier")) {
					m_fDmgMult = subkey->GetFloat();
				} else if (FStrEq(name, "Origin")) {
					m_bSetOrigin = true;
					sscanf(subkey->GetString(),"%f %f %f", &m_Origin.x, &m_Origin.y, &m_Origin.z);
				} else if (FStrEq(name, "SpreadRadius")) {
					sscanf(subkey->GetString(), "%f %f %f", &m_SpreadRadius.x, &m_SpreadRadius.y, &m_SpreadRadius.z);
					m_bHasSpreadRadius = true;
				} else if (FStrEq(name, "StickToGround")) {
					m_fStickToGround = subkey->GetFloat();
				} else if (FStrEq(name, "SpawnAtEntity")) {
					m_SpawnAtEntity = subkey->GetString();
				} else if (FStrEq(name, "FastUpdate")) {
					m_bFastUpdate = subkey->GetBool();
				}
			}
			return true;
		}

		virtual bool Spawn( const Vector &here, CUtlVector<CHandle<CBaseEntity>> *ents ) override
		{
			static ConVarRef tf_merasmus_lifetime("tf_merasmus_lifetime");
			static ConVarRef tf_eyeball_boss_lifetime("tf_eyeball_boss_lifetime");
			float tf_merasmus_lifetime_old = tf_merasmus_lifetime.GetFloat();
			float tf_eyeball_boss_lifetime_old = tf_eyeball_boss_lifetime.GetFloat();
			
			tf_merasmus_lifetime.SetValue(m_fLifetime);
			tf_eyeball_boss_lifetime.SetValue(m_fLifetime);

			Vector spawnpos = m_bSetOrigin || !m_SpawnAtEntity.empty() ? m_Origin : here;

			if (!m_SpawnAtEntity.empty()) {
				CBaseEntity *entity_target = servertools->FindEntityByName(nullptr, m_SpawnAtEntity.c_str());
				if (entity_target == nullptr) {
					return false;
				}
				spawnpos += entity_target->GetAbsOrigin();
			}
			// Spread entites and stick them to the ground
			if (m_bHasSpreadRadius) {
				spawnpos.x += RandomFloat(-m_SpreadRadius.x, m_SpreadRadius.x);
				spawnpos.y += RandomFloat(-m_SpreadRadius.y, m_SpreadRadius.y);
				spawnpos.z += RandomFloat(-m_SpreadRadius.z, m_SpreadRadius.z);
			}

			if (m_fStickToGround != 0.0f) {
				trace_t result;
				UTIL_TraceLine(spawnpos, spawnpos + Vector(0,0, -m_fStickToGround), MASK_SOLID_BRUSHONLY, null, COLLISION_GROUP_NONE, &result);
				if (result.DidHit()) {
					spawnpos = result.endpos;
				}
			}

			CBaseEntity *boss = nullptr;
			if (m_Type < 32)
				boss = CHalloweenBaseBoss::SpawnBossAtPos((CHalloweenBaseBoss::HalloweenBossType) m_Type, spawnpos, m_iTeamNum, nullptr);
			else
				boss = CZombie::SpawnAtPos(spawnpos, m_fLifetime, m_iTeamNum, nullptr, (CZombie::SkeletonType_t) (m_Type - 32));

			if (boss == nullptr) {
				Warning("SpawnBoss: CHalloweenBaseBoss::SpawnBossAtPos(type %d, teamnum %d) failed\n", m_Type, m_iTeamNum);
				return false;
			}
			
			tf_merasmus_lifetime.SetValue(tf_merasmus_lifetime_old);
			tf_eyeball_boss_lifetime.SetValue(tf_eyeball_boss_lifetime_old);

			if (m_iHealth > 0) {
				boss->SetMaxHealth(m_iHealth);
				boss->SetHealth   (m_iHealth);
			}

			for (auto it1 = m_Attachements.begin(); it1 != m_Attachements.end(); ++it1) {
				it1->SpawnTemplate(boss);
			}

			ents->AddToTail(boss);
			boss_spawners[boss] = this;

			return true;
		}
		
		CWaveSpawnPopulator *m_Populator = nullptr;
		string_t m_strClassIcon = NULL_STRING;
		bool m_bSpawnCurrencyPack = true;
		std::vector<CHandle<CBaseEntity>> m_Instances;
		
		float m_fLifetime = FLT_MAX;
		float m_fSpeed = FLT_MAX;
		float m_fDmgMult = 1.0f;
		bool m_bFastUpdate = false;

	private:
		bool m_bIsMiniBoss = true;
		bool m_bIsCrit = false;
		std::vector<PointTemplateInfo> m_Attachements;
		int m_Type = (int) CHalloweenBaseBoss::INVALID;
		int m_iTeamNum = 5;
		int m_iHealth = 0;
		bool m_bSetOrigin = false;
		bool m_bHasSpreadRadius = false;
		float m_fStickToGround = 0.0f;
		Vector m_SpreadRadius;
		Vector m_Origin;
		std::string m_SpawnAtEntity = "";
	};

	enum InternalStateType
	{
		PENDING,
		PRE_SPAWN_DELAY,
		SPAWNING,
		WAIT_FOR_ALL_DEAD,
		DONE
	};

	struct WaveSpawnData
	{
		std::vector<std::string> start_spawn_message;
		std::vector<std::string> first_spawn_message;
		std::vector<std::string> last_spawn_message;
		std::vector<std::string> done_message;
		InternalStateType state;
	};
	
	std::map<CWaveSpawnPopulator *, WaveSpawnData> wavespawns;
	

	DETOUR_DECL_MEMBER(void, CWaveSpawnPopulator_dtor0)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		
	//	DevMsg("CWaveSpawnPopulator %08x: dtor0\n", (uintptr_t)wavespawn);
		wavespawns.erase(wavespawn);
		
		DETOUR_MEMBER_CALL(CWaveSpawnPopulator_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CWaveSpawnPopulator_dtor2)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		
	//	DevMsg("CWaveSpawnPopulator %08x: dtor2\n", (uintptr_t)wavespawn);
		wavespawns.erase(wavespawn);
		
		DETOUR_MEMBER_CALL(CWaveSpawnPopulator_dtor2)();
	}

	void DisplayMessages(std::vector<std::string> &messages ) {
		int linenum = 0;
		for (auto &string : messages) {
			Mod::Pop::Wave_Extensions::ParseColorsAndPrint(string.c_str(), 0.5f, linenum, nullptr);
		}
	}

	/*DETOUR_DECL_MEMBER(void, CWaveSpawnPopulator_Update, InternalStateType state)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		if (state == WAIT_FOR_ALL_DEAD) {
			DisplayMessages(wavespawns[wavespawn].last_spawn_message);
		else if (state == DONE)
			DisplayMessages(wavespawns[wavespawn].done_message);
		
	//	DevMsg("CWaveSpawnPopulator %08x: dtor2\n", (uintptr_t)wavespawn);
		wavespawns.erase(wavespawn);
		
		DETOUR_MEMBER_CALL(CWaveSpawnPopulator_dtor2)();
	}*/

	DETOUR_DECL_MEMBER(void, CWaveSpawnPopulator_SetState, InternalStateType state)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		wavespawns[wavespawn].state = state;
		switch (state) {
			case PRE_SPAWN_DELAY:
				DisplayMessages(wavespawns[wavespawn].start_spawn_message);
				break;
			case SPAWNING:
				DisplayMessages(wavespawns[wavespawn].first_spawn_message);
				break;
			case WAIT_FOR_ALL_DEAD:
				DisplayMessages(wavespawns[wavespawn].last_spawn_message);
				break;
			case DONE:
				DisplayMessages(wavespawns[wavespawn].done_message);
				break;
		}
		
	//	DevMsg("CWaveSpawnPopulator %08x: dtor2\n", (uintptr_t)wavespawn);
		
		DETOUR_MEMBER_CALL(CWaveSpawnPopulator_SetState)(state);
	}

	DETOUR_DECL_STATIC(IPopulationSpawner *, IPopulationSpawner_ParseSpawner, IPopulator *populator, KeyValues *data)
	{
		if (FStrEq(data->GetName(), "PointTemplate")) {
			CPointTemplateSpawner *spawner = new CPointTemplateSpawner(populator);

			if (!spawner->Parse(data))
			{
				Warning( "Warning reading PointTemplate spawner definition\n" );
				delete spawner;
				return nullptr;
			}
			return spawner;
		}

		else if (FStrEq(data->GetName(), "HalloweenBoss")) {
			CHalloweenBossSpawner *spawner = new CHalloweenBossSpawner(populator);

			if (!spawner->Parse(data))
			{
				Warning( "Warning reading HalloweenBoss spawner definition\n" );
				delete spawner;
				return nullptr;
			}
			return spawner;
		}

		auto result = DETOUR_STATIC_CALL(IPopulationSpawner_ParseSpawner)(populator, data);

		return result;
	}

	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv_orig)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		
		// make a temporary copy of the KV subtree for this populator
		// the reason for this: `kv_orig` *might* be a ptr to a shared template KV subtree
		// we'll be deleting our custom keys after we parse them so that the Valve code doesn't see them
		// but we don't want to delete them from the shared template KV subtree (breaks multiple uses of the template)
		// so we use this temp copy, delete things from it, pass it to the Valve code, then delete it
		// (we do the same thing in Pop:TFBot_Extensions)
		KeyValues *kv = kv_orig->MakeCopy();
		
	//	DevMsg("CWaveSpawnPopulator::Parse\n");
		
		std::vector<KeyValues *> del_kv;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			bool del = true;
			if ( FStrEq(name, "StartWaveMessage"))
				wavespawns[wavespawn].start_spawn_message.push_back(subkey->GetString());
			else if ( FStrEq(name, "FirstSpawnMessage"))
				wavespawns[wavespawn].first_spawn_message.push_back(subkey->GetString());
			else if ( FStrEq(name, "LastSpawnMessage"))
				wavespawns[wavespawn].last_spawn_message.push_back(subkey->GetString());
			else if ( FStrEq(name, "DoneMessage"))
				wavespawns[wavespawn].done_message.push_back(subkey->GetString());
			else {
				del = false;
			}
			
			if (del) {
			//	DevMsg("Key \"%s\": processed, will delete\n", name);
				del_kv.push_back(subkey);
			} else {
			//	DevMsg("Key \"%s\": passthru\n", name);
			}
		}
		
		for (auto subkey : del_kv) {
		//	DevMsg("Deleting key \"%s\"\n", subkey->GetName());
			kv->RemoveSubKey(subkey);
			subkey->deleteThis();
		}
		
		bool result = DETOUR_MEMBER_CALL(CWaveSpawnPopulator_Parse)(kv);
		
		// delete the temporary copy of the KV subtree
		kv->deleteThis();
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CWave_ActiveWaveUpdate)
	{
		auto wave = reinterpret_cast<CWave *>(this);
		DETOUR_MEMBER_CALL(CWave_ActiveWaveUpdate)();
		if (wave->IsDoneWithNonSupportWaves()) {
			for(auto spawner : template_spawners) {
				auto populator = spawner->m_Populator;
				if (populator != nullptr && populator->m_bSupportWave) {
					auto &insts = spawner->m_Instances;
					for (auto it = insts.begin(); it != insts.end(); it++) {
						auto inst = it->first;
						if (inst != nullptr && !inst->mark_delete) {
							inst->OnKilledParent(false);
						}
					}
					FOR_EACH_VEC(populator->m_activeVector, i) {
						if (populator->m_activeVector[i] == spawner_sentinel) {
							populator->m_activeVector.Remove(i);
						}
					}
				}
			}
			
			for(auto &entry : boss_spawners) {
				auto spawner = entry.second;
				auto boss = entry.first;
				auto populator = spawner->m_Populator;
				if (populator != nullptr && populator->m_bSupportWave) {
					if (boss != nullptr && boss->IsAlive()) {
						//boss->Remove();
						boss->TakeDamage(CTakeDamageInfo(UTIL_EntityByIndex(0), UTIL_EntityByIndex(0), nullptr, vec3_origin, vec3_origin, boss->GetHealth() * 6 + 1, DMG_PREVENT_PHYSICS_FORCE));
					}
				}
			}
		}
	}

	CHalloweenBossSpawner *GetBossInfo(CBaseEntity *entity)
	{
		auto find = boss_spawners.find(entity);
		if (find != boss_spawners.end()) {
			return find->second;
		}
		return nullptr;
	}

	/* block attempts by MONOCULUS to switch to CEyeballBossTeleport */
	DETOUR_DECL_MEMBER(ActionResult<CEyeballBoss>, CEyeballBossIdle_Update, CEyeballBoss *actor, float dt)
	{
		auto result = DETOUR_MEMBER_CALL(CEyeballBossIdle_Update)(actor, dt);
		
		if (result.transition == ActionTransition::CHANGE_TO && strcmp(result.reason, "Moving...") == 0) {
			if (TFGameRules()->IsMannVsMachineMode() && GetBossInfo(actor) != nullptr) {
				delete result.action;
				
				result.transition = ActionTransition::CONTINUE;
				result.action     = nullptr;
				result.reason     = nullptr;
			}
		}
		
		return result;
	}
	
	
	RefCount rc_CEyeballBossDead_Update__and_is_from_spawner;
	DETOUR_DECL_MEMBER(ActionResult<CEyeballBoss>, CEyeballBossDead_Update, CEyeballBoss *actor, float dt)
	{
		SCOPED_INCREMENT_IF(rc_CEyeballBossDead_Update__and_is_from_spawner,
			(TFGameRules()->IsMannVsMachineMode() && GetBossInfo(actor) != nullptr));
		
		return DETOUR_MEMBER_CALL(CEyeballBossDead_Update)(actor, dt);
	}
	
	/* prevent MONOCULUS's death from spawning a teleport vortex */
	DETOUR_DECL_STATIC(CBaseEntity *, CBaseEntity_Create, const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner)
	{
		if (rc_CEyeballBossDead_Update__and_is_from_spawner > 0 && strcmp(szName, "teleport_vortex") == 0) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(CBaseEntity_Create)(szName, vecOrigin, vecAngles, pOwner);
	}

	/* set MONOCULUS's lifetime from the spawner parameter instead of from the global convars */
	DETOUR_DECL_MEMBER(ActionResult<CEyeballBoss>, CEyeballBossIdle_OnStart, CEyeballBoss *actor, Action<CEyeballBoss> *action)
	{
		auto me = reinterpret_cast<CEyeballBossIdle *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CEyeballBossIdle_OnStart)(actor, action);
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto *info = GetBossInfo(actor);
			if (info != nullptr) {
				me->m_ctLifetime.Start(info->m_fLifetime);
			}
		}
		
		return result;
	}

	inline float GetSpeed(ILocomotion *loco)
	{
		auto entity = loco->GetBot()->GetEntity();
		if (entity != nullptr) {
			auto data = GetBossInfo(entity);
			if (data != nullptr)
				return data->m_fSpeed;
		}
		return FLT_MAX;
	}

	DETOUR_DECL_MEMBER(float, CZombieLocomotion_GetRunSpeed)
	{
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));
		return speed != FLT_MAX ? speed : DETOUR_MEMBER_CALL(CZombieLocomotion_GetRunSpeed)();
	}

	DETOUR_DECL_MEMBER(float, CHeadlessHatmanLocomotion_GetRunSpeed)
	{
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));
		return speed != FLT_MAX ? speed : DETOUR_MEMBER_CALL(CHeadlessHatmanLocomotion_GetRunSpeed)();
	}

	DETOUR_DECL_MEMBER(float, CMerasmusLocomotion_GetRunSpeed)
	{
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));
		return speed != FLT_MAX ? speed : DETOUR_MEMBER_CALL(CMerasmusLocomotion_GetRunSpeed)();
	}

	DETOUR_DECL_MEMBER(void, CEyeballBossLocomotion_Approach, const Vector &goal, float goalweight)
	{
		static ConVarRef tf_eyeball_boss_acceleration("tf_eyeball_boss_acceleration");
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));

		float old_speed = FLT_MAX;
		if (speed != FLT_MAX) {
			old_speed = tf_eyeball_boss_acceleration.GetFloat();
			tf_eyeball_boss_acceleration.SetValue(speed);
		}
		DETOUR_MEMBER_CALL(CEyeballBossLocomotion_Approach)(goal, goalweight);
		if (speed != FLT_MAX) {
			tf_eyeball_boss_acceleration.SetValue(old_speed);
		}
	}

	DETOUR_DECL_MEMBER(float, NextBotGroundLocomotion_GetMaxAcceleration)
	{
		float default_speed = DETOUR_MEMBER_CALL(NextBotGroundLocomotion_GetMaxAcceleration)();
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));
		return speed != FLT_MAX ? Max(speed * 4, default_speed) : default_speed;
	}

	DETOUR_DECL_MEMBER(float, NextBotGroundLocomotion_GetMaxDeceleration)
	{
		float default_speed = DETOUR_MEMBER_CALL(NextBotGroundLocomotion_GetMaxDeceleration)();
		float speed = GetSpeed(reinterpret_cast<ILocomotion *>(this));
		return speed != FLT_MAX ? Max(speed * 4, default_speed) : default_speed;
	}
	
	DETOUR_DECL_MEMBER(int, CZombie_OnTakeDamage_Alive, const CTakeDamageInfo& info)
	{
		auto zombie = reinterpret_cast<CZombie *>(this);
		IGameEvent *event = gameeventmanager->CreateEvent("npc_hurt");
		if ( event )
		{

			event->SetInt("entindex", ENTINDEX(zombie));
			event->SetInt("health", Max(0, zombie->GetHealth()));
			event->SetInt("damageamount", info.GetDamage());
			event->SetBool("crit", (info.GetDamageType() & DMG_CRITICAL) ? true : false);

			CTFPlayer *attackerPlayer = ToTFPlayer(info.GetAttacker());
			if (attackerPlayer)
			{
				event->SetInt("attacker_player", attackerPlayer->GetUserID());

				if ( attackerPlayer->GetActiveTFWeapon() )
				{
					event->SetInt("weaponid", attackerPlayer->GetActiveTFWeapon()->GetWeaponID());
				}
				else
				{
					event->SetInt("weaponid", 0);
				}
			}
			else
			{
				// hurt by world
				event->SetInt("attacker_player", 0);
				event->SetInt("weaponid", 0);
			}

			gameeventmanager->FireEvent(event);
		}
		return DETOUR_MEMBER_CALL(CZombie_OnTakeDamage_Alive)(info);
	}

	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& info)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (info.GetAttacker() != nullptr && !info.GetAttacker()->IsPlayer() && info.GetAttacker()->MyNextBotPointer() != nullptr) {
			auto data = GetBossInfo(info.GetAttacker());
			if (data != nullptr) {
				const_cast<CTakeDamageInfo&>(info).SetDamage(info.GetDamage() * data->m_fDmgMult);
			}
		}
		
		return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(info);
	}

	void DeleteBoss(CBaseEntity *boss, CHalloweenBossSpawner *spawner)
	{
		if (spawner == nullptr && boss != nullptr) {
			spawner = GetBossInfo(boss);
		}

		if (spawner == nullptr)
			return;

		auto populator = spawner->m_Populator;
		if (populator != nullptr) {
			int currency = populator->GetCurrencyAmountPerDeath();

			if (currency > 0) {
				if (!spawner->m_bSpawnCurrencyPack || boss == nullptr) {
					TFGameRules()->DistributeCurrencyAmount(currency, nullptr, true, true, false);
				}
				else {
					QAngle angRand = vec3_angle;
					angRand.y = RandomFloat( -180.0f, 180.0f );
					CCurrencyPackCustom *pCurrencyPack = static_cast<CCurrencyPackCustom *>(CBaseEntity::CreateNoSpawn("item_currencypack_custom", boss->GetAbsOrigin(), angRand, nullptr));

					if (pCurrencyPack)
					{
						pCurrencyPack->SetAmount( currency );
						Vector vecImpulse = RandomVector( -1,1 );
						vecImpulse.z = 1;
						VectorNormalize( vecImpulse );
						Vector vecVelocity = vecImpulse * 250.0;

						DispatchSpawn( pCurrencyPack );
						pCurrencyPack->DropSingleInstance( vecVelocity, nullptr, 0, 0 );
					}
				}
			}
		}
		if (spawner->m_strClassIcon != NULL_STRING ) {
			CTFObjectiveResource *res = TFObjectiveResource();
			res->DecrementMannVsMachineWaveClassCount(spawner->m_strClassIcon, 1 | 8);
		}
	}

	DETOUR_DECL_MEMBER(void, CMannVsMachineStats_RoundEvent_WaveEnd, bool success)
	{
		for (auto it = boss_spawners.begin(); it != boss_spawners.end();) {
			auto spawner = it->second;
			auto inst = it->first;
			DeleteBoss(inst, spawner);
			it = boss_spawners.erase(it);
		}
		DETOUR_MEMBER_CALL(CMannVsMachineStats_RoundEvent_WaveEnd)(success);
	}

	struct NextBotData
    {
        int vtable;
        INextBotComponent *m_ComponentList;              // +0x04
        PathFollower *m_CurrentPath;                     // +0x08
        int m_iManagerIndex;                             // +0x0c
        bool m_bScheduledForNextTick;                    // +0x10
        int m_iLastUpdateTick;                           // +0x14
    };

	/*DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{	
		auto data = GetDataForBot(actor);
		if (data != nullptr && data->fast_update) {
			reinterpret_cast<NextBotData *>(actor->MyNextBotPointer())->m_bScheduledForNextTick = true;
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}*/

	/*DETOUR_DECL_MEMBER(bool, NextBotManager_ShouldUpdate, INextBot *bot)
	{
		auto ent = bot->GetEntity();
		if (ent != nullptr && !ent->IsPlayer()) {
			auto data = GetBossInfo(ent);
			if (data != nullptr && data->m_bFastUpdate) {
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(NextBotManager_ShouldUpdate)(bot);
	}*/
	
	class CMod : public IMod, public IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Pop:WaveSpawn_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_dtor0, "CWaveSpawnPopulator::~CWaveSpawnPopulator [D0]");
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_dtor2, "CWaveSpawnPopulator::~CWaveSpawnPopulator [D2]");
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_SetState, "CWaveSpawnPopulator::SetState");
			MOD_ADD_DETOUR_STATIC(IPopulationSpawner_ParseSpawner, "IPopulationSpawner::ParseSpawner");
			MOD_ADD_DETOUR_MEMBER(CWave_ActiveWaveUpdate, "CWave::ActiveWaveUpdate");
			
			MOD_ADD_DETOUR_MEMBER(CEyeballBossIdle_Update, "CEyeballBossIdle::Update");
			MOD_ADD_DETOUR_MEMBER(CEyeballBossDead_Update, "CEyeballBossDead::Update");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_Create,      "CBaseEntity::Create");
			MOD_ADD_DETOUR_MEMBER(CEyeballBossIdle_OnStart,"CEyeballBossIdle::OnStart");
			
			MOD_ADD_DETOUR_MEMBER(CZombieLocomotion_GetRunSpeed,         "CZombieLocomotion::GetRunSpeed");
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatmanLocomotion_GetRunSpeed, "CHeadlessHatmanLocomotion::GetRunSpeed");
			MOD_ADD_DETOUR_MEMBER(CMerasmusLocomotion_GetRunSpeed,       "CMerasmusLocomotion::GetRunSpeed");
			MOD_ADD_DETOUR_MEMBER(CEyeballBossLocomotion_Approach,      "CEyeballBossLocomotion::Approach");
			MOD_ADD_DETOUR_MEMBER(NextBotGroundLocomotion_GetMaxAcceleration,      "NextBotGroundLocomotion::GetMaxAcceleration");
			MOD_ADD_DETOUR_MEMBER(NextBotGroundLocomotion_GetMaxDeceleration,      "NextBotGroundLocomotion::GetMaxDeceleration");
			
			MOD_ADD_DETOUR_MEMBER(CZombie_OnTakeDamage_Alive,      "CZombie::OnTakeDamage_Alive");

			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage");

			MOD_ADD_DETOUR_MEMBER(CMannVsMachineStats_RoundEvent_WaveEnd, "CMannVsMachineStats::RoundEvent_WaveEnd");

			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse, "CWaveSpawnPopulator::Parse");

			
		}
		
		virtual void OnUnload() override
		{
			wavespawns.clear();
		}
		
		virtual void OnDisable() override
		{
			wavespawns.clear();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void LevelInitPreEntity() override
		{
			wavespawns.clear();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			wavespawns.clear();
		}

		virtual void FrameUpdatePostEntityThink() override
		{
			for(auto spawner : template_spawners) {

				auto &insts = spawner->m_Instances;
				for (auto it = insts.begin(); it != insts.end();) {
					auto inst = it->first;
					if (inst == nullptr || inst->mark_delete) {
						auto populator = spawner->m_Populator;
						if (populator != nullptr) {
							int currency = populator->GetCurrencyAmountPerDeath();

							if (currency > 0) {
								if (!spawner->m_bSpawnCurrencyPack) {
									TFGameRules()->DistributeCurrencyAmount(currency, nullptr, true, true, false);
								}
								else {
									QAngle angRand = vec3_angle;
									angRand.y = RandomFloat( -180.0f, 180.0f );
									CCurrencyPackCustom *pCurrencyPack = static_cast<CCurrencyPackCustom *>(CBaseEntity::CreateNoSpawn("item_currencypack_custom", it->second, angRand, nullptr));

									if (pCurrencyPack)
									{
										pCurrencyPack->SetAmount( currency );
										Vector vecImpulse = RandomVector( -1,1 );
										vecImpulse.z = 1;
										VectorNormalize( vecImpulse );
										Vector vecVelocity = vecImpulse * 250.0;

										DispatchSpawn( pCurrencyPack );
										pCurrencyPack->DropSingleInstance( vecVelocity, nullptr, 0, 0 );
									}
								}
							}


							FOR_EACH_VEC(populator->m_activeVector, i) {
								if (populator->m_activeVector[i] == spawner_sentinel) {
									populator->m_activeVector.Remove(i);
									break;
								}
							}
						}

						if (spawner->m_strClassIcon != NULL_STRING ) {
							CTFObjectiveResource *res = TFObjectiveResource();
							res->DecrementMannVsMachineWaveClassCount(spawner->m_strClassIcon, 1 | 8);
						}
						it = insts.erase(it);
						continue;
					}

					if (!spawner->m_sSpawnCurrencyPack.empty()) {
						for (auto entity : inst->entities) {
							if (entity != nullptr) {
								const char *name = STRING(entity->GetEntityName());
								if (strncmp(spawner->m_sSpawnCurrencyPack.c_str(), name, spawner->m_sSpawnCurrencyPack.size()) == 0) {
									char end_char = name[spawner->m_sSpawnCurrencyPack.size()];
									if (end_char == '&' || end_char == '\0') {
										it->second = entity->GetAbsOrigin();
									}
								}
							}
						}
					}
					it++;
				}
			}
			for (auto it = boss_spawners.begin(); it != boss_spawners.end();) {
				auto spawner = it->second;
				auto inst = it->first;
				if (inst == nullptr || !inst->IsAlive()) {
					DeleteBoss(inst, spawner);
					it = boss_spawners.erase(it);
					continue;
				}

				it++;

				if (spawner != nullptr && spawner->m_bFastUpdate && inst->MyNextBotPointer() != nullptr) {
					reinterpret_cast<NextBotData *>(inst->MyNextBotPointer())->m_bScheduledForNextTick = true;
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_wavespawn_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in CWaveSpawnPopulator::Parse",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_WaveSpawn : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_WaveSpawn cond;
}
