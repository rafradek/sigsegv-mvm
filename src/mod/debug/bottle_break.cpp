#include "mod.h"
#include "stub/tfweaponbase.h"
#include "stub/tfplayer.h"
#include "util/iterate.h"


//////
int	studiohdr_t::GetNumAttachments( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalattachments;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_attachment.Count();
}

const mstudioattachment_t &studiohdr_t::pAttachment( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalAttachment( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalAttachment( pVModel->m_attachment[i].index );
}

virtualmodel_t *studiohdr_t::GetVirtualModel( void ) const
{
	if ( numincludemodels == 0 )
		return NULL;
	return modelinfo->GetVirtualModel( this );
}

const studiohdr_t *virtualgroup_t::GetStudioHdr( ) const
{
	return modelinfo->FindModel( this->cache );
}
//////


namespace Mod::Debug::Bottle_Break
{
	////////////////////////////////////////////////////////////////////////////
	// BEGIN STUFF FROM game/shared/animation.cpp //////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	
//	void SetBodygroup( CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue )
//	{
//		if (! pstudiohdr)
//			return;
//
//		if (iGroup >= pstudiohdr->numbodyparts())
//			return;
//
//		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
//
//		if (iValue >= pbodypart->nummodels)
//			return;
//
//		int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;
//
//		body = (body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
//	}


	int GetBodygroup( CStudioHdr *pstudiohdr, int body, int iGroup )
	{
		if (! pstudiohdr)
			return 0;

		if (iGroup >= pstudiohdr->numbodyparts())
			return 0;

		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );

		if (pbodypart->nummodels <= 1)
			return 0;

		int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;

		return iCurrent;
	}

	const char *GetBodygroupName( CStudioHdr *pstudiohdr, int iGroup )
	{
		if ( !pstudiohdr)
			return "";

		if (iGroup >= pstudiohdr->numbodyparts())
			return "";

		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
		return pbodypart->pszName();
	}

	int FindBodygroupByName( CStudioHdr *pstudiohdr, const char *name )
	{
		if ( !pstudiohdr || !pstudiohdr->IsValid() )
			return -1;

		int group;
		for ( group = 0; group < pstudiohdr->numbodyparts(); group++ )
		{
			mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( group );
			if ( !Q_strcasecmp( name, pbodypart->pszName() ) )
			{
				return group;
			}
		}

		return -1;
	}

	int GetBodygroupCount( CStudioHdr *pstudiohdr, int iGroup )
	{
		if ( !pstudiohdr )
			return 0;

		if (iGroup >= pstudiohdr->numbodyparts())
			return 0;

		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
		return pbodypart->nummodels;
	}

	int GetNumBodyGroups( CStudioHdr *pstudiohdr )
	{
		if ( !pstudiohdr )
			return 0;

		return pstudiohdr->numbodyparts();
	}
	
	////////////////////////////////////////////////////////////////////////////
	// END STUFF FROM game/shared/animation.cpp ////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	
	
	void TweakItemDef(const char *item_name)
	{
		auto pItemDef = GetItemSchema()->GetItemDefinitionByName(item_name);
		if (pItemDef != nullptr) {
			auto pszModelPlayer = reinterpret_cast<const char **>((uintptr_t)pItemDef + 0x70);
			auto bAttachToHands = reinterpret_cast<bool *>       ((uintptr_t)pItemDef + 0x90);
			
			DevMsg("[%s] model_player: was \"%s\"\n", item_name, *pszModelPlayer);
			*pszModelPlayer = strdup("models/weapons/w_models/w_bottle.mdl");
			DevMsg("[%s] model_player: now \"%s\"\n", item_name, *pszModelPlayer);
			
			DevMsg("[%s] attach_to_hands: was %s\n", item_name, (*bAttachToHands ? "true" : "false"));
			*bAttachToHands = false;
			DevMsg("[%s] attach_to_hands: now %s\n", item_name, (*bAttachToHands ? "true" : "false"));
		} else {
			DevMsg("Failed to find item definition for %s\n", item_name);
		}
	}
	
	CON_COMMAND(sig_debug_bottle_break_tweak, "")
	{
		CBaseEntity::PrecacheModel("models/weapons/w_models/w_bottle.mdl");
		
		TweakItemDef(            "TF_WEAPON_BOTTLE");
		TweakItemDef("Upgradeable TF_WEAPON_BOTTLE");
	}
	
	
	CON_COMMAND(sig_debug_bottle_break_tweak2, "")
	{
		CBaseEntity::PrecacheModel("models/weapons/v_models/v_bottle.mdl");
		CBaseEntity::PrecacheModel("models/weapons/w_models/w_bottle.mdl");
		CBaseEntity::PrecacheModel("models/weapons/c_models/c_bottle/c_bottle.mdl");
		CBaseEntity::PrecacheModel("models/weapons/c_models/c_bottle/c_bottle_broken.mdl");
		
		// detour CTFBottle::GetViewModel, then call CBaseCombatWeapon::SetViewModel
		
	}
	
	
	CON_COMMAND(sig_debug_bottle_break_find, "")
	{
		ForEachEntity([](CBaseEntity *pEnt){
			int idx = pEnt->GetModelIndex();
			const model_t *mod = modelinfo->GetModel(idx);
			const char *name = modelinfo->GetModelName(mod);
			
			DevMsg("#%-4d %-40s %s\n", ENTINDEX(pEnt), pEnt->GetClassname(), name);
			
			if (mod == nullptr) return;
			auto studio = const_cast<studiohdr_t *>(modelinfo->GetStudiomodel(mod));
			if (studio == nullptr) return;
			
			for (int i = 0; i < studio->GetNumAttachments(); ++i) {
				DevMsg("- attachment %d: \"%s\"\n", i, studio->pAttachment(i).pszName());
			}
		});
	}
	
	
//	DETOUR_DECL_MEMBER(bool, CEconItemDefinition_BInitFromKV, KeyValues *kv, CUtlVector<CUtlString> *errors)
//	{
//		auto result = DETOUR_MEMBER_CALL(CEconItemDefinition_BInitFromKV)(kv, errors);
//		
//		if (result) {
//			short index = reinterpret_cast<CEconItemDefinition *>(this)->m_iItemDefIndex;
//			if (index == 1 || index == 191) {
//				auto pszModelPlayer = reinterpret_cast<const char **>((uintptr_t)this + 0x70);
//				auto bAttachToHands = reinterpret_cast<bool *>       ((uintptr_t)this + 0x90);
//				auto pszName        = reinterpret_cast<const char **>((uintptr_t)this + 0xd8);
//				
//				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[%s] model_player: was \"%s\"\n", *pszName, *pszModelPlayer);
//				*pszModelPlayer = strdup("models/weapons/w_models/w_bottle.mdl");
//				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[%s] model_player: now \"%s\"\n", *pszName, *pszModelPlayer);
//				
//				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[%s] attach_to_hands: was %s\n", *pszName, (*bAttachToHands ? "true" : "false"));
//				*bAttachToHands = false;
//				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[%s] attach_to_hands: now %s\n", *pszName, (*bAttachToHands ? "true" : "false"));
//			}
//		}
//		
//		return result;
//	}
	
	
	void ScreenText(int line, const char *str)
	{
		float x = RemapValClamped(100,               0, 1920, 0.00f, 1.00f);
		float y = RemapValClamped(100 + (25 * line), 0, 1080, 0.00f, 1.00f);
		
		NDebugOverlay::ScreenText(x, y, str, 0xff, 0xff, 0xff, 0xff, 1.00f);
	}
	
	
//	DETOUR_DECL_STATIC(bool, CheckValveSignature, const void *p1, unsigned int i1, const void *p2, unsigned int i2)
//	{
//		g_bLastSignatureCheck = true;
//		return true;
//	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Bottle_Break")
		{
		//	MOD_ADD_DETOUR_MEMBER(CEconItemDefinition_BInitFromKV, "CEconItemDefinition::BInitFromKV");
		//	MOD_ADD_DETOUR_STATIC(CheckValveSignature, "CheckValveSignature");
		}
		
	//	#warning REMOVE AUTOMATIC BOTTLE SHIT IN Debug:Bottle_Break mod!!!
	//	virtual bool OnLoad() override
	//	{
	//		CBaseEntity::PrecacheModel("models/weapons/w_models/w_bottle.mdl");
	//		this->Enable();
	//		
	//		return true;
	//	}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			NDebugOverlay::Clear();
			
			int line = 0;
			
		//	std::set<int> models;
			
			ForEachEntityByClassname("tf_weapon_bottle", [&](CBaseEntity *pEnt){
				auto pBottle = static_cast<CTFBottle *>(pEnt);
				
				ScreenText(line++, CFmtStr("CTFBottle, ent #%d @ 0x%08X:", ENTINDEX(pBottle), (uintptr_t)pBottle));
				ScreenText(line++, CFmtStr("- m_nBody: %d", (int)pBottle->m_nBody));
				ScreenText(line++, CFmtStr("- bool @ +0x6b6: %s", (*(bool *)((uintptr_t)pBottle + 0x6b6) ? "true" : "false")));
				ScreenText(line++, CFmtStr("- m_bBroken: %s", (pBottle->m_bBroken ? "true" : "false")));
				ScreenText(line++, CFmtStr("- GetWorldModel(): \"%s\"", pBottle->GetWorldModel()));
				ScreenText(line++, CFmtStr("- GetViewModel(0): \"%s\"", pBottle->GetViewModel(0)));
				ScreenText(line++, CFmtStr("- GetViewModel(1): \"%s\"", pBottle->GetViewModel(1)));
				ScreenText(line++, CFmtStr("- model: #%d (\"%s\")", pBottle->GetModelIndex(), modelinfo->GetModelName(pBottle->GetModel())));
				
			//	models.insert(pBottle->GetModelIndex());
				
				int iBGTotal = pBottle->GetNumBodyGroups();
				for (int iGroup = 0; iGroup < iBGTotal; ++iGroup) {
					std::string str = CFmtStr("  - bodygroup #%d (\"%s\"):", iGroup, pBottle->GetBodygroupName(iGroup)).Get();
					
					int iCurrent = pBottle->GetBodygroup(iGroup);
					
					int iBGCount = pBottle->GetBodygroupCount(iGroup);
					for (int iValue = 0; iValue < iBGCount; ++iValue) {
						if (iValue == iCurrent) {
							str += CFmtStr(" [%d]", iValue).Get();
						} else {
							str += CFmtStr("  %d ", iValue).Get();
						}
					}
					
					ScreenText(line++, str.c_str());
				}
				
				++line;
			});
			
			ForEachTFPlayer([&](CTFPlayer *pTFPlayer){
				if (!pTFPlayer->IsAlive())                    return;
				if (pTFPlayer->GetTeamNumber() < TF_TEAM_RED) return;
				
				ScreenText(line++, CFmtStr("CTFPlayer, ent #%d @ 0x%08X (\"%s\"):", ENTINDEX(pTFPlayer), (uintptr_t)pTFPlayer, pTFPlayer->GetPlayerName()));
				
				for (int iVMIndex = 0; iVMIndex < MAX_VIEWMODELS; ++iVMIndex) {
					CBaseViewModel *pVM = pTFPlayer->GetViewModel(iVMIndex);
					if (pVM != nullptr) {
						ScreenText(line++, CFmtStr("- pTFPlayer->GetViewModel(%d): ent #%d \"%s\"", iVMIndex, ENTINDEX(pVM), pVM->GetClassname()));
						ScreenText(line++, CFmtStr("  - m_nBody: %d", (int)pVM->m_nBody));
						ScreenText(line++, CFmtStr("  - model: #%d (\"%s\")", pVM->GetModelIndex(), modelinfo->GetModelName(pVM->GetModel())));
						
					//	models.insert(pVM->GetModelIndex());
				
						int iBGTotal = pVM->GetNumBodyGroups();
						for (int iGroup = 0; iGroup < iBGTotal; ++iGroup) {
							std::string str = CFmtStr("    - bodygroup #%d (\"%s\"):", iGroup, pVM->GetBodygroupName(iGroup)).Get();
							
							int iCurrent = pVM->GetBodygroup(iGroup);
							
							int iBGCount = pVM->GetBodygroupCount(iGroup);
							for (int iValue = 0; iValue < iBGCount; ++iValue) {
								if (iValue == iCurrent) {
									str += CFmtStr(" [%d]", iValue).Get();
								} else {
									str += CFmtStr("  %d ", iValue).Get();
								}
							}
							
							ScreenText(line++, str.c_str());
						}
					} else {
						ScreenText(line++, CFmtStr("- pTFPlayer->GetViewModel(%d): nullptr", iVMIndex));
					}
				}
				
				++line;
			});
			
		//	for (int iModelIndex : models) {
		//		const model_t *pModel         = modelinfo->GetModel(iModelIndex);
		//		const studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel(pModel);
		//		
		//		ScreenText(line++, CFmtStr("Model #%d: \"%s\"", iModelIndex, modelinfo->GetModelName(pModel)));
		//		
		//		int iBGTotal = GetNumBodyGroups(pStudioHdr);
		//		for (int iGroup = 0; iGroup < iBGTotal; ++iGroup) {
		//			std::string str = CFmtStr("- bodygroup #%d (\"%s\"):", iGroup, GetBodygroupName(pStudioHdr, iGroup)).Get();
		//			
		//			int iValueCurrent = GetBodygroup(pStudioHdr, iGroup);
		//			
		//			int iBGCount = GetBodygroupCount(pStudioHdr, iGroup);
		//			for (int iValue = 0; iValue < iBGCount; ++iValue) {
		//				if (iValue == iValueCurrent) {
		//					str += CFmtStr(" [%d]", iValue).Get();
		//				} else {
		//					str += CFmtStr("  %d ", iValue).Get();
		//				}
		//			}
		//			
		//			ScreenText(line++, str.c_str());
		//		}
		//		
		//		++line;
		//	}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_bottle_break", "0", FCVAR_NOTIFY,
		"Debug: investigate tf_weapon_bottle bodygroup issues caused by c_models conversion",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	// item schema diff across LAW update for tf_weapon_bottle:
	// - "model_player"	"models/weapons/w_models/w_bottle.mdl"
	// + "model_player"	"models/weapons/c_models/c_bottle/c_bottle.mdl"
	// + "attach_to_hands" "1"
	
	// offsets of interest:
	// CEconItemDefinition+0x70 char* model_player
	// CEconItemDefinition+0x90 bool  attach_to_hands
	
	// functions of interest:
	// CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName(const char *name)
	
	// econ items of interest:
	// 1   "TF_WEAPON_BOTTLE"
	// 191 "Upgradeable TF_WEAPON_BOTTLE"
	
/*
Bad pstudiohdr in GetSequenceLinearMotion()!
FRM  ESP       EIP       FUNC
  1  ff9dfba0  e777c2d9  ???+0x0
  2  ff9e0bc0  f0de016d  GetSequenceLinearMotion(CStudioHdr*, int, float const*, Vector*)+0x8d
  3  ff9e0c20  f117a32b  CBaseAnimating::GetSequenceMoveDist(CStudioHdr*, int)+0x2b
  4  ff9e0c60  f117a48d  CBaseAnimating::GetSequenceGroundSpeed(CStudioHdr*, int)+0x4d
  5  ff9e0cb0  f117dd93  CBaseAnimating::ResetSequenceInfo()+0x83
  6  ff9e0d20  f0deb24e  CBaseCombatWeapon::SetIdealActivity(Activity)+0xee
  7  ff9e0d60  f0ffca01  CTFWeaponBase::WeaponIdle()+0xb1
  8  ff9e0dd0  f10019d4  CTFWeaponBase::ItemPostFrame()+0xc4
  9  ff9e0e00  f0dfbadf  CBasePlayer::ItemPostFrame()+0x2cf
 10  ff9e0e80  f13174b6  CBasePlayer::PostThink()+0x426
 11  ff9e0f80  f1576334  CTFPlayer::PostThink()+0x14
 12  ff9e1010  f1324c50  CPlayerMove::RunPostThink(CBasePlayer*)+0xc0
 13  ff9e1080  f1325616  CPlayerMove::RunCommand(CBasePlayer*, CUserCmd*, IMoveHelper*)+0x8e6
 14  ff9e1140  f13058bc  CBasePlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)+0x9c
 15  ff9e1160  f1553855  CTFPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)+0x125
 16  ff9e11d0  f131eb37  CBasePlayer::PhysicsSimulate()+0x607
 17  ff9e12a0  f12eebbb  Physics_SimulateEntity(CBaseEntity*)+0x45b
 18  ff9e1330  f12ef07a  Physics_RunThinkFunctions(bool)+0x22a
 19  ff9e13d0  f1222941  CServerGameDLL::GameFrame(bool)+0x161
 20  ff9e1450  eadff9aa  __SourceHook_FHCls_IServerGameDLLGameFramefalse::Func(bool)+0x17a
 21  ff9e14a0  f5e14520  SV_Think(bool)+0x1e0
 22  ff9e1530  f5e15749  SV_Frame(bool)+0x179
 23  ff9e15a0  f5d87d07  _Host_RunFrame_Server(bool)+0x177
 24  ff9e1630  f5d88a19  _Host_RunFrame(float)+0x449
 25  ff9e1730  f5d96671  CHostState::State_Run(float)+0xe1
 26  ff9e1750  f5d968be  CHostState::FrameUpdate(float)+0x9e
 27  ff9e1780  f5d969c9  HostState_Frame(float)+0x19
 28  ff9e17a0  f5e2cb1e  CEngine::Frame()+0x38e
 29  ff9e1870  f5e29b26  CDedicatedServerAPI::RunFrame()+0x26
 30  ff9e1890  f6bf8fb2  RunServer()+0x42
 31  ff9e18b0  f5e29c1d  CModAppSystemGroup::Main()+0x9d
 32  ff9e18d0  f5e6d9c0  CAppSystemGroup::Run()+0x30
 33  ff9e1900  f5e2a91f  CDedicatedServerAPI::ModInit(ModInfo_t&)+0x1df
 34  ff9e1930  f6bf8d13  CDedicatedAppSystemGroup::Main()+0x93
 35  ff9e1970  f6c1c010  CAppSystemGroup::Run()+0x30
 36  ff9e19a0  f6c1c010  CAppSystemGroup::Run()+0x30
 37  ff9e19d0  f6bbe546  main+0x1c6
 38  ff9e1cd0  080489b9  main+0x239
 39  ff9e4d30  f7340186  __libc_start_main+0xf6
 40  ff9e4da0  08048b51  _start+0x21
*/
}
