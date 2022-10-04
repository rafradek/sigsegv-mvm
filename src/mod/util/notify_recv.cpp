#include "mod.h"
#include "mod/util/notify_shared.h"
#include "util/socket_old.h"

// TODO: move to common.h
#include <con_nprint.h>


namespace Mod::Util::Notify_Recv
{
	ConVar cvar_trace("sig_util_notify_recv_trace", "0", FCVAR_NOTIFY,
		"Trace notify messages as they are received");
	
	
	void Handle_Con_NPrintf(bf_read& msg)
	{
		int idx         = msg.ReadLong();
		const char *str = msg.ReadAndAllocateString();
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Con_NPrintf] %d \"%s\"\n",
				idx, str);
		}
		
		engine->Con_NPrintf(idx, "%s", str);
		
		delete[] str;
	}
	
	void Handle_Con_NXPrintf(bf_read& msg)
	{
		con_nprint_s info;
		info.index            = msg.ReadLong();
		info.time_to_live     = msg.ReadBitFloat();
		info.color[0]         = msg.ReadBitFloat();
		info.color[1]         = msg.ReadBitFloat();
		info.color[2]         = msg.ReadBitFloat();
		info.fixed_width_font = msg.ReadOneBit();
		const char *str = msg.ReadAndAllocateString();
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Con_NXPrintf] %d %5.3f <%5.3f %5.3f %5.3f> %d \"%s\"\n",
				info.index, info.time_to_live, info.color[0], info.color[1], info.color[2], info.fixed_width_font, str);
		}
		
		engine->Con_NXPrintf(&info, "%s", str);
		
		delete[] str;
	}
	
	
	void Hook_Notifys(bf_read& msg)
	{
		switch (msg.ReadOneBit()) {
			
		case NT_NPRINTF:  Handle_Con_NPrintf (msg); break;
		case NT_NXPRINTF: Handle_Con_NXPrintf(msg); break;
			
		default:
			Warning("Unknown notify type\n");
			break;
		}
	}
	
	
	struct DelayedPacket
	{
		DelayedPacket(int bits, int tick, const uint8_t *src) :
			bits(bits), tick(tick)
		{
			this->ptr = new uint8_t[BitByte(bits)];
			memcpy(this->ptr, src, BitByte(bits));
		}
		~DelayedPacket()
		{
			if (this->ptr != nullptr) {
				delete[] this->ptr;
				this->ptr = nullptr;
			}
		}
		
		DelayedPacket(const DelayedPacket&) = delete;
		
		int bits;
		int tick;
		uint8_t *ptr = nullptr;
	};
	
	std::list<DelayedPacket> queued_packets;
	
	
	void ProcessPacket(const DelayedPacket& packet)
	{
		bf_read src(packet.ptr, BitByte(packet.bits), packet.bits);
		
		while (src.GetNumBitsRead() < packet.bits) {
			Hook_Notifys(src);
		}
	}
	
	void ProcessQueuedPackets()
	{
		for (auto it = queued_packets.begin(); it != queued_packets.end(); ) {
			const auto& packet = *it;
			
			if (packet.tick <= enginetools->ClientTick()) {
				ProcessPacket(packet);
				it = queued_packets.erase(it);
			} else {
				++it;
			}
		}
	}
	
	
	FirehoseRecv *recv = nullptr;
	void ReceivePackets()
	{
		static std::vector<uint8_t> buffer;
		buffer.resize(65536);
		
		size_t len;
		while ((len = recv->Recv(buffer.size(), buffer.data())) != 0) {
			bf_read hdr(buffer.data(), buffer.size());
			
			uint32_t bits = hdr.ReadUBitLong(32);
			uint32_t tick = hdr.ReadUBitLong(32);
			
			queued_packets.emplace_back(bits, tick, buffer.data() + 8);
		}
	}
	
	
	DETOUR_DECL_STATIC(void, CDebugOverlay_DrawAllOverlays)
	{
		ReceivePackets();
		ProcessQueuedPackets();
		
		DETOUR_STATIC_CALL(CDebugOverlay_DrawAllOverlays)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Notify_Recv")
		{
			// ideally we'd detour something else but actually this should be fine for us
			MOD_ADD_DETOUR_STATIC(CDebugOverlay_DrawAllOverlays, "[client] CDebugOverlay::DrawAllOverlays");
		}
		
		virtual void OnEnable() override
		{
			recv = new FirehoseRecv(NOTIFY_PORT);
		}
		virtual void OnDisable() override
		{
			delete recv;
			recv = nullptr;
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_notify_recv", "0", FCVAR_NOTIFY,
		"Utility: notify forwarding: client receive",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
