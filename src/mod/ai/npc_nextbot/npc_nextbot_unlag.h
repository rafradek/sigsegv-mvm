
	
namespace Mod::AI::NPC_Nextbot
{
	
	#define MAX_LAYER_RECORDS 15

	struct LayerRecord
	{
		int m_sequence;
		float m_cycle;
		float m_weight;
		int m_order;

		LayerRecord()
		{
			m_sequence = 0;
			m_cycle = 0;
			m_weight = 0;
			m_order = 0;
		}

		LayerRecord( const LayerRecord& src )
		{
			m_sequence = src.m_sequence;
			m_cycle = src.m_cycle;
			m_weight = src.m_weight;
			m_order = src.m_order;
		}
	};

	struct LagRecord
	{
	public:
		LagRecord()
		{
			m_fFlags = 0;
			m_vecOrigin.Init();
			m_vecAngles.Init();
			m_vecMinsPreScaled.Init();
			m_vecMaxsPreScaled.Init();
			m_flSimulationTime = -1;
			m_masterSequence = 0;
			m_masterCycle = 0;
		}

		LagRecord( const LagRecord& src )
		{
			m_fFlags = src.m_fFlags;
			m_vecOrigin = src.m_vecOrigin;
			m_vecAngles = src.m_vecAngles;
			m_vecMinsPreScaled = src.m_vecMinsPreScaled;
			m_vecMaxsPreScaled = src.m_vecMaxsPreScaled;
			m_flSimulationTime = src.m_flSimulationTime;
			for( int layerIndex = 0; layerIndex < MAX_LAYER_RECORDS; ++layerIndex )
			{
				m_layerRecords[layerIndex] = src.m_layerRecords[layerIndex];
			}
			m_masterSequence = src.m_masterSequence;
			m_masterCycle = src.m_masterCycle;
		}

		// Did player die this frame
		int						m_fFlags;

		// Player position, orientation and bbox
		Vector					m_vecOrigin;
		QAngle					m_vecAngles;
		Vector					m_vecMinsPreScaled;
		Vector					m_vecMaxsPreScaled;

		float					m_flSimulationTime;	
		
		// Player animation details, so we can get the legs in the right spot.
		LayerRecord				m_layerRecords[MAX_LAYER_RECORDS];
		int						m_masterSequence;
		float					m_masterCycle;
	};

	class LagCompensatedEntity : public AutoList<LagCompensatedEntity>
	{
	public:
		LagCompensatedEntity(CBaseAnimatingOverlay *entity) { this->entity = entity; }
		virtual bool WantsLagCompensation(CBasePlayer *player, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits);

		CBaseAnimatingOverlay *entity;
		CUtlFixedLinkedList< LagRecord > track;
		bool restore;
		LagRecord restoreData;
		LagRecord changeData;

	};
}