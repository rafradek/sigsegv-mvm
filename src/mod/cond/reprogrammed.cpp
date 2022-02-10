#include "mod.h"
#include "stub/tfbot.h"
#include "stub/team.h"
#include "stub/objects.h"
#include "stub/tf_player_resource.h"
#include "stub/tf_shareddefs.h"
#include "stub/gamerules.h"
#include "stub/tfbot_behavior.h"
#include "re/nextbot.h"
#include "util/backtrace.h"
#include "util/iterate.h"
#include "stub/projectiles.h"
#include "stub/populators.h"


namespace Mod::Cond::Reprogrammed
{
	constexpr uint8_t s_Buf_UpdateMission[] = {
		0x8d, 0x45, 0xa8,                               // +0000  lea eax,[ebp-0x58]
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0003  mov dword ptr [esp+0xc],false
		0xc7, 0x44, 0x24, 0x08, 0x01, 0x00, 0x00, 0x00, // +000B  mov dword ptr [esp+0x8],true
		0xc7, 0x44, 0x24, 0x04, 0x03, 0x00, 0x00, 0x00, // +0013  mov dword ptr [esp+0x4],TF_TEAM_PVE_INVADERS
		0x89, 0x04, 0x24,                               // +001B  mov [esp],eax
		0xc7, 0x45, 0xa8, 0x00, 0x00, 0x00, 0x00,       // +001E  mov [ebp-0x58],0x00000000
		0xc7, 0x45, 0xac, 0x00, 0x00, 0x00, 0x00,       // +0025  mov [ebp-0x54],0x00000000
		0xc7, 0x45, 0xb0, 0x00, 0x00, 0x00, 0x00,       // +002C  mov [ebp-0x50],0x00000000
		0xc7, 0x45, 0xb4, 0x00, 0x00, 0x00, 0x00,       // +0033  mov [ebp-0x4c],0x00000000
		0xc7, 0x45, 0xb8, 0x00, 0x00, 0x00, 0x00,       // +003A  mov [ebp-0x48],0x00000000
		0xe8,                                           // +0041  call CollectPlayers<CTFPlayer>
	};
	
	struct CPatch_CMissionPopulator_UpdateMission : public CPatch
	{
		CPatch_CMissionPopulator_UpdateMission() : CPatch(sizeof(s_Buf_UpdateMission)) {}
		
		virtual const char *GetFuncName() const override { return "CMissionPopulator::UpdateMission"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ 0x0100
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_UpdateMission);
			
			mask.SetRange(0x00 + 2, 1, 0x00);
			mask.SetRange(0x1e + 2, 1, 0x00);
			mask.SetRange(0x25 + 2, 1, 0x00);
			mask.SetRange(0x2c + 2, 1, 0x00);
			mask.SetRange(0x33 + 2, 1, 0x00);
			mask.SetRange(0x3a + 2, 1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* change the teamnum to TEAM_ANY */
			buf .SetDword(0x13 + 4, TEAM_ANY);
			mask.SetDword(0x13 + 4, 0xffffffff);
			
			return true;
		}
	};
	
	struct CPatch_CMissionPopulator_UpdateMissionDestroySentries : public CPatch_CMissionPopulator_UpdateMission
	{
		virtual const char *GetFuncName() const override { return "CMissionPopulator::UpdateMissionDestroySentries"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0800; } // @ 0x0442
		
		/* exact same pattern matching and replacment as the other patch */
	};
	
	
	constexpr uint8_t s_Buf_CheckStuck[] = {
		0x89, 0x04, 0x24,                   // +0000  mov [esp],eax
		0xe8, 0x1e, 0x1d, 0x28, 0x00,       // +0003  call CBaseEntity::GetTeamNumber
		0x83, 0xf8, 0x03,                   // +0008  cmp eax,TF_TEAM_PVE_INVADERS
		0x0f, 0x84, 0xa5, 0x01, 0x00, 0x00, // +000B  jz +0x1a5
	};
//	constexpr uint8_t s_Buf_CheckStuck_after[] = {
//		0x89, 0x04, 0x24,                   // +0000  mov [esp],eax
//		0xff, 0x15, 0xff, 0xff, 0xff, 0xff, // +0003  call [<STUB> CBaseEntity::IsBot]
//		0x90, 0x90,                         // +0009  nop nop
//		0x0f, 0x85, 0xa5, 0x01, 0x00, 0x00, // +000B  jnz +0x1a5
//	};
	
	using FPtr_IsBot = bool (CBasePlayer:: *)() const;
	struct CPatch_CTFGameMovement_CheckStuck : public CPatch
	{
		CPatch_CTFGameMovement_CheckStuck() : CPatch(sizeof(s_Buf_CheckStuck)) {}
		
		virtual const char *GetFuncName() const override { return "CTFGameMovement::CheckStuck"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x00a0; } // @ 0x008a
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CheckStuck);
			
			mask.SetRange(0x03 + 1, 4, 0x00);
			mask.SetRange(0x0b + 2, 4, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* indirect call through pointer */
			buf[0x03] = 0xff;
			buf[0x04] = 0x15;
			buf.SetDword(0x03 + 2, (uint32_t)&s_CBasePlayer_IsBot);
			
			/* pad out extra space with NOPs */
			buf[0x09] = 0x90;
			buf[0x0a] = 0x90;
			
			/* invert the jump condition code */
			buf[0x0c] = 0x85;
			
			mask.SetRange(0x03, 0x08, 0xff);
			mask[0x0c] = 0xff;
			
			return true;
		}
		
		static FPtr_IsBot s_CBasePlayer_IsBot;
	};
	FPtr_IsBot CPatch_CTFGameMovement_CheckStuck::s_CBasePlayer_IsBot = &CBasePlayer::IsBot;
	
	
#if 0
	constexpr uint8_t s_Buf_GetShootSound[] = {
		0xe8, 0xf1, 0x51, 0x1a, 0x00,             // +0000  call CBaseEntity::GetTeamNumber
		0x89, 0xc7,                               // +0005  mov edi,eax
		0xa1, 0x1c, 0xc9, 0x5c, 0x01,             // +0007  mov eax,ds:g_pGameRules
		0x85, 0xc0,                               // +000C  test eax,eax
		0x74, 0x0e,                               // +000E  jz +0xXX
		0x80, 0xb8, 0x66, 0x09, 0x00, 0x00, 0x00, // +0010  cmp byte ptr [eax+m_bPlayingMannVsMachine],0x00
		0x74, 0x05,                               // +0017  jz +0xXX
		0x83, 0xff, 0x03,                         // +0019  cmp edi,3
		0x74, 0x48,                               // +001C  jz +0xXX
	};
	
	struct CPatch_CTFWeaponBase_GetShootSound : public CPatch
	{
		CPatch_CTFWeaponBase_GetShootSound() : CPatch(sizeof(s_Buf_GetShootSound)) {}
		
		virtual const char *GetFuncName() const override { return "CTFWeaponBase::GetShootSound"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0100; } // @ +0x0043
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_GetShootSound);
			
			void *addr__g_pGameRules = AddrManager::GetAddr("g_pGameRules");
			if (addr__g_pGameRules == nullptr) return false;
			
			int off__m_bPlayingMannVsMachine;
			if (!Prop::FindOffset(off__m_bPlayingMannVsMachine, "CTFGameRules", "m_bPlayingMannVsMachine")) return false;
			
			buf.SetDword(0x07 + 1, (uint32_t)addr__g_pGameRules);
			buf.SetDword(0x10 + 2, (uint32_t)off__m_bPlayingMannVsMachine);
			
			mask.SetRange(0x00 + 1, 0x4, 0x00);
			mask.SetRange(0x0e + 1, 0x1, 0x00);
			mask.SetRange(0x17 + 1, 0x1, 0x00);
			mask.SetRange(0x1c + 1, 0x1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* make the jump for checking IsMiniBoss occur regardless of teamnum */
			buf [0x1c] = 0xeb;
			mask[0x1c] = 0xff;
			
			return true;
		}
	};
#endif
	
	
	ConVar cvar_hellmet("sig_cond_reprogrammed_hellmet", "1", FCVAR_NOTIFY,
		"Mod: make some tweaks to TF_COND_REPROGRAMMED that Hell-met requested");
	
	
	bool WeaponHasMiniBossSounds(CBaseCombatWeapon *weapon)
	{
		CEconItemView *item_view = weapon->GetItem();
		if (item_view == nullptr) return false;
		
		CEconItemDefinition *item_def = item_view->GetItemDefinition();
		if (item_def == nullptr) return false;
		
		static int idx_visuals_mvm_boss = []{
			for (int i = 0; i < NUM_VISUALS_BLOCKS; ++i) {
				if (g_TeamVisualSections[i] == nullptr)                  continue;
				if (FStrEq(g_TeamVisualSections[i], "visuals_mvm_boss")) return i;
			}
			return -1;
		}();
		if (idx_visuals_mvm_boss == -1) return false;
		
		/* NO: item definition has no "visuals_mvm_boss" block */
		perteamvisuals_t *visuals_boss = item_def->m_Visuals[idx_visuals_mvm_boss];
		if (visuals_boss == nullptr) return false;
		
		// Hack: attack projectiles attribute requires matching team number to work properly
		int attackProjectiles = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attackProjectiles, attack_projectiles);
		if (attackProjectiles != 0) return false;

		for (int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i) {
			if (visuals_boss->m_Sounds[i] != nullptr) return true;
		}
		/* NO: "visuals_mvm_boss" block lacks any "sound_*" entries */
		return false;
	}
	
	
	void ChangeWeaponAndWearableTeam(CTFPlayer *player, int team)
	{
		DevMsg("ChangeWeaponAndWearableTeam (#%d \"%s\"): teamnum %d => %d\n",
			ENTINDEX(player), player->GetPlayerName(), player->GetTeamNumber(), team);
		
		if (team != TF_TEAM_RED && team != TF_TEAM_BLUE) {
		//	DevMsg("  requested a weapon/wearable change to non-TF teamnum %d; refusing to do that\n", team);
			return;
		}
		
		for (int i = player->WeaponCount() - 1; i >= 0; --i) {
			CBaseCombatWeapon *weapon = player->GetWeapon(i);
			if (weapon == nullptr) continue;
			
			int pre_team = weapon->GetTeamNumber();
			int pre_skin = weapon->m_nSkin;
			
			if (pre_team == TF_TEAM_RED || pre_team == TF_TEAM_BLUE) {
				/* don't change the team of weapons that have special giant-specific sounds, because the client only
				 * uses those sound overrides if the weapon is on TF_TEAM_BLUE, and there's no other easy workaround */
				if (pre_team == TF_TEAM_BLUE && TFGameRules()->IsMannVsMachineMode() && (!player->IsMiniBoss() || !WeaponHasMiniBossSounds(weapon))) {
				//	ConColorMsg(Color(0xff, 0x00, 0xff, 0xff),
				//		"- weapon with itemdefidx %4d: no miniboss sounds\n", [](CBaseCombatWeapon *weapon){
				//		CEconItemView *item_view = weapon->GetItem();
				//		if (item_view == nullptr) return -1;
				//		
				//		return item_view->GetItemDefIndex();
				//	}(weapon));
					weapon->ChangeTeam(team);
				} else {
				//	ConColorMsg(Color(0xff, 0x00, 0xff, 0xff),
				//		"- weapon with itemdefidx %4d: HAS MINIBOSS SOUNDS!\n", [](CBaseCombatWeapon *weapon){
				//		CEconItemView *item_view = weapon->GetItem();
				//		if (item_view == nullptr) return -1;
				//		
				//		return item_view->GetItemDefIndex();
				//	}(weapon));
				}
			} else {
		//		DevMsg("  weapon %d (#%d \"%s\"): refusing to call ChangeTeam\n",
		//			i, ENTINDEX(weapon), weapon->GetClassname());
			}
			weapon->m_nSkin = (team == TF_TEAM_BLUE ? 1 : 0);
			
			int post_team = weapon->GetTeamNumber();
			int post_skin = weapon->m_nSkin;
			
		//	DevMsg("  weapon %d (#%d \"%s\"): [Team:%d Skin:%d] => [Team:%d Skin:%d]\n",
		//		i, ENTINDEX(weapon), weapon->GetClassname(), pre_team, pre_skin, post_team, post_skin);
		}
		
		for (int i = player->GetNumWearables() - 1; i >= 0; --i) {
			CEconWearable *wearable = player->GetWearable(i);
			if (wearable == nullptr) continue;
			
			int pre_team = wearable->GetTeamNumber();
			int pre_skin = wearable->m_nSkin;
			
			if (pre_team == TF_TEAM_RED || pre_team == TF_TEAM_BLUE) {
				wearable->ChangeTeam(team);
			} else {
		//		DevMsg("  wearable %d (#%d \"%s\"): refusing to call ChangeTeam\n",
		//			i, ENTINDEX(wearable), wearable->GetClassname());
			}
			wearable->m_nSkin = (team == TF_TEAM_BLUE ? 1 : 0);
			
			int post_team = wearable->GetTeamNumber();
			int post_skin = wearable->m_nSkin;
			
		//	DevMsg("  wearable %d (#%d \"%s\"): [Team:%d Skin:%d] => [Team:%d Skin:%d]\n",
		//		i, ENTINDEX(wearable), wearable->GetClassname(), pre_team, pre_skin, post_team, post_skin);
		}
	}
	
	bool stop_auto_assignment = false;

	void OnAddReprogrammed(CTFPlayer *player)
	{
		DevMsg("OnAddReprogrammed(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		if (!cvar_hellmet.GetBool()) {
			player->m_Shared->StunPlayer(5.0f, 0.65f, TF_STUNFLAG_NOSOUNDOREFFECT | TF_STUNFLAG_SLOWDOWN, nullptr);
		}
		
		/* added this check to prevent problems */
		
		if (player->IsBot()) {
			if (player->GetTeamNumber() == TF_TEAM_BLUE) {
				DevMsg("  currently on TF_TEAM_BLUE: calling ForceChangeTeam(TF_TEAM_RED)\n");
				player->ForceChangeTeam(TF_TEAM_RED, false);
			} else {
				DevMsg("  currently on teamnum %d; not calling ForceChangeTeam\n", player->GetTeamNumber());
			}
			if (cvar_hellmet.GetBool()) {
				ChangeWeaponAndWearableTeam(player, TF_TEAM_RED);
			}
			if (cvar_hellmet.GetBool()) {
				CTFBot *bot = ToTFBot(player);
				if (bot != nullptr) {
					bot->GetVisionInterface()->ForgetAllKnownEntities();
				}
			}
		}
		else {
			
			stop_auto_assignment = true;
			if (player->GetTeamNumber() == TF_TEAM_BLUE) {
				
				player->ForceChangeTeam(TF_TEAM_RED, false);

				if (cvar_hellmet.GetBool()) {
					ChangeWeaponAndWearableTeam(player, TF_TEAM_RED);
				}
			}
			else {

				player->ForceChangeTeam(TF_TEAM_BLUE, false);

				if (cvar_hellmet.GetBool()) {
					ChangeWeaponAndWearableTeam(player, TF_TEAM_BLUE);
				}
			}
			
			stop_auto_assignment = false;
		}
		
		/* ensure that all weapons and wearables have their colors updated */
		
		
		/* this used to be in CTFPlayerShared::OnAddReprogrammed on the client
		 * side, but we now have to do it from the server side */
		if (!cvar_hellmet.GetBool()) {
			DispatchParticleEffect("sapper_sentry1_fx", PATTACH_POINT_FOLLOW, player, "head");
		}
		
		
	}

	std::vector<CHandle<CTFPlayer>> bots_killed;
	THINK_FUNC_DECL(SetPlayerResourceTeamRed)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		PlayerResource()->m_iTeam.SetIndex(TF_TEAM_RED, ENTINDEX(player));
		if (gpGlobals->curtime - player->GetDeathTime() < 0.5f) {
			player->SetNextThink(gpGlobals->curtime + 0.01f, "SetPlayerResourceTeamRed");
		}
	}
 
	void OnRemoveReprogrammed(CTFPlayer *player)
	{
		DevMsg("OnRemoveReprogrammed(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		/* added this check to prevent problems */
		if (player->IsBot()) {
			// Delayed team switch when the bot is dying
			if (player->m_lifeState == LIFE_DYING) {
				if (player->GetTeamNumber() == TF_TEAM_RED)
					player->ForceChangeTeam(TF_TEAM_BLUE, false);
					
				// Fix death notice appearing as opposing team
				PlayerResource()->m_iTeam.SetIndex(TF_TEAM_RED, ENTINDEX(player));
				//THINK_FUNC_SET(player, SetPlayerResourceTeamRed, gpGlobals->curtime);
				bots_killed.push_back(player);
			}
			else {
				if (player->GetTeamNumber() == TF_TEAM_RED) {
					DevMsg("  currently on TF_TEAM_RED: calling ForceChangeTeam(TF_TEAM_BLUE)\n");
					player->ForceChangeTeam(TF_TEAM_BLUE, false);
				} else {
					DevMsg("  currently on teamnum %d; not calling ForceChangeTeam\n", player->GetTeamNumber());
				}
			}
			if (cvar_hellmet.GetBool()) {
				if (player->m_lifeState == LIFE_DYING) {
					// hack hack hack: make wearable gibs be red
					player->m_nSkin = 0;
				} else {
					ChangeWeaponAndWearableTeam(player, TF_TEAM_BLUE);
				}
			}
			if (cvar_hellmet.GetBool()) {
				CTFBot *bot = ToTFBot(player);
				if (bot != nullptr) {
					bot->GetVisionInterface()->ForgetAllKnownEntities();
				}
			}
		}
		else {
			stop_auto_assignment = true;
			if (player->GetTeamNumber() == TF_TEAM_RED) {
				player->ForceChangeTeam(TF_TEAM_BLUE, false);
				if (cvar_hellmet.GetBool()) {
					if (player->m_lifeState == LIFE_DYING) {
						// hack hack hack: make wearable gibs be red
						player->m_nSkin = 0;
					} else {
						ChangeWeaponAndWearableTeam(player, TF_TEAM_BLUE);
					}
				}
			}
			else if (player->GetTeamNumber() == TF_TEAM_BLUE) {
				player->ForceChangeTeam(TF_TEAM_RED, false);
				if (cvar_hellmet.GetBool()) {
					if (player->m_lifeState == LIFE_DYING) {
						// hack hack hack: make wearable gibs be red
						player->m_nSkin = 1;
					} else {
						ChangeWeaponAndWearableTeam(player, TF_TEAM_RED);
					}
				}
			}
			stop_auto_assignment = false;
		}
		
		/* ensure that all weapons and wearables have their colors updated;
		 * we don't do this in the case of LIFE_DYING, however, because
		 * CTFPlayer::Event_Killed calls CTFPlayerShared::RemoveAllCond, and we
		 * end up making the ragdoll wearables and dropped hats etc the wrong
		 * color compared to the player ragdoll itself */
		
		
		/* this is far from ideal; we can only remove ALL particle effects from
		 * the server side */
		if (!cvar_hellmet.GetBool()) {
			StopParticleEffects(player);
		}
		
		
		
	}
	
	class NeutralSwitch : public CBaseEntity 
	{
	public:
		void Switch() {
			bots_killed.push_back(reinterpret_cast<CTFPlayer *>(this));
			CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		}
	};

	void OnAddReprogrammedNeutral(CTFPlayer *player)
	{
		
		DevMsg("OnAddReprogrammedNeutral(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		if (player->GetTeamNumber() != TEAM_SPECTATOR) {
			DevMsg("  currently on TF_TEAM_BLUE: calling ForceChangeTeam(TF_TEAM_RED)\n");
			player->ChangeTeamBase(TEAM_SPECTATOR, false, true, false);
		} else {
			DevMsg("  currently on teamnum %d; not calling ForceChangeTeam\n", player->GetTeamNumber());
		}
	}

	void OnRemoveReprogrammedNeutral(CTFPlayer *player)
	{
		DevMsg("OnRemoveReprogrammedNeutral(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		CTFBot *bot = ToTFBot(player);

		if (player->GetTeamNumber() == TEAM_SPECTATOR) {
			if (bot != nullptr && player->m_lifeState == LIFE_DYING) {
				//bots_killed.push_back(reinterpret_cast<CTFPlayer *>(player));
				//player->ThinkSet(&NeutralSwitch::Switch, gpGlobals->curtime+4.0f, "AutoKick");
				
				player->ForceChangeTeam(TF_TEAM_BLUE, true);
				//bot->SetAttribute(CTFBot::AttributeType::ATTR_REMOVE_ON_DEATH);
				//player->ForceChangeTeam(TEAM_SPECTATOR, true);
			}
			else
				player->ForceChangeTeam(TF_TEAM_BLUE, false);
		}

		if (bot != nullptr) {
			bot->GetVisionInterface()->ForgetAllKnownEntities();
		}
	}

	void OnAddMVMBotRadiowave(CTFPlayer *player)
	{
		if (!player->IsBot() )
			player->m_Shared->StunPlayer( 5.0f, 1.0f, 1 | 2 | 32 , nullptr); //movement control noeffect
	}

	DETOUR_DECL_MEMBER(int, CTFGameRules_GetTeamAssignmentOverride, CTFPlayer *pPlayer, int iWantedTeam, bool b1)
	{
		if (stop_auto_assignment) {
			return iWantedTeam;
		}

		return DETOUR_MEMBER_CALL(CTFGameRules_GetTeamAssignmentOverride)(pPlayer, iWantedTeam, b1);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionAdded, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		if (cond == TF_COND_REPROGRAMMED) {
			OnAddReprogrammed(shared->GetOuter());
			return;
		}
		
		if (cond == TF_COND_HALLOWEEN_HELL_HEAL && TFGameRules()->IsMannVsMachineMode()) {
			OnAddReprogrammedNeutral(shared->GetOuter());
			return;
		}

		DETOUR_MEMBER_CALL(CTFPlayerShared_OnConditionAdded)(cond);
		if (cond == TF_COND_MVM_BOT_STUN_RADIOWAVE) {
			OnAddMVMBotRadiowave(shared->GetOuter());
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionRemoved, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		if (cond == TF_COND_REPROGRAMMED) {
			OnRemoveReprogrammed(shared->GetOuter());
			return;
		}

		if (cond == TF_COND_HALLOWEEN_HELL_HEAL && TFGameRules()->IsMannVsMachineMode()) {
			OnRemoveReprogrammedNeutral(shared->GetOuter());
			return;
		}
		DETOUR_MEMBER_CALL(CTFPlayerShared_OnConditionRemoved)(cond);
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotScenarioMonitor_Update, CTFBot *actor, float dt)
	{
		if (actor->m_Shared->InCond(TF_COND_REPROGRAMMED)) {
			return ActionResult<CTFBot>::Continue();
		}
		
		return DETOUR_MEMBER_CALL(CTFBotScenarioMonitor_Update)(actor, dt);
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		auto result = DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
		
		if (result.transition == ActionTransition::CONTINUE && TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() != TF_TEAM_BLUE)
		{
			if (actor->ShouldAutoJump()) {
				actor->GetLocomotionInterface()->Jump();
			}
			
			/* ammo regen */
			if (cvar_hellmet.GetBool()) {
				actor->GiveAmmo(100, 1, true);
				actor->GiveAmmo(100, 2, true);
				actor->GiveAmmo(100, 3, true);
			}
		}
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(CTFPlayer *, CTFPlayer_FindPartnerTauntInitiator)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (cvar_hellmet.GetBool() && player->IsBot() && TFGameRules()->IsMannVsMachineMode() && player->m_Shared->InCond(TF_COND_REPROGRAMMED)) {
			return nullptr;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_FindPartnerTauntInitiator)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_BetweenRounds_Start)
	{
		DETOUR_MEMBER_CALL(CTFGameRules_BetweenRounds_Start)();
		
		if (cvar_hellmet.GetBool() && TFGameRules()->IsMannVsMachineMode()) {
		//	for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
		//		auto obj = ToBaseObject(IBaseObjectAutoList::AutoList()[i]);
		//		if (obj == nullptr) continue;
		//		
		//		CBaseEntity *owner = obj->GetOwnerEntity();
		//		if (owner == nullptr) {
		//			obj->DetonateObject();
		//		}
		//	}
			
			ForEachEntityByClassname("bot_hint_engineer_nest", [](CBaseEntity *ent){
				auto nest = static_cast<CTFBotHintEngineerNest *>(ent);
				
				if (nest->IsStaleNest()) {
					DevMsg("CTFGameRules::BetweenRounds_Start: Detonating stale engie nest #%d\n", ENTINDEX(nest));
					nest->DetonateStaleNest();
				}
			});
		}
	}

	//Red giant backstab instakill fix
	DETOUR_DECL_MEMBER(float, CTFKnife_GetMeleeDamage, CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage) {
		float ret = DETOUR_MEMBER_CALL(CTFKnife_GetMeleeDamage)(pTarget, piDamageType, piCustomDamage);

		if (pTarget->IsPlayer() && ret == pTarget->GetHealth() * 2 && ToTFPlayer(pTarget)->IsMiniBoss()) {
			CTFKnife *weapon = reinterpret_cast<CTFKnife *>(this);
			if (ToTFPlayer(weapon->GetOwner())->IsBot()) {
				float flBonusDmg = 1.f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, flBonusDmg, mult_dmg );
				float flBaseDamage = 250.f * flBonusDmg; 

				// Minibosses:  Adjust damage when backstabbing based on level of armor piercing
				// Base amount is 25% of normal damage.  Each level adds 25% to a cap of 125%.
				float flArmorPiercing = 25.f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon->GetOwner(), flArmorPiercing, armor_piercing );
				flBaseDamage *= Clamp( flArmorPiercing / 100.0f, 0.25f, 1.25f );	
				return flBaseDamage;
			}
		}
		return ret;
	}
	
	//Red giant sentry buster instakill fix
	RefCount rc_CTFBotMissionSuicideBomber_Detonate;
	DETOUR_DECL_MEMBER(void, CTFBotMissionSuicideBomber_Detonate, CTFBot *actor)
	{
		SCOPED_INCREMENT(rc_CTFBotMissionSuicideBomber_Detonate);
		DETOUR_MEMBER_CALL(CTFBotMissionSuicideBomber_Detonate)(actor);
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, const CTakeDamageInfo& info)
	{

		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFBotMissionSuicideBomber_Detonate) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			
			CBaseEntity *pAttacker = info.GetAttacker();
			CTFBot *pTFAttackerBot = ToTFBot( pAttacker );
			if ( pTFAttackerBot && player->IsMiniBoss() && (info.GetDamageType() & DMG_BLAST) && info.GetDamage() > 600.0f )
			{
				CTakeDamageInfo newinfo = info;
				newinfo.SetDamage( 600.f );
				return DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage)(newinfo);
			}
		
		}
		return DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage)(info);
	}

	DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, const CTakeDamageInfo& info)
	{
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFBotMissionSuicideBomber_Detonate) {
			auto character = reinterpret_cast<CBaseCombatCharacter *>(this);
			if (!character->IsPlayer() && !character->IsBaseObject()) {
				CTakeDamageInfo newinfo = info;
				newinfo.SetDamage( 600.f );
				return DETOUR_MEMBER_CALL(CBaseCombatCharacter_OnTakeDamage)(newinfo);
			}
		}
		return DETOUR_MEMBER_CALL(CBaseCombatCharacter_OnTakeDamage)(info);
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMedicHeal_Update, CTFBot *actor, float dt)
	{
		auto result = DETOUR_MEMBER_CALL(CTFBotMedicHeal_Update)(actor, dt);
		
		if (cvar_hellmet.GetBool() && actor->GetTeamNumber() != TF_TEAM_BLUE && TFGameRules()->IsMannVsMachineMode()) {
			bool is_changeto_fetchflag =
				(result.transition == ActionTransition::CHANGE_TO &&
				strcmp(result.reason, "Everyone is gone! Going for the flag") == 0 &&
				strcmp(result.action->GetName(), "FetchFlag") == 0);
			
			if (is_changeto_fetchflag) {
				DevMsg("CTFBotMedicHeal::Update: Preventing CHANGE_TO transition to CTFBotFetchFlag for medic bot #%d\n", ENTINDEX(actor));
				
				delete result.action;
				
				result.transition = ActionTransition::SUSPEND_FOR;
				result.action     = CTFBotMedicRetreat::New();
				result.reason     = "Retreating to find another patient to heal";
			}
		}
		
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CObjectSapper_IsValidRoboSapperTarget, CTFPlayer *player)
	{
		auto result = DETOUR_MEMBER_CALL(CObjectSapper_IsValidRoboSapperTarget)(player);

		return result || (player != nullptr && player->IsAlive() && player->m_Shared->InCond(TF_COND_REPROGRAMMED) && !player->m_Shared->IsInvulnerable() && !player->m_Shared->InCond(TF_COND_SAPPED));
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_Disguise, int team, int iclass, CTFPlayer *victim, bool onKill)
	{
		if (onKill && TFGameRules()->IsMannVsMachineMode() && team == TF_TEAM_BLUE && reinterpret_cast<CTFPlayerShared *>(this)->GetOuter()->GetTeamNumber() == TF_TEAM_BLUE)
			team = TF_TEAM_RED;
		DETOUR_MEMBER_CALL(CTFPlayerShared_Disguise)(team, iclass, victim, onKill);
	}

	RefCount rc_CTFPlayer_ModifyOrAppendCriteria;

	DETOUR_DECL_MEMBER(void, CTFPlayer_ModifyOrAppendCriteria, void *criteria)
	{
		SCOPED_INCREMENT_IF(rc_CTFPlayer_ModifyOrAppendCriteria, TFGameRules()->IsMannVsMachineMode() && *(reinterpret_cast<CTFPlayer *>(this)->GetPlayerClass()->GetCustomModel()) != 0 );
		DETOUR_MEMBER_CALL(CTFPlayer_ModifyOrAppendCriteria)(criteria);
	}

	DETOUR_DECL_STATIC(bool, TF_IsHolidayActive)
	{
		if (rc_CTFPlayer_ModifyOrAppendCriteria)
			return false;
		else
			return DETOUR_STATIC_CALL(TF_IsHolidayActive)();
	}
	
	RefCount rc_CTFGameRules_FireGameEvent__teamplay_round_start;
	DETOUR_DECL_MEMBER(void, CTFGameRules_FireGameEvent, IGameEvent *event)
	{
		SCOPED_INCREMENT_IF(rc_CTFGameRules_FireGameEvent__teamplay_round_start,
			(event != nullptr && strcmp(event->GetName(), "teamplay_round_start") == 0));
		DETOUR_MEMBER_CALL(CTFGameRules_FireGameEvent)(event);
	}
	
	DETOUR_DECL_STATIC(int, CollectPlayers_CTFPlayer, CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		if (rc_CTFGameRules_FireGameEvent__teamplay_round_start > 0 && (team == TF_TEAM_BLUE && !isAlive && !shouldAppend)) {
			CUtlVector<CTFPlayer *> tempVector;
			CollectPlayers(&tempVector, TEAM_ANY);
			
			for (auto player : tempVector) {
				if (player->IsBot()) {
					if (player->IsAlive() && player->GetTeamNumber() == TEAM_SPECTATOR) {
						player->ForceChangeTeam(TF_TEAM_BLUE, true);
						player->ForceChangeTeam(TEAM_SPECTATOR, true);
					}
					else {
						playerVector->AddToTail(player);
					}
				}
			}

			return playerVector->Count();
		}
		
		if (team == TEAM_SPECTATOR && isAlive) {
			team = RandomInt(TEAM_SPECTATOR, TF_TEAM_BLUE);
		}
		return DETOUR_STATIC_CALL(CollectPlayers_CTFPlayer)(playerVector, team, isAlive, shouldAppend);
	}
	
	int getteam = -1;
    DETOUR_DECL_MEMBER(bool, CObjectSentrygun_FindTarget)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
        bool result = DETOUR_MEMBER_CALL(CObjectSentrygun_FindTarget)();
        
        if (!result){
			if (sentry->GetTeamNumber() > TEAM_SPECTATOR) {
				int objs = TFTeamMgr()->GetTeam(TEAM_SPECTATOR)->GetNumObjects();
				bool hasplayers = false;
				if (objs == 0) {
					for (int i = 1; i <= gpGlobals->maxClients; ++i) {
						CBasePlayer *player = UTIL_PlayerByIndex(i);
						if (player != nullptr && player->GetTeamNumber() == TEAM_SPECTATOR && player->IsAlive() ) {
							hasplayers = true;
							break;
						}
					}
				}
				if (objs || hasplayers) {
					getteam = TEAM_SPECTATOR;
					result = DETOUR_MEMBER_CALL(CObjectSentrygun_FindTarget)();
				}
			}
			else if (sentry->GetTeamNumber() == TEAM_SPECTATOR) {
				int objs = TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetNumObjects();
				bool hasplayers = false;
				if (objs == 0) {
					for (int i = 1; i <= gpGlobals->maxClients; ++i) {
						CBasePlayer *player = UTIL_PlayerByIndex(i);
						if (player != nullptr && player->GetTeamNumber() == TF_TEAM_RED && player->IsAlive() ) {
							hasplayers = true;
							break;
						}
					}
				}
				if (objs || hasplayers) {
					getteam = TF_TEAM_RED;
					result = DETOUR_MEMBER_CALL(CObjectSentrygun_FindTarget)();
				}
			}
        }
		getteam = -1;
        return result;
	}

    DETOUR_DECL_MEMBER(CTFTeam *, CTFTeamManager_GetTeam, int team)
	{
		if (getteam >= 0)
			team = getteam;
		return DETOUR_MEMBER_CALL(CTFTeamManager_GetTeam)(team);
	}

	DETOUR_DECL_MEMBER(bool, NextBotPlayer_CTFPlayer_IsBot)
	{
		auto *bot = reinterpret_cast<NextBotPlayer<CTFPlayer> *>(this);
		return DETOUR_MEMBER_CALL(NextBotPlayer_CTFPlayer_IsBot)() && !(bot->GetTeamNumber() == TEAM_SPECTATOR && bot->IsAlive());
	}

	CDetour *detour_isbot = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		std::vector<CBasePlayer *> spec_players;
		CTeam *team_spec = TFTeamMgr()->GetTeam(TEAM_SPECTATOR);
		for (int i = 0; i < team_spec->GetNumPlayers(); i++) {
			CBasePlayer *bot = team_spec->GetPlayer(i);
		//ForEachPlayer([&](CBasePlayer *bot) {
			if (bot->IsBot() && bot->IsAlive()) {
				
				spec_players.push_back(bot);
			}
		}
		for (CBasePlayer *bot : spec_players) {
			team_spec->RemovePlayerNonVirtual(bot);
		}

		bool result = DETOUR_MEMBER_CALL(CTFBotSpawner_Spawn)(where, ents);
		
		for (CBasePlayer *bot : spec_players) {
			team_spec->AddPlayerNonVirtual(bot);
		}

		return result;
	}

	DETOUR_DECL_MEMBER(void, CWave_ActiveWaveUpdate)
	{
		auto wave = reinterpret_cast<CWave *>(this);
		DETOUR_MEMBER_CALL(CWave_ActiveWaveUpdate)();
		if (wave->IsDoneWithNonSupportWaves()) {
			for (int i = 1; i < gpGlobals->maxClients; i++) {
				CBasePlayer *player = UTIL_PlayerByIndex(i);
				if (player != nullptr && player->GetTeamNumber() == TEAM_SPECTATOR && player->IsAlive() && player->IsBot()) {
					player->CommitSuicide(true, false);
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(bool, CTraceFilterObject_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		CTFPlayer *entityme = ToTFPlayer(const_cast< CBaseEntity * >(EntityFromEntityHandle(reinterpret_cast<CTraceFilterSimple*>(this)->GetPassEntity())));
		CTFPlayer *entityhit = ToTFPlayer(EntityFromEntityHandle(pServerEntity));

		if (entityme != nullptr && entityhit != nullptr && entityme->GetTeamNumber() == TEAM_SPECTATOR) {
			return entityme->GetTeamNumber() != entityhit->GetTeamNumber();
		}
		return DETOUR_MEMBER_CALL(CTraceFilterObject_ShouldHitEntity)(pServerEntity, contentsMask);
	}
	
	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		static float angpre[34];
		
		ForEachTFBot([](CTFBot *player) {
			if (player->GetTeamNumber() == TF_TEAM_RED) {
				PlayerResource()->m_iTeam.SetIndex(TF_TEAM_BLUE, ENTINDEX(player));
			}
		}); 
		DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
		ForEachTFBot([](CTFBot *player) {
			if (player->GetTeamNumber() == TF_TEAM_RED) {
				PlayerResource()->m_iTeam.SetIndex(TF_TEAM_RED, ENTINDEX(player));
			}
		}); 
	}

	RefCount rc_CTFBot_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		static ConVarRef cvar_player_team("sig_mvm_jointeam_blue_allow_force"); 
		auto bot = reinterpret_cast<CTFBot *>(this);
		SCOPED_INCREMENT_IF(rc_CTFBot_Event_Killed, cvar_player_team.GetBool() || bot->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(CTFBot_Event_Killed)(info);
	}

	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		if (rc_CTFBot_Event_Killed && strcmp(event->GetName(), "mvm_mission_update") == 0) {
			mgr->FreeEvent(event);
			return false;
		}
		
		return DETOUR_MEMBER_CALL(IGameEventManager2_FireEvent)(event, bDontBroadcast);
	}

	DETOUR_DECL_MEMBER(bool, CTFBotMedicHeal_IsVisibleToEnemy, CTFBot *me, const Vector &where)
	{
		if (TFGameRules()->IsMannVsMachineMode() && me->GetTeamNumber() == TF_TEAM_RED) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMedicHeal_IsVisibleToEnemy)(me, where);
	}

#if 0
	DETOUR_DECL_MEMBER(const char *, CTFWeaponBase_GetShootSound, int iIndex)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTFWeaponBase_GetShootSound)(iIndex);
		
		DevMsg("CTFWeaponBase::GetShootSound(#%d, classname \"%s\", teamnum %d, index %d): \"%s\"\n",
			ENTINDEX(weapon), weapon->GetClassname(), weapon->GetTeamNumber(), iIndex, result);
		
		if (strcmp(weapon->GetClassname(), "tf_weapon_minigun") == 0 ||
			strcmp(weapon->GetClassname(), "tf_weapon_rocketlauncher") == 0) {
			BACKTRACE();
		}
		
		return result;
	}
#endif

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Cond:Reprogrammed")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionAdded,   "CTFPlayerShared::OnConditionAdded");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionRemoved, "CTFPlayerShared::OnConditionRemoved");
			
			/* fix: disallow reprogrammed bots from auto-switching to FetchFlag etc */
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_Update, "CTFBotScenarioMonitor::Update");
			
			/* fix: allow reprogrammed bots to AutoJump */
			/* fix: make reprogrammed bots have infinite ammo */
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FindPartnerTauntInitiator, "CTFPlayer::FindPartnerTauntInitiator");
			
			/* fix: make end-of-wave destroy buildings built by red-team engie bots */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_BetweenRounds_Start, "CTFGameRules::BetweenRounds_Start");
			
			/* fix: make medic bots on red team in MvM mode handle "everyone is gone" less stupidly */
			MOD_ADD_DETOUR_MEMBER(CTFBotMedicHeal_Update, "CTFBotMedicHeal::Update");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");

			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Detonate, "CTFBotMissionSuicideBomber::Detonate");
			
			MOD_ADD_DETOUR_MEMBER(CTFKnife_GetMeleeDamage, "CTFKnife::GetMeleeDamage");

			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetTeamAssignmentOverride, "CTFGameRules::GetTeamAssignmentOverride");

			MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_OnTakeDamage, "CBaseCombatCharacter::OnTakeDamage");

			MOD_ADD_DETOUR_MEMBER(CObjectSapper_IsValidRoboSapperTarget, "CObjectSapper::IsValidRoboSapperTarget");

			// Fix yer disguising as blue team when killing reprogrammed bots
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_Disguise, "CTFPlayerShared::Disguise");

			// Stop red robots from doing halloween taunt
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ModifyOrAppendCriteria, "CTFPlayer::ModifyOrAppendCriteria");
			
			MOD_ADD_DETOUR_STATIC(TF_IsHolidayActive, "TF_IsHolidayActive");

			// Allow spectator sentries to target red targets
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_FindTarget, "CObjectSentrygun::FindTarget");
			MOD_ADD_DETOUR_MEMBER(CTFTeamManager_GetTeam, "CTFTeamManager::GetTeam");

			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn, "CTFBotSpawner::Spawn");
			//detour_isbot = new CDetour("NextBotPlayer<CTFPlayer>::IsBot", GET_MEMBER_CALLBACK(NextBotPlayer_CTFPlayer_IsBot), GET_MEMBER_INNERPTR(NextBotPlayer_CTFPlayer_IsBot));
			
			// Fix spectator bots collision
			MOD_ADD_DETOUR_MEMBER(CTraceFilterObject_ShouldHitEntity, "CTraceFilterObject::ShouldHitEntity");

			// Prevent spy killing announcement if the spy is on the same team as players
			MOD_ADD_DETOUR_MEMBER(CTFBot_Event_Killed, "CTFBot::Event_Killed");
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");

			// Stop retreat to cover AI of red bot medics
			MOD_ADD_DETOUR_MEMBER(CTFBotMedicHeal_IsVisibleToEnemy, "CTFBotMedicHeal::IsVisibleToEnemy");

			/* fix: make mission populators aware of red-team mission bots */
			this->AddPatch(new CPatch_CMissionPopulator_UpdateMission());
			this->AddPatch(new CPatch_CMissionPopulator_UpdateMissionDestroySentries());
			
			/* fix: make tf_resolve_stuck_players apply to all bots in MvM, rather than blu-team players */
			this->AddPatch(new CPatch_CTFGameMovement_CheckStuck());

			/* fix hardcoded teamnum check when forcing bots to move to team spec at round change */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent, "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_STATIC(CollectPlayers_CTFPlayer,   "CollectPlayers<CTFPlayer>");
			MOD_ADD_DETOUR_MEMBER(CWave_ActiveWaveUpdate,   "CWave::ActiveWaveUpdate");


			// Hide red bots from the scoreboard
			// MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks,                  "SV_ComputeClientPacks");
			
		//	/* fix: make giant weapon sounds apply to miniboss players on any team */
		//	this->AddPatch(new CPatch_CTFWeaponBase_GetShootSound());
			// ^^^ unreliable, since weapons are predicted client-side
		}
		
		virtual void InvokeLoad() 
		{
			//detour_isbot->Load();
		}

		virtual void InvokeUnload() 
		{
			//detour_isbot->Unload();
			//delete detour_isbot;
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		virtual void FrameUpdatePostEntityThink() override
		{
			if (PlayerResource() == nullptr) return;
			
			for (int i = 0; i < bots_killed.size(); i++) {
				CTFPlayer *bot = bots_killed[i];
				if (bot != nullptr && gpGlobals->curtime - bot->GetDeathTime() < 0.5f) {
					PlayerResource()->m_iTeam.SetIndex(TF_TEAM_RED, ENTINDEX(bot));
				}
				else {
					bots_killed.erase(bots_killed.begin()+i);
					i--;
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_cond_reprogrammed", "0", FCVAR_NOTIFY,
		"Mod: reimplement TF_COND_REPROGRAMMED",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
