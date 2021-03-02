#include "mod.h"
#include "util/backtrace.h"


class IServerMessageHandler;

class CNetMessage : public INetMessage
{
public:
	bool m_bReliable;
	INetChannel *m_NetChannel;
};

class SVC_UserMessage : public CNetMessage
{
public:
	IServerMessageHandler *m_pMessageHandler;
	int m_nMsgType;
	int m_nLength;
	bf_read m_DataIn;
	bf_write m_DataOut;
};

class CTFPlayer;
class CUpgradeInfo;


namespace Mod::Debug::UserMsg_Overflow
{
	DETOUR_DECL_MEMBER(void, CBaseServer_BroadcastMessage1, INetMessage& msg, bool onlyActive, bool reliable)
	{
		DevMsg("[BroadcastMessage1] \"%s\"\n", msg.GetName());
		if (stricmp(msg.GetName(), "svc_UserMessage") == 0) {
			auto usermsg = static_cast<SVC_UserMessage *>(&msg);
			DevMsg("  type %d\n", usermsg->m_nMsgType);
			DevMsg("  len  %d\n", usermsg->m_nLength);
		}
		
		DETOUR_MEMBER_CALL(CBaseServer_BroadcastMessage1)(msg, onlyActive, reliable);
	}
	
	DETOUR_DECL_MEMBER(void, CBaseServer_BroadcastMessage2, INetMessage& msg, IRecipientFilter& filter)
	{
		DevMsg("[BroadcastMessage2] \"%s\"\n", msg.GetName());
		if (stricmp(msg.GetName(), "svc_UserMessage") == 0) {
			auto usermsg = static_cast<SVC_UserMessage *>(&msg);
			DevMsg("  type %d\n", usermsg->m_nMsgType);
			DevMsg("  len  %d\n", usermsg->m_nLength);
			
			DevMsg("  m_DataIn.TotalBytesAvailable() = %d\n", usermsg->m_DataIn.TotalBytesAvailable());
			DevMsg("  m_DataOut.GetNumBytesWritten() = %d\n", usermsg->m_DataOut.GetNumBytesWritten());
			DevMsg("  m_DataOut.GetNumBytesLeft() = %d\n", usermsg->m_DataOut.GetNumBytesLeft());
			DevMsg("  m_DataOut.GetNumBitsWritten() = %d\n", usermsg->m_DataOut.GetNumBitsWritten());
		}
		
		DETOUR_MEMBER_CALL(CBaseServer_BroadcastMessage2)(msg, filter);
	}
	
	
	DETOUR_DECL_MEMBER(bool, SVC_UserMessage_WriteToBuffer, bf_write& buffer)
	{
		int bits_before = buffer.GetNumBitsWritten();
		auto result = DETOUR_MEMBER_CALL(SVC_UserMessage_WriteToBuffer)(buffer);
		int bits_after = buffer.GetNumBitsWritten();
		
		auto usermsg = reinterpret_cast<SVC_UserMessage *>(this);
		
		DevMsg("[W SVC_UserMessage] (buf:%5d->%-5d max:%-7d) type:%2d|%s bits:%-3d\n",
			bits_before, bits_after, buffer.GetMaxNumBits(),
			usermsg->m_nMsgType, g_SMAPI->GetUserMessage(usermsg->m_nMsgType), bits_after - bits_before);
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, SVC_UserMessage_ReadFromBuffer, bf_read& buffer)
	{
		int bits_before = buffer.GetNumBitsRead() - 6;
		auto result = DETOUR_MEMBER_CALL(SVC_UserMessage_ReadFromBuffer)(buffer);
		int bits_after = buffer.GetNumBitsRead();
		
		auto usermsg = reinterpret_cast<SVC_UserMessage *>(this);
		
		DevMsg("[R SVC_UserMessage] (buf:%5d->%-5d max:%-7d) type:%2d|%s bits:%-3d\n",
			bits_before, bits_after, buffer.m_nDataBits,
			usermsg->m_nMsgType, g_SMAPI->GetUserMessage(usermsg->m_nMsgType), bits_after - bits_before);
		
		return result;
	}
	
	DETOUR_DECL_STATIC(void, UserMessageBegin,IRecipientFilter &filter,const char *name)
	{
		DevMsg("UserMessage %s\n",name);
		//BACKTRACE();
		DETOUR_STATIC_CALL(UserMessageBegin)(filter,name);
	}

	DETOUR_DECL_MEMBER(void, CMannVsMachineStats_SendUpgradesToPlayer, CTFPlayer *player, CUtlVector<CUpgradeInfo> *upgrades)
	{
		DevMsg("CMannVsMachineStats::SendUpgradesToPlayer BEGIN\n");
		BACKTRACE();
		DETOUR_MEMBER_CALL(CMannVsMachineStats_SendUpgradesToPlayer)(player, upgrades);
		DevMsg("CMannVsMachineStats::SendUpgradesToPlayer END\n");
	}
	

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:UserMsg_Overflow")
		{
		//	MOD_ADD_DETOUR_MEMBER(CBaseServer_BroadcastMessage1, "CBaseServer::BroadcastMessage1");
		//	MOD_ADD_DETOUR_MEMBER(CBaseServer_BroadcastMessage2, "CBaseServer::BroadcastMessage2");
			
			MOD_ADD_DETOUR_MEMBER(SVC_UserMessage_WriteToBuffer,  "SVC_UserMessage::WriteToBuffer");
			MOD_ADD_DETOUR_MEMBER(SVC_UserMessage_ReadFromBuffer, "SVC_UserMessage::ReadFromBuffer");
			
			MOD_ADD_DETOUR_MEMBER(CMannVsMachineStats_SendUpgradesToPlayer, "CMannVsMachineStats::SendUpgradesToPlayer");

			MOD_ADD_DETOUR_STATIC(UserMessageBegin,                 "UserMessageBegin");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_usermsg_overflow", "0", FCVAR_NOTIFY,
		"Debug: buffer overflow in net message (svc_UserMessage)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
