#include "stub/sendprop.h"


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


MemberFuncThunk<CVEngineServer *, IChangeInfoAccessor *, const edict_t *> CVEngineServer::ft_GetChangeAccessor("CVEngineServer::GetChangeAccessor");
MemberFuncThunk<CFrameSnapshotManager *, PackedEntity *, int, int> CFrameSnapshotManager::ft_GetPreviouslySentPacket("CFrameSnapshotManager::GetPreviouslySentPacket");
MemberFuncThunk<CFrameSnapshotManager *, bool, CFrameSnapshot*, int, int> CFrameSnapshotManager::ft_UsePreviouslySentPacket("CFrameSnapshotManager::UsePreviouslySentPacket");
MemberFuncThunk<CFrameSnapshotManager *, PackedEntity*, CFrameSnapshot*, int> CFrameSnapshotManager::ft_CreatePackedEntity("CFrameSnapshotManager::CreatePackedEntity");
MemberFuncThunk<CFrameSnapshotManager *, PackedEntity*, CFrameSnapshot*, int> CFrameSnapshotManager::ft_GetPackedEntity("CFrameSnapshotManager::GetPackedEntity");

GlobalThunk<CFrameSnapshotManager> g_FrameSnapshotManager("g_FrameSnapshotManager");
GlobalThunk<PropTypeFns[DPT_NUMSendPropTypes]> g_PropTypeFns("g_PropTypeFns");

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

GlobalThunk<SendVarProxyFn> DLLSendProxy_StringToString("SendProxy_StringToString");
GlobalThunk<SendVarProxyFn> DLLSendProxy_VectorXYToVectorXY("SendProxy_VectorXYToVectorXY");
GlobalThunk<SendVarProxyFn> DLLSendProxy_QAngles("SendProxy_QAngles");
GlobalThunk<SendVarProxyFn> DLLSendProxy_Color32ToInt("SendProxy_Color32ToInt");
GlobalThunk<SendVarProxyFn> DLLSendProxy_EHandleToInt("SendProxy_EHandleToInt");
GlobalThunk<SendVarProxyFn> DLLSendProxy_IntAddOne("SendProxy_IntAddOne");
GlobalThunk<SendVarProxyFn> DLLSendProxy_ShortAddOne("SendProxy_ShortAddOne");
GlobalThunk<SendVarProxyFn> DLLSendProxy_StringT_To_String("SendProxy_StringT_To_String");
GlobalThunk<SendVarProxyFn> DLLSendProxy_AngleToFloat("SendProxy_AngleToFloat");
GlobalThunk<SendVarProxyFn> DLLSendProxy_Empty("SendProxy_Empty");