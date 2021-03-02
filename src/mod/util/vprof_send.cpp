#include "mod.h"
#include "mod/util/vprof_shared.h"
#include "util/firehose_send.h"


/* class CVProfNode happens to declare class CVProfRecorder as a friend; since
 * we'd also like to be able to access private members, we can just "borrow"
 * the name of the friendly class and impersonate him; there's no risk of a
 * name collision with the real class CVProfRecorder because it's not public */
class CVProfRecorder
{
public:
	static size_t Serialize(std::vector<uint8_t>& buffer)
	{
		bf_write dst(buffer.data(), buffer.size());
		
		/* leave room for the 32-bit message size header at the beginning */
		dst.SeekToBit(32);
		
		Serialize_R(dst, g_VProfCurrentProfile.GetRoot(), true);
		
		size_t bytes = dst.GetNumBytesWritten();
		DevMsg("CVProfRecorder::Serialize: %d bytes\n", bytes);
		
		dst.SeekToBit(0);
		dst.WriteLong(bytes - 4);
		
		return bytes;
		
		/* TODO: put in safeguards to ensure that we won't mindlessly overflow
		 * our limited size buffer if there's a ton of data! */
	}
	static void Serialize_R(bf_write& dst, CVProfNode *node, bool is_root = false)
	{
		dst.WriteByte((is_root ? VP_ROOT : VP_NODE));
		{
			dst.WriteString(node->GetName());
			
			dst.WriteLong(node->GetUniqueNodeID());
			dst.WriteLong(node->GetBudgetGroupID());
			
			dst.WriteLong(node->m_nRecursions);
			
			dst.WriteLong(node->GetCurCalls());
			dst.WriteLong(node->GetPrevCalls());
			dst.WriteLong(node->GetTotalCalls());
			
			dst.WriteLongLong(node->m_CurFrameTime.GetLongCycles());
			dst.WriteLongLong(node->m_PrevFrameTime.GetLongCycles());
			dst.WriteLongLong(node->m_TotalTime.GetLongCycles());
			dst.WriteLongLong(node->m_PeakTime.GetLongCycles());
		}
		
		if (node->GetChild() != nullptr) {
			dst.WriteByte(VP_DOWN);
			Serialize_R(dst, node->GetChild());
			dst.WriteByte(VP_UP);
		}
		
		if (node->GetSibling() != nullptr) {
			Serialize_R(dst, node->GetSibling());
		}
	}
};


namespace Mod::Util::VProf_Send
{
	ConVar cvar_host("sig_util_vprof_send_host", "jgvm-win8", FCVAR_NOTIFY,
		"Network host to use for VProf transmission");
	ConVar cvar_port("sig_util_vprof_send_port", "30001", FCVAR_NOTIFY,
		"Network port to use for VProf transmission");
	
	
	// TODO:
	// pause profiling while we serialize/transmit, then resume it
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:VProf_Send") {}
		
		virtual void OnEnable() override
		{
			assert(this->m_Sender == nullptr);
			
			this->m_Sender = new FirehoseSend(cvar_host.GetString(), cvar_port.GetInt());
			
			this->m_Buffer.resize(VPROF_DATA_LIMIT);
		}
		virtual void OnDisable() override
		{
			assert(this->m_Sender != nullptr);
			
			delete this->m_Sender;
			this->m_Sender = nullptr;
			
			this->m_Buffer.clear();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			this->SendFrame();
		}
		
	private:
		void SendFrame()
		{
			assert(!this->m_Buffer.empty());
			
			size_t bytes = CVProfRecorder::Serialize(this->m_Buffer);
			this->m_Sender->Send(bytes, this->m_Buffer.data());
		}
		
		std::vector<uint8_t> m_Buffer;
		FirehoseSend *m_Sender = nullptr;
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_vprof_send", "0", FCVAR_NOTIFY,
		"Utility: vprof forwarding: server send",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
