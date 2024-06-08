#include "mod.h"
#include "stub/usermessages_sv.h"
#include "util/backtrace.h"


#define TRACE_ENABLE 1
#define TRACE_TERSE  1
#include "util/trace.h"


namespace Mod::Util::JumboFrames
{
	constexpr int MAX_ROUTABLE_PAYLOAD = 65000;
	
	
	/* horribly reentrant- and thread-unsafe */
	uint8_t packet[MAX_ROUTABLE_PAYLOAD];
	
	
#if defined _LINUX
	
	class CPatch_ConVar_Max : public CPatch
	{
	public:
		CPatch_ConVar_Max(const char *name, float old_max, float new_max) :
			CPatch(sizeof(float)), m_strName(name), m_flOldMax(old_max), m_flNewMax(new_max) {}
		
		virtual const char *GetFuncName() const override { return this->m_strName.c_str(); }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0040; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0040; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetFloat(0x00, this->m_flOldMax);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetFloat(0x00, this->m_flNewMax);
			mask.SetRange(0x00, 4, 0xff);
			return true;
		}
		
	private:
		std::string m_strName;
		float m_flOldMax;
		float m_flNewMax;
	};
	
	
	constexpr uint8_t buf_NET_SendPacket_default[] = {
		0xbe, 0xec, 0x04, 0x00, 0x00, // +0000  mov esi,1260
	};
	struct CPatch_Net_SendPacket_Default : public CPatch
	{
		CPatch_Net_SendPacket_Default() : CPatch(sizeof(buf_NET_SendPacket_default)) {}
		virtual const char *GetFuncName() const override { return "NET_SendPacket"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x1200; } // @ +0x0c00
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendPacket_default);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* replace the immediate value */
			buf.SetDword(0x00 + 1, MAX_ROUTABLE_PAYLOAD);
			mask.SetRange(0x00 + 1, 4, 0xff);
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendPacket_clamp[] = {
		0xb8, 0xec, 0x04, 0x00, 0x00,       // +0000  mov eax,0x4ec
		0x81, 0xfe, 0xec, 0x04, 0x00, 0x00, // +0005  cmp esi,0x4ec
		0x0f, 0x4d, 0xf0,                   // +000B  cmovge esi,eax
	};
	struct CPatch_Net_SendPacket_Clamp : public CPatch
	{
		CPatch_Net_SendPacket_Clamp() : CPatch(sizeof(buf_NET_SendPacket_clamp)) {}
		virtual const char *GetFuncName() const override { return "NET_SendPacket"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x1200; } // @ +0x0855
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendPacket_clamp);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the entire MIN operation */
			buf.SetRange(0x00, sizeof(buf_NET_SendPacket_clamp), 0x90);
			mask.SetRange(0x00, sizeof(buf_NET_SendPacket_clamp), 0xff);
			return true;
		}
	};
	
	
#if 0
	constexpr uint8_t buf_NET_SendLong_prologue[] = {
		0x55,                               // +0000  push ebp
		0x89, 0xe5,                         // +0001  mov esp,esp
		0x57,                               // +0003  push edi
		0x56,                               // +0004  push esi
		0x53,                               // +0005  push ebx
		0x81, 0xec, 0x8c, 0x05, 0x00, 0x00, // +0006  sub esp,0x58c
	};
	struct CPatch_Net_SendLong_Prologue : public CPatch
	{
		CPatch_Net_SendLong_Prologue() : CPatch(sizeof(buf_NET_SendLong_prologue)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_prologue);
			mask.SetRange(0x06 + 2, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			uint32_t old_size = buf.GetDword(0x06 + 2);
			if (old_size >= 0x600) return false;
			
			buf.SetDword(0x06 + 2, MAX_ROUTABLE_PAYLOAD + 0x800);
			mask.SetRange(0x06 + 2, 4, 0xff);
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_epilogue[] = {
		0x81, 0xc4, 0x8c, 0x05, 0x00, 0x00, // +0000  add esp,0x58c
		0x5b,                               // +0006  pop ebx
		0x5e,                               // +0007  pop esi
		0x5f,                               // +0008  pop edi
		0x5d,                               // +0009  pop ebp
		0xc3,                               // +000A  ret
	};
	struct CPatch_Net_SendLong_Epilogue : public CPatch
	{
		CPatch_Net_SendLong_Epilogue() : CPatch(sizeof(buf_NET_SendLong_epilogue)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x0497
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_epilogue);
			mask.SetRange(0x00 + 2, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			uint32_t old_size = buf.GetDword(0x00 + 2);
			if (old_size >= 0x600) return false;
			
			buf.SetDword(0x00 + 2, MAX_ROUTABLE_PAYLOAD + 0x800);
			mask.SetRange(0x00 + 2, 4, 0xff);
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_packet_ref1[] = {
		0xc7, 0x85, 0xd8, 0xfa, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, // +0000  mov dword ptr [ebp+packet],NET_HEADER_FLAG_SPLITPACKET
		0x8b, 0x85, 0xb4, 0xfa, 0xff, 0xff,                         // +0009  mov eax,[ebp+nSequenceNumber]
		0x66, 0x89, 0x9d, 0xe2, 0xfa, 0xff, 0xff,                   // +000F  mov word ptr [ebp+packet+0xa],bx
		0x85, 0xd2,                                                 // +0016  test edx,edx
		0x89, 0x85, 0xdc, 0xfa, 0xff, 0xff,                         // +0018  mov dword ptr [ebp+packet+0x4],eax
	};
	struct CPatch_Net_SendLong_PacketRef1 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef1() : CPatch(sizeof(buf_NET_SendLong_packet_ref1)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x0132
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref1);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x09 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x0f + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x18 + 2, 2, 0x00); // top 16 bits still masked in
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetDword(0x00 + 2, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0x0));
			mask.SetRange(0x00 + 2, 4, 0xff);
			buf.SetDword(0x0f + 3, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0xa));
			mask.SetRange(0x00 + 2, 4, 0xff);
			buf.SetDword(0x18 + 2, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0x4));
			mask.SetRange(0x00 + 2, 4, 0xff);
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_packet_ref2[] = {
		0x8d, 0x8d, 0xe4, 0xfa, 0xff, 0xff,       // +0000  lea ecx,[ebp+packet+0xc]
		0x89, 0xf8,                               // +0006  mov eax,edi
		0x8b, 0x9d, 0xd4, 0xfa, 0xff, 0xff,       // +0008  mov ebx,[ebp+????]
		0x89, 0x0c, 0x24,                         // +000E  mov [esp],ecx
		0x8b, 0x95, 0xd0, 0xfa, 0xff, 0xff,       // +0011  mov edx,[ebp+????]
		0x0f, 0x4e, 0xde,                         // +0017  cmovle ebx,esi
		0xc1, 0xe0, 0x08,                         // +001A  shl eax,8
		0x66, 0x03, 0x85, 0xbe, 0xfa, 0xff, 0xff, // +001D  add ax,[ebp+????]
		0x89, 0x54, 0x24, 0x04,                   // +0024  mov [esp+4],edx
		0x89, 0x5c, 0x24, 0x08,                   // +0028  mov [esp+8],ebx
		0x66, 0x89, 0x85, 0xe0, 0xfa, 0xff, 0xff, // +002C  mov word ptr [ebp+packet+0x8],ax
		0xe8, 0x85, 0x5e, 0x23, 0x00,             // +0033  call memcpy
	};
	struct CPatch_Net_SendLong_PacketRef2 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef2() : CPatch(sizeof(buf_NET_SendLong_packet_ref2)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x035b
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref2);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x08 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x11 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x1d + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x2c + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x33 + 1, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetDword(0x00 + 2, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0x0));
			mask.SetRange(0x00 + 2, 4, 0xff);
			buf.SetDword(0x2c + 3, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0x8));
			mask.SetRange(0x2c + 3, 4, 0xff);
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_packet_ref3[] = {
		0x8d, 0x95, 0xd8, 0xfa, 0xff, 0xff,             // +0000  lea edx,[ebp+packet]
		0xc7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,       // +0006  mov dword ptr [esp],0x0
		0x8b, 0x45, 0x1c,                               // +000D  mov eax,[ebp+????]
		0xc7, 0x44, 0x24, 0x18, 0xff, 0xff, 0xff, 0xff, // +0010  mov dword ptr [esp+0x18],0xffffffff
		0x89, 0x54, 0x24, 0x08,                         // +0018  mov [esp+0x8],edx
		0x89, 0x4c, 0x24, 0x14,                         // +001C  mov [esp+0x14],ecx
		0x8b, 0x4d, 0x10,                               // +0020  mov ecx,[ebp+????]
		0x89, 0x44, 0x24, 0x10,                         // +0023  mov [esp+0x10],eax
		0x8d, 0x43, 0x0c,                               // +0027  lea eax,[ebx+????]
		0x89, 0x44, 0x24, 0x0c,                         // +002A  mov [ebp+0xc],eax
		0x89, 0x4c, 0x24, 0x04,                         // +002E  mov [ebp+0x4],ecx
		0xe8, 0x09, 0xb0, 0xff, 0xff,                   // +0032  call NET_SendTo
	};
	struct CPatch_Net_SendLong_PacketRef3 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef3() : CPatch(sizeof(buf_NET_SendLong_packet_ref3)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x03c0
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref3);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x0d + 2, 1, 0x00);
			mask.SetRange(0x20 + 2, 1, 0x00);
			mask.SetRange(0x27 + 2, 1, 0x00);
			mask.SetRange(0x32 + 1, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.SetDword(0x00 + 2, -(0x700 + MAX_ROUTABLE_PAYLOAD + 0x0));
			mask.SetRange(0x00 + 2, 4, 0xff);
			return true;
		}
	};
#endif
	
	constexpr uint8_t buf_NET_SendLong_packet_ref1[] = {
		0xc7, 0x85, 0xd8, 0xfa, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, // +0000  mov dword ptr [ebp+packet],NET_HEADER_FLAG_SPLITPACKET
		0x8b, 0x85, 0xb4, 0xfa, 0xff, 0xff,                         // +0009  mov eax,[ebp+nSequenceNumber]
		0x66, 0x89, 0x9d, 0xe2, 0xfa, 0xff, 0xff,                   // +000F  mov word ptr [ebp+packet+0xa],bx
		0x85, 0xd2,                                                 // +0016  test edx,edx
		0x89, 0x85, 0xdc, 0xfa, 0xff, 0xff,                         // +0018  mov dword ptr [ebp+packet+0x4],eax
	};
	struct CPatch_Net_SendLong_PacketRef1 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef1() : CPatch(sizeof(buf_NET_SendLong_packet_ref1)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x0132
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref1);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x09 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x0f + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x18 + 2, 2, 0x00); // top 16 bits still masked in
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf[0x00 + 1] = 0x05;
			buf.SetDword(0x00 + 2, (uint32_t)(packet + 0x0));
			mask.SetRange(0x00 + 1, 5, 0xff);
			
			buf[0x0f + 2] = 0x1d;
			buf.SetDword(0x0f + 3, (uint32_t)(packet + 0xa));
			mask.SetRange(0x0f + 2, 5, 0xff);
			
			buf[0x18 + 0] = 0x90;
			buf[0x18 + 1] = 0xa3;
			buf.SetDword(0x18 + 2, (uint32_t)(packet + 0x4));
			mask.SetRange(0x18 + 0, 6, 0xff);
			
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_packet_ref2[] = {
		0x8d, 0x8d, 0xe4, 0xfa, 0xff, 0xff,       // +0000  lea ecx,[ebp+packet+0xc]
		0x89, 0xf8,                               // +0006  mov eax,edi
		0x8b, 0x9d, 0xd4, 0xfa, 0xff, 0xff,       // +0008  mov ebx,[ebp+????]
		0x89, 0x0c, 0x24,                         // +000E  mov [esp],ecx
		0x8b, 0x95, 0xd0, 0xfa, 0xff, 0xff,       // +0011  mov edx,[ebp+????]
		0x0f, 0x4e, 0xde,                         // +0017  cmovle ebx,esi
		0xc1, 0xe0, 0x08,                         // +001A  shl eax,8
		0x66, 0x03, 0x85, 0xbe, 0xfa, 0xff, 0xff, // +001D  add ax,[ebp+????]
		0x89, 0x54, 0x24, 0x04,                   // +0024  mov [esp+4],edx
		0x89, 0x5c, 0x24, 0x08,                   // +0028  mov [esp+8],ebx
		0x66, 0x89, 0x85, 0xe0, 0xfa, 0xff, 0xff, // +002C  mov word ptr [ebp+packet+0x8],ax
		0xe8, 0x85, 0x5e, 0x23, 0x00,             // +0033  call memcpy
	};
	struct CPatch_Net_SendLong_PacketRef2 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef2() : CPatch(sizeof(buf_NET_SendLong_packet_ref2)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x035b
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref2);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x08 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x11 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x1d + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x2c + 3, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x33 + 1, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf[0x00 + 1] = 0x0d;
			buf.SetDword(0x00 + 2, (uint32_t)(packet + 0x0));
			mask.SetRange(0x00 + 1, 5, 0xff);
			
			buf[0x2c + 0] = 0x90;
			buf[0x2c + 1] = 0x66;
			buf[0x2c + 2] = 0xa3;
			buf.SetDword(0x2c + 3, (uint32_t)(packet + 0x8));
			mask.SetRange(0x2c + 0, 7, 0xff);
			
			return true;
		}
	};
	
	constexpr uint8_t buf_NET_SendLong_packet_ref3[] = {
		0x8d, 0x95, 0xd8, 0xfa, 0xff, 0xff,             // +0000  lea edx,[ebp+packet]
		0xc7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,       // +0006  mov dword ptr [esp],0x0
		0x8b, 0x45, 0x1c,                               // +000D  mov eax,[ebp+????]
		0xc7, 0x44, 0x24, 0x18, 0xff, 0xff, 0xff, 0xff, // +0010  mov dword ptr [esp+0x18],0xffffffff
		0x89, 0x54, 0x24, 0x08,                         // +0018  mov [esp+0x8],edx
		0x89, 0x4c, 0x24, 0x14,                         // +001C  mov [esp+0x14],ecx
		0x8b, 0x4d, 0x10,                               // +0020  mov ecx,[ebp+????]
		0x89, 0x44, 0x24, 0x10,                         // +0023  mov [esp+0x10],eax
		0x8d, 0x43, 0x0c,                               // +0027  lea eax,[ebx+????]
		0x89, 0x44, 0x24, 0x0c,                         // +002A  mov [ebp+0xc],eax
		0x89, 0x4c, 0x24, 0x04,                         // +002E  mov [ebp+0x4],ecx
		0xe8, 0x09, 0xb0, 0xff, 0xff,                   // +0032  call NET_SendTo
	};
	struct CPatch_Net_SendLong_PacketRef3 : public CPatch
	{
		CPatch_Net_SendLong_PacketRef3() : CPatch(sizeof(buf_NET_SendLong_packet_ref3)) {}
		virtual const char *GetFuncName() const override { return "NET_SendLong"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0700; } // @ +0x03c0
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(buf_NET_SendLong_packet_ref3);
			mask.SetRange(0x00 + 2, 2, 0x00); // top 16 bits still masked in
			mask.SetRange(0x0d + 2, 1, 0x00);
			mask.SetRange(0x20 + 2, 1, 0x00);
			mask.SetRange(0x27 + 2, 1, 0x00);
			mask.SetRange(0x32 + 1, 4, 0x00);
			return true;
		}
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf[0x00 + 1] = 0x15;
			buf.SetDword(0x00 + 2, (uint32_t)(packet + 0x0));
			mask.SetRange(0x00 + 1, 5, 0xff);
			
			return true;
		}
	};
	
#endif
	
	
	// net_maxfragments (actual: net_blocksize)
	// net_maxroutable
	// sv_maxroutable
	
	// MAX_ROUTABLE_PAYLOAD
	// NET_MAX_MESSAGE
	
	
	
	// cvar net_udp_rcvbuf
	// not important
	
	// cvar net_blocksize
	// default: 1260.0
	// max:     1260.0
	// min:      256.0
	// referenced in CNetChan::Setup
	// in singleplayer, 288000 is used instead
	
	// cvar net_maxroutable
	// default: 1260.0
	// max:     1260.0
	// min:      576.0
	// referenced:
	// - CBaseClient::UpdateUserSettings => CBaseClient::SetMaxRoutablePayloadSize
	// - CNetChan::SendDatagram          => CNetChan::SetMaxRoutablePayloadSize
	
	// constant NET_MAX_PAYLOAD
	// Source2007:  96000
	// Source2013: 288000
	
	// constant NET_MAX_MESSAGE
	// Source2007:  96016
	// Source2013: 288016
	
	// constant MAX_ROUTABLE_PAYLOAD
	// Source2007: 1260
	// Source2013: 1260 (assumed)
	// referenced (Source2007):
	// - CBaseClientState::SendConnectPacket           [not important: limits cl->sv initial connection]
	// - CBaseClientState::ProcessConnectionlessPacket [not important: limits bf_read.ReadString buffer size]
	// - CMatchmaking::HandleSystemLinkReply           [not important]
	// - CMatchmaking::UpdateSearch                    [not important]
	// - CMatchmaking::SendJoinRequest                 [not important]
	// - CMatchmaking::HandleSystemLinkSearch          [not important]
	// - CBaseClient::UpdateUserSettings               [not important: provides default value for net_maxroutable.GetInt]
	// - net_blocksize                                 [not important: provides max value for net_blocksize]
	// - CNetChan::CNetChan                            [not important: initial value for m_nMaxRoutablePayloadSize]
	// - NET_SendTo                                    [not important: debug-only console warning]
	// - NET_SendLong                                  [important: sets size of stack local 'packet']
	// - NET_SendPacket                                [important: sets nMaxRoutable if chan is nullptr]
	// - NET_OutOfBandPrintf                           [not important: limits string length]
	
	// constant MAX_USER_MAXROUTABLE_SIZE
	// referenced in NET_SendPacket [important: clamps nMaxRoutable]
	
	
	
	
	
	// we can totally get away with just expanding the limits for packet send on
	// the server side, since that's the direction where the most data is going
	
	
	
	
	// NET_SendLong: increase the stack allocation for local variable 'packet'
	// - currently:
	//   - esp adjustment: 0x58c
	//   - packet @ [ebp-0x528]
	// - improved for MAX_ROUTABLE_PAYLOAD up to 65536:
	//   - esp adjustment: 0x58c + 0x10000 + 0x400
	//   - packet @ [ebp-0x10700]
	// - NOTE: need to adjust esp at func epilogue too! (because dumb)
	
	// NET_SendPacket: override the clamping comparisons
	// - replace 3 instances of immediate value 0x4ec:
	//   - mov eax,0x4ec
	//     cmp esi,0x4ec
	//   - mov esi,0x4ec
	
	// convar net_blocksize: override the maximum value
	
	// convar net_maxroutable: override the maximum value
	
	// convar sv_maxroutable: override the maximum value
	
	
	typedef int SOCKET;
	
	
	DETOUR_DECL_STATIC(int, NET_SendPacket, INetChannel *chan, int sock, const netadr_t& to, const unsigned char *data, int length, bf_write *pVoicePayload, bool bUseCompression)
	{
		TRACE("[chan: %08x] [sock: %d] [length: %d] [bUseCompression: %s]",
			(uintptr_t)chan,
			sock,
			length,
			(bUseCompression ? "true" : "false"));
		
		return DETOUR_STATIC_CALL(chan, sock, to, data, length, pVoicePayload, bUseCompression);
	}
	
	DETOUR_DECL_STATIC(int, NET_SendLong, INetChannel *chan, int sock, SOCKET s, const char *buf, int len, const struct sockaddr *to, int tolen, int nMaxRoutableSize)
	{
		TRACE("[chan: %08x] [sock: %d] [len: %d] [nMaxRoutableSize: %d]",
			(uintptr_t)chan,
			sock,
			len,
			nMaxRoutableSize);
		
		return DETOUR_STATIC_CALL(chan, sock, s, buf, len, to, tolen, nMaxRoutableSize);
	}
	
	DETOUR_DECL_STATIC(int, NET_SendTo, bool verbose, SOCKET s, const char *buf, int len, const struct sockaddr *to, int tolen, int iGameDataLength)
	{
		TRACE("[len: %d] [iGameDataLength: %d]",
			len,
			iGameDataLength);
		
		return DETOUR_STATIC_CALL(verbose, s, buf, len, to, tolen, iGameDataLength);
	}
	
	DETOUR_DECL_MEMBER(int, CNetChan_GetMaxRoutablePayloadSize)
	{
		static ConVarRef net_maxroutable("net_maxroutable");
		
		//auto result = DETOUR_MEMBER_CALL();
		auto result = net_maxroutable.GetInt();
		
		TRACE("%d", result);
		return result;
	}
	
	
	// FIXME: net_maxroutable isn't actually changeable on the dedicated server
	// because it has FCVAR_USERINFO
	// (sm_cvar can change it though)
	
	// FIXME: CNetChan::m_MaxReliablePayloadSize is set to net_blocksize.GetInt,
	// but only when the net channel is initially setup
	// so you have to set net_maxfragments, then do net_setup, for the server to
	// actually send >1260 bytes of reliable data per packet
	
	// FIXME: client apparently needs to also have some tweaks, because when it
	// receives a massive packet, it still only believes that there are ~4
	// fragments in it, instead of ~250 fragments, so it skips data and fails to
	// complete the connection process
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:JumboFrames")
		{
			this->AddPatch(new CPatch_ConVar_Max("net_blocksize",   1260.0f, (float)MAX_ROUTABLE_PAYLOAD));
			this->AddPatch(new CPatch_ConVar_Max("net_maxroutable", 1260.0f, (float)MAX_ROUTABLE_PAYLOAD));
			this->AddPatch(new CPatch_ConVar_Max("sv_maxroutable",  1260.0f, (float)MAX_ROUTABLE_PAYLOAD));
			
			this->AddPatch(new CPatch_Net_SendPacket_Default());
			this->AddPatch(new CPatch_Net_SendPacket_Clamp());
			
//			this->AddPatch(new CPatch_Net_SendLong_Prologue());
			this->AddPatch(new CPatch_Net_SendLong_PacketRef1());
			this->AddPatch(new CPatch_Net_SendLong_PacketRef2());
			this->AddPatch(new CPatch_Net_SendLong_PacketRef3());
//			this->AddPatch(new CPatch_Net_SendLong_Epilogue());
			
			MOD_ADD_DETOUR_STATIC(NET_SendPacket, "NET_SendPacket");
			MOD_ADD_DETOUR_STATIC(NET_SendLong,   "NET_SendLong");
			MOD_ADD_DETOUR_STATIC(NET_SendTo,     "NET_SendTo");
			
			MOD_ADD_DETOUR_MEMBER(CNetChan_GetMaxRoutablePayloadSize, "CNetChan::GetMaxRoutablePayloadSize");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_jumboframes", "0", FCVAR_NOTIFY,
		"Utility: override packet size limits",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_util_jumboframes_blastdata, "")
	{
		int size = std::stoi(args[1], nullptr, 0);
		
		CReliableBroadcastRecipientFilter filter;
		
		for (int i = 0; i < size; i += 3) {
			bf_write *msg = engine->UserMessageBegin(&filter, 58);
			if (msg != nullptr) {
				engine->MessageEnd();
			}
		}
	}
}
