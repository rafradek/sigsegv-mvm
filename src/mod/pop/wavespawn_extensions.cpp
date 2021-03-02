#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "stub/populators.h"

namespace Mod::Pop::Wave_Extensions
{
	void ParseColorsAndPrint(const char *line, float gameTextDelay, int &linenum, CTFPlayer* player = nullptr);
}

namespace Mod::Pop::WaveSpawn_Extensions
{
	
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
	
	
	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Pop:WaveSpawn_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_dtor0, "CWaveSpawnPopulator::~CWaveSpawnPopulator [D0]");
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_dtor2, "CWaveSpawnPopulator::~CWaveSpawnPopulator [D2]");
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_SetState, "CWaveSpawnPopulator::SetState");
			
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
