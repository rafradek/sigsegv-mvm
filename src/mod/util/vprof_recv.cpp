#include "mod.h"
#include "mod/util/vprof_shared.h"
#include "util/firehose_recv.h"


//void ResetActiveVProfile()
//{
//	// set g_pVProfileForDisplay to &g_VProfCurrentProfile
//}
//void SetActiveVProfile(CVProfile *profile)
//{
//	// set g_pVProfileForDisplay
//}
//CVProfile *GetActiveVProfile()
//{
//	// get g_pVProfileForDisplay
//}


/* class CVProfNode happens to declare class CVProfRecorder as a friend; since
 * we'd also like to be able to access private members, we can just "borrow"
 * the name of the friendly class and impersonate him; there's no risk of a
 * name collision with the real class CVProfRecorder because it's not public */
class CVProfRecorder
{
public:
	static void Deserialize(const std::vector<uint8_t>& buffer)
	{
		bf_read src(buffer.data(), buffer.size());
		
		Deserialize_R(src, g_VProfCurrentProfile.GetRoot());
	}
	static void Deserialize_R(bf_read& src, CVProfNode *parent)
	{
		CVProfNode *node = nullptr;
		
		while (src.GetNumBytesLeft() > 0) {
			uint8_t msg = src.ReadByte();
			
			if (msg == VP_ROOT || msg == VP_NODE) {
				if (msg == VP_ROOT) {
					/* gasp, placement new! */
					g_VProfCurrentProfile.GetRoot()->~CVProfNode();
					node = new (g_VProfCurrentProfile.GetRoot()) CVProfNode("", 0, nullptr, "", 0);
					
				//	assert(node->m_pParent  == nullptr);
				//	assert(node->m_pSibling == nullptr);
				//	assert(node->m_pChild   == nullptr);
				} else {
					node = new CVProfNode("", 0, parent, "", 0);
					
					node->m_pSibling = parent->m_pChild;
					parent->m_pChild = node;
				}
				
				char name[2048];
				src.ReadString(name, sizeof(name));
				node->m_pszName = s_NamePool.Allocate(name);
				
				node->SetUniqueNodeID (src.ReadLong());
				node->SetBudgetGroupID(src.ReadLong());
				
				node->m_nRecursions = src.ReadLong();
				
				node->m_nCurFrameCalls  = src.ReadLong();
				node->m_nPrevFrameCalls = src.ReadLong();
				node->m_nTotalCalls     = src.ReadLong();
				
				node->m_CurFrameTime  = CCycleCount(src.ReadLongLong());
				node->m_PrevFrameTime = CCycleCount(src.ReadLongLong());
				node->m_TotalTime     = CCycleCount(src.ReadLongLong());
				node->m_PeakTime      = CCycleCount(src.ReadLongLong());
			} else if (msg == VP_DOWN) {
				assert(node != nullptr);
				Deserialize_R(src, node);
			} else if (msg == VP_UP) {
				return;
			} else {
				assert(false);
				// TODO: complain loudly and short-circuit the whole recursion
				// but don't crash the whole damn thing
			}
		}
	}
	
	static void FreeNodes()
	{
		FreeNodes_R(g_VProfCurrentProfile.GetRoot(), true);
	}
	static void FreeNodes_R(CVProfNode *node, bool is_root = false)
	{
		CVProfNode *next;
		for (CVProfNode *child = node->GetChild(); child != nullptr; child = next) {
			next = child->GetSibling();
			FreeNodes_R(child);
		}
		
		if (is_root) {
			node->m_pChild = nullptr;
		} else {
			delete node;
		}
	}
	
	static void TreePre()
	{
	//	g_VProfCurrentProfile.Stop();
	//	g_VProfCurrentProfile.Reset();
	//	FreeNodes();
		
	//	g_VProfCurrentProfile.~CVProfile();
	//	new (&g_VProfCurrentProfile) CVProfile();
		
		// TODO: do GetVProfPanel()->Reset() to clear out old cruft in the GUI
	}
	static void TreePost()
	{
	//	g_VProfCurrentProfile.Start();
		g_VProfCurrentProfile.Pause();
	}
	
	static CStringPool s_NamePool;
};
CStringPool CVProfRecorder::s_NamePool;


// TODO: clear out the string pool at certain points
// - on enable?
// - on disable?
// - on every received update?


namespace Mod::Util::VProf_Recv
{
	ConVar cvar_port("sig_util_vprof_recv_port", "30001", FCVAR_NOTIFY,
		"Network port to use for VProf reception");
	ConVar cvar_trace("sig_util_vprof_recv_trace", "0", FCVAR_NOTIFY,
		"Trace vprof messages as they are received");
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:VProf_Recv") {}
		
		virtual void OnEnable() override
		{
			assert(this->m_Recver == nullptr);
			
			this->m_Recver = new FirehoseRecv(cvar_port.GetInt());
			
			this->m_Buffer.clear();
		}
		virtual void OnDisable() override
		{
			assert(this->m_Recver != nullptr);
			
			delete this->m_Recver;
			this->m_Recver = nullptr;
			
			this->m_Buffer.clear();
			
			// TODO: completely clear out the active CVProfile
			// because if we free the string pool and/or get unloaded,
			// then the stuff we put in the tree might point into segfault land
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			this->RecvFrame();
		}
		
	private:
		void RecvFrame()
		{
			DevMsg("RecvFrame: begin\n");
			
			size_t avail = this->m_Recver->Available();
			
			size_t total;
			
			if (avail < sizeof(total)) {
				DevMsg("RecvFrame: %u bytes are available, but we need 4+ for header\n", avail);
				return;
			}
			
			if (this->m_Recver->Recv(sizeof(total), reinterpret_cast<uint8_t *>(&total), true) < sizeof(total)) {
				DevMsg("RecvFrame: 4+ bytes are available, but we somehow couldn't read all of them\n");
				return;
			}
			
			if (avail < sizeof(total) + total) {
				DevMsg("RecvFrame: %u bytes are available, but we need %u for the entire message\n", avail, sizeof(total) + total);
				return;
			}
			
			this->m_Recver->Skip(sizeof(total));
			
			DevMsg("RecvFrame: resizing buffer to fit %u bytes\n", total);
			this->m_Buffer.resize(total);
			
			DevMsg("RecvFrame: starting recv loop\n");
			for (size_t recvd = 0; recvd < total; ) {
				recvd += this->m_Recver->Recv((total - recvd), this->m_Buffer.data() + recvd);
				DevMsg("RecvFrame: after recv, progress is now %u/%u\n", recvd, total);
			}
			DevMsg("RecvFrame: finished recv loop\n");
			
			// now: we should have a buffer whose size is exactly right and whose
			// contents are guaranteed to consist of an entire frame message
			
			CVProfRecorder::TreePre();
			CVProfRecorder::Deserialize(this->m_Buffer);
			CVProfRecorder::TreePost();
			
			DevMsg("RecvFrame: end\n");
		}
		
		std::vector<uint8_t> m_Buffer;
		FirehoseRecv *m_Recver = nullptr;
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_vprof_recv", "0", FCVAR_NOTIFY,
		"Utility: vprof forwarding: client receive",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	// FIXME: IFrameListener doesn't work on windows
	
	
	CON_COMMAND(sig_util_vprof_recv_kick, "")
	{
		DevMsg("Kicking vprof receiver\n");
		s_Mod.FrameUpdatePostEntityThink();
	}
	
	CON_COMMAND(sig_util_vprof_recv_pause, "")
	{
		DevMsg("g_VProfCurrentProfile.Pause()\n");
		g_VProfCurrentProfile.Pause();
	}
	CON_COMMAND(sig_util_vprof_recv_resume, "")
	{
		DevMsg("g_VProfCurrentProfile.Resume()\n");
		g_VProfCurrentProfile.Resume();
	}
	CON_COMMAND(sig_util_vprof_recv_start, "")
	{
		DevMsg("g_VProfCurrentProfile.Start()\n");
		g_VProfCurrentProfile.Start();
	}
	CON_COMMAND(sig_util_vprof_recv_stop, "")
	{
		DevMsg("g_VProfCurrentProfile.Stop()\n");
		g_VProfCurrentProfile.Stop();
	}
}
