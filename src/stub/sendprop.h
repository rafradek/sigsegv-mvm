#ifndef _INCLUDE_SIGSEGV_STUB_SENDPROP_H_
#define _INCLUDE_SIGSEGV_STUB_SENDPROP_H_

#include "sdk2013/mempool.h"

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

extern GlobalThunk<CFrameSnapshotManager> g_FrameSnapshotManager;
extern GlobalThunk<PropTypeFns[DPT_NUMSendPropTypes]> g_PropTypeFns;
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

class CPropMapStack : public CDatatableStack
{
public:
						CPropMapStack( CSendTablePrecalc *pPrecalc, const CStandardSendProxies *pSendProxies ) :
							CDatatableStack( pPrecalc, (unsigned char*)1, -1 )
						{
							m_pPropMapStackPrecalc = pPrecalc;
							m_pSendProxies = pSendProxies;
						}

	static inline bool IsNonPointerModifyingProxy( SendTableProxyFn fn, const CStandardSendProxies *pSendProxies )
	{
		if ( fn == pSendProxies->m_DataTableToDataTable ||
			 fn == pSendProxies->m_SendLocalDataTable )
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
		
		//const SendProp *pProp = m_pPropMapStackPrecalc->m_DatatableProps[pNode->m_iDatatableProp];

		for ( int iChild=0; iChild < pNode->m_Children.Count(); iChild++ )
		{
			CSendNode *pCurChild = pNode->m_Children[iChild];
			const SendProp *pChildProp = m_pPropMapStackPrecalc->m_DatatableProps[pCurChild->m_iDatatableProp];
			
			unsigned char *pNewStructBase = NULL;
			if ( pStructBase )
			{
				pNewStructBase = CallPropProxy( pCurChild, pCurChild->m_iDatatableProp, pStructBase );
			}
			m_pIsPointerModifyingProxy[pCurChild->m_RecursiveProxyIndex] = 
				IsNonPointerModifyingProxy(pChildProp->GetDataTableProxyFn(), m_pSendProxies) ? 
					m_pIsPointerModifyingProxy[pNode->m_RecursiveProxyIndex] : pCurChild;
			m_iBaseOffset[pCurChild->m_RecursiveProxyIndex] = (int)pStructBase;

			RecurseAndCallProxies( pCurChild, pNewStructBase );
		}
	}

public:
	CSendTablePrecalc *m_pPropMapStackPrecalc;
	const CStandardSendProxies *m_pSendProxies;
	const CSendNode *m_pIsPointerModifyingProxy[MAX_PROXY_RESULTS] {nullptr};
	int m_iBaseOffset[MAX_PROXY_RESULTS] {0};
};

class CServerDatatableStack : public CDatatableStack
{
public:
	CSendTablePrecalc					*m_pPrecalc;
	CUtlMemory<CSendProxyRecipients>	*m_pRecipients;
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

static inline int Array_GetLength( const unsigned char *pStruct, const SendProp *pProp, int objectID )
{
	// Get the array length from the proxy.
	ArrayLengthSendProxyFn proxy = pProp->GetArrayLengthProxy();
	
	if ( proxy )
	{
		int nElements = proxy( pStruct, objectID );
		
		// Make sure it's not too big.
		if ( nElements > pProp->GetNumElements() )
		{
			Assert( false );
			nElements = pProp->GetNumElements();
		}

		return nElements;
	}
	else
	{	
		return pProp->GetNumElements();
	}
}

static inline void Array_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	SendProp *pArrayProp = pProp->GetArrayProp();
	AssertMsg( pArrayProp, "Array_Encode: missing m_pArrayProp for SendProp '%s'.", pProp->m_pVarName );
	
	int nElements = Array_GetLength( pStruct, pProp, objectID );

	// Write the number of elements.
	pOut->WriteUBitLong( nElements, Q_log2( pProp->GetNumElements() ) + 1);

	unsigned char *pCurStructOffset = (unsigned char*)pStruct + pArrayProp->GetOffset();
	for ( int iElement=0; iElement < nElements; iElement++ )
	{
		DVariant var;

		// Call the proxy to get the value, then encode.
		pArrayProp->GetProxyFn()( pArrayProp, pStruct, pCurStructOffset, &var, iElement, objectID );
		g_PropTypeFns[pArrayProp->GetType()].Encode( pStruct, &var, pArrayProp, pOut, objectID ); 
		
		pCurStructOffset += pProp->GetElementStride();
	}
}

using PropEncodeFunc = void (*)( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID );
static PropEncodeFunc encode_functions[DPT_NUMSendPropTypes] {
	Int_Encode,
	Float_Encode,
	Vector_Encode,
	VectorXY_Encode,
	String_Encode,
	Array_Encode,
	nullptr,
};

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

#endif