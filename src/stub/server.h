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
	virtual void UpdateUserSettings();
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
	int				m_nDeltaTick;
	int				m_nStringTableAckTick;
	int				m_nSignonTick;
	void * m_pLastSnapshot;
	int pad;

	CFrameSnapshot	*m_pBaseline;
	int				m_nBaselineUpdateTick;
	CBitVec<MAX_EDICTS>	m_BaselinesSent;
	int				m_nBaselineUsed;

	int				m_nForceWaitForTick;
	
	bool			m_bFakePlayer;
};

class CBaseServer : public IServer
{
public:
    CBaseClient *CreateFakeClient(const char *name) { return ft_CreateFakeClient(this, name); }
	
	float    GetCPUUsage() { return vt_GetCPUUsage(this); }
    int &GetMaxClientsRef() {return *(int *)((uintptr_t)(this) + 0x14C);}
private:
    static MemberFuncThunk<CBaseServer *, CBaseClient *, const char *>              ft_CreateFakeClient;
	
	static MemberVFuncThunk<CBaseServer *, float>              vt_GetCPUUsage;
};

class CGameServer : public CBaseServer
{
public:
	int GetNumEdicts() {return *(int *)((uintptr_t)(this) + 0x1E4);}
};

class CHLTVServer : public IGameEventListener2, public CBaseServer
{
public:
    void StartMaster(CBaseClient *client)  {        ft_StartMaster(this, client); }
    int CountClientFrames()                { return ft_CountClientFrames(this); }
    void RemoveOldestFrame()               {        ft_RemoveOldestFrame(this); }
    
private:
    static MemberFuncThunk<CHLTVServer *, void, CBaseClient *> ft_StartMaster;
    static MemberFuncThunk<CHLTVServer *, int>                 ft_CountClientFrames;
    static MemberFuncThunk<CHLTVServer *, void>                ft_RemoveOldestFrame;
};

class CGameClient 
{
public:
    bool ShouldSendMessages()                      { return ft_ShouldSendMessages(this); }
	//bool SpawnServer(const char *szMapName, const char *szMapFile, const char *startspot)  { return ft_SpawnServer(this, szMapName, szMapFile, startspot); }
    
private:
    static MemberFuncThunk<CGameClient *, bool>              ft_ShouldSendMessages;
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

extern GlobalThunk<CHLTVServer *> hltv;
#endif
