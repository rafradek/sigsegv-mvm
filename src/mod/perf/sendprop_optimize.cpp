#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/tfweaponbase.h"
#include "sdk2013/mempool.h"
#include <forward_list>

#define PROP_INDEX_VECTOR_ELEM_MARKER 0x8000
#define PROP_INDEX_VECTORXY_ELEM_MARKER 0x4000

#define PROP_INDEX_INVALID 0xffff

#define	FLAG_IS_COMPRESSED	(1<<31)
#define Bits2Bytes(b) ((b+7)>>3)

#define NORMAL_FRACTIONAL_BITS		11
#define NORMAL_DENOMINATOR			( (1<<(NORMAL_FRACTIONAL_BITS)) - 1 )
#define NORMAL_RESOLUTION			(1.0/(NORMAL_DENOMINATOR))

#define PROP_INDEX_WRITE_LENGTH_NOT_7_BIT 0x8000

typedef intptr_t PackedEntityHandle_t;

class CVEngineServer : public IVEngineServer
{
public:
    IChangeInfoAccessor *GetChangeAccessorStatic(const edict_t *pEdict)                      { return ft_GetChangeAccessor(this, pEdict); }
    
private:
    static MemberFuncThunk<CVEngineServer *, IChangeInfoAccessor *, const edict_t *>              ft_GetChangeAccessor;
};

abstract_class IChangeFrameList
{
public:
    
    // Call this to delete the object.
    virtual void	Release() = 0;

    // This just returns the value you passed into AllocChangeFrameList().
    virtual int		GetNumProps() = 0;

    // Sets the change frames for the specified properties to iFrame.
    virtual void	SetChangeTick( const int *pPropIndices, int nPropIndices, const int iTick ) = 0;

    // Get a list of all properties with a change frame > iFrame.
    virtual int		GetPropsChangedAfterTick( int iTick, int *iOutProps, int nMaxOutProps ) = 0;

    virtual IChangeFrameList* Copy() = 0; // return a copy of itself


protected:
    // Use Release to delete these.
    virtual			~IChangeFrameList() {}
};	

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
        
        //for (int i = 0; i < 3; i++)
		//    m_LastChangeTicks[i].resize(nProperties);
	}


// IChangeFrameList implementation.
public:

	virtual void	Release()
	{
        m_CopyCounter--;
        //DevMsg("release counter %d\n", m_CopyCounter);
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
        //CFastTimer timer;
        //timer.Start();
        
        //for ( int i = 0; i < 4; i++) {
        //    m_LastChangeTicks[i].clear();
        //}

        //int maxPropIndex = pPropIndices[nPropIndices - 1];
        //int delta = iTick - m_LastChangeTickNum;
        /*for (int i = 3; i >= 0; i--) {
            if (i - delta >= 0)
                m_LastChangeTicks[i].assign(m_LastChangeTicks[i - delta].begin(), m_LastChangeTicks[i - delta].end());
            else
                m_LastChangeTicks[i].clear();
        }*/
        //timer.End();
        //CFastTimer timer2;
        //timer2.Start();
        bool same = m_LastChangeTicks.size() == nPropIndices;
        m_LastChangeTicks.resize(nPropIndices);
		for ( int i=0; i < nPropIndices; i++ )
		{
            
            int prop = pPropIndices[i];
			m_ChangeTicks[ prop ] = iTick;
            
            same = same && m_LastChangeTicks[i] == prop;
            m_LastChangeTicks[i] = prop;
            
            /*int c = m_LastChanges.size();
            for (int j = c - 1; j >= 0; j--) {
                if (m_LastChanges[j].first == prop) {
                    m_LastChanges.erase(m_LastChanges.begin() + j);
                    break;
                }
            }

            m_LastChanges.push_back({prop, iTick});*/
		}

        if (!same) {
            m_LastSameTickNum = iTick;
        }
        m_LastChangeTickNum = iTick;
        if (m_LastChangeTicks.capacity() > m_LastChangeTicks.size() * 8)
            m_LastChangeTicks.shrink_to_fit();

        //timer.End();
        //DevMsg("Time set %d %d %.9f\n", m_ChangeTicks.Count(), m_LastChangeTicks.size(), timer.GetDuration().GetSeconds());
	}

	virtual int		GetPropsChangedAfterTick( int iTick, int *iOutProps, int nMaxOutProps )
	{
        int nOutProps = 0;
        //int stuff = 0;
        //CFastTimer timer;
        //timer.Start();
        if (iTick + 1 >= m_LastSameTickNum) {
            if (iTick >= m_LastChangeTickNum) {
                return 0;
            }

            nOutProps = m_LastChangeTicks.size();

            //memcpy(iOutProps, vec.data(), c * sizeof(int));
            for ( int i=0; i < nOutProps; i++ )
            {
                iOutProps[i] = m_LastChangeTicks[i];
            }

            //timer.End();
            //DevMsg("Time get 2 %d %d %d %.9f\n", m_ChangeTicks.Count(),iTick, m_LastChangeTickNum, timer.GetDuration().GetSeconds());
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
            //timer.End();
            //DevMsg("Time get 3 %d %d %d %.9f\n", m_ChangeTicks.Count(),iTick, m_LastChangeTickNum, timer.GetDuration().GetSeconds());
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

class PackedEntity
{
public:
    PackedEntity();
    ~PackedEntity();

	void		SetNumBits( int nBits );
	int			GetNumBits() const;
	int			GetNumBytes() const;
    
	void*		GetData();
	void		FreeData();

    
	void				SetShouldCheckCreationTick( bool bState );
    
    IChangeFrameList* SnagChangeFrameList();

	void				SetServerAndClientClass( ServerClass *pServerClass, ClientClass *pClientClass );
	bool		AllocAndCopyPadded( const void *pData, unsigned long size );
    
		// Access the recipients array.
	const CSendProxyRecipients*	GetRecipients() const;
	int							GetNumRecipients() const;

	void				SetRecipients( const CUtlMemory<CSendProxyRecipients> &recipients );

    ServerClass *m_pServerClass;	// Valid on the server
    ClientClass	*m_pClientClass;	// Valid on the client
        
    int			m_nEntityIndex;		// Entity index.
    int			m_ReferenceCount;	// reference count;

    CUtlVector<CSendProxyRecipients>	m_Recipients;

    void				*m_pData;				// Packed data.
    int					m_nBits;				// Number of bits used to encode.
    IChangeFrameList	*m_pChangeFrameList;	// Only the most current 

    // This is the tick this PackedEntity was created on
    unsigned int		m_nSnapshotCreationTick : 31;
    unsigned int		m_nShouldCheckCreationTick : 1;
};


inline IChangeFrameList* PackedEntity::SnagChangeFrameList()
{
	IChangeFrameList *pRet = m_pChangeFrameList;
	m_pChangeFrameList = NULL;
	return pRet;
}

const CSendProxyRecipients*	PackedEntity::GetRecipients() const
{
	return m_Recipients.Base();
}


int PackedEntity::GetNumRecipients() const
{
	return m_Recipients.Count();
}


void PackedEntity::SetRecipients( const CUtlMemory<CSendProxyRecipients> &recipients )
{
	m_Recipients.CopyArray( recipients.Base(), recipients.Count() );
}


bool PackedEntity::AllocAndCopyPadded( const void *pData, unsigned long size )
{
	FreeData();
	
	unsigned long nBytes = PAD_NUMBER( size, 4 );

	// allocate the memory
	m_pData = malloc( nBytes );

	if ( !m_pData )
	{
		Assert( m_pData );
		return false;
	}
	
	Q_memcpy( m_pData, pData, size );
	SetNumBits( nBytes * 8 );
	
	return true;
}

inline void PackedEntity::FreeData()
{
	if ( m_pData )
	{
		free(m_pData);
		m_pData = NULL;
	}
}

inline void PackedEntity::SetNumBits( int nBits )
{
	Assert( !( nBits & 31 ) );
	m_nBits = nBits;
}

inline int PackedEntity::GetNumBits() const
{
	Assert( !( m_nBits & 31 ) ); 
	return m_nBits & ~(FLAG_IS_COMPRESSED); 
}

inline int PackedEntity::GetNumBytes() const
{
	return Bits2Bytes( m_nBits ); 
}

inline void* PackedEntity::GetData()
{
	return m_pData;
}

inline void PackedEntity::SetShouldCheckCreationTick( bool bState )
{
	m_nShouldCheckCreationTick = bState ? 1 : 0;
}

void PackedEntity::SetServerAndClientClass( ServerClass *pServerClass, ClientClass *pClientClass )
{
	m_pServerClass = pServerClass;
	m_pClientClass = pClientClass;
	if ( pServerClass )
	{
		Assert( pServerClass->m_pTable );
		SetShouldCheckCreationTick( pServerClass->m_pTable->HasPropsEncodedAgainstTickCount() );
	}
}

class CFrameSnapshot;

class CFrameSnapshotManager 
{
    friend class CFrameSnapshot;

public:
	CFrameSnapshotManager( void );
	virtual ~CFrameSnapshotManager( void );

public:
	virtual void			LevelChanged();
    PackedEntity* GetPreviouslySentPacket( int iEntity, int iSerialNumber ) { return ft_GetPreviouslySentPacket(this, iEntity, iSerialNumber); }
    bool UsePreviouslySentPacket( CFrameSnapshot* pSnapshot, int entity, int entSerialNumber ) { return ft_UsePreviouslySentPacket(this, pSnapshot, entity, entSerialNumber); }
    PackedEntity*	CreatePackedEntity( CFrameSnapshot* pSnapshot, int entity ) { return ft_CreatePackedEntity(this, pSnapshot, entity); }

	CUtlLinkedList<CFrameSnapshot*, unsigned short>		m_FrameSnapshots;
    CClassMemoryPool< PackedEntity >					m_PackedEntitiesPool;

	int								m_nPackedEntityCacheCounter;  // increase with every cache access
	CUtlVector<int>	m_PackedEntityCache;	// cache for uncompressed packed entities

	// The most recently sent packets for each entity
	PackedEntityHandle_t	m_pPackedData[ MAX_EDICTS ];
    
private:
    static MemberFuncThunk<CFrameSnapshotManager *, PackedEntity *, int, int>              ft_GetPreviouslySentPacket;
    static MemberFuncThunk<CFrameSnapshotManager *, bool, CFrameSnapshot*, int, int>       ft_UsePreviouslySentPacket;
    static MemberFuncThunk<CFrameSnapshotManager *, PackedEntity*, CFrameSnapshot*, int>   ft_CreatePackedEntity;
};

typedef struct
{
	void			(*Encode)( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID );
	void			(*Decode)( void *pInfo );
	int				(*CompareDeltas)( const SendProp *pProp, bf_read *p1, bf_read *p2 );
	void			(*FastCopy)( 
		const SendProp *pSendProp, 
		const RecvProp *pRecvProp, 
		const unsigned char *pSendData, 
		unsigned char *pRecvData, 
		int objectID );

	const char*		(*GetTypeNameString)();
	bool			(*IsZero)( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp );
	void			(*DecodeZero)( void *pInfo );
	bool			(*IsEncodedZero) ( const SendProp *pProp, bf_read *p );
	void			(*SkipProp) ( const SendProp *pProp, bf_read *p );
} PropTypeFns;

MemberFuncThunk<CVEngineServer *, IChangeInfoAccessor *, const edict_t *> CVEngineServer::ft_GetChangeAccessor("CVEngineServer::GetChangeAccessor");
MemberFuncThunk<CFrameSnapshotManager *, PackedEntity *, int, int> CFrameSnapshotManager::ft_GetPreviouslySentPacket("CFrameSnapshotManager::GetPreviouslySentPacket");
MemberFuncThunk<CFrameSnapshotManager *, bool, CFrameSnapshot*, int, int> CFrameSnapshotManager::ft_UsePreviouslySentPacket("CFrameSnapshotManager::UsePreviouslySentPacket");
MemberFuncThunk<CFrameSnapshotManager *, PackedEntity*, CFrameSnapshot*, int> CFrameSnapshotManager::ft_CreatePackedEntity("CFrameSnapshotManager::CreatePackedEntity");

GlobalThunk<CFrameSnapshotManager> g_FrameSnapshotManager("g_FrameSnapshotManager");
GlobalThunk<PropTypeFns[DPT_NUMSendPropTypes]> g_PropTypeFns("g_PropTypeFns");
GlobalThunk<SendTable> DT_TFPlayer_g_SendTable("DT_TFPlayer::g_SendTable");
GlobalThunk<ServerClass> g_CTFPlayer_ClassReg("g_CTFPlayer_ClassReg");

class CDeltaBitsWriter
{
public:
				CDeltaBitsWriter( bf_write *pBuf );
				~CDeltaBitsWriter();

	// Write the next property index. Returns the number of bits used.
	void		WritePropIndex( int iProp );

	// Access the buffer it's outputting to.
	bf_write*	GetBitBuf();

private:
	bf_write	*m_pBuf;
	int			m_iLastProp;
};

inline CDeltaBitsWriter::CDeltaBitsWriter( bf_write *pBuf )
{
	m_pBuf = pBuf;
	m_iLastProp = -1;
}

inline bf_write* CDeltaBitsWriter::GetBitBuf()
{
	return m_pBuf;
}

FORCEINLINE void CDeltaBitsWriter::WritePropIndex( int iProp )
{
	unsigned int diff = iProp - m_iLastProp;
	m_iLastProp = iProp;
	// Expanded inline for maximum efficiency.
	//m_pBuf->WriteOneBit( 1 );
	//m_pBuf->WriteUBitVar( diff - 1 );
	COMPILE_TIME_ASSERT( MAX_DATATABLE_PROPS <= 0x1000u );
	int n = ((diff < 0x11u) ? -1 : 0) + ((diff < 0x101u) ? -1 : 0);
	m_pBuf->WriteUBitLong( diff*8 - 8 + 4 + n*2 + 1, 8 + n*4 + 4 + 2 + 1 );
}

inline CDeltaBitsWriter::~CDeltaBitsWriter()
{
	m_pBuf->WriteOneBit( 0 );
}

class CDeltaBitsReader
{
public:
	CDeltaBitsReader( bf_read *pBuf );
	~CDeltaBitsReader();

	// Write the next property index. Returns the number of bits used.
	unsigned int ReadNextPropIndex();
	unsigned int ReadNextPropIndex_Continued();
	void	SkipPropData( const SendProp *pProp );
	int		ComparePropData( CDeltaBitsReader* pOut, const SendProp *pProp );
	void	CopyPropData( bf_write* pOut, const SendProp *pProp );

	// If you know you're done but you're not at the end (you haven't called until
	// ReadNextPropIndex returns -1), call this so it won't assert in its destructor.
	void		ForceFinished();

private:
	bf_read		*m_pBuf;
	int			m_iLastProp;
};


FORCEINLINE CDeltaBitsReader::CDeltaBitsReader( bf_read *pBuf )
{
	m_pBuf = pBuf;
	m_iLastProp = -1;
}

FORCEINLINE CDeltaBitsReader::~CDeltaBitsReader()
{
	// Make sure they read to the end unless they specifically said they don't care.
	Assert( !m_pBuf );
}

FORCEINLINE void CDeltaBitsReader::ForceFinished()
{
#ifdef DBGFLAG_ASSERT
	m_pBuf = NULL;
#endif
}

FORCEINLINE unsigned int CDeltaBitsReader::ReadNextPropIndex()
{
	Assert( m_pBuf );

	if ( m_pBuf->GetNumBitsLeft() >= 7 )
	{
		uint bits = m_pBuf->ReadUBitLong( 7 );
		if ( bits & 1 )
		{
			uint delta = bits >> 3;
			if ( bits & 6 )
			{
				delta = m_pBuf->ReadUBitVarInternal( (bits & 6) >> 1 );
			}
			m_iLastProp = m_iLastProp + 1 + delta;
			Assert( m_iLastProp < MAX_DATATABLE_PROPS );
			return m_iLastProp;
		}
		m_pBuf->m_iCurBit -= 6; // Unread six bits we shouldn't have looked at
	}
	else
	{
		// Not enough bits for a property index.
		if ( m_pBuf->ReadOneBit() )
		{
			// Expected a zero bit! Force an overflow!
			m_pBuf->Seek(-1);
		}
	}
	ForceFinished();
	return ~0u;
}

FORCEINLINE void CDeltaBitsReader::SkipPropData( const SendProp *pProp )
{
	g_PropTypeFns[ pProp->GetType() ].SkipProp( pProp, m_pBuf );
}

FORCEINLINE void CDeltaBitsReader::CopyPropData( bf_write* pOut, const SendProp *pProp )
{
	int start = m_pBuf->GetNumBitsRead();
	g_PropTypeFns[ pProp->GetType() ].SkipProp( pProp, m_pBuf );
	int len = m_pBuf->GetNumBitsRead() - start;
	m_pBuf->Seek( start );
	pOut->WriteBitsFromBuffer( m_pBuf, len );
}

FORCEINLINE int CDeltaBitsReader::ComparePropData( CDeltaBitsReader *pInReader, const SendProp *pProp )
{
	bf_read *pIn = pInReader->m_pBuf;
	return g_PropTypeFns[pProp->m_Type].CompareDeltas( pProp, m_pBuf, pIn );
}

class CFastLocalTransferPropInfo
{
public:
    unsigned short	m_iRecvOffset;
    unsigned short	m_iSendOffset;
    unsigned short	m_iProp;
};


class CFastLocalTransferInfo
{
public:
    CUtlVector<CFastLocalTransferPropInfo> m_FastInt32;
    CUtlVector<CFastLocalTransferPropInfo> m_FastInt16;
    CUtlVector<CFastLocalTransferPropInfo> m_FastInt8;
    CUtlVector<CFastLocalTransferPropInfo> m_FastVector;
    CUtlVector<CFastLocalTransferPropInfo> m_OtherProps;	// Props that must be copied slowly (proxies and all).
};

class CSendNode
{
public:

	// Child datatables.
	CUtlVector<CSendNode*>	m_Children;

	// The datatable property that leads us to this CSendNode.
	// This indexes the CSendTablePrecalc or CRecvDecoder's m_DatatableProps list.
	// The root CSendNode sets this to -1.
	short					m_iDatatableProp;

	// The SendTable that this node represents.
	// ALL CSendNodes have this.
	const SendTable	*m_pTable;

	//
	// Properties in this table.
	//

	// m_iFirstRecursiveProp to m_nRecursiveProps defines the list of propertise
	// of this node and all its children.
	unsigned short	m_iFirstRecursiveProp;
	unsigned short	m_nRecursiveProps;


	// See GetDataTableProxyIndex().
	unsigned short	m_DataTableProxyIndex;
	
	// See GetRecursiveProxyIndex().
	unsigned short	m_RecursiveProxyIndex;
};

class CSendTablePrecalc
{
public:
    					CSendTablePrecalc();
	virtual				~CSendTablePrecalc();
    
    class CProxyPathEntry
    {
    public:
        unsigned short m_iDatatableProp;	// Lookup into CSendTablePrecalc or CRecvDecoder::m_DatatableProps.
        unsigned short m_iProxy;
    };
    class CProxyPath
    {
    public:
        unsigned short m_iFirstEntry;	// Index into m_ProxyPathEntries.
        unsigned short m_nEntries;
    };
    
    CUtlVector<CProxyPathEntry> m_ProxyPathEntries;	// For each proxy index, this is all the DT proxies that generate it.
    CUtlVector<CProxyPath> m_ProxyPaths;			// CProxyPathEntries lookup into this.
    
    // These are what CSendNodes reference.
    // These are actual data properties (ints, floats, etc).
    CUtlVector<const SendProp*>	m_Props;

    // Each datatable in a SendTable's tree gets a proxy index, and its properties reference that.
    CUtlVector<unsigned char> m_PropProxyIndices;
    
    // CSendNode::m_iDatatableProp indexes this.
    // These are the datatable properties (SendPropDataTable).
    CUtlVector<const SendProp*>	m_DatatableProps;

    // This is the property hierarchy, with the nodes indexing m_Props.
    CSendNode				m_Root;

    // From whence we came.
    SendTable				*m_pSendTable;

    // For instrumentation.
    void			*m_pDTITable;

    // This is precalculated in single player to allow faster direct copying of the entity data
    // from the server entity to the client entity.
    CFastLocalTransferInfo	m_FastLocalTransfer;

    // This tells how many data table properties there are without SPROP_PROXY_ALWAYS_YES.
    // Arrays allocated with this size can be indexed by CSendNode::GetDataTableProxyIndex().
    int						m_nDataTableProxies;
    
    // Map prop offsets to indices for properties that can use it.
    CUtlMap<unsigned short, unsigned short> m_PropOffsetToIndexMap;
};

abstract_class CDatatableStack
{
public:
	
							CDatatableStack( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID );

	// This must be called before accessing properties.
	void Init( bool bExplicitRoutes=false );

	// The stack is meant to be used by calling SeekToProp with increasing property
	// numbers.
	void			SeekToProp( int iProp );

	bool			IsCurProxyValid() const;
	bool			IsPropProxyValid(int iProp ) const;
	int				GetCurPropIndex() const;
	
	unsigned char*	GetCurStructBase() const;
	
	int				GetObjectID() const;

	// Derived classes must implement this. The server gets one and the client gets one.
	// It calls the proxy to move to the next datatable's data.
	virtual void RecurseAndCallProxies( CSendNode *pNode, unsigned char *pStructBase ) = 0;


public:
	CSendTablePrecalc *m_pPrecalc;
	
	enum
	{
		MAX_PROXY_RESULTS = 256
	};

	// These point at the various values that the proxies returned. They are setup once, then 
	// the properties index them.
	unsigned char *m_pProxies[MAX_PROXY_RESULTS];
	unsigned char *m_pStructBase;
	int m_iCurProp;

protected:

	const SendProp *m_pCurProp;
	
	int m_ObjectID;

	bool m_bInitted;
};

inline bool CDatatableStack::IsPropProxyValid(int iProp ) const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[iProp]] != 0;
}

inline bool CDatatableStack::IsCurProxyValid() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]] != 0;
}

inline int CDatatableStack::GetCurPropIndex() const
{
	return m_iCurProp;
}

inline unsigned char* CDatatableStack::GetCurStructBase() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]]; 
}

inline void CDatatableStack::SeekToProp( int iProp )
{
	Assert( m_bInitted );
	
	m_iCurProp = iProp;
	m_pCurProp = m_pPrecalc->m_Props[iProp];
}

inline int CDatatableStack::GetObjectID() const
{
	return m_ObjectID;
}

CDatatableStack::CDatatableStack( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID )
{
	m_pPrecalc = pPrecalc;

	m_pStructBase = pStructBase;
	m_ObjectID = objectID;
	
	m_iCurProp = 0;
	m_pCurProp = NULL;

	m_bInitted = false;

#ifdef _DEBUG
	memset( m_pProxies, 0xFF, sizeof( m_pProxies ) );
#endif
}


void CDatatableStack::Init( bool bExplicitRoutes )
{
	if ( bExplicitRoutes )
	{
		memset( m_pProxies, 0xFF, sizeof( m_pProxies[0] ) * m_pPrecalc->m_ProxyPaths.Count() );
	}
	else
	{
		// Walk down the tree and call all the datatable proxies as we hit them.
		RecurseAndCallProxies( &m_pPrecalc->m_Root, m_pStructBase );
	}

	m_bInitted = true;
}

class CPropMapStack : public CDatatableStack
{
public:
						CPropMapStack( CSendTablePrecalc *pPrecalc, const CStandardSendProxies *pSendProxies ) :
							CDatatableStack( pPrecalc, (unsigned char*)1, -1 )
						{
							m_pPropMapStackPrecalc = pPrecalc;
							m_pSendProxies = pSendProxies;
						}

	bool IsNonPointerModifyingProxy( SendTableProxyFn fn, const CStandardSendProxies *pSendProxies )
	{
		if ( fn == m_pSendProxies->m_DataTableToDataTable ||
			 fn == m_pSendProxies->m_SendLocalDataTable )
		{
			return true;
		}
		
		if( pSendProxies->m_ppNonModifiedPointerProxies )
		{
			CNonModifiedPointerProxy *pCur = *pSendProxies->m_ppNonModifiedPointerProxies;
			while ( pCur )
			{
				if ( pCur->m_Fn == fn )
					return true;
				pCur = pCur->m_pNext;
			}
		}

		return false;
	}

	inline unsigned char*	CallPropProxy( CSendNode *pNode, int iProp, unsigned char *pStructBase )
	{
		if ( !pStructBase )
			return 0;
		
		const SendProp *pProp = m_pPropMapStackPrecalc->m_DatatableProps[iProp];
		if ( IsNonPointerModifyingProxy( pProp->GetDataTableProxyFn(), m_pSendProxies ) )
		{
			// Note: these are offset by 1 (see the constructor), otherwise it won't recurse
			// during the Init call because pCurStructBase is 0.
			return pStructBase + pProp->GetOffset();
		}
		else
		{
			return 0;
		}
	}

	virtual void RecurseAndCallProxies( CSendNode *pNode, unsigned char *pStructBase )
	{
		// Remember where the game code pointed us for this datatable's data so 
		m_pProxies[ pNode->m_RecursiveProxyIndex ] = pStructBase;

		for ( int iChild=0; iChild < pNode->m_Children.Count(); iChild++ )
		{
			CSendNode *pCurChild = pNode->m_Children[iChild];
			
			unsigned char *pNewStructBase = NULL;
			if ( pStructBase )
			{
				pNewStructBase = CallPropProxy( pCurChild, pCurChild->m_iDatatableProp, pStructBase );
			}

			RecurseAndCallProxies( pCurChild, pNewStructBase );
		}
	}

public:
	CSendTablePrecalc *m_pPropMapStackPrecalc;
	const CStandardSendProxies *m_pSendProxies;
};

class CFrameSnapshotEntry
{
public:
    ServerClass*			m_pClass;
    int						m_nSerialNumber;
    // Keeps track of the fullpack info for this frame for all entities in any pvs:
    PackedEntityHandle_t	m_pPackedData;
};

class CFrameSnapshot
{
	DECLARE_FIXEDSIZE_ALLOCATOR( CFrameSnapshot );
public:
	CFrameSnapshot();
	~CFrameSnapshot();

    CInterlockedInt			m_ListIndex;	// Index info CFrameSnapshotManager::m_FrameSnapshots.

    // Associated frame. 
    int						m_nTickCount; // = sv.tickcount
    
    // State information
    CFrameSnapshotEntry		*m_pEntities;	
	int						m_nNumEntities; // = sv.num_edicts
};

struct PackWork_t
{
    int				nIdx;
    edict_t			*pEdict;
    CFrameSnapshot	*pSnapshot;
};

class DecodeInfo : public CRecvProxyData
{
public:
		
	// Copy everything except val.
	void			CopyVars( const DecodeInfo *pOther );

public:
	
	//
	// NOTE: it's valid to pass in m_pRecvProp and m_pData and m_pSrtuct as null, in which 
	// case the buffer is advanced but the property is not stored anywhere. 
	//
	// This is used by SendTable_CompareDeltas.
	//
	void			*m_pStruct;			// Points at the base structure
	void			*m_pData;			// Points at where the variable should be encoded. 

	const SendProp 	*m_pProp;		// Provides the client's info on how to decode and its proxy.
	bf_read			*m_pIn;			// The buffer to get the encoded data from.

	char			m_TempStr[DT_MAX_STRING_BUFFERSIZE];	// m_Value.m_pString is set to point to this.
};

// PROP ENCODING FUNCTIONS
static inline void Int_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	int nValue = pVar->m_Int;
	
	if ( pProp->GetFlags() & SPROP_VARINT)
	{
		if ( pProp->GetFlags() & SPROP_UNSIGNED )
		{
			pOut->WriteVarInt32( nValue );
		}
		else
		{
			pOut->WriteSignedVarInt32( nValue );
		}
	}
	else
	{
		// If signed, preserve lower bits and then re-extend sign if nValue < 0;
		// if unsigned, preserve all 32 bits no matter what. Bonus: branchless.
		int nPreserveBits = ( 0x7FFFFFFF >> ( 32 - pProp->m_nBits ) );
		nPreserveBits |= ( pProp->GetFlags() & SPROP_UNSIGNED ) ? 0xFFFFFFFF : 0;
		int nSignExtension = ( nValue >> 31 ) & ~nPreserveBits;

		nValue &= nPreserveBits;
		nValue |= nSignExtension;

#ifdef DBGFLAG_ASSERT
		// Assert that either the property is unsigned and in valid range,
		// or signed with a consistent sign extension in the high bits
		if ( pProp->m_nBits < 32 )
		{
			if ( pProp->GetFlags() & SPROP_UNSIGNED )
			{
				AssertMsg3( nValue == pVar->m_Int, "Unsigned prop %s needs more bits? Expected %i == %i", pProp->GetName(), nValue, pVar->m_Int );
			}
			else 
			{
				AssertMsg3( nValue == pVar->m_Int, "Signed prop %s needs more bits? Expected %i == %i", pProp->GetName(), nValue, pVar->m_Int );
			}
		}
		else
		{
			// This should never trigger, but I'm leaving it in for old-time's sake.
			Assert( nValue == pVar->m_Int );
		}
#endif

		pOut->WriteUBitLong( nValue, pProp->m_nBits, false );
	}
}

static inline void EncodeFloat( const SendProp *pProp, float fVal, bf_write *pOut, int objectID )
{
	// Check for special flags like SPROP_COORD, SPROP_NOSCALE, and SPROP_NORMAL.
	int flags = pProp->GetFlags();
	if ( flags & SPROP_COORD )
	{
		pOut->WriteBitCoord( fVal );
	}
	else if ( flags & ( SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL ) )
	{
		COMPILE_TIME_ASSERT( SPROP_COORD_MP_INTEGRAL == (1<<15) );
		COMPILE_TIME_ASSERT( SPROP_COORD_MP_LOWPRECISION == (1<<14) );
		pOut->WriteBitCoordMP( fVal, ((flags >> 15) & 1), ((flags >> 14) & 1) );
	}
	else if ( flags & SPROP_NORMAL )
	{
		pOut->WriteBitNormal( fVal );
	}
	else // standard clamped-range float
	{
		unsigned long ulVal;
		int nBits = pProp->m_nBits;
		if ( flags & SPROP_NOSCALE )
		{
			union { float f; uint32 u; } convert = { fVal };
			ulVal = convert.u;
			nBits = 32;
		}
		else if( fVal < pProp->m_fLowValue )
		{
			// clamp < 0
			ulVal = 0;
		}
		else if( fVal > pProp->m_fHighValue )
		{
			// clamp > 1
			ulVal = ((1 << pProp->m_nBits) - 1);
		}
		else
		{
			float fRangeVal = (fVal - pProp->m_fLowValue) * pProp->m_fHighLowMul;
			if ( pProp->m_nBits <= 22 )
			{
				// this is the case we always expect to hit
				ulVal = FastFloatToSmallInt( fRangeVal );
			}
			else
			{
				// retain old logic just in case anyone relies on its behavior
				ulVal = RoundFloatToUnsignedLong( fRangeVal );
			}
		}
		pOut->WriteUBitLong(ulVal, nBits);
	}
}

static inline void Float_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeFloat( pProp, pVar->m_Float, pOut, objectID );
}

static inline void Vector_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeFloat(pProp, pVar->m_Vector[0], pOut, objectID);
	EncodeFloat(pProp, pVar->m_Vector[1], pOut, objectID);
	// Don't write out the third component for normals
	if ((pProp->GetFlags() & SPROP_NORMAL) == 0)
	{
		EncodeFloat(pProp, pVar->m_Vector[2], pOut, objectID);
	}
	else
	{
		// Write a sign bit for z instead!
		int	signbit = (pVar->m_Vector[2] <= -NORMAL_RESOLUTION);
		pOut->WriteOneBit( signbit );
	}
}

static inline void VectorXY_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeFloat(pProp, pVar->m_Vector[0], pOut, objectID);
	EncodeFloat(pProp, pVar->m_Vector[1], pOut, objectID);
}

static inline void String_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	// First count the string length, then do one WriteBits call.
	int len;
	for ( len=0; len < DT_MAX_STRING_BUFFERSIZE-1; len++ )
	{
		if( pVar->m_pString[len] == 0 )
		{
			break;
		}
	}	
		
	// Optionally write the length here so deltas can be compared faster.
	pOut->WriteUBitLong( len, DT_MAX_STRING_BITS );
	pOut->WriteBits( pVar->m_pString, len * 8 );
}

class CClientFrame
{
public:

	CClientFrame( void );
	CClientFrame( int tickcount );
	CClientFrame( CFrameSnapshot *pSnapshot );
	virtual ~CClientFrame();
	void Init( CFrameSnapshot *pSnapshot );
	void Init( int tickcount );

	// Accessors to snapshots. The data is protected because the snapshots are reference-counted.
	inline CFrameSnapshot*	GetSnapshot() const { return m_pSnapshot; };
	void					SetSnapshot( CFrameSnapshot *pSnapshot );
	void					CopyFrame( CClientFrame &frame );
	virtual bool		IsMemPoolAllocated() { return true; }

public:

	// State of entities this frame from the POV of the client.
	int					last_entity;	// highest entity index
	int					tick_count;	// server tick of this snapshot

	// Used by server to indicate if the entity was in the player's pvs
	CBitVec<MAX_EDICTS>	transmit_entity; // if bit n is set, entity n will be send to client
	CBitVec<MAX_EDICTS>	*from_baseline;	// if bit n is set, this entity was send as update from baseline
	CBitVec<MAX_EDICTS>	*transmit_always; // if bit is set, don't do PVS checks before sending (HLTV only)

	CClientFrame*		m_pNext;

private:

	CFrameSnapshot		*m_pSnapshot;
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
    std::vector<unsigned short> player_prop_write_offset[33];

    std::vector<int> player_prop_value_old[33];

    unsigned short *player_prop_offsets;

    // prop indexes that are stopped from being send to players
    unsigned char *player_prop_cull;

    bool *player_local_exclusive_send_proxy;

    CSendNode **player_send_nodes;

    bool force_player_update[33];

    // This is used to check if a player had a forced full update
    bool player_not_force_updated[33];

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
            if (objectID > 0 && objectID < 34) {
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
                        
                    SendProp *pProp = pTable->m_pPrecalc->m_Props[iToProp];

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
                    // Set local player table recipient to the current player. Guess that if a single player is set as recipient then its a local only table
                    /*int recipient_count = pRecipients->Count();
                    for (int i = 0; i < recipient_count; i++) {
                        auto &recipient = (*pRecipients)[i];
                        uint64_t bits = recipient.m_Bits.GetDWord(0) | ((uint64_t)recipient.m_Bits.GetDWord(1) << 32);
                        
                        // local player only
                        if (bits && !(bits & (bits-1))) {
                            bits = 1LL << (objectID - 1);
                            recipient.m_Bits.SetDWord(0, bits & 0xFFFFFFFF);
                            recipient.m_Bits.SetDWord(1, bits >> 32);
                        }
                        else {
                            bits = (~recipient.m_Bits.GetDWord(0)) | ((uint64_t)(~recipient.m_Bits.GetDWord(1)) << 32);
                            // Other players only
                            if (bits && !(bits & (bits-1))) {
                                bits = 1LL << (objectID - 1);
                                recipient.m_Bits.SetDWord(0, ~(bits & 0xFFFFFFFF));
                                recipient.m_Bits.SetDWord(1, ~(bits >> 32));
                            }
                        }
                    }*/

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
                        SendProp *pProp = pPrecalc->m_Props[propOffsets[i]];

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
                            pStructBaseOffset, 
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
                    SendProp *pProp = pTable->m_pPrecalc->m_Props[iToProp];
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

    DETOUR_DECL_MEMBER(__gcc_regcall void, CAttributeManager_ClearCache)
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
                        weapon->GetNetworkable()->GetEdict()->m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
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
            return pVarData;
        }
        return nullptr;
    }

    DETOUR_DECL_STATIC(void*, SendProxy_SendHealersDataTable, const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID)
    {
        auto result = DETOUR_STATIC_CALL(SendProxy_SendHealersDataTable)(pProp, pStruct, pVarData, pRecipients, objectID);
        if (result == nullptr) {
            pRecipients->ClearAllRecipients();
        }
        return pVarData;
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
                SendProp *pProp = playerSendTable->m_pPrecalc->m_Props[iToProp];

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
                    
                    if ( pProp->GetType() == DPT_Array )
                    {
                        offset = pProp->GetArrayProp()->GetOffset() + (int)pmStack.GetCurStructBase() - 1;
                        elementCount = pProp->m_nElements;
                        elementStride = pProp->m_ElementStride;
                    }

                    player_prop_offsets[iToProp] = offset;


                    //if (pProp->GetType() == DPT_Vector || pProp->GetType() == DPT_Vector )
                    //    propIndex |= PROP_INDEX_VECTOR_ELEM_MARKER;

                    if ( offset != 0)
                    {
                        int offset_off = offset;
                        for ( int j = 0; j < elementCount; j++ )
                        {
                            AddOffsetToList(offset_off, iToProp);
                            if (pProp->GetType() == DPT_Vector) {
                                AddOffsetToList(offset_off + 4, iToProp);
                                AddOffsetToList(offset_off + 8, iToProp);
                            }
                            else if (pProp->GetType() == DPT_VectorXY) {
                                AddOffsetToList(offset_off + 4, iToProp);
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
            sv_parallel_packentities.SetValue(true);
        }

		virtual void LevelInitPostEntity() override
		{
            world_edict = INDEXENT(0);
            world_accessor = static_cast<CVEngineServer *>(engine)->GetChangeAccessorStatic(world_edict);
            world_change_info = &g_SharedEdictChangeInfo->m_ChangeInfos[0];
            
            ConVarRef sv_parallel_packentities("sv_parallel_packentities");
            sv_parallel_packentities.SetValue(true);
		}

	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_sendprop_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve sendprop encoding performance by preventing full updates on clients",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
