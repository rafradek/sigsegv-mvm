#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"


namespace Mod::Debug::Tele_AutoDetonate
{
	struct CFlaggedEntitiesEnum
	{
		void **vtable;
		CBaseEntity **m_pList;
		int m_listMax;
		int m_flagMask;
		int m_count;
	};
	
	
	// jeez, valve, learn to spell
	RefCount rc_CObjectTeleporter_RecieveTeleportingPlayer;
	DETOUR_DECL_MEMBER(void, CObjectTeleporter_RecieveTeleportingPlayer, CTFPlayer *player)
	{
		NDebugOverlay::Clear();
		
		SCOPED_INCREMENT(rc_CObjectTeleporter_RecieveTeleportingPlayer);
		DETOUR_MEMBER_CALL(player);
	}
	
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInBox, SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		DETOUR_MEMBER_CALL(listMask, mins, maxs, coarseTest, pIterator);
		
		if (rc_CObjectTeleporter_RecieveTeleportingPlayer > 0) {
			NDebugOverlay::Box(mins, vec3_origin, maxs - mins, 0xff, 0xff, 0xff, 0x10, 3600.0f);
			
			auto flagged = reinterpret_cast<CFlaggedEntitiesEnum *>(pIterator);
			
			DevMsg("Entities in box:\n");
			for (int i = 0; i < flagged->m_count; ++i) {
				DevMsg("  #%-2d \"%s\" \"%s\"\n", i, flagged->m_pList[i]->GetClassname(), STRING(flagged->m_pList[i]->GetEntityName()));
				
				if (strcmp(flagged->m_pList[i]->GetClassname(), "func_brush") == 0) {
					NDebugOverlay::EntityBounds(flagged->m_pList[i], 0xff, 0x00, 0x00, 0x10, 3600.0f);
					
					NDebugOverlay::EntityText(ENTINDEX(flagged->m_pList[i]), 0, CFmtStrN<64>("#%d", ENTINDEX(flagged->m_pList[i])), 3600.0f, 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityText(ENTINDEX(flagged->m_pList[i]), 2, flagged->m_pList[i]->GetClassname(),                3600.0f, 0xff, 0xff, 0xff, 0xff);
				}
			}
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Tele_AutoDetonate")
		{
			MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_RecieveTeleportingPlayer, "CObjectTeleporter::RecieveTeleportingPlayer");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInBox,   "ISpatialPartition::EnumerateElementsInBox");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_tele_autodetonate", "0", FCVAR_NOTIFY,
		"Debug: figure out why 2-way teles like to detonate themselves sometimes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
