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
#include "mod/attr/custom_attributes.h"


namespace Mod::Cond::Reprogrammed
{
	constexpr uint8_t s_Buf_UpdateMission[] = {
#ifdef PLATFORM_64BITS
		0xba, 0x01, 0x00, 0x00, 0x00,                    // +0x0000 mov     edx, 1
		0x48, 0xc7, 0x45, 0x90, 0x00, 0x00, 0x00, 0x00,  // +0x0005 mov     [rbp+var_70], 0
		0xbe, 0x03, 0x00, 0x00, 0x00,                    // +0x000d mov     esi, 3
		0x48, 0x89, 0xc7,                                // +0x0012 mov     rdi, rax
		0x48, 0xc7, 0x45, 0x98, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     [rbp+var_68], 0
		0x48, 0xc7, 0x45, 0xa0, 0x00, 0x00, 0x00, 0x00,  // +0x001d mov     [rbp+var_60], 0
		0x48, 0xc7, 0x45, 0xa8, 0x00, 0x00, 0x00, 0x00,  // +0x0025 mov     [rbp+var_58], 0
		0x48, 0x89, 0x85, 0x58, 0xff, 0xff, 0xff,        // +0x002d mov     [rbp+var_A8], rax
		0xe8,                                            // +0x0034 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_13; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xB9, 0x01, 0x00, 0x00, 0x00,                   // +0000  mov     ecx, 1
		0xBA, 0x03, 0x00, 0x00, 0x00,                   // +0005  mov     edx, 3
		0xC7, 0x45, 0xB8, 0x00, 0x00, 0x00, 0x00,       // +000A  mov     dword ptr [ebp-48h], 0
		0x6A, 0x00,                                     // +0011  push    0
		0x8D, 0x45, 0xB8,                               // +0013  lea     eax, [ebp-48h]
		0xc7, 0x45, 0xac, 0x00, 0x00, 0x00, 0x00,       // +0016  mov [ebp-0x54],0x00000000
		0xc7, 0x45, 0xb0, 0x00, 0x00, 0x00, 0x00,       // +001D  mov [ebp-0x50],0x00000000
		0xc7, 0x45, 0xb4, 0x00, 0x00, 0x00, 0x00,       // +0024  mov [ebp-0x4c],0x00000000
		0xc7, 0x45, 0xb8, 0x00, 0x00, 0x00, 0x00,       // +002b  mov [ebp-0x48],0x00000000
		0xe8,                                           // +0032  call CollectPlayers<CTFPlayer>
#endif
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
#ifdef PLATFORM_64BITS
			mask.SetRange(0x05 + 3, 1, 0x00);
			mask.SetRange(0x15 + 3, 1, 0x00);
			mask.SetRange(0x1d + 3, 1, 0x00);
			mask.SetRange(0x25 + 3, 1, 0x00);
			mask.SetRange(0x2d + 3, 4, 0x00);
#else
			mask.SetRange(0x0a + 2, 5, 0x00);
			mask.SetRange(0x13 + 2, 1, 0x00);
			mask.SetRange(0x16 + 2, 1, 0x00);
			mask.SetRange(0x1d + 2, 1, 0x00);
			mask.SetRange(0x24 + 2, 1, 0x00);
			mask.SetRange(0x2b + 2, 1, 0x00);
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* change the teamnum to TEAM_ANY */
#ifdef PLATFORM_64BITS
			buf .SetDword(0x0d + 1, TEAM_ANY);
			mask.SetDword(0x0d + 1, 0xffffffff);
#else
			buf .SetDword(0x05 + 1, TEAM_ANY);
			mask.SetDword(0x05 + 1, 0xffffffff);
#endif
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
#ifdef PLATFORM_64BITS
		0xe8, 0x85, 0x61, 0x2a, 0x00,        // +0x0000 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xf8, 0x03,                    // +0x0005 cmp     eax, TF_TEAM_PVE_INVADERS
		0x0f, 0x84, 0x4c, 0x02, 0x00, 0x00,  // +0x0008 jz      loc_AAD7D0
#else
		0x50,                               // +0000  push eax
		0xe8, 0x1e, 0x1d, 0x28, 0x00,       // +0001  call CBaseEntity::GetTeamNumber
		0x83, 0xC4, 0x10,                   // +0006  add esp, 10h
		0x83, 0xf8, 0x03,                   // +0009  cmp eax,TF_TEAM_PVE_INVADERS
		0x0f, 0x84, 0xa5, 0x01, 0x00, 0x00, // +000C  jz +0x1a5
#endif
	};
//	constexpr uint8_t s_Buf_CheckStuck_after[] = {
//		0x50,                               // +0000  push eax
//		0xff, 0x15, 0xff, 0xff, 0xff, 0xff, // +0001  call [<STUB> CBaseEntity::IsBot]
//		0x83, 0xC4, 0x10,                   // +0007  add esp, 10h
//		0x84, 0xc0,                         // +000a  test al, al
//		0x0f, 0x85, 0xa5, 0x01, 0x00, 0x00, // +000c  jnz +0x1a5
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

#ifdef PLATFORM_64BITS
			mask.SetRange(0x00 + 1, 4, 0x00);
			mask.SetRange(0x08 + 2, 4, 0x00);
#else
			mask.SetRange(0x01 + 1, 4, 0x00);
			mask.SetRange(0x06 + 2, 1, 0x00);
			mask.SetRange(0x0c + 2, 4, 0x00);
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
			/* test result */
			buf[0x05] = 0x90;
			buf[0x06] = 0x84;
			buf[0x07] = 0xc0;

			/* invert the jump condition code */
			buf[0x09] = 0x85;
#else
			// Move add esp
			buf[0x09] = buf[0x08];
			buf[0x08] = buf[0x07];
			buf[0x07] = buf[0x06];

			/* indirect call through pointer */
			buf[0x01] = 0xff;
			buf[0x02] = 0x15;
			buf.SetDword(0x01 + 2, (uint32_t)&s_CBasePlayer_IsBot);
			
			/* test result */
			buf[0x0a] = 0x84;
			buf[0x0b] = 0xc0;
			
			/* invert the jump condition code */
			buf[0x0d] = 0x85;
			
			mask.SetRange(0x01, 0x0b, 0xff);
			mask[0x0d] = 0xff;
#endif
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
#ifdef PLATFORM_64BITS
			buf.SetDword(0x00 + 1, AddrManager::GetAddrOffset("CBasePlayer::IsBot", GetFuncName(), 0x05 + this->GetActualOffset()));
#endif
			return true;
		}
		static FPtr_IsBot s_CBasePlayer_IsBot;
	};
	FPtr_IsBot CPatch_CTFGameMovement_CheckStuck::s_CBasePlayer_IsBot = &CBasePlayer::IsBot;
	
	
	constexpr uint8_t s_Buf_CTFPistol_ScoutPrimary_Push[] = {
#ifdef PLATFORM_64BITS
		0x83, 0xbb, 0xac, 0x10, 0x00, 0x00, 0x02,  // +0x0000 cmp     dword ptr [rbx+10ACh], 2
		0x0f, 0x84, 0x7d, 0x00, 0x00, 0x00,        // +0x0007 jz      loc_BF8EE0
		0x41, 0x83, 0xff, 0xfe,                    // +0x000d cmp     r15d, 0FFFFFFFEh
		0x74, 0x0d,                                // +0x0011 jz      short loc_BF8E76
#else
		0x83, 0xbb, 0x8c, 0x0e, 0x00, 0x00, 0x02,  // +0x0000 cmp     dword ptr [ebx+0E8Ch], 2
		0x74, 0x70,                                // +0x0007 jz      short loc_971E40
		0x83, 0xbd, 0xb4, 0xfe, 0xff, 0xff, 0xfe,  // +0x0009 cmp     [ebp+var_14C], 0FFFFFFFEh
		0x74, 0x14,                                // +0x0010 jz      short loc_971DED
#endif
	};
	
	struct CPatch_CTFPistol_ScoutPrimary_Push : public CPatch
	{
		CPatch_CTFPistol_ScoutPrimary_Push() : CPatch(sizeof(s_Buf_CTFPistol_ScoutPrimary_Push)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPistol_ScoutPrimary::Push"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0060; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0160; } // @ 0x0110
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPistol_ScoutPrimary_Push);
			
#ifdef PLATFORM_64BITS
			mask.SetRange(0x00 + 2, 4, 0x00);
			mask.SetRange(0x07 + 2, 4, 0x00);
			mask.SetRange(0x11 + 1, 1, 0x00);
#else
			mask.SetRange(0x00 + 2, 4, 0x00);
			mask.SetRange(0x07 + 1, 1, 0x00);
			mask.SetRange(0x09 + 2, 1, 0x00);
			mask.SetRange(0x10 + 1, 1, 0x00);
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			// Change jz to jnz (always jump)
#ifdef PLATFORM_64BITS
			buf[0x11] = 0x75;
			mask[0x11] = 0xff;
#else
			buf[0x10] = 0x75;
			mask[0x10] = 0xff;
#endif
			
			return true;
		}
		
	};

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
				if (pre_team == TF_TEAM_BLUE && TFGameRules()->IsMannVsMachineMode()) {
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
		
		// Check if already applied
		if (player->GetCustomVariableBool<"reprogrammedcond">()) return;

		player->SetCustomVariable<"reprogrammedcond">(Variant(true));

		if (!cvar_hellmet.GetBool()) {
			player->m_Shared->StunPlayer(5.0f, 0.65f, TF_STUNFLAG_NOSOUNDOREFFECT | TF_STUNFLAG_SLOWDOWN, nullptr);
		}
		
		/* added this check to prevent problems */
		
		if (player->IsBot()) {
			if (player->GetTeamNumber() == TF_TEAM_BLUE) {
				DevMsg("  currently on TF_TEAM_BLUE: calling ForceChangeTeam(TF_TEAM_RED)\n");
				if (player->m_Shared->InCond(TF_COND_DISGUISING)) {
					player->m_Shared->RemoveCond(TF_COND_DISGUISING);
				}
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
				
				if (player->m_Shared->InCond(TF_COND_DISGUISING)) {
					player->m_Shared->RemoveCond(TF_COND_DISGUISING);
				}
				player->ForceChangeTeam(TF_TEAM_RED, false);

				if (cvar_hellmet.GetBool()) {
					ChangeWeaponAndWearableTeam(player, TF_TEAM_RED);
				}
			}
			else {

				if (player->m_Shared->InCond(TF_COND_DISGUISING)) {
					player->m_Shared->RemoveCond(TF_COND_DISGUISING);
				}
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
		
		ResolvePlayerStuck(player, player->GetModelScale(), 1.6f, true);
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
		if (!player->GetCustomVariable<"reprogrammedcond">()) return;

		DevMsg("OnRemoveReprogrammed(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		player->SetCustomVariable<"reprogrammedcond">(Variant(false));
		
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
		
		
		ResolvePlayerStuck(player, player->GetModelScale(), 1.6f, true);
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
		
		// Check if already applied
		if (player->GetCustomVariableBool<"reprogrammedneutralcond">()) return;

		player->SetCustomVariable<"reprogrammedneutralcond">(Variant(true));

		if (player->GetTeamNumber() != TEAM_SPECTATOR) {
			DevMsg("  currently on TF_TEAM_BLUE: calling ForceChangeTeam(TF_TEAM_RED)\n");
			player->ChangeTeamBase(TEAM_SPECTATOR, false, true, false);
		} else {
			DevMsg("  currently on teamnum %d; not calling ForceChangeTeam\n", player->GetTeamNumber());
		}

		ResolvePlayerStuck(player, player->GetModelScale(), 1.6f, true);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 

		if (player->GetCustomVariableBool<"KilledAsNeutral">()) {
			player->SetCustomVariable<"KilledAsNeutral">(Variant(false));
		}
		
		DETOUR_MEMBER_CALL();
	}

	void OnRemoveReprogrammedNeutral(CTFPlayer *player)
	{
		if (!player->GetCustomVariable<"reprogrammedneutralcond">()) return;
		DevMsg("OnRemoveReprogrammedNeutral(#%d \"%s\")\n", ENTINDEX(player), player->GetPlayerName());
		
		player->SetCustomVariable<"reprogrammedneutralcond">(Variant(false));

		if (!player->IsAlive()) {
			player->SetCustomVariable<"KilledAsNeutral">(Variant(true));
		}
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
		ResolvePlayerStuck(player, player->GetModelScale(), 1.6f, true);
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

		return DETOUR_MEMBER_CALL(pPlayer, iWantedTeam, b1);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionAdded, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		if (cond == TF_COND_REPROGRAMMED) {
			OnAddReprogrammed(shared->GetOuter());
			return;
		}
		
		if (cond == GetExtraConditionCount() - 1) {
			OnAddReprogrammedNeutral(shared->GetOuter());
			return;
		}

		DETOUR_MEMBER_CALL(cond);
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

		if (cond == GetExtraConditionCount() - 1) {
			OnRemoveReprogrammedNeutral(shared->GetOuter());
			return;
		}
		DETOUR_MEMBER_CALL(cond);
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotScenarioMonitor_Update, CTFBot *actor, float dt)
	{
		if (actor->m_Shared->InCond(TF_COND_REPROGRAMMED)) {
			return ActionResult<CTFBot>::Continue();
		}
		
		return DETOUR_MEMBER_CALL(actor, dt);
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		
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
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_BetweenRounds_Start)
	{
		DETOUR_MEMBER_CALL();
		
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
		float ret = DETOUR_MEMBER_CALL(pTarget, piDamageType, piCustomDamage);

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
		DETOUR_MEMBER_CALL(actor);
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
				return DETOUR_MEMBER_CALL(newinfo);
			}
		
		}
		return DETOUR_MEMBER_CALL(info);
	}

	DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, const CTakeDamageInfo& info)
	{
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFBotMissionSuicideBomber_Detonate) {
			auto character = reinterpret_cast<CBaseCombatCharacter *>(this);
			if (!character->IsPlayer() && !character->IsBaseObject()) {
				CTakeDamageInfo newinfo = info;
				newinfo.SetDamage( 600.f );
				return DETOUR_MEMBER_CALL(newinfo);
			}
		}
		return DETOUR_MEMBER_CALL(info);
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMedicHeal_Update, CTFBot *actor, float dt)
	{
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		
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
		auto result = DETOUR_MEMBER_CALL(player);
		auto builder = reinterpret_cast<CObjectSapper *>(this)->GetBuilder();
		return result || (player != nullptr && !(builder != nullptr && player->GetTeamNumber() == builder->GetTeamNumber()) && player->IsAlive() && player->m_Shared->InCond(TF_COND_REPROGRAMMED) && !player->m_Shared->IsInvulnerable() && !player->m_Shared->InCond(TF_COND_SAPPED));
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_Disguise, int team, int iclass, CTFPlayer *victim, bool onKill)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		int ourTeam = shared->GetOuter()->GetTeamNumber();
		if (onKill && TFGameRules()->IsMannVsMachineMode() && team == TF_TEAM_BLUE && ourTeam == TF_TEAM_BLUE)
			team = TF_TEAM_RED;

		if (team == TEAM_SPECTATOR && ourTeam == TEAM_SPECTATOR ) {
			auto bot = ToTFBot(shared->GetOuter());
			if (bot != nullptr) {
				auto threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(false);
				ConVarRef sig_mvm_jointeam_blue_allow_force("sig_mvm_jointeam_blue_allow_force");
				team = sig_mvm_jointeam_blue_allow_force.GetBool() ? TF_TEAM_BLUE : TF_TEAM_RED;
				if (threat != nullptr && threat->GetEntity() != nullptr) {
					team = threat->GetEntity()->GetTeamNumber();
				}
			}
		}
		bool setDisguiseTeamToSpec = false;
		if (ourTeam != TEAM_SPECTATOR && victim != nullptr && (victim->IsAlive() || victim->GetCustomVariableBool<"KilledAsNeutral">())) {
			team = GetEnemyTeam(ourTeam);
			setDisguiseTeamToSpec = true;
		}
		DETOUR_MEMBER_CALL(team, iclass, victim, onKill);
		if (setDisguiseTeamToSpec) {
			shared->m_nDesiredDisguiseTeam = TEAM_SPECTATOR;
		}
	}

	RefCount rc_CTFPlayer_ModifyOrAppendCriteria;

	DETOUR_DECL_MEMBER(void, CTFPlayer_ModifyOrAppendCriteria, void *criteria)
	{
		SCOPED_INCREMENT_IF(rc_CTFPlayer_ModifyOrAppendCriteria, TFGameRules()->IsMannVsMachineMode() && *(reinterpret_cast<CTFPlayer *>(this)->GetPlayerClass()->GetCustomModel()) != 0 );
		DETOUR_MEMBER_CALL(criteria);
	}

	DETOUR_DECL_STATIC(bool, TF_IsHolidayActive)
	{
		if (rc_CTFPlayer_ModifyOrAppendCriteria)
			return false;
		else
			return DETOUR_STATIC_CALL();
	}
	
	RefCount rc_CTFGameRules_FireGameEvent__teamplay_round_start;
	DETOUR_DECL_MEMBER(void, CTFGameRules_FireGameEvent, IGameEvent *event)
	{
		SCOPED_INCREMENT_IF(rc_CTFGameRules_FireGameEvent__teamplay_round_start,
			(event != nullptr && strcmp(event->GetName(), "teamplay_round_start") == 0));
		DETOUR_MEMBER_CALL(event);
	}
	
	RefCount rc_CBaseObject_FindSnapToBuildPos;
	RefCount rc_CBaseObject_FindSnapToBuildPos_spec;
	RefCount rc_CollectPlayers_SpecEnemy;
	RefCount rc_CollectPlayers_SpecFriend;
	DETOUR_DECL_STATIC(int, CollectPlayers_CTFPlayer, CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		static bool reentrancy = false;
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
		// TF2 update inlined collectplayers here, using patch instead
		// if (rc_CollectPlayers_Enemy && !reentrancy) {
		// 	reentrancy = true;
		// 	CollectPlayers(playerVector, team == TEAM_SPECTATOR ? TF_TEAM_RED : TEAM_SPECTATOR, isAlive, shouldAppend);
		// 	if (team == TEAM_SPECTATOR) {
		// 		team = TF_TEAM_BLUE;
		// 	}
		// 	shouldAppend = true;
		// 	reentrancy = false;
		// }
		
		// if (rc_CBaseObject_FindSnapToBuildPos && !reentrancy) {
		// 	reentrancy = true;
		// 	if (rc_CBaseObject_FindSnapToBuildPos_spec) {
		// 		CollectPlayers(playerVector, team == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED, isAlive, shouldAppend);
		// 	}
		// 	else {
		// 		CollectPlayers(playerVector, TEAM_SPECTATOR, isAlive, shouldAppend);
		// 	}
		// 	shouldAppend = true;
		// 	reentrancy = false;
		// }
		
		if (team == TEAM_SPECTATOR && isAlive && !reentrancy && !rc_CollectPlayers_SpecFriend && !rc_CollectPlayers_SpecEnemy) {
			team = RandomInt(TEAM_SPECTATOR, TF_TEAM_BLUE);
		}

		if (team == TEAM_SPECTATOR && rc_CollectPlayers_SpecEnemy) {

			CollectPlayers(playerVector, TF_TEAM_RED, isAlive, shouldAppend);
			CollectPlayers(playerVector, TF_TEAM_BLUE, isAlive, true);
		}
		return DETOUR_STATIC_CALL(playerVector, team, isAlive, shouldAppend);
	}
	
	int getteam = -1;
    DETOUR_DECL_MEMBER(bool, CObjectSentrygun_FindTarget)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
        bool result = DETOUR_MEMBER_CALL();
        
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
					result = DETOUR_MEMBER_CALL();
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
					result = DETOUR_MEMBER_CALL();
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
		return DETOUR_MEMBER_CALL(team);
	}

	DETOUR_DECL_MEMBER(bool, NextBotPlayer_CTFPlayer_IsBot)
	{
		auto *bot = reinterpret_cast<NextBotPlayer<CTFPlayer> *>(this);
		return DETOUR_MEMBER_CALL() && !(bot->GetTeamNumber() == TEAM_SPECTATOR && bot->IsAlive());
	}

	CDetour *detour_isbot = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		std::vector<CHandle<CTFPlayer>> spec_players;
		CTFTeam *team_spec = TFTeamMgr()->GetTeam(TEAM_SPECTATOR);
		for (int i = 0; i < team_spec->GetNumPlayers(); i++) {
			CBasePlayer *bot = team_spec->GetPlayer(i);
			if (bot != nullptr && bot->IsBot() && bot->IsAlive()) {
				
				spec_players.push_back(bot->GetRefEHandle());
				team_spec->m_aPlayers->FindAndRemove(bot);
				i--;
				if (team_spec->m_hLeader == bot) {
					team_spec->m_hLeader = nullptr;
				}
			}
		}

		bool result = DETOUR_MEMBER_CALL(where, ents);

		for (auto bot : spec_players) {
			if (bot != nullptr && !bot->IsMarkedForDeletion() && bot->GetTeamNumber() == TEAM_SPECTATOR) {
				
				if (team_spec->m_aPlayers->Find(bot) == -1) {
					team_spec->AddPlayerNonVirtual(bot);
				}
			}
		}

		return result;
	}

	DETOUR_DECL_MEMBER(void, CWave_ActiveWaveUpdate)
	{
		auto wave = reinterpret_cast<CWave *>(this);
		DETOUR_MEMBER_CALL();
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
		CTraceFilterSimple *filter = reinterpret_cast<CTraceFilterSimple*>(this);
		
        // Always a player so ok to cast directly
        CBaseEntity *entityme = reinterpret_cast<CBaseEntity *>(const_cast<IHandleEntity *>(filter->GetPassEntity()));
		
		if (entityme != nullptr && entityme->GetTeamNumber() == TEAM_SPECTATOR) {
			CBaseEntity *entityhit = EntityFromEntityHandle(pServerEntity);
			if (entityhit != nullptr && entityhit->IsPlayer()) {
				if (entityme->GetTeamNumber() == entityhit->GetTeamNumber()) {
					return false;
				}
				else if (Mod::Attr::Custom_Attributes::GetFastAttributeIntExternal(entityme, 0, Mod::Attr::Custom_Attributes::NOT_SOLID_TO_PLAYERS) == 0 && 
					Mod::Attr::Custom_Attributes::GetFastAttributeIntExternal(entityhit, 0, Mod::Attr::Custom_Attributes::NOT_SOLID_TO_PLAYERS) == 0){
					return true;
				}
			}
			if (entityhit != nullptr && entityhit->IsBaseObject()) {
				if (entityme->GetTeamNumber() != entityhit->GetTeamNumber() && Mod::Attr::Custom_Attributes::GetFastAttributeIntExternal(entityme, 0, Mod::Attr::Custom_Attributes::NOT_SOLID_TO_PLAYERS) == 0) {
					return true;
				}
				else if (entityme->GetTeamNumber() == entityhit->GetTeamNumber() && ToBaseObject(entityhit)->GetBuilder() != entityme && ToBaseObject(entityhit)->GetType() != OBJ_TELEPORTER){
					return false;
				}
			}
		}
		return DETOUR_MEMBER_CALL(pServerEntity, contentsMask);
	}

	RefCount rc_CTFBot_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		static ConVarRef cvar_player_team("sig_mvm_jointeam_blue_allow_force"); 
		auto bot = reinterpret_cast<CTFBot *>(this);
		SCOPED_INCREMENT_IF(rc_CTFBot_Event_Killed, cvar_player_team.GetBool() || bot->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(info);
	}

	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		if (rc_CTFBot_Event_Killed && strcmp(event->GetName(), "mvm_mission_update") == 0) {
			mgr->FreeEvent(event);
			return false;
		}
		
		return DETOUR_MEMBER_CALL(event, bDontBroadcast);
	}

	DETOUR_DECL_MEMBER(bool, CTFBotMedicHeal_IsVisibleToEnemy, CTFBot *me, const Vector &where)
	{
		if (TFGameRules()->IsMannVsMachineMode() && me->GetTeamNumber() == TF_TEAM_RED) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(me, where);
	}
	
	RefCount rc_CTFPlayer_ApplyPushFromDamage_block;
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyPushFromDamage, const CTakeDamageInfo& info, Vector vec)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		auto block = (player->IsMiniBoss() || player->GetModelScale() > 1.0f) && player->GetTeamNumber() != TF_TEAM_BLUE;
		auto playerAttacker = block ? ToTFPlayer(info.GetAttacker()) : nullptr;
		if (block && playerAttacker != player && playerAttacker != nullptr && playerAttacker->m_Shared->m_bRageDraining 
				&& playerAttacker->GetPlayerClass()->GetClassIndex() == TF_CLASS_HEAVYWEAPONS) {
			int rage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(playerAttacker, rage, generate_rage_on_dmg);
			if (rage != 0) {
				block = false;
			} 
		}
		SCOPED_INCREMENT_IF(rc_CTFPlayer_ApplyPushFromDamage_block, block);
		DETOUR_MEMBER_CALL(info, vec);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyAbsVelocityImpulse, const Vector *v1)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		if (rc_CTFPlayer_ApplyPushFromDamage_block) return;
		
		DETOUR_MEMBER_CALL(v1);
	}

	DETOUR_DECL_MEMBER(const char *, CTFWeaponBase_GetShootSound, int iIndex)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		auto player = weapon->GetTFPlayerOwner();

		int oldWeaponTeam = weapon->GetTeamNumber();
		if (player != nullptr && player->IsMiniBoss() && player->GetTeamNumber() != TF_TEAM_BLUE && WeaponHasMiniBossSounds(weapon)) {
			weapon->SetTeamNumber(TF_TEAM_BLUE);
		}

		auto result = DETOUR_MEMBER_CALL(iIndex);

		weapon->SetTeamNumber(oldWeaponTeam);
		return result;
	}

	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		std::vector<std::pair<CBaseEntity *, int>> weaponsTeamSwitched;
		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->IsMiniBoss() && player->GetTeamNumber() != TF_TEAM_BLUE) {
				auto weapon = player->GetActiveTFWeapon();
				if (weapon != nullptr && WeaponHasMiniBossSounds(weapon)) {
					weaponsTeamSwitched.push_back({weapon, weapon->GetTeamNumber()});
					weapon->SetTeamNumber(TF_TEAM_BLUE);
				}
			}
		}); 
		DETOUR_STATIC_CALL(clientCount, clients, snapshot);
		for (auto &weapon : weaponsTeamSwitched) {
			weapon.first->SetTeamNumber(weapon.second);
		}
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_Update, CTFBot *actor, float dt)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		//DevMsg("Bomberupdate %d %d\n", me->m_hTarget != nullptr, me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && !me->m_hTarget->IsBaseObject());
		if (me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && me->m_hTarget->GetTeamNumber() != actor->GetTeamNumber() && me->m_hTarget->IsBaseObject()) {
			for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
				auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
				if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() != actor->GetTeamNumber() && !obj->m_bPlacing) {
					me->m_hTarget = obj;
					me->m_vecTargetPos = obj->GetAbsOrigin();
					me->m_vecDetonatePos = obj->GetAbsOrigin();
					break;
				}
			}
		}
		//DevMsg("\n[Update]\n");
		//DevMsg("reached goal %d,detonating %d,failures %d, %d %f %f\n",me->m_bDetReachedGoal, me->m_bDetonating ,me->m_nConsecutivePathFailures,ENTINDEX(me->m_hTarget), me->m_vecDetonatePos.x, me->m_vecTargetPos.x);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_FindSnapToBuildPos, CBaseObject *pObjectOverride)
	{
		auto me = reinterpret_cast<CBaseObject *>(this);
		bool success = DETOUR_MEMBER_CALL(pObjectOverride);
		
		if (pObjectOverride != nullptr) return success;
		
		if (success) return true;

		SCOPED_INCREMENT(rc_CollectPlayers_SpecFriend);
		//SCOPED_INCREMENT(rc_CBaseObject_FindSnapToBuildPos);
		//SCOPED_INCREMENT_IF(rc_CBaseObject_FindSnapToBuildPos_spec, me->GetBuilder() != nullptr && me->GetBuilder()->GetTeamNumber() == TEAM_SPECTATOR);
		
		if (!me->MustBeBuiltOnAttachmentPoint()) return false;

		CTFPlayer *pPlayer = me->GetBuilder();

		if ( !pPlayer ) return false;

		bool bSnappedToPoint = false;
		bool bShouldAttachToParent = false;

		Vector vecNearestBuildPoint = vec3_origin;

		// See if there are any nearby build positions to snap to
		float flNearestPoint = 9999;
		int i;

		if (me->IsHostileUpgrade() && me->GetTeamNumber() == TEAM_SPECTATOR) return false;

		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TEAM_SPECTATOR, true );
		FOR_EACH_VEC( playerVector, i )
		{
			if ( !playerVector[i]->IsBot() )
				continue;

			if ( me->FindBuildPointOnPlayer( playerVector[i], pPlayer, flNearestPoint, vecNearestBuildPoint ) )
			{
				bSnappedToPoint = true;
				bShouldAttachToParent = true;
			}
		}

		auto specTeam = TFTeamMgr()->GetTeam(TEAM_SPECTATOR);
		// look for nearby buildpoints on other objects
		for ( i = 0; i < specTeam->GetNumObjects(); i++ )
		{
			CBaseObject *pObject = specTeam->GetObject(i);
			if (pObject && !pObject->m_bPlacing)
			{
				if ( me->FindNearestBuildPoint( pObject, pPlayer, flNearestPoint, vecNearestBuildPoint, false) )
				{
					bSnappedToPoint = true;
					bShouldAttachToParent = true;
				}
			}
		}

		if ( !bSnappedToPoint )
		{
			me->AddEffects( EF_NODRAW );
		}
		else
		{
			me->RemoveEffects( EF_NODRAW );

			if ( bShouldAttachToParent )
			{
				me->AttachObjectToObject( me->GetBuiltOnEntity(), me->m_iBuiltOnPoint, vecNearestBuildPoint);
			}

			me->m_vecBuildOrigin = vecNearestBuildPoint;
		}

		return bSnappedToPoint;
	}

	DETOUR_DECL_MEMBER(CTFTeam *, CTFPlayer_GetOpposingTFTeam)
	{
		auto me = reinterpret_cast<CTFPlayer *>(this);
		if (me->GetTeamNumber() == TEAM_SPECTATOR && me->IsAlive()) {
			return TFTeamMgr()->GetTeam(RandomInt(TF_TEAM_RED, TF_TEAM_BLUE));
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CTFKnife_CanPerformBackstabAgainstTarget, CTFPlayer *target )
	{
		bool ret = DETOUR_MEMBER_CALL(target);

		if ( !ret && TFGameRules() && TFGameRules()->IsMannVsMachineMode() && target->GetTeamNumber() == TEAM_SPECTATOR )
		{
			if ( target->m_Shared->InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
				return true;

			if ( target->m_Shared->InCond( TF_COND_SAPPED ) && !target->IsMiniBoss() )
				return true;
		}
		return ret;
	}

	ConVar sig_cond_reprogrammed_neutral_spy_disguise("sig_cond_reprogrammed_neutral_spy_disguise", "0", FCVAR_GAMEDLL, "Make neutral bots ignore disguised spies");
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		CTFBot *me = reinterpret_cast<CTFBot *>(reinterpret_cast<IVision *>(this)->GetBot()->GetEntity());
		if (me->GetTeamNumber() != TEAM_SPECTATOR || !sig_cond_reprogrammed_neutral_spy_disguise.GetBool()) return DETOUR_MEMBER_CALL(ent);
		
		auto player = ToTFPlayer(ent);
		int restoreTeam = -1;
		if (player != nullptr) {
			if (player->m_Shared->InCond( TF_COND_DISGUISED ) && player->m_Shared->GetDisguiseTeam() != player->GetTeamNumber()) {
				restoreTeam = player->m_Shared->m_nDisguiseTeam;
				player->m_Shared->m_nDisguiseTeam = me->GetTeamNumber();
			}
		}
		bool ret = DETOUR_MEMBER_CALL(ent);
		if (restoreTeam != -1) {
			player->m_Shared->m_nDisguiseTeam = restoreTeam;
		}

		return ret;
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_DeathSound, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->IsBot()) {
			auto teamnum = player->GetTeamNumber();
			if (teamnum == TF_TEAM_RED || teamnum == TEAM_SPECTATOR) {
				player->SetTeamNumber(TF_TEAM_BLUE);
			}
			DETOUR_MEMBER_CALL(info);
			player->SetTeamNumber(teamnum);
		}
		else
			DETOUR_MEMBER_CALL(info);
	}

	DETOUR_DECL_MEMBER(void, CTFPistol_ScoutPrimary_Push)
	{
		SCOPED_INCREMENT(rc_CollectPlayers_SpecEnemy);
		DETOUR_MEMBER_CALL();
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_C2)
	{
		DETOUR_MEMBER_CALL();
        reinterpret_cast<CTFPlayerShared *>(this)->m_ConditionData->EnsureCount(GetExtraConditionCount());
	}
	
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		if (nCond < 0 || nCond >= GetExtraConditionCount()) {
            return;
        }
		DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);
	}
	
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_RemoveAllCond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		int maxCond = shared->m_ConditionData->Count();
		auto condData = shared->GetCondData();
		for (int i = GetNumberOfTFConds(); i < maxCond; i++) {
			if (condData.InCond(i)) {
				shared->RemoveCond((ETFCond)i);
			}
		}
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		// Remove reprogrammed cond before changing team, apply back if team not changed
		OnRemoveReprogrammed(player);
		OnRemoveReprogrammedNeutral(player);
		int preTeam = player->GetTeamNumber();
		DETOUR_MEMBER_CALL(iTeamNum, b1, b2, b3);
		if (preTeam == player->GetTeamNumber()) {
			OnRemoveReprogrammed(player);
			OnRemoveReprogrammedNeutral(player);
		}
	}

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
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");

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


			// Fix reprogrammed weapons not making giant sounds
			MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks,                  "SV_ComputeClientPacks");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetShootSound, "CTFWeaponBase::GetShootSound");

			MOD_ADD_DETOUR_MEMBER(CBaseObject_FindSnapToBuildPos, "CBaseObject::FindSnapToBuildPos");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOpposingTFTeam, "CTFPlayer::GetOpposingTFTeam");

			// Allow spectator team bots to be backstabbed at any angle when sapped
			MOD_ADD_DETOUR_MEMBER(CTFKnife_CanPerformBackstabAgainstTarget, "CTFKnife::CanPerformBackstabAgainstTarget [clone]");
			
			// Fix spectator team bots ignoring disguise
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");

            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_C2, "CTFPlayerShared::CTFPlayerShared");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_AddCond, "CTFPlayerShared::AddCond");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_RemoveAllCond, "CTFPlayerShared::RemoveAllCond");
			
			// Fix spectator team bots death sound
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_DeathSound, "CTFPlayer::DeathSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ChangeTeam,                 "CTFPlayer::ChangeTeam");

			// Fix spectator team shortstop push
			// TF2 update inlined collectplayers here, using patch instead
            //MOD_ADD_DETOUR_MEMBER(CTFPistol_ScoutPrimary_Push, "CTFPistol_ScoutPrimary::Push");
			this->AddPatch(new CPatch_CTFPistol_ScoutPrimary_Push());

			// Change sentry buster target if the sentry gun is on the same team
			// MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Update,                  "CTFBotMissionSuicideBomber::Update");
			
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
			ForEachTFPlayer([](CTFPlayer *player){
				auto &shared = player->m_Shared.Get();
				int maxCond = shared.m_ConditionData->Count();
				auto condData = shared.GetCondData();
				for (int i = GetNumberOfTFConds(); i < maxCond; i++) {
					if (condData.InCond(i)) {
						auto &data = shared.m_ConditionData.Get()[i];
						float duration = data.m_flExpireTime;
						if (duration != -1) {
							data.m_flExpireTime -= gpGlobals->frametime;

							if ( data.m_flExpireTime <= 0 )
							{
								shared.RemoveCond((ETFCond)i);
							}
						}
					}
				}
			});

			if (PlayerResource() == nullptr) return;
			
			for (size_t i = 0; i < bots_killed.size(); i++) {
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
