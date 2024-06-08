#ifndef _INCLUDE_SIGSEGV_STUB_HLTVSERVER_H_
#define _INCLUDE_SIGSEGV_STUB_HLTVSERVER_H_


#include "link/link.h"
#include "stub/sendprop.h"

typedef struct CustomFile_s
{
	CRC32_t			crc;
	unsigned int	reqID;
} CustomFile_t;

class CBaseClient : public IGameEventListener2, public IClient, public IClientMessageHandler {
public:
	virtual void UpdateUserSettings() = 0;

	void UpdateSendState()                     { vt_UpdateSendState(this); }
	void SendSnapshot(CClientFrame * frame)    { vt_SendSnapshot(this, frame); }
	bool UpdateAcknowledgedFramecount(int tick){ return vt_UpdateAcknowledgedFramecount(this, tick); }
	
	int				m_nClientSlot;	
	int				m_nEntityIndex;	
	
	int				m_UserID;
	char			m_Name[MAX_PLAYER_NAME_LENGTH];
	char			m_GUID[SIGNED_GUID_LEN + 1];

	CSteamID		m_SteamID;
	
	uint32			m_nFriendsID;
	char			m_FriendsName[MAX_PLAYER_NAME_LENGTH];

	KeyValues		*m_ConVars;
	bool			m_bConVarsChanged;
	bool			m_bInitialConVarsSet;
	bool			m_bSendServerInfo;
	IServer		*m_Server;
	bool			m_bIsHLTV;
	bool			m_bIsReplay;
	int				m_clientChallenge;
	
	CRC32_t			m_nSendtableCRC;

	CustomFile_t	m_nCustomFiles[MAX_CUSTOM_FILES];
	int				m_nFilesDownloaded;

	INetChannel		*m_NetChannel;
	int				m_nSignonState;
	int             pad;
	int				m_nDeltaTick;
	int				m_nStringTableAckTick;
	int				m_nSignonTick;
	void * m_pLastSnapshot; //0x0dc

	CFrameSnapshot	*m_pBaseline; //0x0e0
	int				m_nBaselineUpdateTick; // 0x0e4
	CBitVec<MAX_EDICTS>	m_BaselinesSent; //0x0e8
	int				m_nBaselineUsed; // 0x1e8

	int				m_nForceWaitForTick; // 0x1ec
	
	bool			m_bFakePlayer; // 0x1f0
private:
	static MemberVFuncThunk<CBaseClient *, void>                 vt_UpdateSendState;
	static MemberVFuncThunk<CBaseClient *, void, CClientFrame *> vt_SendSnapshot;
	static MemberVFuncThunk<CBaseClient *, bool, int>            vt_UpdateAcknowledgedFramecount;
	
};

class CBaseServer : public IServer
{
public:
    CBaseClient *CreateFakeClient(const char *name) { return ft_CreateFakeClient(this, name); }
	
	float    GetCPUUsage() { return vt_GetCPUUsage(this); }
	void     UserInfoChanged(int slot) { vt_UserInfoChanged(this, slot); }
    int &GetMaxClientsRef() {return *(int *)((uintptr_t)(this) + 0x14C);}
private:
    static MemberFuncThunk<CBaseServer *, CBaseClient *, const char *>              ft_CreateFakeClient;
	
	static MemberVFuncThunk<CBaseServer *, float>              vt_GetCPUUsage;
	static MemberVFuncThunk<CBaseServer *, void, int>          vt_UserInfoChanged;
};

class CGameServer : public CBaseServer
{
public:
	int GetNumEdicts() {return *(int *)((uintptr_t)(this) + 0x1E4);}
};

class CClientFrameManager 
{
public:
    int CountClientFrames()                { return ft_CountClientFrames(this); }
    void RemoveOldestFrame()               {        ft_RemoveOldestFrame(this); }

private:
    static MemberFuncThunk<CClientFrameManager *, int>                 ft_CountClientFrames;
    static MemberFuncThunk<CClientFrameManager *, void>                ft_RemoveOldestFrame;
};

class CHLTVServer : public IGameEventListener2, public CBaseServer
{
public:
    void StartMaster(CBaseClient *client)  {        ft_StartMaster(this, client); }
    
private:
    static MemberFuncThunk<CHLTVServer *, void, CBaseClient *> ft_StartMaster;
};

class CGameClient : public CBaseClient
{
public:
    bool ShouldSendMessages()                      { return ft_ShouldSendMessages(this); }
    void SetupPackInfo(CFrameSnapshot * snapshot)  { return ft_SetupPackInfo(this, snapshot); }
    void SetupPrevPackInfo()                       { return ft_SetupPrevPackInfo(this); }
    CClientFrame *GetSendFrame()                   { return ft_GetSendFrame(this); }
	//bool SpawnServer(const char *szMapName, const char *szMapFile, const char *startspot)  { return ft_SpawnServer(this, szMapName, szMapFile, startspot); }
    
private:
    static MemberFuncThunk<CGameClient *, bool>                   ft_ShouldSendMessages;
    static MemberFuncThunk<CGameClient *, void, CFrameSnapshot *> ft_SetupPackInfo;
    static MemberFuncThunk<CGameClient *, void>                   ft_SetupPrevPackInfo;
    static MemberFuncThunk<CGameClient *, CClientFrame *>         ft_GetSendFrame;
};

class CHLTVClient : public CBaseClient {

};

class CNetworkStringTable  : public INetworkStringTable
{
public:
	virtual			~CNetworkStringTable( void );
	virtual void	Dump( void );
	virtual void	Lock( bool bLock );

	void UpdateMirrorTable (int tick) { ft_UpdateMirrorTable(this, tick); }
	void DeleteAllStrings ()          { ft_DeleteAllStrings(this); }

	TABLEID				m_id;
	char				*m_pszTableName;
	int					m_nMaxEntries;
	int					m_nEntryBits;
	int					m_nTickCount;
	int					m_nLastChangedTick;
	bool				m_bChangeHistoryEnabled : 1;
	bool				m_bLocked : 1;
	bool				m_bAllowClientSideAddString : 1;
	bool				m_bUserDataFixedSize : 1;
	bool				m_bIsFilenames : 1;
	int					m_nUserDataSize;
	int					m_nUserDataSizeBits;
	pfnStringChanged	m_changeFunc;
	void				*m_pObject;
	CNetworkStringTable	*m_pMirrorTable;

private:
	static MemberFuncThunk<CNetworkStringTable *, void, int>              ft_UpdateMirrorTable;
	static MemberFuncThunk<CNetworkStringTable *, void>                   ft_DeleteAllStrings;
};

class CNetworkStringTableItem
{
	struct itemchange_s {
		int				tick;
		int				length;
		unsigned char	*data;
	};
	unsigned char	*m_pUserData;
	int				m_nUserDataLength;
	int				m_nTickChanged;
	int				m_nTickCreated;
	CUtlVector< itemchange_s > *m_pChangeList;	
};

abstract_class IEntityFactory
{
public:
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( IServerNetworkable *pNetworkable ) = 0;
	virtual size_t GetEntitySize() = 0;
};

abstract_class IEntityFactoryDictionary
{
public:
	virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName ) = 0;
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable ) = 0;
	virtual IEntityFactory *FindFactory( const char *pClassName ) = 0;
	virtual const char *GetCannonicalName( const char *pClassName ) = 0;
};

class CEntityFactoryDictionary : public IEntityFactoryDictionary
{
public:
	CUtlDict< IEntityFactory *, unsigned short > m_Factories;
};


extern GlobalThunk<CHLTVServer *> hltv;

class CHostState
{
public:
	int	m_currentState;
};
extern GlobalThunk<CHostState> g_HostState;
#endif
