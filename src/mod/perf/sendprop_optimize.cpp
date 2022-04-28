#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/tfweaponbase.h"
#include <forward_list>
#include "stub/sendprop.h"

int global_frame_list_counter = 0;
bool is_client_hltv = false;


class CachedChangeFrameList : public IChangeFrameList
{
public:

    CachedChangeFrameList()
	{

	}

	void	Init( int nProperties, int iCurTick )
	{
		m_ChangeTicks.SetSize( nProperties );
		for ( int i=0; i < nProperties; i++ )
			m_ChangeTicks[i] = iCurTick;
        
	}


public:

	virtual void	Release()
	{
        m_CopyCounter--;
        if (m_CopyCounter < 0) {
            delete this;
        }
	}

	virtual IChangeFrameList* Copy()
	{
        m_CopyCounter++;
        return this;
	}

	virtual int		GetNumProps()
	{
		return m_ChangeTicks.Count();
	}

	virtual void	SetChangeTick( const int *pPropIndices, int nPropIndices, const int iTick )
	{
        bool same = m_LastChangeTicks.size() == nPropIndices;
        m_LastChangeTicks.resize(nPropIndices);
		for ( int i=0; i < nPropIndices; i++ )
		{
            
            int prop = pPropIndices[i];
			m_ChangeTicks[ prop ] = iTick;
            
            same = same && m_LastChangeTicks[i] == prop;
            m_LastChangeTicks[i] = prop;

		}

        if (!same) {
            m_LastSameTickNum = iTick;
        }
        m_LastChangeTickNum = iTick;
        if (m_LastChangeTicks.capacity() > m_LastChangeTicks.size() * 8)
            m_LastChangeTicks.shrink_to_fit();
	}

	virtual int		GetPropsChangedAfterTick( int iTick, int *iOutProps, int nMaxOutProps )
	{
        int nOutProps = 0;
        if (iTick + 1 >= m_LastSameTickNum) {
            if (iTick >= m_LastChangeTickNum) {
                return 0;
            }

            nOutProps = m_LastChangeTicks.size();

            for ( int i=0; i < nOutProps; i++ )
            {
                iOutProps[i] = m_LastChangeTicks[i];
            }

            return nOutProps;
        }
        else {
            int c = m_ChangeTicks.Count();

            for ( int i=0; i < c; i++ )
            {
                if ( m_ChangeTicks[i] > iTick )
                {
                    iOutProps[nOutProps] = i;
                    ++nOutProps;
                }
            }
            return nOutProps;
        }
	}

// IChangeFrameList implementation.
protected:

	virtual			~CachedChangeFrameList()
	{
	}

private:
	// Change frames for each property.
	CUtlVector<int>		m_ChangeTicks;

    int m_LastChangeTickNum = 0;
    int m_LastSameTickNum = 0;
    int m_CopyCounter = 0;
	std::vector<int> m_LastChangeTicks;
};

namespace Mod::Perf::SendProp_Optimize
{
    
    SendTable *playerSendTable;
    ServerClass *playerServerClass;
    // key: prop offset, value: prop index

    struct PropIndexData
    {
        int offset = 0;
        unsigned short index1 = PROP_INDEX_INVALID;
        unsigned short index2 = PROP_INDEX_INVALID;
    };

    std::vector<PropIndexData> prop_offset_sendtable;

    // key: prop index value: write bit index
    std::vector<unsigned short> player_prop_write_offset[39];

    std::vector<int> player_prop_value_old[39];

    unsigned short *player_prop_offsets;

    // prop indexes that are stopped from being send to players
    unsigned char *player_prop_cull;

    bool *player_local_exclusive_send_proxy;

    CSendNode **player_send_nodes;

    // This can be set to force a full update
    bool force_player_update[39];

    // This is used to check if a player had a forced full update
    bool player_not_force_updated[39];

    bool firsttime = true;
    bool lastFullEncode = false;
    
    CSharedEdictChangeInfo *g_SharedEdictChangeInfo;

    int full_updates = 0;
    int last_tick = 0;

    std::unordered_map<const SendTable *, CPropMapStack *> pm_stacks;

    SendTableProxyFn datatable_sendtable_proxy;
    edict_t * world_edict;
    RefCount rc_SendTable_WriteAllDeltaProps;

    DETOUR_DECL_STATIC(int, SendTable_CalcDelta, SendTable *pTable,
	
	void *pFromState,
	const int nFromBits,

	void *pToState,
	const int nToBits,

	int *pDeltaProps,
	int nMaxDeltaProps,

	const int objectID)
	{
        if (rc_SendTable_WriteAllDeltaProps) {
            if (objectID > 0 && objectID <= gpGlobals->maxClients) {
                bf_read toBits( "SendTable_CalcDelta/toBits", pToState, BitByte(nToBits), nToBits );
                CDeltaBitsReader toBitsReader( &toBits );
                unsigned int iToProp = toBitsReader.ReadNextPropIndex();
                int lastprop = 0;
                int lastoffset = 0;
                int lastoffsetpostskip = 0;
                for ( ; ; iToProp = toBitsReader.ReadNextPropIndex())
                { 
                    int write_offset = toBits.GetNumBitsRead();
                    if (iToProp != lastprop + 1) {
                        if (iToProp == ~0u)
                            break;
                        
                        if (iToProp != 0 && iToProp != ~0u) {
                            Msg("CalcDeltaWriteAllDeltaProps %d %d %d %d %d\n", objectID, pFromState, pToState, nFromBits, nToBits);
                            Msg("Incorrect prop index read to %d %d %d %d %d\n", iToProp, lastoffset, lastoffsetpostskip, write_offset, lastprop);

                            for (int i = lastoffset; i < write_offset; i++) {
                                toBits.Seek(i);
                                Msg("%d", toBits.ReadOneBit());
                            }
                            Msg("\n");
                            assert(true);
                        }
                    }
                        
                    const SendProp *pProp = pTable->m_pPrecalc->m_Props[iToProp];

                    lastprop = iToProp;
                    lastoffset = write_offset;
                    toBitsReader.SkipPropData(pProp);
                    lastoffsetpostskip = toBits.GetNumBitsRead();
                }
            }
        }
        int result = DETOUR_STATIC_CALL(SendTable_CalcDelta)(pTable, pFromState, nFromBits, pToState, nToBits, pDeltaProps, nMaxDeltaProps, objectID);
        return result;
	}

    StaticFuncThunk<void, int , edict_t *, void *, CFrameSnapshot *> ft_SV_PackEntity("SV_PackEntity");
    StaticFuncThunk<IChangeFrameList*, int, int> ft_AllocChangeFrameList("AllocChangeFrameList");
    StaticFuncThunk<void, PackWork_t &> ft_PackWork_t_Process("PackWork_t::Process");
    StaticFuncThunk<bool, const SendTable *, const void *, bf_write *, int, CUtlMemory<CSendProxyRecipients> *, bool> ft_SendTable_Encode("SendTable_Encode");
    StaticFuncThunk<void, ServerClass *, int, const void *, int> ft_SV_EnsureInstanceBaseline("SV_EnsureInstanceBaseline");
    

    static inline void SV_PackEntity( 
        int edictIdx, 
        edict_t* edict, 
        void* pServerClass,
        CFrameSnapshot *pSnapshot )
    {
        ft_SV_PackEntity(edictIdx, edict, pServerClass, pSnapshot);
    }

    static inline void PackWork_t_Process( 
        PackWork_t &work )
    {
        ft_PackWork_t_Process(work);
    }

    static inline IChangeFrameList* AllocChangeFrameList(int props, int tick)
    {
        return ft_AllocChangeFrameList(props, tick);
    }

    static inline bool SendTable_Encode(const SendTable *table, const void *entity, bf_write *buf, int edictnum, CUtlMemory<CSendProxyRecipients> *recip, bool zeromem)
    {
        return ft_SendTable_Encode(table, entity, buf, edictnum, recip, zeromem);
    }

    static inline void SV_EnsureInstanceBaseline(ServerClass *serverClass, int edictnum, const void *data, int size)
    {
        return ft_SV_EnsureInstanceBaseline(serverClass, edictnum, data, size);
    }

    int count_tick;

    static inline bool DoEncodePlayer(edict_t *edict, int objectID, CFrameSnapshot *snapshot) {
        ServerClass *serverclass = playerServerClass;

        player_not_force_updated[objectID - 1] = false;

        SendTable *pTable = playerServerClass->m_pTable;
        if (player_prop_value_old[objectID - 1].empty()) {
            player_prop_value_old[objectID - 1].resize(prop_offset_sendtable.size());
        }

        if (player_prop_write_offset[objectID - 1].empty()) {
            player_prop_write_offset[objectID - 1].resize(pTable->m_pPrecalc->m_Props.Count() + 1);
        }
        
        int offsetcount = prop_offset_sendtable.size();
        auto old_value_data = player_prop_value_old[objectID - 1].data();
        auto write_offset_data = player_prop_write_offset[objectID - 1].data();
        //DevMsg("crash3\n");

        int propOffsets[100];
        int propChangeOffsets = 0;

        void * pStruct = edict->GetUnknown();

        // Remember last write offsets 
        // player_prop_write_offset_last[objectID - 1] = player_prop_write_offset[objectID - 1];
        CTFPlayer *player = ToTFPlayer(GetContainingEntity(edict));
        bool bot = player != nullptr && player->IsBot();
        for (int i = 0; i < offsetcount; i++) {
            PropIndexData &data = prop_offset_sendtable[i];
            int valuepre = old_value_data[i];
            int valuepost = *(int*)(pStruct + data.offset);
            if (valuepre != valuepost ) {
                if (propChangeOffsets >= 100) {
                    return false;
                }
                if (propChangeOffsets && (propOffsets[propChangeOffsets - 1] == data.index1 || propOffsets[propChangeOffsets - 1] == data.index2))
                    continue;

                old_value_data[i] = valuepost;
                if (!(bot && player_prop_cull[data.index1] < 254 && player_local_exclusive_send_proxy[player_prop_cull[data.index1]])) {
                    propOffsets[propChangeOffsets++] = data.index1;
                }

                if (data.index2 != PROP_INDEX_INVALID && !(bot && player_prop_cull[data.index2] < 254 && player_local_exclusive_send_proxy[player_prop_cull[data.index2]])) {
                    propOffsets[propChangeOffsets++] = data.index2;
                }
                //DevMsg("Detected value change prop vector %d %d %s\n", vec_prop_offsets[j], i, pPrecalc->m_Props[i]->GetName());
            }
        }

        if (!firsttime) {
            bool force_update = force_player_update[objectID - 1];
            force_player_update[objectID - 1] = false;

            int edictnum = objectID;
            edict_t *edict = INDEXENT(objectID);
            
            // Prevent bots from having punch angle set, which in turn prevents full edict changes
            //CTFPlayer *player = ToTFPlayer(GetContainingEntity(edict));
            //if (player != nullptr && player->IsBot()) {
            //    player->m_Local->m_vecPunchAngle->Init();
            //    player->m_Local->m_vecPunchAngleVel->Init();
            //}

            if ((edict->m_fStateFlags & FL_EDICT_CHANGED) && !force_update) {
                
                CFrameSnapshotManager &snapmgr = g_FrameSnapshotManager;
                PackedEntity *pPrevFrame = snapmgr.GetPreviouslySentPacket( objectID, edict->m_NetworkSerialNumber /*snapshot->m_pEntities[ edictnum ].m_nSerialNumber*/ );
                
                if (pPrevFrame != nullptr) {
                    CSendTablePrecalc *pPrecalc = pTable->m_pPrecalc;

                    //Copy previous frame data
                    const void *oldFrameData = pPrevFrame->GetData();
                    const int oldFrameDataLegth = pPrevFrame->GetNumBits();
                    
                    ALIGN4 char packedData[4096] ALIGN4_POST;
                    memcpy(packedData, oldFrameData, pPrevFrame->GetNumBytes());

                    bf_write prop_writer("packed data writer", packedData, sizeof(packedData));
                    bf_read prop_reader("packed data reader", packedData, sizeof(packedData));
                    IChangeFrameList *pChangeFrame = NULL;

                    // Insertion sort on prop indexes 
                    int i = 1;
                    while (i < propChangeOffsets) {
                        int x = propOffsets[i];
                        int j = i - 1;
                        while (j >= 0 && propOffsets[j] > x) {
                                propOffsets[j+1] = propOffsets[j];
                            j = j - 1;
                        }
                        propOffsets[j+1] = x;
                        i = i + 1;
                    }

                    int total_bit_offset_change = 0;
                    //DevMsg("offsets %d %d\n", propChangeOffsets, p->m_nChangeOffsets);

                    //unsigned char sizetestbuf[DT_MAX_STRING_BUFFERSIZE+2];
                    //bf_write sizetestbuf_write("packed data writer", sizetestbuf, 16);

                    //int bit_offsetg = player_prop_write_offset[objectID - 1][player_prop_write_offset[objectID - 1].size()-1];
                    //DevMsg("max write offset %d %d\n", bit_offsetg, propChangeOffsets);

                    PropTypeFns *encode_fun = g_PropTypeFns;
                    for (int i = 0; i < propChangeOffsets; i++) {

                        //DevMsg("prop %d %d\n", i, propOffsets[i]);
                        int bit_offset = write_offset_data[propOffsets[i]];

                        prop_writer.SeekToBit(bit_offset);
                        prop_reader.Seek(bit_offset);
                        DVariant var;
                        const SendProp *pProp = pPrecalc->m_Props[propOffsets[i]];

                        //DevMsg("max write offset %d %s %d\n", propOffsets[i], pProp->GetName(), bit_offset);


                        void *pStructBaseOffset;
                        
                        pStructBaseOffset = pStruct + player_prop_offsets[propOffsets[i]] - pProp->GetOffset();

                        var.m_Type = (SendPropType)pProp->m_Type;
                        pProp->GetProxyFn()( 
                            pProp,
                            pStructBaseOffset, 
                            pStructBaseOffset + pProp->GetOffset(), 
                            &var, 
                            0, // iElement
                            edictnum
                            );

                        //DevMsg("prop %d %d %d %s %s\n",player_prop_offsets[propOffsets[i]], propOffsets[i], pStructBaseOffset + pProp->GetOffset(),pProp->GetName(), var.ToString());
                        
                        int varlen = true;//pProp->GetFlags() & (SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL | SPROP_VARINT);

                        DecodeInfo decodeInfo;
                        decodeInfo.m_pRecvProp = nullptr; // Just skip the data if the proxies are screwed.
                        decodeInfo.m_pProp = pProp;
                        decodeInfo.m_pIn = &prop_reader;
                        decodeInfo.m_ObjectID = edictnum;
                        decodeInfo.m_Value.m_Type = (SendPropType)pProp->m_Type;
                        encode_fun[pProp->m_Type].Decode(&decodeInfo);

                        //sizetestbuf_write.SeekToBit(0);
                        encode_fun[pProp->m_Type].Encode( 
                            (const unsigned char *) pStructBaseOffset, 
                            &var, 
                            pProp, 
                            &prop_writer, 
                            edictnum
                            ); 
                    
                        //if (varlen) {
                            

                            int bit_offset_change = prop_writer.GetNumBitsWritten() - prop_reader.GetNumBitsRead();

                            // Move all bits left or right
                            if (bit_offset_change != 0) {
                                //DevMsg("offset change %s\n", pProp->GetName());
                                return false;
                                
                                /*DevMsg("offset change %s %d %d %d %d\n", pProp->GetName(), propOffsets[i], bit_offset_change, sizetestbuf_write.GetNumBitsWritten(), (prop_reader.GetNumBitsRead() - bit_offset));
                                int propcount = pPrecalc->m_Props.Count();
                                for (int j = propOffsets[i] + 1; j < propcount; j++) {
                                    player_prop_write_offset[objectID - 1][j] += bit_offset_change;
                                }
                                
                                char movedata[4096];
                                bf_read prop_reader_move("packed data reader", movedata, 4096);
                                prop_reader_move.Seek(prop_reader.GetNumBitsRead());
                                memcpy(movedata, pOut->GetData(), Bits2Bytes(pPrevFrame->GetNumBits() + total_bit_offset_change));
                                
                                pOut->WriteBits(sizetestbuf, sizetestbuf_write.GetNumBitsWritten());
                                
                                bit_offset = pOut->GetNumBitsWritten();

                                pOut->WriteBitsFromBuffer(&prop_reader_move, (pPrevFrame->GetNumBits() + total_bit_offset_change) - prop_reader.GetNumBitsRead());
                                pOut->SeekToBit(bit_offset);

                                total_bit_offset_change += bit_offset_change;*/
                                /*prop_reader.Seek(bit_offset + prop_reader.GetNumBitsRead());
                                
                                unsigned char shift[18000];
                                bf_write shift_write("packed data writer", shift, 18000);
                                shift_write.*/

                            }
                            //else {
                            //    prop_writer.WriteBits(sizetestbuf, sizetestbuf_write.GetNumBitsWritten());
                            //}
                        //}
                    }

                    if (hltv != nullptr && hltv->IsActive()) {
                        pChangeFrame = pPrevFrame->m_pChangeFrameList;
                        pChangeFrame = pChangeFrame->Copy();
                    }
                    else {
                        pChangeFrame = pPrevFrame->SnagChangeFrameList();
                    } 

                    pChangeFrame->SetChangeTick( propOffsets, propChangeOffsets, snapshot->m_nTickCount );

                    //unsigned char tempData[ sizeof( CSendProxyRecipients ) * MAX_DATATABLE_PROXIES ];
                    CUtlMemory< CSendProxyRecipients > recip(pPrevFrame->GetRecipients(), pTable->m_pPrecalc->m_nDataTableProxies );

                    {
                        PackedEntity *pPackedEntity = snapmgr.CreatePackedEntity( snapshot, edictnum );
                        pPackedEntity->m_pChangeFrameList = pChangeFrame;
                        pPackedEntity->SetServerAndClientClass( serverclass, NULL );
                        pPackedEntity->AllocAndCopyPadded( packedData, pPrevFrame->GetNumBytes() );
                        pPackedEntity->SetRecipients( recip );
                        
                    }
                    edict->m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
                    
                    player_not_force_updated[objectID - 1] = true;
                    return true;
                }
            }
        }
        return false;
    }

    DETOUR_DECL_STATIC(void, SendTable_WritePropList,
        const SendTable *pTable,
        const void *pState,
        const int nBits,
        bf_write *pOut,
        const int objectID,
        const int *pCheckProps,
        const int nCheckProps
        )
    {
        if (pTable == playerSendTable) {
            if ( nCheckProps == 0 ) {
                // Write single final zero bit, signifying that there no changed properties
                pOut->WriteOneBit( 0 );
                return;
            }
            CDeltaBitsWriter deltaBitsWriter( pOut );
	        bf_read inputBuffer( "SendTable_WritePropList->inputBuffer", pState, BitByte( nBits ), nBits );

            auto pPrecalc = pTable->m_pPrecalc;
            unsigned short *offset_data = player_prop_write_offset[objectID - 1].data();
            for (int i = 0; i < nCheckProps; i++) {
                int propid = pCheckProps[i];
                int offset = offset_data[propid];
                if (offset == 0)
                    continue;
                
                auto pProp = pPrecalc->m_Props[propid];
                
			    deltaBitsWriter.WritePropIndex(propid);

                int len;
                inputBuffer.Seek( offset );
                g_PropTypeFns[ pProp->GetType() ].SkipProp( pProp, &inputBuffer );
                len = inputBuffer.GetNumBitsRead() - offset;

                /*int j = propid+1;
                do {
                    len = offset_data[j] - offset - 7;
                    j++;
                }
                while (len < 0);*/

                inputBuffer.Seek(offset);
                pOut->WriteBitsFromBuffer(&inputBuffer, len);
            }
            //CTimeAdder timer(&timespent1);
            //DETOUR_STATIC_CALL(SendTable_WritePropList)(pTable, pState, nBits, pOut, objectID, pCheckProps, nCheckProps);
            //timer.End();
            //DevMsg("Write prop list time %d %.9f\n", gpGlobals->tickcount, timer.GetDuration().GetSeconds());
                
            return;
        }
        DETOUR_STATIC_CALL(SendTable_WritePropList)(pTable, pState, nBits, pOut, objectID, pCheckProps, nCheckProps);
    }

    DETOUR_DECL_STATIC(int, SendTable_CullPropsFromProxies,
	const SendTable *pTable,
	
	const int *pStartProps,
	int nStartProps,

	const int iClient,
	
	const CSendProxyRecipients *CullPropsFromProxies,
	const int nOldStateProxies, 
	
	const CSendProxyRecipients *pNewStateProxies,
	const int nNewStateProxies,
	
	int *pOutProps,
	int nMaxOutProps
	)
    {
        if (pTable == playerSendTable) {
            int count = 0;
            auto pPrecalc = pTable->m_pPrecalc;
            for (int i = 0; i <nStartProps; i++) {
                int prop = pStartProps[i];
                //DevMsg("prop %d %d", prop, player_prop_cull[prop]);
                int proxyindex = player_prop_cull[prop];
                //DevMsg("%s", pPrecalc->m_Props[prop]->GetName());
                if (proxyindex < 254 ) {
                    //DevMsg("node %s\n", player_send_nodes[proxyindex]->m_pTable->GetName());
                    if (pNewStateProxies[proxyindex].m_Bits.IsBitSet(iClient)) {
                        pOutProps[count++] = prop;
                    }
                }
                else {
                    //DevMsg("node none\n");
                    pOutProps[count++] = prop;
                }
            }
            /*if (pOldStateProxies)
            {
                for (int i = 0; i < nOldStateProxies; i++) {
                    if (pNewStateProxies[i].m_Bits.IsBitSet(iClient) && !pOldStateProxies[i].m_Bits.IsBitSet(iClient))
                    {
                        CSendNode *node = player_send_nodes[i];
                        int start = node->m_iFirstRecursiveProp;
                        for (int j = 0; i < node->m_nRecursiveProps; i++) {
                            pOutProps[count++] = start + j;
                        }
                    }
                }
            }*/
            //DevMsg("player %d %d %d %d\n", nNewStateProxies, pPrecalc->m_nDataTableProxies, count, nStartProps);
            return count;
            //return DETOUR_STATIC_CALL(SendTable_CullPropsFromProxies)(pTable, pStartProps, nStartProps, iClient, pOldStateProxies, nOldStateProxies, pNewStateProxies, nNewStateProxies, pOutProps, nMaxOutProps);
            
        }
        else {
            memcpy(pOutProps, pStartProps, nStartProps * sizeof(int));
            return nStartProps;
        }
        //
    }

    DETOUR_DECL_MEMBER(void, CBaseServer_WriteDeltaEntities, CBaseClient *client, CClientFrame *to, CClientFrame *from, bf_write &pBuf )
    {
        DETOUR_MEMBER_CALL(CBaseServer_WriteDeltaEntities)(client, to, from, pBuf);
        if (from != nullptr)
            DevMsg("Write delta for %d from %d to %d\n", client->IsHLTV(), from->GetSnapshot()->m_nTickCount, to->GetSnapshot()->m_nTickCount);
    }

    DETOUR_DECL_MEMBER(int, PackedEntity_GetPropsChangedAfterTick, int iTick, int *iOutProps, int nMaxOutProps )
    {
        int result = DETOUR_MEMBER_CALL(PackedEntity_GetPropsChangedAfterTick)(iTick, iOutProps, nMaxOutProps);
        return result;
    }

    ConVar cvar_threads("sig_threads_par", "1", FCVAR_NONE);
    DETOUR_DECL_MEMBER(void, CParallelProcessor_PackWork_t_Run, PackWork_t *work, long items, long maxthreads, void *pool)
	{
        /*CFastTimer timer;
        maxthreads = cvar_threads.GetInt();
        timer.Start();
        for (int i = 0; i < items; i++) {
            SV_PackEntity(work->nIdx, work->pEdict, work->pSnapshot->m_pEntities[ work->nIdx ].m_pClass, work->pSnapshot );
            work++;
        }
        timer.End();*/

        //DevMsg("hmm\n");

        //std::vector<PackWork_t> work_do;

        //CFastTimer timer;
        //timer.Start();
        int player_index_end = 0;
        int max_players = gpGlobals->maxClients;
        CFrameSnapshotManager &snapmgr = g_FrameSnapshotManager;
        for (int i = 0; i < items; i++) {
            PackWork_t *work_i = work + i;
            edict_t *edict = work_i->pEdict;
            int objectID = work_i->nIdx;
            if (objectID > max_players) {
                player_index_end = i;
                break;
            }

            if (!(edict->m_fStateFlags & FL_EDICT_CHANGED)) {
                if (snapmgr.UsePreviouslySentPacket(work_i->pSnapshot, objectID, edict->m_NetworkSerialNumber))
                    continue;
            }

            if (GetContainingEntity(edict)->IsPlayer()) {
                lastFullEncode = !DoEncodePlayer(edict, objectID, work_i->pSnapshot );
                if (!lastFullEncode)
                    continue;
            }

            edict->m_fStateFlags |= FL_EDICT_CHANGED;
            PackWork_t_Process(*work_i);

            // Update player prop write offsets
            
            PackedEntity *pPrevFrame = snapmgr.GetPreviouslySentPacket( objectID, edict->m_NetworkSerialNumber /*snapshot->m_pEntities[ edictnum ].m_nSerialNumber*/ );
            if (pPrevFrame != nullptr && GetContainingEntity(edict)->IsPlayer()) {
                SendTable *pTable = playerServerClass->m_pTable;
                if (player_prop_write_offset[objectID - 1].empty()) {
                    player_prop_write_offset[objectID - 1].resize(pTable->m_pPrecalc->m_Props.Count() + 1);
                }

                bf_read toBits( "SendTable_CalcDelta/toBits", pPrevFrame->GetData(), BitByte(pPrevFrame->GetNumBits()), pPrevFrame->GetNumBits() );
                CDeltaBitsReader toBitsReader( &toBits );

                unsigned int iToProp = toBitsReader.ReadNextPropIndex();
                int propcount = pTable->m_pPrecalc->m_Props.Count();
                unsigned short *offset_data = player_prop_write_offset[objectID - 1].data();
                
                // Required for later writeproplist
                offset_data[pTable->m_pPrecalc->m_Props.Count()] = pPrevFrame->GetNumBits() + 7;

                int lastbit = 0;
                int lastprop = 0;
                for ( ; iToProp < MAX_DATATABLE_PROPS; iToProp = toBitsReader.ReadNextPropIndex())
                { 
                    const SendProp *pProp = pTable->m_pPrecalc->m_Props[iToProp];
                    /*if (iToProp != lastprop + 1) {
                        for (int i = lastprop + 1; i < iToProp; i++) {
                            offset_data[i] = 65535;
                        }
                    }*/
                    int write_offset = toBits.GetNumBitsRead();
                    
                    //if (toBits.GetNumBitsRead() - lastbit != 7) {
                    //    write_offset |= PROP_INDEX_WRITE_LENGTH_NOT_7_BIT;
                    //}

                    offset_data[iToProp] = write_offset;

                    toBitsReader.SkipPropData(pProp);
                    lastbit = toBits.GetNumBitsRead();
                    lastprop = iToProp; 
                }
                firsttime = false;
            }
        }
        //timer.End();
        //DevMsg("Timer encode players %.9f\n", timer.GetDuration().GetSeconds());
        for (int i = player_index_end; i < items; i++) {
            PackWork_t *work_i = work + i;
            edict_t *edict = work_i->pEdict;
            int objectID = work_i->nIdx;
            if (!(edict->m_fStateFlags & FL_EDICT_CHANGED)) {
                bool send = snapmgr.UsePreviouslySentPacket(work_i->pSnapshot, objectID, edict->m_NetworkSerialNumber);
                if (send) {
                    continue;
                }
            }
            //CFastTimer timer2;
            //timer2.Start();
            edict->m_fStateFlags |= FL_EDICT_CHANGED;
            PackWork_t_Process(*work_i);
            //timer2.End();
            //DevMsg("Timer encode other %s %.9f\n", GetContainingEntity(edict)->GetClassname() ,timer2.GetDuration().GetSeconds());
        }
        //DETOUR_MEMBER_CALL(CParallelProcessor_PackWork_t_Run)(work_do.data(), work_do.size(), maxthreads, pool);
        //DevMsg("duration for %d %f\n", maxthreads, timer.GetDuration().GetSeconds());
    }

    ConVar cvar_crash("sig_crash", "0", FCVAR_NONE);

    DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CAttributeManager_ClearCache)
	{
        static int called_by_weapon = 0;
        auto mgr = reinterpret_cast<CAttributeManager *>(this);

        bool isplayer = mgr->m_hOuter != nullptr && mgr->m_hOuter->IsPlayer();
        if (isplayer && !called_by_weapon) {
            force_player_update[ENTINDEX(mgr->m_hOuter) - 1] = true;
        }
        if (!isplayer)
            called_by_weapon++;
        DETOUR_MEMBER_CALL(CAttributeManager_ClearCache)();
        if (!isplayer)
            called_by_weapon--;
    }
        
    DETOUR_DECL_MEMBER(void, CTFPlayer_AddObject, CBaseObject *object)
	{
        DETOUR_MEMBER_CALL(CTFPlayer_AddObject)(object);
        force_player_update[ENTINDEX(reinterpret_cast<CTFPlayer *>(this)) - 1] = true;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveObject, CBaseObject *object)
	{
        DETOUR_MEMBER_CALL(CTFPlayer_RemoveObject)(object);
        force_player_update[ENTINDEX(reinterpret_cast<CTFPlayer *>(this)) - 1] = true;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
        auto shared = reinterpret_cast<CTFPlayerShared *>(this);
        if (pProvider != shared->GetConditionProvider(nCond))
            force_player_update[ENTINDEX(shared->GetOuter()) - 1] = true;
            
		DETOUR_MEMBER_CALL(CTFPlayerShared_AddCond)(nCond, flDuration, pProvider);
	}

    IChangeInfoAccessor *world_accessor = nullptr;
    CEdictChangeInfo *world_change_info = nullptr;

    DETOUR_DECL_MEMBER(IChangeInfoAccessor *, CBaseEdict_GetChangeAccessor)
	{
        return world_accessor;
    }

    DETOUR_DECL_STATIC(void, PackEntities_Normal, int clientCount, CGameClient **clients, CFrameSnapshot *snapshot)
	{
        
        edict_t *worldedict = INDEXENT(0);
        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t *edict = worldedict + i;
            if (!edict->IsFree() && edict->m_fStateFlags & (FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED)) {
                CTFPlayer *ent = reinterpret_cast<CTFPlayer *>(GetContainingEntity(edict));
                bool isalive = ent->IsAlive();
                if (ent->GetFlags() & FL_FAKECLIENT && (!isalive && ent->GetDeathTime() + 1.0f < gpGlobals->curtime)) {
                    edict->m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
                }
                if (ent->GetFlags() & FL_FAKECLIENT && ((i + gpGlobals->tickcount) % 2) != 0) {
                    CBaseEntity *weapon = ent->GetActiveWeapon();
                    if (weapon != nullptr) {
                        weapon->edict()->m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
                    }
                }
            }
        }

        DETOUR_STATIC_CALL(PackEntities_Normal)(clientCount, clients, snapshot);
        g_SharedEdictChangeInfo->m_nChangeInfos = MAX_EDICT_CHANGE_INFOS;
        world_accessor->SetChangeInfoSerialNumber(0);

    }

    /*DETOUR_DECL_MEMBER(int, SendTable_WriteAllDeltaProps, int iTick, int *iOutProps, int nMaxOutProps)
	{
		int result = DETOUR_MEMBER_CALL(SendTable_WriteAllDeltaProps)(iTick, iOutProps, nMaxOutProps);
        if (result == -1)
            result = 0
        return result;
    }*/
    

    DETOUR_DECL_STATIC(int, SendTable_WriteAllDeltaProps, const SendTable *pTable,					
        const void *pFromData,
        const int	nFromDataBits,
        const void *pToData,
        const int nToDataBits,
        const int nObjectID,
        bf_write *pBufOut)
    {

        SCOPED_INCREMENT_IF(rc_SendTable_WriteAllDeltaProps, nObjectID != -1);
        //if (nObjectID != -1)
        //    DevMsg("F %s\n", pTable->GetName());
        return DETOUR_STATIC_CALL(SendTable_WriteAllDeltaProps)(pTable, pFromData, nFromDataBits, pToData, nToDataBits, nObjectID, pBufOut);
    }

    DETOUR_DECL_STATIC(IChangeFrameList*, AllocChangeFrameList, int nProperties, int iCurTick)
    {
        CachedChangeFrameList *pRet = new CachedChangeFrameList;
        pRet->Init( nProperties, iCurTick);
        return pRet;
    }

    DETOUR_DECL_STATIC(void*, SendProxy_SendPredictableId, const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID)
    {
        auto result = DETOUR_STATIC_CALL(SendProxy_SendPredictableId)( pProp, pStruct, pVarData, pRecipients, objectID);
        if (result == nullptr && objectID <= gpGlobals->maxClients && objectID != 0 ) {
            pRecipients->ClearAllRecipients();
            return (void *) pVarData;
        }
        return nullptr;
    }

    DETOUR_DECL_STATIC(void*, SendProxy_SendHealersDataTable, const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID)
    {
        auto result = DETOUR_STATIC_CALL(SendProxy_SendHealersDataTable)(pProp, pStruct, pVarData, pRecipients, objectID);
        if (result == nullptr) {
            pRecipients->ClearAllRecipients();
        }
        return (void *) pVarData;
    }

    DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		auto ret = DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
        if (ret != nullptr) {
            edict_t *edict = ret->edict();
            auto serverClass = ret->GetServerClass();
            if (edict != nullptr && serverClass != nullptr && serverClass->m_InstanceBaselineIndex == INVALID_STRING_INDEX) {
                SendTable *pSendTable = serverClass->m_pTable;

                //
                // create entity baseline
                //
                
                ALIGN4 char packedData[MAX_PACKEDENTITY_DATA] ALIGN4_POST;
                bf_write writeBuf( "SV_CreateBaseline->writeBuf", packedData, sizeof( packedData ) );


                // create basline from zero values
                if ( !SendTable_Encode(
                    pSendTable, 
                    ret, 
                    &writeBuf, 
                    ENTINDEX(ret),
                    NULL,
                    false
                    ) )
                {
                    Error("SV_CreateBaseline: SendTable_Encode returned false (ent %d).\n", ENTINDEX(ret));
                }

                // copy baseline into baseline stringtable
                SV_EnsureInstanceBaseline( serverClass, ENTINDEX(ret), packedData, writeBuf.GetNumBytesWritten() );
            }
        }
        
        return ret;
	}

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf:SendProp_Optimize")
		{
            MOD_ADD_DETOUR_MEMBER(CParallelProcessor_PackWork_t_Run,   "CParallelProcessor<PackWork_t>::Run");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
            //MOD_ADD_DETOUR_STATIC(SendTable_Encode,   "SendTable_Encode");
            MOD_ADD_DETOUR_MEMBER(CBaseEdict_GetChangeAccessor,   "CBaseEdict::GetChangeAccessor");
            MOD_ADD_DETOUR_MEMBER(CAttributeManager_ClearCache,   "CAttributeManager::ClearCache [clone]");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_AddObject,   "CTFPlayer::AddObject");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveObject,"CTFPlayer::RemoveObject");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_AddCond,"CTFPlayerShared::AddCond", LOWEST);
            MOD_ADD_DETOUR_STATIC(PackEntities_Normal,   "PackEntities_Normal");
            MOD_ADD_DETOUR_STATIC(SendTable_WritePropList,   "SendTable_WritePropList");
            MOD_ADD_DETOUR_STATIC(AllocChangeFrameList,   "AllocChangeFrameList");
            //MOD_ADD_DETOUR_MEMBER(CBaseServer_WriteDeltaEntities,   "CBaseServer::WriteDeltaEntities");
			//MOD_ADD_DETOUR_MEMBER(PackedEntity_GetPropsChangedAfterTick, "PackedEntity::GetPropsChangedAfterTick");
		    MOD_ADD_DETOUR_STATIC(SendTable_CullPropsFromProxies, "SendTable_CullPropsFromProxies");
		    //MOD_ADD_DETOUR_STATIC(SendProxy_SendPredictableId, "SendProxy_SendPredictableId");
		    MOD_ADD_DETOUR_STATIC(SendProxy_SendHealersDataTable, "SendProxy_SendHealersDataTable");
			MOD_ADD_DETOUR_STATIC(CreateEntityByName,                "CreateEntityByName");
			//MOD_ADD_DETOUR_STATIC(SendTable_WriteAllDeltaProps, "SendTable_WriteAllDeltaProps");
            
		}

        virtual void PreLoad() {
			g_SharedEdictChangeInfo = engine->GetSharedEdictChangeInfo();
            SendTable &table = DT_TFPlayer_g_SendTable;
            playerSendTable = &table;
            ServerClass &svclass = g_CTFPlayer_ClassReg;
            playerServerClass = &svclass;
		}

        void AddOffsetToList(int offset, int index) {
            int size = prop_offset_sendtable.size();
            for (int i = 0; i < size; i++) {
                if (prop_offset_sendtable[i].offset == (unsigned short) offset) {
                    prop_offset_sendtable[i].index2 = (unsigned short) index;
                    return;
                }
            }
            prop_offset_sendtable.emplace_back();
            PropIndexData &data = prop_offset_sendtable.back();
            data.offset = (unsigned short) offset;
            data.index1 = (unsigned short) index;
        };

        SendTableProxyFn local_sendtable_proxy;

        void RecurseStack(unsigned char* stack, CSendNode *node, CSendTablePrecalc *precalc)
        {
            //stack[node->m_RecursiveProxyIndex] = strcmp(node->m_pTable->GetName(), "DT_TFNonLocalPlayerExclusive") == 0;
            stack[node->m_RecursiveProxyIndex] = node->m_DataTableProxyIndex;
            if (node->m_DataTableProxyIndex < 254) {
                player_send_nodes[node->m_DataTableProxyIndex] = node;
                player_local_exclusive_send_proxy[node->m_DataTableProxyIndex] = precalc->m_DatatableProps[node->m_iDatatableProp]->GetDataTableProxyFn() == local_sendtable_proxy;
            }
            
            //Msg("data %d %d %s %d\n", node->m_RecursiveProxyIndex, stack[node->m_RecursiveProxyIndex], node->m_pTable->GetName(), node->m_nRecursiveProps);
            for (int i = 0; i < node->m_Children.Count(); i++) {
                CSendNode *child = node->m_Children[i];
                RecurseStack(stack, child, precalc);
            }
        }

        virtual bool OnLoad() {
            //DevMsg("Crash1\n");
            CStandardSendProxies* sendproxies = gamedll->GetStandardSendProxies();
            datatable_sendtable_proxy = sendproxies->m_DataTableToDataTable;
            local_sendtable_proxy = sendproxies->m_SendLocalDataTable;
            int propcount = playerSendTable->m_pPrecalc->m_Props.Count();
            //DevMsg("%s %d %d\n", pTable->GetName(), propcount, pTable->GetNumProps());
            
            CPropMapStack pmStack( playerSendTable->m_pPrecalc, sendproxies );
            player_prop_offsets = new unsigned short[propcount];
            player_prop_cull = new unsigned char[propcount];
            player_local_exclusive_send_proxy = new bool[playerSendTable->m_pPrecalc->m_nDataTableProxies];
            player_send_nodes = new CSendNode *[playerSendTable->m_pPrecalc->m_nDataTableProxies];
            pmStack.Init();

            //int reduce_coord_prop_offset = 0;

            //DevMsg("Crash2\n");
            
            unsigned char proxyStack[256];

            RecurseStack(proxyStack, &playerSendTable->m_pPrecalc->m_Root , playerSendTable->m_pPrecalc);

            void *predictable_id_func = nullptr;
            for (int i = 0; i < playerSendTable->m_pPrecalc->m_DatatableProps.Count(); i++) {
                const char *name  = playerSendTable->m_pPrecalc->m_DatatableProps[i]->GetName();
                if (strcmp(name, "predictable_id") == 0) {
                    predictable_id_func = playerSendTable->m_pPrecalc->m_DatatableProps[i]->GetDataTableProxyFn();
                }
            }

            for (int iToProp = 0; iToProp < playerSendTable->m_pPrecalc->m_Props.Count(); iToProp++)
            { 
                const SendProp *pProp = playerSendTable->m_pPrecalc->m_Props[iToProp];

                pmStack.SeekToProp( iToProp );

                
                //player_local_exclusive_send_proxy[proxyStack[playerSendTable->m_pPrecalc->m_PropProxyIndices[iToProp]]] = player_prop_cull[iToProp] < 254 && pProp->GetDataTableProxyFn() == sendproxies->m_SendLocalDataTable;
                
                //bool local2 = pProp->GetDataTableProxyFn() == sendproxies->m_SendLocalDataTable;
               // bool local = player_local_exclusive_send_proxy[player_prop_cull[iToProp]];
                //Msg("Local %s %d %d %d %d\n",pProp->GetName(), local, local2, sendproxies->m_SendLocalDataTable, pProp->GetDataTableProxyFn());

                player_prop_cull[iToProp] = proxyStack[playerSendTable->m_pPrecalc->m_PropProxyIndices[iToProp]];
                if ((int)pmStack.GetCurStructBase() != 0) {

                    int offset = pProp->GetOffset() + (int)pmStack.GetCurStructBase() - 1;
                    
                    int elementCount = 1;
                    int elementStride = 0;
                    int propIdToUse = iToProp;
                    if ( pProp->GetType() == DPT_Array )
                    {
                        offset = pProp->GetArrayProp()->GetOffset() + (int)pmStack.GetCurStructBase() - 1;
                        elementCount = pProp->m_nElements;
                        elementStride = pProp->m_ElementStride;
                        pProp = pProp->GetArrayProp();
                        propIdToUse -= iToProp - 1;
                    }

                    player_prop_offsets[propIdToUse] = offset;


                    //if (pProp->GetType() == DPT_Vector || pProp->GetType() == DPT_Vector )
                    //    propIndex |= PROP_INDEX_VECTOR_ELEM_MARKER;

                    if ( offset != 0)
                    {
                        int offset_off = offset;
                        for ( int j = 0; j < elementCount; j++ )
                        {
                            AddOffsetToList(offset_off, propIdToUse);
                            if (pProp->GetType() == DPT_Vector) {
                                AddOffsetToList(offset_off + 4, propIdToUse);
                                AddOffsetToList(offset_off + 8, propIdToUse);
                            }
                            else if (pProp->GetType() == DPT_VectorXY) {
                                AddOffsetToList(offset_off + 4, propIdToUse);
                            }
                            offset_off += elementStride;
                        }
                    }
                
                }
                else {
                    player_prop_offsets[iToProp] = -1;
                }

                //int bitsread_pre = toBits.GetNumBitsRead();

                /*if (pProp->GetFlags() & SPROP_COORD_MP)) {
                    if ((int)pmStack.GetCurStructBase() != 0) {
                        reduce_coord_prop_offset += toBits.GetNumBitsRead() - bitsread_pre;
                        player_prop_coord.push_back(iToProp);
                        Msg("bits: %d\n", toBits.GetNumBitsRead() - bitsread_pre);
                    }
                }*/
            }
            
            //Msg("End\n");

            world_edict = INDEXENT(0);
            if (world_edict != nullptr){
                world_accessor = static_cast<CVEngineServer *>(engine)->GetChangeAccessorStatic(world_edict);
                world_change_info = &g_SharedEdictChangeInfo->m_ChangeInfos[0];
            }

            CDetour *detour = new CDetour("SendProxy_SendPredictableId", predictable_id_func, GET_STATIC_CALLBACK(SendProxy_SendPredictableId), GET_STATIC_INNERPTR(SendProxy_SendPredictableId));
            this->AddDetour(detour);
            return detour->Load();
        }

        virtual void OnUnload() override
		{
	        UnloadDetours();
            CFrameSnapshotManager &snapmgr = g_FrameSnapshotManager;
            /*edict_t * world_edict = INDEXENT(0);
                if (!world_edict[i].IsFree()) {
                    PackedEntity *pPrevFrame = snapmgr.GetPreviouslySentPacket( i, world_edict[i].m_NetworkSerialNumber);
                    if (pPrevFrame != nullptr && pPrevFrame->m_pChangeFrameList != nullptr){
                        pPrevFrame->m_pChangeFrameList->Release();
                        pPrevFrame->m_pChangeFrameList = AllocChangeFrameList(pPrevFrame->m_pServerClass->m_pTable->m_pPrecalc->m_Props.Count(), gpGlobals->tickcount);
                    }
                }
            }*/
            /*auto changelist = frame_first;
            while (changelist != nullptr) {
                auto changeframenew = AllocChangeFrameList(changelist->GetNumProps(), gpGlobals->tickcount);
                memcpy(changelist, changeframenew, sizeof(CChangeFrameList));

                changelist = changelist->next;
            }*/

            for (CFrameSnapshot *snap : snapmgr.m_FrameSnapshots) {
                for (int i = 0; i < snap->m_nNumEntities; i++) {
                    CFrameSnapshotEntry *entry = snap->m_pEntities + i;
                    if (entry != nullptr){
                        
                        PackedEntity *packedEntity = reinterpret_cast< PackedEntity * >(entry->m_pPackedData);
                        if (packedEntity != nullptr && packedEntity->m_pChangeFrameList != nullptr) {
                            
                            packedEntity->m_pChangeFrameList->Release();
                            packedEntity->m_pChangeFrameList = AllocChangeFrameList(packedEntity->m_pServerClass->m_pTable->m_pPrecalc->m_Props.Count(), gpGlobals->tickcount);
                        }
                    }
                }
            }
            for (int i = 0; i < 2048; i++) {
                PackedEntity *packedEntity = reinterpret_cast< PackedEntity * >(snapmgr.m_pPackedData[i]);
                if (packedEntity != nullptr && packedEntity->m_pChangeFrameList != nullptr) {
                            
                    packedEntity->m_pChangeFrameList->Release();
                    packedEntity->m_pChangeFrameList = AllocChangeFrameList(packedEntity->m_pServerClass->m_pTable->m_pPrecalc->m_Props.Count(), gpGlobals->tickcount);
                }
            }
            
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
        
        virtual void OnEnable() override
		{
            ConVarRef sv_parallel_packentities("sv_parallel_packentities");
            ConVarRef sv_instancebaselines("sv_instancebaselines");
            sv_parallel_packentities.SetValue(true);
            //sv_instancebaselines.SetValue(false);
            ConVarRef sv_maxreplay("sv_maxreplay");
            if (sv_maxreplay.GetFloat() == 0.0f) {
                sv_maxreplay.SetValue(0.05f);
            }
            
        }

		virtual void LevelInitPostEntity() override
		{
            world_edict = INDEXENT(0);
            world_accessor = static_cast<CVEngineServer *>(engine)->GetChangeAccessorStatic(world_edict);
            world_change_info = &g_SharedEdictChangeInfo->m_ChangeInfos[0];
            
            ConVarRef sv_parallel_packentities("sv_parallel_packentities");
            ConVarRef sv_instancebaselines("sv_instancebaselines");
            sv_parallel_packentities.SetValue(true);
            //sv_instancebaselines.SetValue(false);
            ConVarRef sv_maxreplay("sv_maxreplay");
            if (sv_maxreplay.GetFloat() == 0.0f) {
                sv_maxreplay.SetValue(0.05f);
            }
		}

	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_sendprop_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve sendprop encoding performance by preventing full updates on clients",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
