#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "stub/objects.h"
#include "stub/tf_player_resource.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "util/clientmsg.h"
#include "util/admin.h"
#include "util/iterate.h"
#include "stub/populators.h"
#include "mod/pop/popmgr_extensions.h"

// TODO: move to common.h
#include <igamemovement.h>
#include <in_buttons.h>

class CLaserDot {};

namespace Mod::MvM::JoinTeam_Blue_Allow
{
	using CollectPlayersFunc_t = int (*)(CUtlVector<CTFPlayer *> *, int, bool, bool);
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller1[] = {
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0008  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +0010  mov dword ptr [esp+0x4],<int:team>
		0x89, 0x04, 0x24,                               // +0018  mov [esp],exx
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +001B  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0022  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0029  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0030  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0037  mov [ebp-0xXXX],0x00000000
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +003E  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller1 : public CPatch
	{
		CPatch_CollectPlayers_Caller1(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller1)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller1);
			
			buf.SetDword(0x00 + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x08 + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x10 + 4, (uint32_t)TEAM);
			
			/* allow any 3-bit source register code */
			mask[0x18 + 1] = 0b11000111;
			
			mask.SetDword(0x1b + 2, 0xfffff003);
			mask.SetDword(0x22 + 2, 0xfffff003);
			mask.SetDword(0x29 + 2, 0xfffff003);
			mask.SetDword(0x30 + 2, 0xfffff003);
			mask.SetDword(0x37 + 2, 0xfffff003);
			
			mask.SetDword(0x3e + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x3e, 5, 0xcc);
			mask.SetRange(0x3e, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x3e + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x3e] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x3e + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller2[] = {
		0x89, 0x04, 0x24,                               // +0000  mov [esp],exx
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0003  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +000B  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +0013  mov dword ptr [esp+0x4],<int:team>
		0xa3, 0x00, 0x00, 0x00, 0x00,                   // +001B  mov ds:0xXXXXXXXX,eax
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0020  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0027  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +002E  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0035  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +003C  mov [ebp-0xXXX],0x00000000
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +0043  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller2 : public CPatch
	{
		CPatch_CollectPlayers_Caller2(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller2)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller2);
			
			/* allow any 3-bit source register code */
			mask[0x00 + 1] = 0b11000111;
			
			buf.SetDword(0x03 + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x0b + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x13 + 4, (uint32_t)TEAM);
			
			mask.SetDword(0x1b + 1, 0x00000000);
			
			mask.SetDword(0x20 + 2, 0xfffff003);
			mask.SetDword(0x27 + 2, 0xfffff003);
			mask.SetDword(0x2e + 2, 0xfffff003);
			mask.SetDword(0x35 + 2, 0xfffff003);
			mask.SetDword(0x3c + 2, 0xfffff003);
			
			mask.SetDword(0x43 + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x43, 5, 0xcc);
			mask.SetRange(0x43, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x43 + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x43] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x43 + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller3[] = {
		0x01, 0x45, 0x00,                               // +0000  add [ebp-0xXX],eax
		0x8b, 0x45, 0x08,                               // +0003  mov eax,[ebp+this]
		0x8b, 0x80, 0x00, 0x00, 0x00, 0x00,             // +0006  mov eax,[eax+0xXXXX]
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +000C  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0014  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +001C  mov dword ptr [esp+0x4],<int:team>
		0x01, 0x45, 0x00,                               // +0024  add [ebp-0xXX],eax
		0x8d, 0x45, 0x00,                               // +0027  lea eax,[ebp-0xXX]
		0x89, 0x04, 0x24,                               // +002A  mov [esp],exx
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +002D  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller3 : public CPatch
	{
		CPatch_CollectPlayers_Caller3(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller3)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller3);
			
			mask[0x00 + 2] = 0x00;
			
			mask.SetDword(0x06 + 2, 0x00000000);
			
			buf.SetDword(0x0c + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x14 + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x1c + 4, (uint32_t)TEAM);
			
			mask[0x24 + 2] = 0x00;
			mask[0x27 + 2] = 0x00;
			
			/* allow any 3-bit source register code */
			mask[0x2a + 1] = 0b11000111;
			
			mask.SetDword(0x2d + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x2d, 5, 0xcc);
			mask.SetRange(0x2d, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x2d + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x2d] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x2d + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_RadiusSpyScan[] = {
		0x8b, 0x87, 0x00, 0x00, 0x00, 0x00, // +0000  mov exx,[exx+offsetof(CTFPlayerShared, m_pOuter)]
		0x89, 0x04, 0x24,                   // +0006  mov [esp],exx
		0xe8, 0x00, 0x00, 0x00, 0x00,       // +0009  call CBaseEntity::GetTeamNumber
		0x83, 0xf8, 0x02,                   // +000E  cmp eax,TF_TEAM_RED
	};
	
	struct CPatch_RadiusSpyScan : public CPatch
	{
		CPatch_RadiusSpyScan() : CPatch(sizeof(s_Buf_RadiusSpyScan)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::RadiusSpyScan"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0020; } // @ +0x000c
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_RadiusSpyScan);
			
			int off_CTFPlayerShared_m_pOuter;
			if (!Prop::FindOffset(off_CTFPlayerShared_m_pOuter, "CTFPlayerShared", "m_pOuter")) return false;
			buf.SetDword(0x00 + 2, off_CTFPlayerShared_m_pOuter);
			
			/* allow any 3-bit source or destination register code */
			mask[0x00 + 1] = 0b11000000;
			
			/* allow any 3-bit source register code */
			mask[0x06 + 1] = 0b11000111;
			
			mask.SetDword(0x09 + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* replace 'cmp eax,TF_TEAM_RED' with 'cmp eax,eax; nop' */
			buf[0x0e + 0] = 0x39;
			buf[0x0e + 1] = 0xc0;
			buf[0x0e + 2] = 0x90;
			
			mask.SetRange(0x0e, 3, 0xff);
			
			return true;
		}
	};
	
	
	ConVar cvar_max("sig_mvm_jointeam_blue_allow_max", "-1", FCVAR_NOTIFY,
		"Blue humans in MvM: max humans to allow on blue team (-1 for no limit)");
	
	ConVar cvar_flag_pickup("sig_mvm_bluhuman_flag_pickup", "0", FCVAR_NOTIFY,
		"Blue humans in MvM: allow picking up the flag");
	ConVar cvar_flag_capture("sig_mvm_bluhuman_flag_capture", "0", FCVAR_NOTIFY,
		"Blue humans in MvM: allow scoring a flag Capture by carrying it to the capture zone");
	
	ConVar cvar_spawn_protection("sig_mvm_bluhuman_spawn_protection", "1", FCVAR_NOTIFY,
		"Blue humans in MvM: enable spawn protection invulnerability");
	ConVar cvar_spawn_no_shoot("sig_mvm_bluhuman_spawn_noshoot", "1", FCVAR_NOTIFY,
		"Blue humans in MvM: when spawn protection invulnerability is enabled, disallow shooting from spawn");
	
	ConVar cvar_infinite_cloak("sig_mvm_bluhuman_infinite_cloak", "1", FCVAR_NOTIFY,
		"Blue humans in MvM: enable infinite spy cloak meter");
	ConVar cvar_infinite_cloak_deadringer("sig_mvm_bluhuman_infinite_cloak_deadringer", "0", FCVAR_NOTIFY,
		"Blue humans in MvM: enable infinite spy cloak meter (Dead Ringer)");

	ConVar cvar_infinite_ammo("sig_mvm_bluhuman_infinite_ammo", "1", FCVAR_NOTIFY,
		"Blue humans in MvM: infinite ammo");

	ConVar cvar_teleport("sig_mvm_bluhuman_teleport", "0", FCVAR_NOTIFY,
		"Blue humans in MvM: teleport to engiebot teleport");

	ConVar cvar_teleport_player("sig_mvm_bluhuman_teleport_player", "0", FCVAR_NOTIFY,
		"Blue humans in MvM: teleport bots and players to player teleport exit");
	
	std::map<CHandle<CTFPlayer>, float> player_deploy_time; 
	// TODO: probably need to add in a check for TF_COND_REPROGRAMMED here and:
	// - exclude humans on TF_TEAM_BLUE who are in TF_COND_REPROGRAMMED
	// - include humans on TF_TEAM_RED who are in TF_COND_REPROGRAMMED
	
	bool IsMvMBlueHuman(CTFPlayer *player)
	{
		if (player == nullptr)                       return false;
		if (player->GetTeamNumber() != TF_TEAM_BLUE) return false;
		if (player->IsBot())                         return false;
		if (!TFGameRules()->IsMannVsMachineMode())   return false;
		
		return true;
	}
	
	int GetMvMBlueHumanCount()
	{
		int count = 0;
		
		ForEachTFPlayer([&](CTFPlayer *player){
			if (IsMvMBlueHuman(player)) {
				++count;
			}
		});
		
		return count;
	}
	
	bool IsInBlueSpawnRoom(CTFPlayer *player)
	{
		player->UpdateLastKnownArea();
		auto area = static_cast<CTFNavArea *>(player->GetLastKnownArea());
		return (area != nullptr && area->HasTFAttributes(BLUE_SPAWN_ROOM));
	}
	
	static int CollectPlayers_EnemyTeam(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		extern ConVar cvar_force;
		ClientMsgAll("see enemy team %d\n", cvar_force.GetBool());
		return CollectPlayers(playerVector, cvar_force.GetBool() ? TF_TEAM_RED : TF_TEAM_BLUE,  isAlive, shouldAppend);
	}

	static int CollectPlayers_RedAndBlue(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		(void) CollectPlayers(playerVector, TF_TEAM_RED,  isAlive, shouldAppend);
		return CollectPlayers(playerVector, TF_TEAM_BLUE, isAlive, true);
	}
	
	static int CollectPlayers_RedAndBlue_IsBot(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		CUtlVector<CTFPlayer *> tempVector;
		CollectPlayers(&tempVector, TF_TEAM_RED,  isAlive, true);
		CollectPlayers(&tempVector, TF_TEAM_BLUE, isAlive, true);
		
		if (!shouldAppend) {
			playerVector->RemoveAll();
		}
		
		for (auto player : tempVector) {
			if (player->IsBot()) {
				playerVector->AddToTail(player);
			}
		}
		
		return playerVector->Count();
	}
	
	static int CollectPlayers_RedAndBlue_NotBot(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		CUtlVector<CTFPlayer *> tempVector;
		CollectPlayers(&tempVector, TF_TEAM_RED,  isAlive, true);
		CollectPlayers(&tempVector, TF_TEAM_BLUE, isAlive, true);
		
		if (!shouldAppend) {
			playerVector->RemoveAll();
		}
		
		for (auto player : tempVector) {
			if (!player->IsBot()) {
				playerVector->AddToTail(player);
			}
		}
		
		return playerVector->Count();
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFGameRules_GetTeamAssignmentOverride, CTFPlayer *pPlayer, int iWantedTeam, bool b1)
	{
		/* it's important to let the call happen, because pPlayer->m_nCurrency
		 * is set to its proper value in the call (stupid, but whatever) */
		auto iResult = DETOUR_MEMBER_CALL(CTFGameRules_GetTeamAssignmentOverride)(pPlayer, iWantedTeam, b1);
		
		// debug message for the "sometimes bots don't get put on TEAM_SPECTATOR properly at wave end" situation
		if (TFGameRules()->IsMannVsMachineMode() && pPlayer->IsBot() && iResult != iWantedTeam) {
			DevMsg("[CTFGameRules::GetTeamAssignmentOverride] Bot [ent:%d userid:%d name:\"%s\"]: on team %d, wanted %d, forced onto %d!\n",
				ENTINDEX(pPlayer), pPlayer->GetUserID(), pPlayer->GetPlayerName(), pPlayer->GetTeamNumber(), iWantedTeam, iResult);
			BACKTRACE();
		}
		
		extern ConVar cvar_force;
		//DevMsg("Get team assignment %d %d %d\n", iWantedTeam, cvar_force.GetBool());

		if (TFGameRules()->IsMannVsMachineMode() && !pPlayer->IsFakeClient() && !pPlayer->IsBot() 
			&& ((iWantedTeam == TF_TEAM_BLUE && iResult != iWantedTeam) || (iWantedTeam == TF_TEAM_RED && cvar_force.GetBool())) ) {
			// NOTE: if the pop file had custom param 'AllowJoinTeamBlue 1', then disregard admin-only restrictions
			extern ConVar cvar_adminonly;
			if (cvar_force.GetBool() || Mod::Pop::PopMgr_Extensions::PopFileIsOverridingJoinTeamBlueConVarOn() ||
				!cvar_adminonly.GetBool() || PlayerIsSMAdmin(pPlayer)) {
				if (cvar_max.GetInt() < 0 || GetMvMBlueHumanCount() < cvar_max.GetInt()) {
					DevMsg("Player #%d \"%s\" requested team %d but was forced onto team %d; overriding to allow them to join team %d.\n",
						ENTINDEX(pPlayer), pPlayer->GetPlayerName(), iWantedTeam, iResult, iWantedTeam);
					iResult = TF_TEAM_BLUE;
				} else {
					if (cvar_force.GetBool())
						iResult = TEAM_SPECTATOR;
					DevMsg("Player #%d \"%s\" requested team %d but was forced onto team %d; would have overridden to allow joining team %d but limit has been met.\n",
						ENTINDEX(pPlayer), pPlayer->GetPlayerName(), iWantedTeam, iResult, iWantedTeam);
					ClientMsg(pPlayer, "Cannot join team blue: the maximum number of human players on blue team has already been met.\n");
				}
			} else {
				ClientMsg(pPlayer, "You are not authorized to use this command because you are not a SourceMod admin. Sorry.\n");
			}
		}
		
		//DevMsg("Get team assignment result %d\n", iResult);
		
		return iResult;
	}
	
	ConVar cvar_allow_reanimators("sig_mvm_jointeam_blue_allow_revive", "0", FCVAR_NOTIFY, "Allow reanimators for the blue players");
	DETOUR_DECL_STATIC(CTFReviveMarker *, CTFReviveMarker_Create, CTFPlayer *player)
	{
		if (!cvar_allow_reanimators.GetBool() && IsMvMBlueHuman(player)) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(CTFReviveMarker_Create)(player);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_OnNavAreaChanged, CNavArea *enteredArea, CNavArea *leftArea)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		DETOUR_MEMBER_CALL(CTFPlayer_OnNavAreaChanged)(enteredArea, leftArea);
		
		if (IsMvMBlueHuman(player) &&
			(enteredArea == nullptr ||  static_cast<CTFNavArea *>(enteredArea)->HasTFAttributes(BLUE_SPAWN_ROOM)) &&
			(leftArea    == nullptr || !static_cast<CTFNavArea *>(leftArea)   ->HasTFAttributes(BLUE_SPAWN_ROOM))) {
			player->m_Shared->m_bInUpgradeZone = false;
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_ClientCommandKeyValues, edict_t *pEntity, KeyValues *pKeyValues)
	{
		if (FStrEq(pKeyValues->GetName(), "MvM_UpgradesDone")) {
			CTFPlayer *player = ToTFPlayer(GetContainingEntity(pEntity));
			if (IsMvMBlueHuman(player)) {
				player->m_Shared->m_bInUpgradeZone = false;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFGameRules_ClientCommandKeyValues)(pEntity, pKeyValues);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsAllowedToPickUpFlag)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (IsMvMBlueHuman(player) && !cvar_flag_pickup.GetBool()) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_IsAllowedToPickUpFlag)();
	}
	
	DETOUR_DECL_MEMBER(void, CCaptureZone_ShimTouch, CBaseEntity *pOther)
	{
		auto zone = reinterpret_cast<CCaptureZone *>(this);
		
		[&]{
			if (zone->IsDisabled()) return;
			
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player == nullptr)       return;
			if (!IsMvMBlueHuman(player)) return;
			
			if (!cvar_flag_capture.GetBool()) return;
			
			if (!player->HasItem()) return;
			CTFItem *item = player->GetItem();
			
			if (item->GetItemID() != TF_ITEM_CAPTURE_FLAG) return;
			
			auto flag = dynamic_cast<CCaptureFlag *>(item);
			if (flag == nullptr) return;
			
			// skipping flag->m_nType check out of laziness
			
			if (!TFGameRules()->FlagsMayBeCapped()) return;
			if (zone->GetTeamNumber() == TEAM_UNASSIGNED || player->GetTeamNumber() == TEAM_UNASSIGNED || zone->GetTeamNumber() == player->GetTeamNumber()) {
				
				bool wasTaunting = player->m_Shared->InCond(TF_COND_FREEZE_INPUT);
				//if (!wasTaunting) {
				//	 //Taunt(TAUNT_BASE_WEAPON,0);
				//}
				player->m_Shared->AddCond(TF_COND_FREEZE_INPUT, 0.7f);
				if (player_deploy_time.find(player) == player_deploy_time.end() || !wasTaunting || gpGlobals->curtime - player_deploy_time[player] > 2.2f) {
					player_deploy_time[player] = gpGlobals->curtime;
					player->SetAbsVelocity(vec3_origin);
					player->PlaySpecificSequence("primary_deploybomb");
				}
				else {
					if (!wasTaunting)
						player_deploy_time.erase(player);
					else if (gpGlobals->curtime - player_deploy_time[player] >= 1.9f) {
						#warning Should have flag->IsCaptured() check in here
						//	if (flag->IsCaptured() || zone->GetTeamNumber() == TEAM_UNASSIGNED || player->GetTeamNumber() == TEAM_UNASSIGNED || zone->GetTeamNumber() == player->GetTeamNumber()) {
						zone->Capture(player);
						player_deploy_time.erase(player);
					}
				}
			}
			//player->m_Shared->StunPlayer(1.0f, 1.0f, TF_STUNFLAGS_CONTROL | TF_STUNFLAGS_CONTROL, nullptr);
		}();
		
		DETOUR_MEMBER_CALL(CCaptureZone_ShimTouch)(pOther);
	}
	
	
//	DETOUR_DECL_MEMBER(void, CPlayerMove_StartCommand, CBasePlayer *player, CUserCmd *ucmd)
//	{
//		DETOUR_MEMBER_CALL(CPlayerMove_StartCommand)(player, ucmd);
//		
//		DevMsg("CPlayerMove::StartCommand(#%d): buttons = %08x\n", ENTINDEX(player), ucmd->buttons);
//		
//		/* ideally we'd either do this or not do this based on the value of
//		 * CanBotsAttackWhileInSpawnRoom in g_pPopulationManager, but tracking
//		 * down the offset of that boolean is more work than it's worth */
//		CTFPlayer *tfplayer = ToTFPlayer(player);
//		if (cvar_spawn_protection.GetBool() && cvar_spawn_no_shoot.GetBool() && IsMvMBlueHuman(tfplayer) && IsInBlueSpawnRoom(tfplayer)) {
//			ucmd->buttons &= ~(IN_ATTACK | IN_ATTACK2 | IN_ATTACK3);
//			DevMsg("- stripped attack buttons: %08x\n", ucmd->buttons);
//		}
//	}
	
	
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
			/* collect players on BOTH teams */
			return CollectPlayers_RedAndBlue_IsBot(playerVector, team, isAlive, shouldAppend);
		}
		
		return DETOUR_STATIC_CALL(CollectPlayers_CTFPlayer)(playerVector, team, isAlive, shouldAppend);
	}
	
	
	RefCount rc_CTFPlayerShared_RadiusSpyScan;
	int radius_spy_scan_teamnum = TEAM_INVALID;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_RadiusSpyScan)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		
		/* instead of restricting the ability to team red, restrict it to human players */
		if (player->IsBot()) {
			return;
		}
		
		SCOPED_INCREMENT(rc_CTFPlayerShared_RadiusSpyScan);
		radius_spy_scan_teamnum = player->GetTeamNumber();
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_RadiusSpyScan)();
	}
	
	static int CollectPlayers_RadiusSpyScan(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		/* sanity checks */
		assert(rc_CTFPlayerShared_RadiusSpyScan > 0);
		assert(radius_spy_scan_teamnum == TF_TEAM_RED || radius_spy_scan_teamnum == TF_TEAM_BLUE);
		
		/* rather than always affecting blue players, affect players on the opposite team of the player with the ability */
		return CollectPlayers(playerVector, GetEnemyTeam(radius_spy_scan_teamnum), isAlive, shouldAppend);
	}
	
	
	/* log cases where bots are spawning at weird times (not while the wave is running) */
	DETOUR_DECL_MEMBER(void, CTFBot_Spawn)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		DETOUR_MEMBER_CALL(CTFBot_Spawn)();
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			// ========= MANN VS MACHINE MODE ROUND STATE TRANSITIONS ==========
			// [CPopulationManager::JumpToWave]                     --> PREROUND
			// [CPopulationManager::StartCurrentWave]               --> RND_RUNNING
			// [CPopulationManager::WaveEnd]                        --> BETWEEN_RNDS
			// [CPopulationManager::WaveEnd]                        --> GAME_OVER
			// -----------------------------------------------------------------
			// [CTeamplayRoundBasedRules::CTeamplayRoundBasedRules] --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_INIT]         --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_RND_RUNNING]  --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_PREGAME]      --> STARTGAME
			// [CTeamplayRoundBasedRules::CheckReadyRestart]        --> RESTART
			// [CTeamplayRoundBasedRules::State_Enter_RESTART]      --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_STARTGAME]    --> PREROUND
			// [CTeamplayRoundBasedRules::CheckWaitingForPlayers]   --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_PREROUND]     --> RND_RUNNING
			// [CTeamplayRoundBasedRules::State_Enter_PREROUND]     --> BETWEEN_RNDS
			// [CTeamplayRoundBasedRules::State_Think_TEAM_WIN]     --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_TEAM_WIN]     --> GAME_OVER
			// =================================================================
			
			bool sketchy;
			switch (TFGameRules()->State_Get()) {
				case GR_STATE_INIT:         sketchy = true;  break;
				case GR_STATE_PREGAME:      sketchy = true;  break;
				case GR_STATE_STARTGAME:    sketchy = false; break;
				case GR_STATE_PREROUND:     sketchy = true;  break;
				case GR_STATE_RND_RUNNING:  sketchy = false; break;
				case GR_STATE_TEAM_WIN:     sketchy = true;  break;
				case GR_STATE_RESTART:      sketchy = true;  break;
				case GR_STATE_STALEMATE:    sketchy = true;  break;
				case GR_STATE_GAME_OVER:    sketchy = true;  break;
				case GR_STATE_BONUS:        sketchy = true;  break;
				case GR_STATE_BETWEEN_RNDS: sketchy = true;  break;
				default:                    sketchy = true;  break;
			}
			
			if (sketchy) {
				DevMsg("[CTFBot::Spawn] Bot [ent:%d userid:%d name:\"%s\"]: spawned during game state %s!\n",
					ENTINDEX(bot), bot->GetUserID(), bot->GetPlayerName(), GetRoundStateName(TFGameRules()->State_Get()));
				BACKTRACE();
			}
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_PlayerReadyStatus_ShouldStartCountdown)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFGameRules_PlayerReadyStatus_ShouldStartCountdown)();
		if (TFGameRules()->IsMannVsMachineMode()) {
			bool notReadyPlayerBlue = false;
			bool notReadyPlayerRed = false;
			bool hasPlayer = false;
			ForEachTFPlayer([&](CTFPlayer *player){
				if (notReadyPlayerRed || notReadyPlayerBlue) return;
				if (player->GetTeamNumber() < 2) return;
				if (player->IsBot()) return;
				if (player->IsFakeClient()) return;
				hasPlayer = true;
				if (!TFGameRules()->IsPlayerReady(ENTINDEX(player))) {
					if (player->GetTeamNumber() == TF_TEAM_BLUE)
						notReadyPlayerBlue = true;
					else
						notReadyPlayerRed = true;
					return;
				}
			});
			
			return hasPlayer && !notReadyPlayerRed && !notReadyPlayerBlue;
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFBaseBoss_OnTakeDamage, CTakeDamageInfo &info)
	{
		float damage = DETOUR_MEMBER_CALL(CTFBaseBoss_OnTakeDamage)(info);
		if (info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == reinterpret_cast<CBaseEntity *>(this)->GetTeamNumber())
			return 0;
		return damage;
	}

	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		bool result = DETOUR_MEMBER_CALL(CTFBotVision_IsIgnored)(ent);
		ConVarRef sig_mvm_teleporter_aggro("sig_mvm_teleporter_aggro");
		if (!result) {
			auto vision = reinterpret_cast<IVision *>(this);
			CTFBot *bot = static_cast<CTFBot *>(vision->GetBot()->GetEntity());
			if (bot->GetTeamNumber() == TF_TEAM_RED && (!sig_mvm_teleporter_aggro.GetBool() || rtti_cast<CTFWeaponBaseMelee *>(bot->GetActiveWeapon()) != nullptr)) {
				if (bot != nullptr && (bot->GetActiveWeapon()) != nullptr && ent->IsBaseObject()) {
					CBaseObject *obj = ToBaseObject(ent);
					if (obj != nullptr && obj->GetType() == OBJ_TELEPORTER) {
						return true;
					}
				}
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAmmo, int count, const char *name)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		bool change = player->GetTeamNumber() == TF_TEAM_BLUE && !cvar_infinite_ammo.GetBool() && !player->IsBot();

		if (change)
			player->SetTeamNumber(TF_TEAM_RED);
		DETOUR_MEMBER_CALL(CTFPlayer_RemoveAmmo)(count, name);

		if (change)
			player->SetTeamNumber(TF_TEAM_BLUE);
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SentryThink)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		bool stopammo = TFGameRules()->IsMannVsMachineMode() && !cvar_infinite_ammo.GetBool() && IsMvMBlueHuman(owner);
		if (stopammo)
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);

		DETOUR_MEMBER_CALL(CObjectSentrygun_SentryThink)();

		if (stopammo)
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_ShouldQuickBuild)
	{
		CBaseObject *obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *builder = ToTFPlayer(obj->GetBuilder());
		bool changedteam = builder != nullptr && IsMvMBlueHuman(builder);
		
		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_RED);

		bool ret = DETOUR_MEMBER_CALL(CBaseObject_ShouldQuickBuild)();

		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_BLUE);

		return ret;
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_DeterminePlaybackRate)
	{
		CBaseObject *obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *builder = ToTFPlayer(obj->GetBuilder());
		bool changedteam = !cvar_teleport_player.GetBool() && builder != nullptr && IsMvMBlueHuman(builder);
		
		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_RED);

		DETOUR_MEMBER_CALL(CObjectTeleporter_DeterminePlaybackRate)();

		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_BLUE);

	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ShouldRespawnQuickly, CTFPlayer *player)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFGameRules_ShouldRespawnQuickly)(player);
		ret |= TFGameRules()->IsMannVsMachineMode() && player->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT && !player->IsBot();
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CTFKnife_CanPerformBackstabAgainstTarget, CTFPlayer *target )
	{
		bool ret = DETOUR_MEMBER_CALL(CTFKnife_CanPerformBackstabAgainstTarget)(target);

		if ( !ret && TFGameRules() && TFGameRules()->IsMannVsMachineMode() && target->GetTeamNumber() == TF_TEAM_RED )
		{
			if ( target->m_Shared->InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
				return true;

			if ( target->m_Shared->InCond( TF_COND_SAPPED ) && !target->IsMiniBoss() )
				return true;
		}
		return ret;
	}

	void TeleportAfterSpawn(CTFPlayer *player)
	{
		float distanceToBomb = std::numeric_limits<float>::max();
		CObjectTeleporter *teleOut = nullptr;
		CCaptureZone *zone = nullptr;

		for (auto elem : ICaptureZoneAutoList::AutoList()) {
			zone = rtti_scast<CCaptureZone *>(elem);
			if (zone != nullptr)
				break;
		}
		if (zone == nullptr)
			return;
			
		CTFPlayer *playerbot = ToTFBot(player);
		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			auto tele = rtti_cast<CObjectTeleporter *>(IBaseObjectAutoList::AutoList()[i]);
			if (tele != nullptr && tele->GetTeamNumber() == player->GetTeamNumber() && tele->IsFunctional()) {
				if (((cvar_teleport_player.GetBool() && ToTFBot(tele->GetBuilder()) == nullptr) || 
						(playerbot == nullptr && (tele->GetBuilder() == nullptr || ToTFBot(tele->GetBuilder()) != nullptr))) ) {
					float dist = tele->WorldSpaceCenter().DistToSqr(zone->WorldSpaceCenter());
					if ( dist < distanceToBomb) {
						teleOut = tele;
						distanceToBomb = dist;
					}
				}
			}
		}
		if (teleOut != nullptr) {
			auto vec = teleOut->WorldSpaceCenter();
			vec.z += teleOut->CollisionProp()->OBBMaxs().z;
			bool is_space_to_spawn = IsSpaceToSpawnHere(vec);
			if (!is_space_to_spawn)
				vec.z += 50.0f;
			if (is_space_to_spawn || IsSpaceToSpawnHere(vec)){
				player->Teleport(&(vec),&(teleOut->GetAbsAngles()),&(player->GetAbsVelocity()));
				player->EmitSound("MVM.Robot_Teleporter_Deliver");
			}
		}
	}
	
	THINK_FUNC_DECL(TeleportAfterSpawnThink)
	{
		TeleportAfterSpawn(reinterpret_cast<CTFPlayer *>(this));
	}

	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(CTFGameRules_OnPlayerSpawned)(player);
		bool bluhuman = IsMvMBlueHuman(player);
		CTFPlayer *playerbot = ToTFBot(player);
		if ((bluhuman && cvar_teleport.GetBool()) || cvar_teleport_player.GetBool()) {

			if (player->IsBot()) {
				// Bots need a delay in telein in case they have reprogrammed mode on, to prevent teleports from teleporting opposite team bots
				THINK_FUNC_SET(player, TeleportAfterSpawnThink, gpGlobals->curtime + 1.0f);
			}
			else {
				TeleportAfterSpawn(player);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Touch, CBaseEntity *toucher)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_Touch)(toucher);
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->IsMiniBoss() && toucher->IsBaseObject()) {
			CBaseObject *obj = ToBaseObject(toucher);
			if (obj != nullptr && (obj->GetType() != OBJ_SENTRYGUN || obj->m_bDisposableBuilding || obj->m_bMiniBuilding)) {

				CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, 4.0f * std::max(obj->GetMaxHealth(), obj->GetHealth()), DMG_BLAST);
				obj->TakeDamage(info);
			}
		}
	}

	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		static float angpre[34];
		
		ForEachTFPlayer([](CTFPlayer *player) {
			if (IsMvMBlueHuman(player)) {
				PlayerResource()->m_iTeam.SetIndex(TF_TEAM_RED, ENTINDEX(player));
			}
		}); 
		DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
		ForEachTFPlayer([](CTFPlayer *player) {
			if (IsMvMBlueHuman(player)) {
				PlayerResource()->m_iTeam.SetIndex(TF_TEAM_BLUE, ENTINDEX(player));
			}
		}); 
	}
	
	CBaseEntity *laser_dot = nullptr;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_CreateSniperDot)
	{
		if (IsMvMBlueHuman(reinterpret_cast<CTFSniperRifle *>(this)->GetTFPlayerOwner())) {
			return;
		}
		DETOUR_MEMBER_CALL(CTFSniperRifle_CreateSniperDot)();
	}
	
	DETOUR_DECL_STATIC(CBaseEntity *, CSniperDot_Create, Vector &origin, CBaseEntity *owner, bool visibleDot)
	{
		return laser_dot = DETOUR_STATIC_CALL(CSniperDot_Create)(origin, owner, visibleDot);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAllOwnedEntitiesFromWorld, bool explode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool blueHuman = IsMvMBlueHuman(player);
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		DETOUR_MEMBER_CALL(CTFPlayer_RemoveAllOwnedEntitiesFromWorld)(explode);
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
	}
	
	struct ScoreStats
	{
		int m_iPrevDamage;
		int m_iPrevDamageAssist;
		int m_iPrevDamageBoss;
		int m_iPrevHealing;
		int m_iPrevHealingAssist;
		int m_iPrevDamageBlocked;
		int m_iPrevCurrencyCollected;
		int m_iPrevBonusPoints;

		void Reset()
		{
			m_iPrevDamage = 0;
			m_iPrevDamageAssist = 0;
			m_iPrevDamageBoss = 0;
			m_iPrevHealing = 0;
			m_iPrevHealingAssist = 0;
			m_iPrevDamageBlocked = 0;
			m_iPrevCurrencyCollected = 0;
			m_iPrevBonusPoints = 0;
		}
	};
	ScoreStats score_stats[34];

	void DisplayScoreboard(CTFPlayer *playerShow);

	THINK_FUNC_DECL(DisplayScoreboardDelay)
	{
		DisplayScoreboard(nullptr);
	}

	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr && strcmp(event->GetName(), "player_death") == 0 && (event->GetInt( "death_flags" ) & 0x0200 /*TF_DEATH_MINIBOSS*/) == 0) {
			auto player = UTIL_PlayerByUserId(event->GetInt("userid"));
			if (IsMvMBlueHuman(ToTFPlayer(player)))
				event->SetInt("death_flags", (event->GetInt( "death_flags" ) | 0x0200));
		}

		if (event != nullptr && strcmp(event->GetName(), "mvm_reset_stats") == 0) {
			for (auto &stats : score_stats) {
				stats.Reset();
			}
		}

		if (event != nullptr && strcmp(event->GetName(), "player_connect_client") == 0) {
			int index = event->GetInt("index") + 1;
			score_stats[index].Reset();
		}

		if (event != nullptr && strcmp(event->GetName(), "mvm_wave_complete") == 0) {
			CTFPlayerResource *tfres = TFPlayerResource();
			if (tfres != nullptr) {
				for (uint index = 0; index < ARRAYSIZE(score_stats); index++) {
					score_stats[index].m_iPrevDamage += tfres->m_iDamage[index];
					score_stats[index].m_iPrevDamageAssist += tfres->m_iDamageAssist[index];
					score_stats[index].m_iPrevDamageBoss += tfres->m_iDamageBoss[index];
					score_stats[index].m_iPrevDamageBlocked += tfres->m_iDamageBlocked[index];
					score_stats[index].m_iPrevHealing += tfres->m_iHealing[index];
					score_stats[index].m_iPrevHealingAssist += tfres->m_iHealingAssist[index];
					score_stats[index].m_iPrevCurrencyCollected += tfres->m_iCurrencyCollected[index];
					score_stats[index].m_iPrevBonusPoints += tfres->m_iBonusPoints[index];
				}
				PrintToChatAll("To see the scoreboard again, type !scoreboard\n");
				THINK_FUNC_SET(tfres, DisplayScoreboardDelay, gpGlobals->curtime + 1.1f);
			}
		}
		
		return DETOUR_MEMBER_CALL(IGameEventManager2_FireEvent)(event, bDontBroadcast);
	}

	RefCount rc_CTFBot_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		DETOUR_MEMBER_CALL(CTFBot_Event_Killed)(info);
		
		extern ConVar cvar_force;
		if (cvar_force.GetBool() && bot->GetTeamNumber() == TF_TEAM_RED) {
			bool foundMoreSpies = false;
			ForEachTFBot([&](CTFBot *bot2) {
				if (bot2->GetTeamNumber() == TF_TEAM_RED && bot2->IsAlive() && bot2->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY) {
					foundMoreSpies = true;
					return false;
				}
				return true;
			});
			if (!foundMoreSpies) {
				IGameEvent *event = gameeventmanager->CreateEvent("mvm_mission_update");
				if ( event )
				{
					event->SetInt("class", TF_CLASS_SPY);
					event->SetInt("count", 0);
					gameeventmanager->FireEvent(event);
				}
			}
		}
	}

	class ScoreboardHandler : public IMenuHandler
    {
    public:
        ScoreboardHandler() : IMenuHandler() {
		}

		virtual void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item) {
        }

		virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
		}
		
        virtual void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }
    };
	
	void DisplayScoreboard(CTFPlayer *playerShow)
	{
		ScoreboardHandler *handler = new ScoreboardHandler();
		static IBaseMenu *menu = nullptr;
		if (menu != nullptr) {
			menu->Destroy();
		}
        menu = menus->GetDefaultStyle()->CreateMenu(handler);
        
        menu->SetDefaultTitle("Name                        |Score|Damage|Tank|Healing|Support|Cash");
        menu->SetMenuOptionFlags(MENUFLAG_BUTTON_EXIT);

		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->IsBot() || player->GetTeamNumber() != TF_TEAM_BLUE) return;
			int index = ENTINDEX(player);
			char buf[256];
			CTFPlayerResource *tfres = TFPlayerResource();
			int support = tfres->m_iDamageAssist[index] + score_stats[index].m_iPrevDamageAssist + tfres->m_iHealingAssist[index] + score_stats[index].m_iPrevHealingAssist + tfres->m_iDamageBlocked[index] + score_stats[index].m_iPrevDamageBlocked + ((tfres->m_iBonusPoints[index] + score_stats[index].m_iPrevBonusPoints) * 25);

			snprintf(buf, sizeof(buf), "%24.24s|%6d|%6d|%6d|%6d|%6d|%4d", player->GetPlayerName(), tfres->m_iTotalScore[index], tfres->m_iDamage[index] + score_stats[index].m_iPrevDamage, tfres->m_iDamageBoss[index] + score_stats[index].m_iPrevDamageBoss, tfres->m_iHealing[index] + score_stats[index].m_iPrevHealing, support, tfres->m_iCurrencyCollected[index] + score_stats[index].m_iPrevCurrencyCollected);
			ItemDrawInfo info1(buf, ITEMDRAW_DISABLED);
			menu->AppendItem("", info1);
		});
		
		if (playerShow != nullptr) {
        	menu->Display(ENTINDEX(playerShow), 60);
		}
		else {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (player->IsBot()) return;
        		menu->Display(ENTINDEX(player), 60);
			});
		}
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player != nullptr) {
			if (FStrEq(args[0], "sig_scoreboard")) {
				DisplayScoreboard(player);
				return true;
			}
			/*else if (FStrEq(args[0], "upgrade")) {
				if (IsMvMBlueHuman(player) && IsInBlueSpawnRoom(player)) {
					int cannotUpgrade = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(player,cannotUpgrade,cannot_upgrade);
					if (cannotUpgrade) {
						gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "The Upgrade Station is disabled!");
					}
					else
						player->m_Shared->m_bInUpgradeZone = true;
				}
				
				return true;
			}*/
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}
	
	DETOUR_DECL_STATIC(void, Host_Say, edict_t *edict, const CCommand& args, bool team )
	{
		CBaseEntity *entity = GetContainingEntity(edict);
		if (edict != nullptr) {
			const char *p = args.ArgS();
			int len = strlen(p);
			if (*p == '"')
			{
				p++;
				len -=2;
			}
			if (strncmp(p, "!scoreboard",len) == 0 || strncmp(p, "/scoreboard",len) == 0) {
				DisplayScoreboard(ToTFPlayer(entity));
				return;
			}
		}
		DETOUR_STATIC_CALL(Host_Say)(edict, args, team);
	}

	DETOUR_DECL_MEMBER(bool, CVoteController_IsValidVoter, CBasePlayer *player)
	{
		bool blueHuman = IsMvMBlueHuman(ToTFPlayer(player));
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		
		bool ret = DETOUR_MEMBER_CALL(CVoteController_IsValidVoter)(player);

		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CMissionPopulator_UpdateMissionDestroySentries, CBasePlayer *player)
	{
		std::vector<CBaseObject *> sentriesToRestoreTeam;

		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
			if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == TF_TEAM_BLUE && IsMvMBlueHuman(obj->GetBuilder())) {
				sentriesToRestoreTeam.push_back(obj);
				obj->SetTeamNumber(TF_TEAM_RED);
			}
			if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == TF_TEAM_RED && obj->GetBuilder() != nullptr && obj->GetBuilder()->IsBot()) {
				sentriesToRestoreTeam.push_back(obj);
				obj->SetTeamNumber(TF_TEAM_BLUE);
			}
		}
		auto ret = DETOUR_MEMBER_CALL(CMissionPopulator_UpdateMissionDestroySentries)(player);

		for (auto obj : sentriesToRestoreTeam) {
			obj->SetTeamNumber(obj->GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED);
		}
		return ret;
	}
	
	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("MvM:JoinTeam_Blue_Allow")
		{
			/* change the team assignment rules */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetTeamAssignmentOverride, "CTFGameRules::GetTeamAssignmentOverride");
			
			/* don't drop reanimators for blue humans */
			MOD_ADD_DETOUR_STATIC(CTFReviveMarker_Create, "CTFReviveMarker::Create");
			
			/* enable upgrading in blue spawn zones via "upgrade" client command */
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand,                "CTFPlayer::ClientCommand");
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnNavAreaChanged,             "CTFPlayer::OnNavAreaChanged");
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientCommandKeyValues,    "CTFGameRules::ClientCommandKeyValues");
			
			/* allow flag pickup and capture depending on convar values */
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToPickUpFlag, "CTFPlayer::IsAllowedToPickUpFlag");
			MOD_ADD_DETOUR_MEMBER(CCaptureZone_ShimTouch,          "CCaptureZone::ShimTouch");
			
			/* disallow attacking while in the blue spawn room */
		//	MOD_ADD_DETOUR_MEMBER(CPlayerMove_StartCommand, "CPlayerMove::StartCommand");
			
			/* fix hardcoded teamnum check when forcing bots to move to team spec at round change */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent, "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_STATIC(CollectPlayers_CTFPlayer,   "CollectPlayers<CTFPlayer>");
			
			// Allow blue players to ready up
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerReadyStatus_ShouldStartCountdown,   "CTFGameRules::PlayerReadyStatus_ShouldStartCountdown");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBoss_OnTakeDamage,   "CTFBaseBoss::OnTakeDamage");
			// Ignore blue teleporters
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");

			// Disable infinite ammo for players unless toggled
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAmmo, "CTFPlayer::RemoveAmmo");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SentryThink,                  "CObjectSentrygun::SentryThink");

			// Stop blue player teleporters from spinning when disabled
			MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_DeterminePlaybackRate, "CObjectTeleporter::DeterminePlaybackRate");

			// Enable fast redeploy for engineers
			MOD_ADD_DETOUR_MEMBER(CBaseObject_ShouldQuickBuild, "CBaseObject::ShouldQuickBuild");

			// Allow scouts to spawn faster
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ShouldRespawnQuickly, "CTFGameRules::ShouldRespawnQuickly");

			// Red sapped robots can be backstabbed from any range
			MOD_ADD_DETOUR_MEMBER(CTFKnife_CanPerformBackstabAgainstTarget, "CTFKnife::CanPerformBackstabAgainstTarget");

			// Stop blue team sniper dot
			// MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_CreateSniperDot, "CTFSniperRifle::CreateSniperDot");
			//MOD_ADD_DETOUR_STATIC(CSniperDot_Create, "CSniperDot::Create");

			// Forcibly remove blue human buildings when changing classes
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAllOwnedEntitiesFromWorld, "CTFPlayer::RemoveAllOwnedEntitiesFromWorld");

			// Display blue player death notice
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");

			// Show red team spy death message
			//MOD_ADD_DETOUR_MEMBER(CTFBot_Event_Killed, "IGameEventManager2::FireEvent");

			// Display blue player scoreboard
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");
			MOD_ADD_DETOUR_STATIC(Host_Say, "Host_Say");

			// Make voting work properly for blue players
			MOD_ADD_DETOUR_MEMBER(CVoteController_IsValidVoter, "CVoteController::IsValidVoter");

			// Allow sentry busters to hunt blue sentries
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_UpdateMissionDestroySentries, "CMissionPopulator::UpdateMissionDestroySentries");

			/* fix hardcoded teamnum check when clearing MvM checkpoints */
			this->AddPatch(new CPatch_CollectPlayers_Caller2<0x0000, 0x0100, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::ClearCheckpoint"));
			
			/* fix hardcoded teamnum check when restoring MvM checkpoints */
			this->AddPatch(new CPatch_CollectPlayers_Caller1<0x0000, 0x0200, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::RestoreCheckpoint"));
			
			/* fix hardcoded teamnum check when restoring player currency */
			this->AddPatch(new CPatch_CollectPlayers_Caller3<0x0000, 0x0100, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::RestorePlayerCurrency"));
			
			/* fix hardcoded teamnum check when respawning dead players and resetting their sentry stats at wave end */
			this->AddPatch(new CPatch_CollectPlayers_Caller1<0x0000, 0x0400, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue_NotBot>("CWave::WaveCompleteUpdate"));

			// Show only spy bots on the enemy team
			// this->AddPatch(new CPatch_CollectPlayers_Caller1<0x0500, 0x0930, TF_TEAM_BLUE, true, false, CollectPlayers_RedAndBlue>("CTFBot::Event_Killed"));
			
			/* fix hardcoded teamnum checks in the radius spy scan ability */
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_RadiusSpyScan, "CTFPlayerShared::RadiusSpyScan");
			this->AddPatch(new CPatch_CollectPlayers_Caller1<0x0000, 0x0100, TF_TEAM_BLUE, true, false, CollectPlayers_RadiusSpyScan>("CTFPlayerShared::RadiusSpyScan"));
			
			this->AddPatch(new CPatch_RadiusSpyScan());
			
			/* this is purely for debugging the blue-robots-spawning-between-waves situation */
			MOD_ADD_DETOUR_MEMBER(CTFBot_Spawn, "CTFBot::Spawn");
			// Teleport to bot logic
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned,                  "CTFGameRules::OnPlayerSpawned");

			// Player minigiant stomp logic
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Touch,                  "CTFPlayer::Touch");
			
			// Allow blue players to appear on the scoreboard
			// MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks,                  "SV_ComputeClientPacks");
			
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void OnDisable() override
		{
			if (TFGameRules()->IsMannVsMachineMode()) {
				ForEachTFPlayer([](CTFPlayer *player){
					if (player->GetTeamNumber() != TF_TEAM_BLUE) return;
					if (player->IsBot())                         return;
					player->ForceChangeTeam(TF_TEAM_RED, true);
				});
			}
		}

		virtual void FrameUpdatePostEntityThink() override
		{
			if (TFGameRules()->IsMannVsMachineMode()) {
				ForEachTFPlayer([](CTFPlayer *player){
					if (player->GetTeamNumber() != TF_TEAM_BLUE) return;
					if (player->IsBot())                         return;
					
					if (gpGlobals->tickcount % 3 == 1 && cvar_spawn_protection.GetBool() ) {
						if (PointInRespawnRoom(player, player->WorldSpaceCenter(), true)) {
							player->m_Shared->AddCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED, 0.500f);
							
							if (cvar_spawn_no_shoot.GetBool()) {
								player->m_Shared->m_flStealthNoAttackExpire = gpGlobals->curtime + 0.500f;
								// alternative method: set m_Shared->m_bFeignDeathReady to true
							}
						}
						else if (player->m_Shared->m_flStealthNoAttackExpire <= gpGlobals->curtime){
							player->m_Shared->RemoveCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED);
						}

					}

					// Add self blast penalty for flag bearing players
					if (!player->IsMiniBoss() && player->GetActiveTFWeapon() != nullptr) {
						static ConVarRef intel_speed("tf_mvm_bot_flag_carrier_movement_penalty");
						CEconItemAttributeDefinition *attr_def = nullptr;
						if (attr_def == nullptr)
							attr_def = GetItemSchema()->GetAttributeDefinitionByName("self dmg push force decreased");
						
						auto attr = player->GetActiveTFWeapon()->GetItem()->GetAttributeList().GetAttributeByID(attr_def->GetIndex());
						float desiredSpeed = (intel_speed.GetFloat() + 1.0f) * 0.5f;
						if (attr == nullptr && player->HasItem())
							player->GetActiveTFWeapon()->GetItem()->GetAttributeList().SetRuntimeAttributeValue(attr_def, desiredSpeed);
						else if (attr != nullptr && !player->HasItem() && attr->GetValuePtr()->m_Float == desiredSpeed)
							player->GetActiveTFWeapon()->GetItem()->GetAttributeList().RemoveAttribute(attr_def);
					}

					if (cvar_infinite_cloak.GetBool()) {
						bool should_refill_cloak = true;
						
						if (!cvar_infinite_cloak_deadringer.GetBool()) {
							/* check for attribute "set cloak is feign death" */
							auto invis = rtti_cast<CTFWeaponInvis *>(player->Weapon_GetWeaponByType(TF_WPN_TYPE_ITEM1));
							if (invis != nullptr && CAttributeManager::AttribHookValue<int>(0, "set_weapon_mode", invis) == 1) {
								should_refill_cloak = false;
							}
						}
						
						if (should_refill_cloak) {
							player->m_Shared->m_flCloakMeter = 100.0f;
						}
					}

					if (gpGlobals->tickcount % 5 == 0) {
						hudtextparms_t textparam;
						textparam.channel = 1;
						textparam.x = 0.042f;
						textparam.y = 0.93f;
						textparam.effect = 0;
						textparam.r1 = 255;
						textparam.r2 = 255;
						textparam.b1 = 255;
						textparam.b2 = 255;
						textparam.g1 = 255;
						textparam.g2 = 255;
						textparam.a1 = 0;
						textparam.a2 = 0; 
						textparam.fadeinTime = 0.f;
						textparam.fadeoutTime = 0.f;
						textparam.holdTime = 0.5f;
						textparam.fxTime = 1.0f;
						UTIL_HudMessage(player, textparam, CFmtStr("$%d", player->GetCurrency()));
					}
				});
			}
		}
	};
	CMod s_Mod;
	
	
	/* by way of incredibly annoying persistent requests from Hell-met,
	 * I've acquiesced and made this mod convar non-notifying (sigh) */
	ConVar cvar_enable("sig_mvm_jointeam_blue_allow", "0", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Mod: permit client command 'jointeam blue' from human players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	/* default: admin-only mode ENABLED */
	ConVar cvar_adminonly("sig_mvm_jointeam_blue_allow_adminonly", "1", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Mod: restrict this mod's functionality to SM admins only"
		" [NOTE: missions with WaveSchedule param AllowJoinTeamBlue 1 will OVERRIDE this and allow non-admins for the duration of the mission]");

	// force blue team joining, also loads the mod if not loaded
	ConVar cvar_force("sig_mvm_jointeam_blue_allow_force", "0", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Mod: force players to join blue team",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			if (static_cast<ConVar *>(pConVar)->GetBool())
				s_Mod.Toggle(true);
			else
				s_Mod.Toggle(cvar_enable.GetBool());
		});
}
