#include "mod.h"
#include "stub/baseentity.h"
#include "stub/baseplayer.h"
#include "stub/extraentitydata.h"
#include "stub/server.h"
#include "stub/sendprop.h"
#include "util/prop_helper.h"
#include "mod/etc/sendprop_override.h"


namespace Mod::Etc::SendProp_Override
{

    static int propOverrideId;

    bool entityHasOverride[MAX_EDICTS] {};

	SendpropOverrideModule::~SendpropOverrideModule() {
        entityHasOverride[entity->entindex()] = false;
    }

    int SendpropOverrideModule::AddOverride(SendPropOverrideCallback callback, const std::string &name, int index, uintptr_t data) {
        SendProp *prop = nullptr;
        return this->AddOverride(callback, FindSendPropPrecalcIndex(entity->GetServerClass()->m_pTable, name, index, prop), prop, data);
    }

    int SendpropOverrideModule::AddOverride(SendPropOverrideCallback callback, int indexProp, SendProp *prop, uintptr_t data) {
        if (indexProp != -1) {
            if (prop == nullptr) {
                prop = (SendProp *) entity->GetServerClass()->m_pTable->m_pPrecalc->m_Props[indexProp];
            }
            auto insertPos = propOverrides.end();
            for (auto it = propOverrides.begin(); it != propOverrides.end(); it++) {
                if (it->propIndex > indexProp) insertPos = it;
            }
            PropOverride overr {++propOverrideId, callback, prop, indexProp, data};
            propOverrides.insert(insertPos, overr);

            entityHasOverride[entity->entindex()] = true;
            return propOverrideId;
        }
        return -1;
    }

    void SendpropOverrideModule::RemoveOverride(int id) {
        
        for (auto it = propOverrides.begin(); it != propOverrides.end(); it++) {
            auto &overr = *it;
            if (overr.id == id) {
                overr.stopped = true;
                break;
            }
        }
    }

	thread_local int client_num = 0;
    edict_t *world_edict;

	void WriteOverride(const SendTable *pTable,
	const void *pState,
	const int nBits,
	bf_write *pOut,
	const int objectID,
	int *pCheckProps,
	int nCheckProps)
	{
		CSendTablePrecalc *pPrecalc = pTable->m_pPrecalc;
		CDeltaBitsWriter deltaBitsWriter( pOut );

		bf_read inputBuffer( "SendTable_WritePropList->inputBuffer", pState, BitByte( nBits ), nBits );
		CDeltaBitsReader inputBitsReader( &inputBuffer );
		// Ok, they want to specify a small list of properties to check.
		unsigned int iToProp = inputBitsReader.ReadNextPropIndex();
		int i = 0;
        auto entity = GetContainingEntity(world_edict+objectID);
        auto mod = entity->GetEntityModule<SendpropOverrideModule>("sendpropoverride");
        int nextOverridePropIndex = MAX_DATATABLE_PROPS;
        std::vector<PropOverride>::iterator propIterator = mod->propOverrides.begin();
        while (propIterator != mod->propOverrides.end()) {
            if (propIterator->stopped) propIterator++;
            else {
                nextOverridePropIndex = propIterator->propIndex;
                propIterator++;
                break;
            }
        }

		while ( i < nCheckProps )
		{
			// Seek the 'to' state to the current property we want to check.
			while ( iToProp < (unsigned int) pCheckProps[i] )
			{
				inputBitsReader.SkipPropData( pPrecalc->m_Props[iToProp] );
				iToProp = inputBitsReader.ReadNextPropIndex();
			}

			if ( iToProp >= MAX_DATATABLE_PROPS )
			{
				break;
			}
			
			if ( iToProp == (unsigned int) pCheckProps[i] )
			{
				const SendProp *pProp = pPrecalc->m_Props[iToProp];

				deltaBitsWriter.WritePropIndex( iToProp );
				
				if (iToProp == (unsigned int) nextOverridePropIndex) {
                    
                        DecodeInfo decodeInfo;
                        decodeInfo.m_pRecvProp = nullptr; // Just skip the data if the proxies are screwed.
                        decodeInfo.m_pProp = pProp;
                        decodeInfo.m_pIn = &inputBuffer;
                        decodeInfo.m_ObjectID = objectID;
                        decodeInfo.m_Value.m_Type = (SendPropType)pProp->m_Type;
                        g_PropTypeFns[pProp->GetType()].Decode(&decodeInfo);
                        
                        while (iToProp == (unsigned int) nextOverridePropIndex) {
                            PropOverride &rride = *(propIterator-1);
                            auto callback = rride.callback;

                            (* callback)(entity, client_num, decodeInfo.m_Value, rride.id, rride.prop, rride.data);
                            
                            while (propIterator != mod->propOverrides.end()) {
                                if (propIterator->stopped) propIterator++;
                                else {
                                    nextOverridePropIndex = propIterator->propIndex;
                                    propIterator++;
                                    break;
                                }
                            }
                            if (propIterator == mod->propOverrides.end()) break;
                        }
                        g_PropTypeFns[ pProp->GetType() ].Encode((const unsigned char *)entity, &decodeInfo.m_Value, pProp, pOut, objectID);
				}
				else {
					inputBitsReader.CopyPropData( deltaBitsWriter.GetBitBuf(), pProp ); 
				}

				// Seek to the next prop.
				iToProp = inputBitsReader.ReadNextPropIndex();
			}

			++i;
		}

		inputBitsReader.ForceFinished(); // avoid a benign assert
	}
	
	DETOUR_DECL_STATIC(void, SendTable_WritePropList,
	const SendTable *pTable,
	const void *pState,
	const int nBits,
	bf_write *pOut,
	const int objectID,
	int *pCheckProps,
	int nCheckProps
	)
    {
        if (objectID >= 0 && objectID < 2048 && entityHasOverride[objectID]) {
            WriteOverride(pTable, pState, nBits, pOut, objectID, pCheckProps, nCheckProps);
			return;
        }
		DETOUR_STATIC_CALL(pTable, pState, nBits, pOut, objectID, pCheckProps, nCheckProps);
    }
    
    int CheckOverridePropIndex(int *pDeltaProps, int nDeltaProps, const int objectID)
	{
        auto mod = GetContainingEntity(world_edict+objectID)->GetEntityModule<SendpropOverrideModule>("sendpropoverride");
        for (auto it = mod->propOverrides.begin(); it != mod->propOverrides.end(); it++) {
            auto &overr = *it;
            bool added = false;
            if (it+1 != mod->propOverrides.end() && (it+1)->propIndex == overr.propIndex) continue;

            for (int i = 0; i < nDeltaProps; i++) {
                auto num = pDeltaProps[i];
                if (num == overr.propIndex) { 
                    added = true;
                    break;
                }
                if (num > overr.propIndex) { 
                    added = true;
                    nDeltaProps++;
                    for (int j = nDeltaProps - 2; j >= i; j--) {
                        pDeltaProps[j+1] = pDeltaProps[j];
                    }
                    pDeltaProps[i] = overr.propIndex;
                    break;
                }
            }
            if (!added) {
                pDeltaProps[nDeltaProps++] = overr.propIndex;
            }
        }

        return nDeltaProps;
    }
    
    DETOUR_DECL_STATIC(int, SendTable_CalcDelta,
	const SendTable *pTable,
	
	const void *pFromState,
	const int nFromBits,
	
	const void *pToState,
	const int nToBits,
	
	int *pDeltaProps,
	int nMaxDeltaProps,

	const int objectID
	)
    {
		int count = DETOUR_STATIC_CALL(pTable, pFromState, nFromBits, pToState, nToBits, pDeltaProps, nMaxDeltaProps, objectID);
        if (client_num != 0 && objectID >= 0 && objectID < 2048 && entityHasOverride[objectID]) {
            return CheckOverridePropIndex(pDeltaProps, count, objectID);
        }
        return count;
    }
    
    void OnPack(CFrameSnapshot *snapshot) {

        // for (auto mod : AutoList<SendpropOverrideModule>::List()) {
        //     if (!mod->propOverrides.empty()) {
        //         auto pack = g_FrameSnapshotManager.GetRef().GetPackedEntity(snapshot, mod->entity->entindex());
        //         if (pack != nullptr && pack->m_pChangeFrameList != nullptr) {
        //             int changedProps[MAX_DATATABLE_PROPS];
        //             int nChangedProps = pack->m_pChangeFrameList->GetPropsChangedAfterTick(snapshot->m_nTickCount-1, changedProps, MAX_DATATABLE_PROPS);
        //             Msg("ChangedProps %d\n", nChangedProps);
        //             for (auto it = mod->propOverrides.begin(); it != mod->propOverrides.end(); it++) {
        //                 auto &overr = *it;
        //                 if (it+1 != mod->propOverrides.end() && (it+1)->propIndex == overr.propIndex) continue;
        //                 changedProps[nChangedProps++] = overr.propIndex;
        //             }
        //             pack->m_pChangeFrameList->SetChangeTick(changedProps, nChangedProps, snapshot->m_nTickCount);
        //         }
        //     }
        // }
    }

	DETOUR_DECL_MEMBER(void, CBaseServer_WriteDeltaEntities, CBaseClient *client, CClientFrame *to, CClientFrame *from, bf_write &pBuf )
    {
		client_num = client->m_nEntityIndex;
        
        DETOUR_MEMBER_CALL(client, to, from, pBuf);
        client_num = 0;
    }

    class CEntityWriteInfo {
    public:
        virtual	~CEntityWriteInfo() {};
	
        bool			m_bAsDelta;
        CClientFrame	*m_pFrom;
        CClientFrame	*m_pTo;

        int		m_UpdateType;

        int				m_nOldEntity;	// current entity index in m_pFrom
        int				m_nNewEntity;	// current entity index in m_pTo

        int				m_nHeaderBase;
        int				m_nHeaderCount;
        bf_write		*m_pBuf;
        int				m_nClientEntity;

        PackedEntity	*m_pOldPack;
        PackedEntity	*m_pNewPack;
    };

    thread_local PackedEntity *preOldPack = nullptr;
    thread_local CEntityWriteInfo *writeInfo = nullptr;
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, void, SV_DetermineUpdateType, CEntityWriteInfo *u )
    {
        bool hasOverride = entityHasOverride[u->m_nNewEntity];
        if (hasOverride) {
            preOldPack = u->m_pOldPack;
            writeInfo = u;
            u->m_pOldPack = nullptr;
        }
        DETOUR_STATIC_CALL(u);
        if (hasOverride) {
            u->m_pOldPack = preOldPack;
            writeInfo = nullptr;
        }
    }

	DETOUR_DECL_MEMBER(int, PackedEntity_GetPropsChangedAfterTick, int tick, int *iOutProps, int nMaxOutProps)
    {
        auto pack = reinterpret_cast<PackedEntity *>(this);
        auto result = DETOUR_MEMBER_CALL(tick, iOutProps, nMaxOutProps);
        if (entityHasOverride[pack->m_nEntityIndex] && writeInfo != nullptr) {
            writeInfo->m_pOldPack = preOldPack;
            return CheckOverridePropIndex(iOutProps, result, pack->m_nEntityIndex);
        }
        return result;
    }

    DETOUR_DECL_MEMBER(void, CGameServer_SendClientMessages, bool sendSnapshots)
	{
		DETOUR_MEMBER_CALL(sendSnapshots);
        for (auto mod : AutoList<SendpropOverrideModule>::List()) {
            for (auto it = mod->propOverrides.begin(); it != mod->propOverrides.end();) {
                auto &overr = *it;
                if (overr.stopped) {
                    it = mod->propOverrides.erase(it);
                }
                else {
                    it++;
                }
            }

            if (mod->propOverrides.empty()) {
                entityHasOverride[mod->entity->entindex()] = false;
            }
        }
	}

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Etc:SendProp_Override")
		{
			
			MOD_ADD_DETOUR_STATIC_PRIORITY(SendTable_WritePropList,"SendTable_WritePropList", HIGH);
			MOD_ADD_DETOUR_MEMBER(CBaseServer_WriteDeltaEntities,  "CBaseServer::WriteDeltaEntities");
			MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,             "SendTable_CalcDelta");
			MOD_ADD_DETOUR_STATIC(SV_DetermineUpdateType,             "SV_DetermineUpdateType");
			MOD_ADD_DETOUR_MEMBER(PackedEntity_GetPropsChangedAfterTick,             "PackedEntity::GetPropsChangedAfterTick");
			MOD_ADD_DETOUR_MEMBER(CGameServer_SendClientMessages,             "CGameServer::SendClientMessages");
        }

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void LevelInitPostEntity() override
		{
            world_edict = INDEXENT(0);
        }

        virtual void OnEnablePost() override
		{
            world_edict = INDEXENT(0);
        }
    };
    
	CMod s_Mod;
}