#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "util/clientmsg.h"
#include "util/backtrace.h"
#include "util/color.h"
#include "util/rtti.h"
#include "util/demangle.h"
#include "util/iterate.h"


namespace Mod::Debug::Sticky_Detonation_Delay
{
	DETOUR_DECL_MEMBER(bool, CTFPlayer_DoClassSpecialSkill)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL();
		
		ClientMsgAll("[%8.3f] CTFPlayer[#%d]::DoClassSpecialSkill() = %s\n",
			gpGlobals->curtime, ENTINDEX(player), (result ? "TRUE" : "FALSE"));
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFGrenadeLauncher_SecondaryAttack)
	{
		auto weapon = reinterpret_cast<CTFGrenadeLauncher *>(this);
		
		ClientMsgAll("[%8.3f] CTFGrenadeLauncher[#%d]::SecondaryAttack()\n",
			gpGlobals->curtime, ENTINDEX(weapon));
		
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CTFPipebombLauncher_SecondaryAttack)
	{
		auto weapon = reinterpret_cast<CTFPipebombLauncher *>(this);
		
		ClientMsgAll("[%8.3f] CTFPipebombLauncher[#%d]::SecondaryAttack()\n",
			gpGlobals->curtime, ENTINDEX(weapon));
		
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseMelee_SecondaryAttack)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseMelee *>(this);
		
		ClientMsgAll("[%8.3f] CTFWeaponBaseMelee[#%d]::SecondaryAttack()\n",
			gpGlobals->curtime, ENTINDEX(weapon));
		
		DETOUR_MEMBER_CALL();
	}
	
	
	// 1. log calls (with result)
	// 2. add "lie" functionality
	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanAttack, int flags)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(flags);
		
		ClientMsgAll("[%8.3f] CTFPlayer[#%d]::CanAttack(%08x) = %s\n",
			gpGlobals->curtime, ENTINDEX(player), flags, (result ? "TRUE" : "FALSE"));
		
		return result;
	}
	
	// THIS function [pGrenadeLauncher->CanPerformSecondaryAttack()] seems to not get called right after switching or
	// during reloads or right after shooting; so I suspect it only gets called if m_flNextSecondaryAttack has elapsed
	// and that is very likely relevant to the problem...
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_CanPerformSecondaryAttack)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		auto result = DETOUR_MEMBER_CALL();
		
		ClientMsgAll("[%8.3f] CTFWeaponBase[#%d '%s']::CanPerformSecondaryAttack() = %s\n",
			gpGlobals->curtime, ENTINDEX(weapon), weapon->GetClassname(), (result ? "TRUE" : "FALSE"));
		
		return result;
	}
	
	// TODO: figure out when CanPerformSecondaryAttack was added to the game
	
	// ACTUALLY: from looking at the code in SDK2013 CBaseCombatWeapon::ItemPostFrame (which is the sole caller of
	// CanPerformSecondaryAttack), I think the problem is that there are periods of time when ItemPostFrame simply isn't
	// called at all: I think ItemBusyFrame is called instead at those times...?
	
	// Yeah, I'm 99% sure that CTFWeaponBase::ItemBusyFrame (or other weapons' ItemBusyFrame's) needs to be doing the
	// check-for-altfire-and-trigger-player-special-skill logic, and currently is not
	
	
	// add a CON_COMMAND: call CanPerformSecondaryAttack on our currently active weapon and show the result
	
	
	
	
	bool        overlay_seen [3];
	std::string overlay_str1 [3];
	std::string overlay_str2 [3];
	Color32     overlay_color[3];
	
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemPostFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (host != nullptr) {
			CTFPlayer *owner = ToTFPlayer(weapon->GetOwnerEntity());
			if (owner == host) {
				for (int slot = 0; slot <= 2; ++slot) {
					if (weapon != rtti_cast<CTFWeaponBase *>(host->Weapon_GetSlot(slot))) continue;
					
					overlay_seen [slot] = true;
				//	overlay_str1 [slot] = TypeName(weapon);
					overlay_str2 [slot] = "ItemPostFrame";
				//	overlay_str1 [slot] = CFmtStr("[Slot %d] %-20s", slot, TypeName(weapon));
				//	overlay_str2 [slot] = CFmtStr("%*s%s", 30, "", "ItemPostFrame");
					overlay_color[slot] = Color32(0x00, 0xff, 0x00);
				}
			}
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemBusyFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (host != nullptr) {
			CTFPlayer *owner = ToTFPlayer(weapon->GetOwnerEntity());
			if (owner == host) {
				for (int slot = 0; slot <= 2; ++slot) {
					if (weapon != rtti_cast<CTFWeaponBase *>(host->Weapon_GetSlot(slot))) continue;
					
					overlay_seen [slot] = true;
				//	overlay_str1 [slot] = TypeName(weapon);
					overlay_str2 [slot] = "ItemBusyFrame";
				//	overlay_str1 [slot] = CFmtStr("[Slot %d] %-20s", slot, TypeName(weapon));
				//	overlay_str2 [slot] = CFmtStr("%*s%s", 30, "", "ItemBusyFrame");
					overlay_color[slot] = Color32(0xff, 0x00, 0x00);
				}
			}
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	
	#warning TODO: need a live "which buttons are currently being pressed" display (just +attack and +attack2)
	#warning TODO: should show countdown-to-m_flNextPrimaryAttack in the overlay text for active weapon
	#warning TODO: should show countdown-to-m_flNextSecondaryAttack in the overlay text for active weapon
	#warning TODO: should show countdown-to-m_flNextAttack in the overlay text for the player
	#warning TODO: also need to track down who is calling SecondaryAttack during ItemBusyFrame periods
	//             - CTFWeaponBase::ItemBusyFrame does some sort of wimpy thing
	
	#warning TODO: add a toggleable convar which will make it so that ItemBusyFrame ... does a check for IN_ATTACK2 and calls do special clas skill ... or something ... need sleep ...
	
	
	// weapon switch next-attack-times are set in these relevant places:
	// CBaseCombatWeapon::DefaultDeploy
	// CTFWeaponBase::Deploy
	
	// look into the flamethrower airblast -> switch -> switch back -> airblast delay bug too:
	// normally the cooldown for airblast is 0.75 seconds (TODO: confirm),
	// but it goes up to 1.00 seconds if you switch weapons after airblasting
	// (worse/more noticeable with Degreaser, since you have more time after switching to notice the increased cooldown)
	
	
	// 2007xxxxx: stickybomb launcher okay
	// 20140828a: stickybomb launcher bugged
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Sticky_Detonation_Delay")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoClassSpecialSkill, "CTFPlayer::DoClassSpecialSkill");
			
			MOD_ADD_DETOUR_MEMBER(CTFGrenadeLauncher_SecondaryAttack,  "CTFGrenadeLauncher::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFPipebombLauncher_SecondaryAttack, "CTFPipebombLauncher::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_SecondaryAttack,  "CTFWeaponBaseMelee::SecondaryAttack");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanAttack,                     "CTFPlayer::CanAttack");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CanPerformSecondaryAttack, "CTFWeaponBase::CanPerformSecondaryAttack");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemPostFrame, "CTFWeaponBase::ItemPostFrame");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemBusyFrame, "CTFWeaponBase::ItemBusyFrame");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			/* avoid flickering; just maintain what we had on-screen before */
			if (!overlay_seen[0] && !overlay_seen[1] && !overlay_seen[2]) return;
			
			NDebugOverlay::Clear();
			
			CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
			if (host == nullptr) return;
			
			for (int slot = 0; slot <= 2; ++slot) {
				float x = 0.05f;
				float y = 0.05f + (slot * 0.05f);
				
				auto weapon = rtti_cast<CTFWeaponBase *>(host->Weapon_GetSlot(slot));
				if (weapon != nullptr) {
					DemangleName(TypeName(weapon), overlay_str1[slot]);
				} else {
					overlay_str1[slot] = "<unknown>";
				}
				
				if (!overlay_seen[slot]) {
					overlay_str2[slot] = "<inactive>";
				}
				
				if (overlay_seen[slot]) {
					NDebugOverlay::ScreenText(x, y, CFmtStr("[Slot %d] %-20s", slot, overlay_str1[slot].c_str()), 0xff, 0xff, 0xff, 0xff,  100.0f);
					NDebugOverlay::ScreenText(x, y, CFmtStr("%*s%s", 30, "", overlay_str2[slot].c_str()), C32_EXPAND(overlay_color[slot]), 100.0f);
				} else {
					NDebugOverlay::ScreenText(x, y, CFmtStr("[Slot %d] %-20s", slot, overlay_str1[slot].c_str()), 0x80, 0x80, 0x80, 0xff,  100.0f);
					NDebugOverlay::ScreenText(x, y, CFmtStr("%*s%s", 30, "", overlay_str2[slot].c_str()),         0x80, 0x80, 0x80, 0xff,  100.0f);
				}
				
				overlay_seen[slot] = false;
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_sticky_detonation_delay", "0", FCVAR_NOTIFY,
		"Debug: figure out why stickybomb detonation doesn't work during weapon switch",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
