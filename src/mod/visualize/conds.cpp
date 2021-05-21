#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include "util/iterate.h"


namespace Mod::Visualize::Conds
{
	void DrawPlayerConds()
	{
		ForEachTFPlayer([](CTFPlayer *player){
			if (player->GetTeamNumber() < FIRST_GAME_TEAM) return;
			if (!player->IsAlive())                        return;
			
			if (player->IsBot()) return;
			
			CBitVec<192> bits;
			player->m_Shared->GetConditionsBits(bits);
			
			int line = 0;
			
			CFmtStr header("%-4s %-40s  %8s  %8s", "NUM", "CONDITION", "DURATION", "PROVIDER");
			NDebugOverlay::EntityText(ENTINDEX(player), line++, header.Get(), 0.032f, 0xff, 0xff, 0xff, 0xff);
			
			for (int bit = bits.FindNextSetBit(0); bit != -1; bit = bits.FindNextSetBit(bit)) {
				auto cond = (ETFCond)bit;
				
				CFmtStr str_num     ("#%d",   bit);
				CFmtStr str_duration("% .2f", player->m_Shared->GetConditionDuration(cond));
				CFmtStr str_provider("#%d",   ENTINDEX(player->m_Shared->GetConditionProvider(cond)));
				
				CFmtStr str("%4s %-40s  %8s  %8s", str_num.Get(), GetTFConditionName(cond), str_duration.Get(), str_provider.Get());
				NDebugOverlay::EntityText(ENTINDEX(player), line++, str, 0.032f, 0xe0, 0xe0, 0xe0, 0xff);
			}
		});
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Conds") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 2 != 0) return;
			
			DrawPlayerConds();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_conds", "0", FCVAR_NOTIFY,
		"Visualization: draw information about active player conditions",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#if 0

ETFCond g_aDebuffConditions[] = {
	TF_COND_BURNING,
	TF_COND_URINE,
	TF_COND_BLEEDING,
	TF_COND_MAD_MILK,
};

g_MedigunEffects: medigun_charge_types => ETFCond(s)
	0 => TF_COND_INVULNERABLE, TF_COND_INVULNERABLE_WEARINGOFF
	1 => TF_COND_CRITBOOSTED
	2 => TF_COND_MEGAHEAL
	3 => TF_COND_MEDIGUN_UBER_BULLET_RESIST
	4 => TF_COND_MEDIGUN_UBER_BLAST_RESIST
	5 => TF_COND_MEDIGUN_UBER_FIRE_RESIST

UnknownStructType g_MedigunResistConditions[] = {
	{ 0, TF_COND_MEDIGUN_SMALL_BULLET_RESIST, TF_COND_MEDIGUN_UBER_BULLET_RESIST },
	{ 1, TF_COND_MEDIGUN_SMALL_BLAST_RESIST,  TF_COND_MEDIGUN_UBER_BLAST_RESIST  },
	{ 2, TF_COND_MEDIGUN_SMALL_FIRE_RESIST,   TF_COND_MEDIGUN_UBER_FIRE_RESIST   },
};

#endif


namespace Mod::Visualize::Conds_Client
{
	void DrawPlayerConds()
	{
		debugoverlay->ClearAllOverlays();
		
		// offsets only guaranteed valid for ClientWin 20161102a
		constexpr ptrdiff_t OFF_CTFPlayer_m_Shared               = 0x17f0;
		constexpr ptrdiff_t OFF_CTFPlayerShared_m_nPlayerCond    = 0x00cc;
		constexpr ptrdiff_t OFF_CTFPlayerShared_m_nPlayerCondEx  = 0x00d0;
		constexpr ptrdiff_t OFF_CTFPlayerShared_m_nPlayerCondEx2 = 0x00d4;
		constexpr ptrdiff_t OFF_CTFPlayerShared_m_nPlayerCondEx3 = 0x00d8;
		constexpr ptrdiff_t OFF_CBaseEntity_m_iHealth            = 0x00a8;
		
		for (EntitySearchResult cl_ent = clienttools->FirstEntity(); cl_ent != nullptr; cl_ent = clienttools->NextEntity(cl_ent)) {
			if (!clienttools->IsPlayer(cl_ent)) continue;
			
			auto m_nPlayerCond    = reinterpret_cast<const uint32_t *>((uintptr_t)cl_ent + OFF_CTFPlayer_m_Shared + OFF_CTFPlayerShared_m_nPlayerCond);
			auto m_nPlayerCondEx  = reinterpret_cast<const uint32_t *>((uintptr_t)cl_ent + OFF_CTFPlayer_m_Shared + OFF_CTFPlayerShared_m_nPlayerCondEx);
			auto m_nPlayerCondEx2 = reinterpret_cast<const uint32_t *>((uintptr_t)cl_ent + OFF_CTFPlayer_m_Shared + OFF_CTFPlayerShared_m_nPlayerCondEx2);
			auto m_nPlayerCondEx3 = reinterpret_cast<const uint32_t *>((uintptr_t)cl_ent + OFF_CTFPlayer_m_Shared + OFF_CTFPlayerShared_m_nPlayerCondEx3);
			
			std::vector<int> conds;
			for (int i = 0; i < 32; ++i) {
				if (((*m_nPlayerCond >> i) & 1) != 0) conds.push_back(i + 0);
			}
			for (int i = 0; i < 32; ++i) {
				if (((*m_nPlayerCondEx >> i) & 1) != 0) conds.push_back(i + 32);
			}
			for (int i = 0; i < 32; ++i) {
				if (((*m_nPlayerCondEx2 >> i) & 1) != 0) conds.push_back(i + 64);
			}
			for (int i = 0; i < 32; ++i) {
				if (((*m_nPlayerCondEx3 >> i) & 1) != 0) conds.push_back(i + 96);
			}
			
			char buf[0x1000];
			V_strcpy_safe(buf, "Conds:");
			for (int cond : conds) {
				V_strcat_safe(buf, CFmtStr(" %d", cond));
			}
			
			NDebugOverlay::EntityText(clienttools->GetEntIndex(cl_ent), -2, buf, 3600.0f);
			
			auto m_iHealth = reinterpret_cast<const int *>((uintptr_t)cl_ent + OFF_CBaseEntity_m_iHealth);
			NDebugOverlay::EntityText(clienttools->GetEntIndex(cl_ent), -1, CFmtStr("Health: %d", *m_iHealth), 3600.0f);
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Conds_Client") {}
		
		virtual bool OnLoad() override
		{
			if (clienttools  == nullptr) return false;
			if (debugoverlay == nullptr) return false;
			
			return true;
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 2 != 0) return;
			
			DrawPlayerConds();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_conds_client", "0", FCVAR_NOTIFY,
		"Visualization: player conds (crappy client-side edition)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}





	
	
	CON_COMMAND(sig_visualize_conds_client, "")
	{
		if (clienttools == nullptr) {
			Msg("Client tools interface is nullptr!\n");
			return;
		}
		
		

		
		
		// m_Shared @ +0x17f0
		// m_nPlayerCond @ +0x00cc
	}
