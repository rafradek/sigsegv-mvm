#include "mod.h"
#include "util/scope.h"


namespace Mod::Demo::StringTable_Limit
{
	// CDemoPlayer::ReadPacket
	// mem alloc: 0x80000
	// bf_read ctor: 0x80000
	
	// CHLTVServer::ReadCompeleteDemoFile
	// mem alloc: 0x80000
	// bf_read ctor: 0x80000
	
	// CDemoRecorder::RecordStringTables
	// CUtlBuffer::EnsureCapacity: 0x80000
	// bf_write ctor: 0x80000
	
	// CHLTVDemoRecorder::RecordStringTables
	// mem alloc: 0x80000
	// bf_write ctor: 0x80000
	
	// CReplayDemoRecorder::RecordStringTables
	// operator new[]: 0x80000
	// bf_write ctor: 0x80000
	
	
	RefCount rc_CDemoPlayer_ReadPacket;
	DETOUR_DECL_MEMBER(netpacket_t *, CDemoPlayer_ReadPacket)
	{
		SCOPED_INCREMENT(rc_CDemoPlayer_ReadPacket);
		return DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_CHLTVServer_ReadCompeleteDemoFile;
	DETOUR_DECL_MEMBER(void, CHLTVServer_ReadCompeleteDemoFile)
	{
		SCOPED_INCREMENT(rc_CHLTVServer_ReadCompeleteDemoFile);
		DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_CDemoRecorder_RecordStringTables;
	DETOUR_DECL_MEMBER(void, CDemoRecorder_RecordStringTables)
	{
		SCOPED_INCREMENT(rc_CDemoRecorder_RecordStringTables);
		DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_CHLTVDemoRecorder_RecordStringTables;
	DETOUR_DECL_MEMBER(void, CHLTVDemoRecorder_RecordStringTables)
	{
		SCOPED_INCREMENT(rc_CHLTVDemoRecorder_RecordStringTables);
		DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_CReplayDemoRecorder_RecordStringTables;
	DETOUR_DECL_MEMBER(void, CReplayDemoRecorder_RecordStringTables)
	{
		SCOPED_INCREMENT(rc_CReplayDemoRecorder_RecordStringTables);
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void *, CStdMemAlloc_Alloc, size_t nSize)
	{
		if (nSize == 0x80000) {
			if (rc_CDemoPlayer_ReadPacket > 0) {
				DevMsg("[DEMO] CDemoPlayer::ReadPacket -> CStdMemAlloc::Alloc\n");
				nSize *= 2;
			} else if (rc_CHLTVServer_ReadCompeleteDemoFile > 0) {
				DevMsg("[DEMO] CHLTVServer::ReadCompeleteDemoFile -> CStdMemAlloc::Alloc\n");
				nSize *= 2;
			} else if (rc_CHLTVDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CHLTVDemoRecorder::RecordStringTables -> CStdMemAlloc::Alloc\n");
				nSize *= 2;
			} else if (rc_CReplayDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CReplayDemoRecorder::RecordStringTables -> CStdMemAlloc::Alloc\n");
				nSize *= 2;
			}
		}
		
		return DETOUR_MEMBER_CALL(nSize);
	}
	
	DETOUR_DECL_MEMBER(void, CUtlBuffer_EnsureCapacity, int num)
	{
		if (num == 0x80000) {
			if (rc_CDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CDemoPlayer::RecordStringTables -> CUtlBuffer::EnsureCapacity\n");
				num *= 2;
			}
		}
		
		DETOUR_MEMBER_CALL(num);
	}
	
	DETOUR_DECL_MEMBER(void, bf_read_ctor, const char *pDebugName, const void *pData, int nBytes, int nBits)
	{
		if (nBytes == 0x80000) {
			if (rc_CDemoPlayer_ReadPacket > 0) {
				DevMsg("[DEMO] CDemoPlayer::ReadPacket -> bf_read::bf_read\n");
				nBytes *= 2;
			} else if (rc_CHLTVServer_ReadCompeleteDemoFile > 0) {
				DevMsg("[DEMO] CHLTVServer::ReadCompeleteDemoFile -> bf_read::bf_read\n");
				nBytes *= 2;
			}
		}
		
		DETOUR_MEMBER_CALL(pDebugName, pData, nBytes, nBits);
	}
	
	DETOUR_DECL_MEMBER(void, bf_write_ctor, void *pData, int nBytes, int nMaxBits)
	{
		if (nBytes == 0x80000) {
			if (rc_CDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CDemoRecorder::RecordStringTables -> bf_write::bf_write\n");
				nBytes *= 2;
			} else if (rc_CHLTVDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CHLTVDemoRecorder::RecordStringTables -> bf_write::bf_write\n");
				nBytes *= 2;
			} else if (rc_CReplayDemoRecorder_RecordStringTables > 0) {
				DevMsg("[DEMO] CReplayDemoRecorder::RecordStringTables -> bf_write::bf_write\n");
				nBytes *= 2;
			}
		}
		
		DETOUR_MEMBER_CALL(pData, nBytes, nMaxBits);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Demo:StringTable_Limit")
		{
		//	MOD_ADD_DETOUR_MEMBER(CDemoPlayer_ReadPacket,                 "CDemoPlayer::ReadPacket");
		//	MOD_ADD_DETOUR_MEMBER(CHLTVServer_ReadCompeleteDemoFile,      "CHLTVServer::ReadCompeleteDemoFile");
		//	MOD_ADD_DETOUR_MEMBER(CDemoRecorder_RecordStringTables,       "CDemoRecorder::RecordStringTables");
			MOD_ADD_DETOUR_MEMBER(CHLTVDemoRecorder_RecordStringTables,   "CHLTVDemoRecorder::RecordStringTables");
			MOD_ADD_DETOUR_MEMBER(CReplayDemoRecorder_RecordStringTables, "CReplayDemoRecorder::RecordStringTables");
			
			MOD_ADD_DETOUR_MEMBER(CStdMemAlloc_Alloc,        "CStdMemAlloc::Alloc");
			MOD_ADD_DETOUR_MEMBER(CUtlBuffer_EnsureCapacity, "[engine] CUtlBuffer::EnsureCapacity");
			MOD_ADD_DETOUR_MEMBER(bf_read_ctor,              "[engine] bf_read::bf_read");
			MOD_ADD_DETOUR_MEMBER(bf_write_ctor,             "[engine] bf_write::bf_write");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_demo_stringtable_limit", "0", FCVAR_NOTIFY,
		"Mod: override 512KB string table limit in demos",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
