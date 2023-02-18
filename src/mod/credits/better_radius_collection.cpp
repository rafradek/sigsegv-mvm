#include "mod.h"
#include "stub/tfentities.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "mod/attr/custom_attributes.h"


namespace Mod::Credits::Better_Radius_Collection
{
	/* which team is allowed to collect credits? */
	int GetCreditTeamNum()
	{
		/* HACK: make behavior consistent with another mod */
		static ConVarRef sig_mvm_set_credit_team("sig_mvm_set_credit_team");
		if (sig_mvm_set_credit_team.IsValid() && sig_mvm_set_credit_team.GetBool()) {
			return sig_mvm_set_credit_team.GetInt();
		}
		
		return TF_TEAM_RED;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_RadiusCurrencyCollectionCheck)
	{
		/* don't mess with CBonusPack stuff in other game modes */
		if (!TFGameRules()->IsMannVsMachineMode()) {
			DETOUR_MEMBER_CALL(CTFPlayerShared_RadiusCurrencyCollectionCheck)();
			return;
		}
		
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		auto player = shared->GetOuter();
		
		if (player->GetTeamNumber() != GetCreditTeamNum()) return;
		if (!player->IsAlive())                            return;
		
		/* only run every 3rd tick */
		if (gpGlobals->tickcount % 3 != 0) return;

		float radius = player->IsPlayerClass(TF_CLASS_SCOUT) ? 288.0f : 72.0f;
		radius = Mod::Attr::Custom_Attributes::GetFastAttributeFloatExternal(player, radius, Mod::Attr::Custom_Attributes::MULT_CREDIT_COLLECT_RANGE);
		float radius_sqr = Square(radius);
		Vector player_pos = player->GetAbsOrigin();
		
	//	if (!ICurrencyPackAutoList::AutoList().IsEmpty()) {
	//		DevMsg("[%8.3f] ---- %4d items in ICurrencyPackAutoList ----------------------------\n", gpGlobals->curtime, ICurrencyPackAutoList::AutoList().Count());
	//		DevMsg("[%8.3f] Player: 0x%08x, idx %4d, name '%s'\n", gpGlobals->curtime, (uintptr_t)player, ENTINDEX(player), player->GetPlayerName());
	//	}
		
		for (int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i) {
			auto pack = rtti_scast<CCurrencyPack *>(ICurrencyPackAutoList::AutoList()[i]);
			if (pack == nullptr) continue;
			
			if ( pack->GetAbsOrigin().DistToSqr(player_pos) > radius_sqr) continue;
			if (!pack->AffectedByRadiusCollection())                      continue;
			if (!player->FVisible(pack, MASK_OPAQUE))                     continue;
			float time = gpGlobals->curtime - (float)(pack->m_flAnimTime);
			if (time < 0.1f) continue;
	//		DevMsg("[%8.3f] %4d: 0x%08x, idx %4d: touching!\n", gpGlobals->curtime, i, (uintptr_t)pack, ENTINDEX(pack));
			pack->Touch(player);
			
	//		continue;
	//	_continue:
	//		DevMsg("[%8.3f] %4d: 0x%08x, idx %4d: nope.\n", gpGlobals->curtime, i, (uintptr_t)pack, ENTINDEX(pack));
		}
		
	//	if (!ICurrencyPackAutoList::AutoList().IsEmpty()) {
	//		DevMsg("\n");
	//	}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Credits:Better_Radius_Collection")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_RadiusCurrencyCollectionCheck, "CTFPlayerShared::RadiusCurrencyCollectionCheck");
			
			// TODO: spawn a particle effect (dollar signs, sparkles, etc) when
			// credits are picked up, so that it's less jarring when currency
			// packs that are 288 HU away from a scout instantly disappear
			
			// (probably do this as a post-detour of CCurrencyPack::MyTouch;
			// or actually, maybe as a detour of CCurrencyPack::UpdateOnRemove;
			// look at how mvm_cash_explosion is done in that func for example)
			
			// the particle's emitter needs to have emission_duration != 0 or
			// else it'll loop forever
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_credits_better_radius_collection", "0", FCVAR_NOTIFY,
		"Mod: rewrite currency pack collection code to eliminate problematic aspects of the magnet effect",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// props potentially needed
// ========================
// - CTFPlayerShared::m_flNextRadiusCurrencyCollectionCheckTime
// - CTFPlayerShared::m_PulledCurrencyPacks
// + CCurrencyPack::m_bPulled


#if 0

void CTFPlayerShared::RadiusCurrencyCollectionCheck()
{
	CTFPlayer *pPlayer = this->GetOuter();
	
	if (pPlayer->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS && TFGameRules()->IsMannVsMachineMode()) {
		return;
	}
	
	if (!pPlayer->IsAlive()) {
		return;
	}
	
	if (this->m_flNextRadiusCurrencyCollectionCheckTime > gpGlobals->curtime) {
		return;
	}
	
	bool bScout = pPlayer->IsPlayerClass(TF_CLASS_SCOUT);
	float flRadiusSqr = (float)(bScout ? Square(288) : Square(72));
	Vector vecPlayerOrigin = pPlayer->GetAbsOrigin();
	
	/* find new currency packs that we aren't already pulling */
	for (int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i) {
		auto pCurrencyPack = static_cast<CCurrencyPack *>(ICurrencyPackAutoList::AutoList()[i]);
		
		// NOTE: bad ordering here
		if (!pCurrencyPack->AffectedByRadiusCollection())                             continue;
		if ( pCurrencyPack->GetAbsOrigin()->DistToSqr(vecPlayerOrigin) > flRadiusSqr) continue;
		if ( pCurrencyPack->IsPulled())                                               continue;
		if (!pPlayer->FVisible(pCurrencyPack, MASK_OPAQUE))                           continue;
		if (!pCurrencyPack->ValidTouch(pPlayer))                                      continue;
		
		if (bScout) {
			bool bAlreadyPulled = false;
			for (const auto& pulled : this->m_PulledCurrencyPacks) {
				CCurrencyPack *pOtherPack = pulled.hPack;
				if (pOtherPack == nullptr) continue;
				
				if (pCurrencyPack == pOtherPack) {
					bAlreadyPulled = true;
					break;
				}
			}
			
			if (!bAlreadyPulled) {
				CTFPlayerShared::pulledcurrencypacks_t pulled;
				pulled.hPack  = pCurrencyPack;
				pulled.flTime = gpGlobals->curtime + 1.0f;
				
				this->m_PulledCurrencyPacks.AddToTail(pulled);
			}
		} else {
			pCurrencyPack->Touch(pPlayer);
		}
	}
	
	/* deal with currency packs that are currently being pulled */
	for (int i = this->m_PulledCurrencyPacks.Count() - 1; i >= 0; --i) {
		auto& pulled = this->m_PulledCurrencyPacks[i];
		CCurrencyPack *pCurrencyPack = pulled.hPack;
		
		/* remove stale pulled-pack records */
		if (pCurrencyPack == nullptr) {
			// NOTE: could probably do FastRemove here without consequence
			this->m_PulledCurrencyPacks.Remove(i);
			continue;
		}
		
		/* if flTime has expired, then the credits insta-touch the scout */
		if (pulled.flTime > gpGlobals->curtime) {
			Vector2D dir_xy = (pPlayer->GetAbsOrigin() - pCurrencyPack->GetAbsOrigin()).AsVector2D();
			dir_xy.NormalizeInPlace();
			
			pCurrencyPack->RemoveFlag(FL_ONGROUND);
			
			pCurrencyPack->ApplyAbsVelocityImpulse(550.0f * Vector(Vector2DExpand(dir_xy), 0.25f));
		} else {
			pCurrencyPack->Touch(pPlayer);
		}
	}
	
	// NOTE: frankly this seems a bit too infrequent; but if the freq is cranked up,
	// then the magnitude of the velocity impulse would need to be correspondingly reduced
	this->m_flNextRadiusCurrencyCollectionCheckTime = gpGlobals->curtime + (bScout ? 0.15f : 0.25f);
}

#endif
