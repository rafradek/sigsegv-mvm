#include "mod.h"
#include "mod/util/notify_shared.h"
#include "util/socket_old.h"

// TODO: move to common.h
#include <con_nprint.h>


#define MSG_BEGIN() \
	bf_write *msg = sender.Begin();
#define MSG_END() \
	sender.End();


namespace Mod::Util::Notify_Send
{
	ConVar cvar_mtu("sig_util_notify_send_mtu", "65000", FCVAR_NOTIFY,
		"The maximum size of UDP packets to send through the firehose");
	ConVar cvar_delay("sig_util_notify_send_delay", "0", FCVAR_NOTIFY,
		"The number of server ticks to delay sending notify packets to the client");
	
	
	class NotifySend
	{
	public:
		struct Message
		{
			Message(size_t bits, const uint8_t *src, int delay) :
				bits(bits), delay(delay)
			{
				this->ptr = new uint8_t[BitByte(bits)];
				memcpy(this->ptr, src, BitByte(bits));
			}
			~Message()
			{
				if (this->ptr != nullptr) {
					delete[] this->ptr;
					this->ptr = nullptr;
				}
			}
			
			Message(const Message&) = delete;
			
			size_t bits;
			uint8_t *ptr = nullptr;
			int delay;
		};
		
		bf_write *Begin()
		{
			assert(!this->m_bInProgress);
			this->m_bInProgress = true;
			
			this->m_TempBuf.resize(cvar_mtu.GetInt());
			this->m_Writer.StartWriting(this->m_TempBuf.data(), this->m_TempBuf.size());
			
			return &this->m_Writer;
		}
		
		void End()
		{
			assert(this->m_bInProgress);
			this->m_bInProgress = false;
			
			int bits = this->m_Writer.GetNumBitsWritten();
			this->m_Queue.emplace_back(bits, this->m_TempBuf.data(), cvar_delay.GetInt());
		}
		
		void SendAll()
		{
			this->InitPacket();
			
			for (auto it = this->m_Queue.begin(); it != this->m_Queue.end(); ) {
				auto& msg = *it;
				
				if (msg.delay > 0) {
					--msg.delay;
					++it;
				} else {
					this->SendOne(msg);
					it = this->m_Queue.erase(it);
				}
			}
			
			this->Flush();
		}
		
		void Flush()
		{
			if (this->m_nPacketBits > HeaderBits()) {
				auto bits = reinterpret_cast<uint32_t *>(this->m_Packet.data());
				*bits = this->m_nPacketBits - HeaderBits();
				
				auto tick = reinterpret_cast<int *>(this->m_Packet.data()) + 1;
				*tick = gpGlobals->tickcount;
				
				Firehose_Send(NOTIFY_PORT, BitByte(this->m_nPacketBits), this->m_Packet.data());
			}
			
			this->InitPacket();
		}
		
	private:
		static constexpr int HeaderBytes() { return 8; }
		static constexpr int HeaderBits()  { return 64; }
		
		int DataBytesPerPacket() const
		{
			return cvar_mtu.GetInt() - HeaderBytes();
		}
		
		void InitPacket()
		{
			this->m_Packet.resize(cvar_mtu.GetInt());
			this->m_nPacketBits = HeaderBits();
		}
		
		void SendOne(const Message& msg)
		{
			assert(BitByte(msg.bits) <= this->DataBytesPerPacket());
			
			if (BitByte(this->m_nPacketBits + msg.bits) > this->DataBytesPerPacket()) {
				this->Flush();
			}
			
			bf_write dst(this->m_Packet.data(), this->m_Packet.size());
			dst.SeekToBit(this->m_nPacketBits);
			dst.WriteBits(msg.ptr, msg.bits);
			
			this->m_nPacketBits += msg.bits;
		}
		
		bool m_bInProgress = false;
		std::vector<uint8_t> m_TempBuf;
		bf_write m_Writer;
		
		std::list<Message> m_Queue;
		
		std::vector<uint8_t> m_Packet;
		int m_nPacketBits = 0;
	};
	NotifySend sender;
	
	
	DETOUR_DECL_STATIC(void, Con_NPrintf, int idx, const char *fmt, ...)
	{
		static std::unique_ptr<char[]> buf(new char[0x10000]);
		
		va_list va;
		va_start(va, fmt);
		V_vsnprintf(buf.get(), 0x10000, fmt, va);
		va_end(va);
		
		MSG_BEGIN();
			msg->WriteOneBit(NT_NPRINTF);
			msg->WriteLong  (idx);
			msg->WriteString(buf.get());
		MSG_END();
		
		DETOUR_STATIC_CALL(idx, "%s", buf.get());
	}
	
	DETOUR_DECL_MEMBER(void, IVEngineServer_Con_NPrintf, int pos, const char *fmt, ...)
	{
		static std::unique_ptr<char[]> buf(new char[0x10000]);
		
		va_list va;
		va_start(va, fmt);
		V_vsnprintf(buf.get(), 0x10000, fmt, va);
		va_end(va);
		
		MSG_BEGIN();
			msg->WriteOneBit(NT_NPRINTF);
			msg->WriteLong  (pos);
			msg->WriteString(buf.get());
		MSG_END();
		
		DETOUR_MEMBER_CALL(pos, "%s", buf.get());
	}
	
	DETOUR_DECL_MEMBER(void, IVEngineServer_Con_NXPrintf, const struct con_nprint_s *info, const char *fmt, ...)
	{
		static std::unique_ptr<char[]> buf(new char[0x10000]);
		
		va_list va;
		va_start(va, fmt);
		V_vsnprintf(buf.get(), 0x10000, fmt, va);
		va_end(va);
		
		MSG_BEGIN();
			msg->WriteOneBit  (NT_NXPRINTF);
			msg->WriteLong    (info->index);
			msg->WriteBitFloat(info->time_to_live);
			msg->WriteBitFloat(info->color[0]);
			msg->WriteBitFloat(info->color[1]);
			msg->WriteBitFloat(info->color[2]);
			msg->WriteOneBit  (info->fixed_width_font);
			msg->WriteString  (buf.get());
		MSG_END();
		
		DETOUR_MEMBER_CALL(info, "%s", buf.get());
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:Notify_Send")
		{
			MOD_ADD_DETOUR_STATIC(Con_NPrintf,                 "Con_NPrintf");
			MOD_ADD_DETOUR_MEMBER(IVEngineServer_Con_NPrintf,  "IVEngineServer::Con_NPrintf");
			MOD_ADD_DETOUR_MEMBER(IVEngineServer_Con_NXPrintf, "IVEngineServer::Con_NXPrintf");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			sender.SendAll();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_notify_send", "0", FCVAR_NOTIFY,
		"Utility: notify forwarding: server send",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
