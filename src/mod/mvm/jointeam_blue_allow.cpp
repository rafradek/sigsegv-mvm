#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "stub/objects.h"
#include "stub/tf_player_resource.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "util/clientmsg.h"
#include "util/admin.h"
#include "util/iterate.h"
#include "stub/populators.h"
#include "mod/pop/popmgr_extensions.h"
#include "stub/sendprop.h"
#include "stub/server.h"
#include "mod/mvm/player_limit.h"
#include "mod/common/text_hud.h"
#include "mod/etc/sendprop_override.h"

// TODO: move to common.h
#include <igamemovement.h>
#include <in_buttons.h>

class CLaserDot {};

namespace Mod::MvM::JoinTeam_Blue_Allow
{
	using CollectPlayersFunc_t = int (*)(CUtlVector<CTFPlayer *> *, int, bool, bool);
	using CollectPlayersFuncRegcall_t = int (*)(CUtlVector<CTFPlayer *> *, int, bool, bool) __gcc_regcall;
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller1[] = {
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0008  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +0010  mov dword ptr [esp+0x4],<int:team>
		0x89, 0x04, 0x24,                               // +0018  mov [esp],exx
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +001B  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0022  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0029  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0030  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0037  mov [ebp-0xXXX],0x00000000
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +003E  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller1 : public CPatch
	{
		CPatch_CollectPlayers_Caller1(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller1)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller1);
			
			buf.SetDword(0x00 + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x08 + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x10 + 4, (uint32_t)TEAM);
			
			/* allow any 3-bit source register code */
			mask[0x18 + 1] = 0b11000111;
			
			mask.SetDword(0x1b + 2, 0xfffff003);
			mask.SetDword(0x22 + 2, 0xfffff003);
			mask.SetDword(0x29 + 2, 0xfffff003);
			mask.SetDword(0x30 + 2, 0xfffff003);
			mask.SetDword(0x37 + 2, 0xfffff003);
			
			mask.SetDword(0x3e + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x3e, 5, 0xcc);
			mask.SetRange(0x3e, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x3e + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x3e] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x3e + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller1::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller2[] = {
		0x89, 0x04, 0x24,                               // +0000  mov [esp],exx
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0003  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +000B  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +0013  mov dword ptr [esp+0x4],<int:team>
		0xa3, 0x00, 0x00, 0x00, 0x00,                   // +001B  mov ds:0xXXXXXXXX,eax
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0020  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0027  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +002E  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0035  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +003C  mov [ebp-0xXXX],0x00000000
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +0043  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller2 : public CPatch
	{
		CPatch_CollectPlayers_Caller2(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller2)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller2);
			
			/* allow any 3-bit source register code */
			mask[0x00 + 1] = 0b11000111;
			
			buf.SetDword(0x03 + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x0b + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x13 + 4, (uint32_t)TEAM);
			
			mask.SetDword(0x1b + 1, 0x00000000);
			
			mask.SetDword(0x20 + 2, 0xfffff003);
			mask.SetDword(0x27 + 2, 0xfffff003);
			mask.SetDword(0x2e + 2, 0xfffff003);
			mask.SetDword(0x35 + 2, 0xfffff003);
			mask.SetDword(0x3c + 2, 0xfffff003);
			
			mask.SetDword(0x43 + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x43, 5, 0xcc);
			mask.SetRange(0x43, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x43 + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x43] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x43 + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller2::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller3[] = {
		0x01, 0x45, 0x00,                               // +0000  add [ebp-0xXX],eax
		0x8b, 0x45, 0x08,                               // +0003  mov eax,[ebp+this]
		0x8b, 0x80, 0x00, 0x00, 0x00, 0x00,             // +0006  mov eax,[eax+0xXXXX]
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +000C  mov dword ptr [esp+0xc],<bool:shouldAppend>
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0014  mov dword ptr [esp+0x8],<bool:isAlive>
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +001C  mov dword ptr [esp+0x4],<int:team>
		0x01, 0x45, 0x00,                               // +0024  add [ebp-0xXX],eax
		0x8d, 0x45, 0x00,                               // +0027  lea eax,[ebp-0xXX]
		0x89, 0x04, 0x24,                               // +002A  mov [esp],exx
		0xe8, 0x00, 0x00, 0x00, 0x00,                   // +002D  call CollectPlayers<CTFPlayer>
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFunc_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller3 : public CPatch
	{
		CPatch_CollectPlayers_Caller3(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller3)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller3);
			
			mask[0x00 + 2] = 0x00;
			
			mask.SetDword(0x06 + 2, 0x00000000);
			
			buf.SetDword(0x0c + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x14 + 4, (uint32_t)IS_ALIVE);
			buf.SetDword(0x1c + 4, (uint32_t)TEAM);
			
			mask[0x24 + 2] = 0x00;
			mask[0x27 + 2] = 0x00;
			
			/* allow any 3-bit source register code */
			mask[0x2a + 1] = 0b11000111;
			
			mask.SetDword(0x2d + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x2d, 5, 0xcc);
			mask.SetRange(0x2d, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x2d + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x2d] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x2d + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller3::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	
	
	constexpr uint8_t s_Buf_CollectPlayers_Caller4[] = {
		0xb9, 0x01, 0x00, 0x00, 0x00,              // +0x0000 mov     ecx, <bool:isAlive>
		0xba, 0x03, 0x00, 0x00, 0x00,              // +0x0005 mov     edx, <int:team>
		0xc7, 0x45, 0xd0, 0x00, 0x00, 0x00, 0x00,  // +0x000a mov     [ebp+var_30], 0
		0x6a, 0x00,                                // +0x0011 push    0
		0x8d, 0x45, 0xd0,                          // +0x0013 lea     eax, [ebp+var_30]
		0xc7, 0x45, 0xd4, 0x00, 0x00, 0x00, 0x00,  // +0x0016 mov     [ebp+var_2C], 0
		0xc7, 0x45, 0xd8, 0x00, 0x00, 0x00, 0x00,  // +0x001d mov     [ebp+var_28], 0
		0xc7, 0x45, 0xdc, 0x00, 0x00, 0x00, 0x00,  // +0x0024 mov     [ebp+var_24], 0
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x002b mov     [ebp+var_20], 0
		0xe8, 0x00, 0x00, 0x00, 0x00,              // +0x0032 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFuncRegcall_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller4 : public CPatch
	{
		CPatch_CollectPlayers_Caller4(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller4)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller4);
			
			//buf.SetDword(0x00 + 4, (uint32_t)SHOULD_APPEND);
			buf.SetDword(0x00 + 1, (uint32_t)IS_ALIVE);
			buf.SetDword(0x05 + 1, (uint32_t)TEAM);
			
			mask[0x0a + 2] = 0x00;
			mask[0x13 + 2] = 0x00;
			mask[0x16 + 2] = 0x00;
			mask[0x1d + 2] = 0x00;
			mask[0x24 + 2] = 0x00;
			mask[0x2b + 2] = 0x00;
			
			mask.SetDword(0x32 + 1, 0x00);
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x32, 5, 0xcc);
			mask.SetRange(0x32, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x32 + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x32] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x32 + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller4::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller4::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller4::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller4::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};

	constexpr uint8_t s_Buf_CollectPlayers_Caller5[] = {
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0000 mov     edx, <int:team>
		0x6a, 0x00,                                // +0x0005 push    0
		0x8d, 0x45, 0xe0,                          // +0x0007 lea     eax, [ebp-20h]
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x000a mov     dword ptr [ebp-20h], 0
		0xc7, 0x45, 0xe4, 0x00, 0x00, 0x00, 0x00,  // +0x0011 mov     dword ptr [ebp-1Ch], 0
		0xc7, 0x45, 0xe8, 0x00, 0x00, 0x00, 0x00,  // +0x0018 mov     dword ptr [ebp-18h], 0
		0xc7, 0x45, 0xec, 0x00, 0x00, 0x00, 0x00,  // +0x001f mov     dword ptr [ebp-14h], 0
		0xc7, 0x45, 0xf0, 0x00, 0x00, 0x00, 0x00,  // +0x0026 mov     dword ptr [ebp-10h], 0
		0xe8, 0x98, 0xcb, 0xff, 0xff,              // +0x002d call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFuncRegcall_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller5 : public CPatch
	{
		CPatch_CollectPlayers_Caller5(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller5)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller5);
			
			//buf.SetDword(0x00 + 4, (uint32_t)SHOULD_APPEND);
			//buf.SetDword(0x00 + 1, (uint32_t)IS_ALIVE);
			buf.SetDword(0x00 + 1, (uint32_t)TEAM);
			
			mask[0x07 + 2] = 0x00;
			mask[0x11 + 2] = 0x00;
			mask[0x18 + 2] = 0x00;
			mask[0x1f + 2] = 0x00;
			mask[0x26 + 2] = 0x00;
			mask[0x0a + 2] = 0x00;
			
			mask.SetDword(0x2d + 1, 0x00);
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x2d, 5, 0xcc);
			mask.SetRange(0x2d, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x2d + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x2d] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x2d + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller5::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller5::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller5::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller5::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};

	constexpr uint8_t s_Buf_CollectPlayers_Caller6[] = {
		0xc7, 0x45, 0xd0, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     dword ptr [ebp-30h], 0
		0x6a, 0x00,                                // +0x0007 push    0
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0009 mov     edx, <int:team>
		0xc7, 0x45, 0xd4, 0x00, 0x00, 0x00, 0x00,  // +0x000e mov     dword ptr [ebp-2Ch], 0
		0xc7, 0x45, 0xd8, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     dword ptr [ebp-28h], 0
		0xc7, 0x45, 0xdc, 0x00, 0x00, 0x00, 0x00,  // +0x001c mov     dword ptr [ebp-24h], 0
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x0023 mov     dword ptr [ebp-20h], 0
		0xe8, 0xc7, 0xcd, 0xff, 0xff,              // +0x002a call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFuncRegcall_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller6 : public CPatch
	{
		CPatch_CollectPlayers_Caller6(const char *func_name) : CPatch(sizeof(s_Buf_CollectPlayers_Caller6)), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CollectPlayers_Caller6);
			
			//buf.SetDword(0x00 + 4, (uint32_t)SHOULD_APPEND);
			//buf.SetDword(0x00 + 1, (uint32_t)IS_ALIVE);
			buf.SetDword(0x09 + 1, (uint32_t)TEAM);
			
			mask[0x00 + 2] = 0x00;
			mask[0x0e + 2] = 0x00;
			mask[0x15 + 2] = 0x00;
			mask[0x1c + 2] = 0x00;
			mask[0x23 + 2] = 0x00;
			
			mask.SetDword(0x2a + 1, 0x00);
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(0x2a, 5, 0xcc);
			mask.SetRange(0x2a, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (0x2a + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[0x2a] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(0x2a + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Caller6::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Caller6::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Caller6::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Caller6::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const char *m_pszFuncName;
	};
	

	constexpr uint8_t s_Buf_CollectPlayers_Caller7[] = {
		0xc7, 0x45, 0xd0, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     dword ptr [ebp-30h], 0
		0x6a, 0x00,                                // +0x0007 push    0
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0009 mov     edx, <int:team>
		0xc7, 0x45, 0xd4, 0x00, 0x00, 0x00, 0x00,  // +0x000e mov     dword ptr [ebp-2Ch], 0
		0xc7, 0x45, 0xd8, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     dword ptr [ebp-28h], 0
		0xc7, 0x45, 0xdc, 0x00, 0x00, 0x00, 0x00,  // +0x001c mov     dword ptr [ebp-24h], 0
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x0023 mov     dword ptr [ebp-20h], 0
		0xe8, 0xc7, 0xcd, 0xff, 0xff,              // +0x002a call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
	};
	
	template<uint32_t OFF_MIN, uint32_t OFF_MAX, int TEAM, bool IS_ALIVE, bool SHOULD_APPEND, CollectPlayersFuncRegcall_t REPLACE_FUNC>
	struct CPatch_CollectPlayers_Caller_Regcall : public CPatch
	{
		CPatch_CollectPlayers_Caller_Regcall(const char *func_name, size_t size, const uint8_t *buf, const uint8_t *mask, uintptr_t calloff) : CPatch(size), bufData(buf), maskData(mask), m_callOffset(calloff), m_pszFuncName(func_name) {}
		
		virtual const char *GetFuncName() const override { return this->m_pszFuncName; }
		virtual uint32_t GetFuncOffMin() const override  { return OFF_MIN; }
		virtual uint32_t GetFuncOffMax() const override  { return OFF_MAX; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(bufData);
			mask.CopyFrom(maskData);
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the call instruction with 'int3' opcodes */
			buf .SetRange(m_callOffset, 5, 0xcc);
			mask.SetRange(m_callOffset, 5, 0xff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			auto src = (uint8_t *)this->GetActualLocation() + (m_callOffset + 5);
			auto dst = (uint8_t *)REPLACE_FUNC;
			
			ptrdiff_t off = (dst - src);
			
			/* set the opcode back to 'call' */
			buf[m_callOffset] = 0xe8;
			
			/* put the relative offset into place */
			buf.SetDword(m_callOffset + 1, off);
			
			DevMsg("[CPatch_CollectPlayers_Regcall::AdjustPatchInfo] src: 0x%08x\n", (uintptr_t)src);
			DevMsg("[CPatch_CollectPlayers_Regcall::AdjustPatchInfo] dst: 0x%08x\n", (uintptr_t)dst);
			DevMsg("[CPatch_CollectPlayers_Regcall::AdjustPatchInfo] off: 0x%08x\n", (uintptr_t)off);
			DevMsg("[CPatch_CollectPlayers_Regcall::AdjustPatchInfo] BUF DUMP:\n");
			buf.Dump();
			
			return true;
		}
		
	private:
		const uint8_t *bufData;
		const uint8_t *maskData;
		uintptr_t m_callOffset;
		const char *m_pszFuncName;
	};

	constexpr uint8_t s_Buf_CollectPlayers_Common_Regcall[] = {
#ifdef PLATFORM_64BITS
		0x48, 0xc7, 0x45, 0xc0, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     [rbp+var_40], 0
		0x48, 0xc7, 0x45, 0xc8, 0x00, 0x00, 0x00, 0x00,  // +0x0008 mov     [rbp+var_38], 0
		0x48, 0xc7, 0x45, 0xd0, 0x00, 0x00, 0x00, 0x00,  // +0x0010 mov     [rbp+var_30], 0
		0xe8, 0x54, 0xcc, 0xff, 0xff,                    // +0x0018 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else 
		0xc7, 0x45, 0xd4, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     [ebp+var_2C], 0
		0xc7, 0x45, 0xd8, 0x00, 0x00, 0x00, 0x00,  // +0x0007 mov     [ebp+var_28], 0
		0xc7, 0x45, 0xdc, 0x00, 0x00, 0x00, 0x00,  // +0x000e mov     [ebp+var_24], 0
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     [ebp+var_20], 0
		0xe8, 0x16, 0x97, 0xfe, 0xff,              // +0x001c call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};

	constexpr uint8_t s_Mask_CollectPlayers_Common_Regcall[] = {
#ifdef PLATFORM_64BITS
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0000 mov     [ebp+var_2C], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0007 mov     [ebp+var_28], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x000e mov     [ebp+var_24], 0
		0xFF, 0x00, 0x00, 0x00, 0x00,                    // +0x001c call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0000 mov     [ebp+var_2C], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0007 mov     [ebp+var_28], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x000e mov     [ebp+var_24], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0015 mov     [ebp+var_20], 0
		0xFF, 0x00, 0x00, 0x00, 0x00,              // +0x001c call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};
	
#ifdef PLATFORM_64BITS
	constexpr uintptr_t s_CallOff_CollectPlayers_Common_Regcall = 0x18;
#else
	constexpr uintptr_t s_CallOff_CollectPlayers_Common_Regcall = 0x1c;
#endif

	constexpr uint8_t s_Buf_CPopulationManager_RestorePlayerCurrency[] = {
#ifdef PLATFORM_64BITS
		0xbe, 0x02, 0x00, 0x00, 0x00,                    // +0x0000 mov     esi, 2
		0x48, 0xc7, 0x45, 0xb0, 0x00, 0x00, 0x00, 0x00,  // +0x0005 mov     [rbp+var_50], 0
		0x48, 0xc7, 0x45, 0xb8, 0x00, 0x00, 0x00, 0x00,  // +0x000d mov     [rbp+var_48], 0
		0x48, 0xc7, 0x45, 0xc0, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     [rbp+var_40], 0
		0x48, 0xc7, 0x45, 0xc8, 0x00, 0x00, 0x00, 0x00,  // +0x001d mov     [rbp+var_38], 0
		0x41, 0x01, 0xc7,                                // +0x0025 add     r15d, eax
		0x48, 0x8d, 0x45, 0xb0,                          // +0x0028 lea     rax, [rbp+var_50]
		0x48, 0x89, 0xc7,                                // +0x002c mov     rdi, rax
		0x48, 0x89, 0x45, 0x98,                          // +0x002f mov     [rbp+var_68], rax
		0xe8, 0xdc, 0xd0, 0xff, 0xff,                    // +0x0033 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0000 mov     edx, 2
		0xc7, 0x45, 0xd0, 0x00, 0x00, 0x00, 0x00,  // +0x0005 mov     dword ptr [ebp-30h], 0
		0xc7, 0x45, 0xd4, 0x00, 0x00, 0x00, 0x00,  // +0x000c mov     dword ptr [ebp-2Ch], 0
		0xc7, 0x45, 0xd8, 0x00, 0x00, 0x00, 0x00,  // +0x0013 mov     dword ptr [ebp-28h], 0
		0x03, 0x81, 0xd0, 0x05, 0x00, 0x00,        // +0x001a add     eax, [ecx+5D0h]
		0xc7, 0x45, 0xdc, 0x00, 0x00, 0x00, 0x00,  // +0x0020 mov     dword ptr [ebp-24h], 0
		0x03, 0x81, 0xcc, 0x05, 0x00, 0x00,        // +0x0027 add     eax, [ecx+5CCh]
		0x31, 0xc9,                                // +0x002d xor     ecx, ecx
		0xc7, 0x45, 0xe0, 0x00, 0x00, 0x00, 0x00,  // +0x002f mov     dword ptr [ebp-20h], 0
		0xc7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,  // +0x0036 mov     dword ptr [esp], 0
		0x89, 0x45, 0xc4,                          // +0x003d mov     [ebp-3Ch], eax
		0x8d, 0x45, 0xd0,                          // +0x0040 lea     eax, [ebp-30h]
		0xe8, 0x3a, 0xd1, 0xff, 0xff,              // +0x0043 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};

	constexpr uint8_t s_Mask_CPopulationManager_RestorePlayerCurrency[] = {
#ifdef PLATFORM_64BITS
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                    // +0x0000 mov     esi, 2
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0005 mov     [rbp+var_50], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x000d mov     [rbp+var_48], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0015 mov     [rbp+var_40], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x001d mov     [rbp+var_38], 0
		0xFF, 0xFF, 0xFF,                                // +0x0025 add     r15d, eax
		0xFF, 0xFF, 0xFF, 0x00,                          // +0x0028 lea     rax, [rbp+var_50]
		0xFF, 0xFF, 0xFF,                                // +0x002c mov     rdi, rax
		0xFF, 0xFF, 0xFF, 0x00,                          // +0x002f mov     [rbp+var_68], rax
		0xFF, 0x00, 0x00, 0x00, 0x00,                    // +0x0033 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,              // +0x0000 mov     edx, 2
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0005 mov     dword ptr [ebp-30h], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x000c mov     dword ptr [ebp-2Ch], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0013 mov     dword ptr [ebp-28h], 0
		0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,        // +0x001a add     eax, [ecx+5D0h]
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0020 mov     dword ptr [ebp-24h], 0
		0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,        // +0x0027 add     eax, [ecx+5CCh]
		0xFF, 0xFF,                                // +0x002d xor     ecx, ecx
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x002f mov     dword ptr [ebp-20h], 0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0036 mov     dword ptr [esp], 0
		0xFF, 0x00, 0x00,                          // +0x003d mov     [ebp-3Ch], eax
		0xFF, 0x00, 0x00,                          // +0x0040 lea     eax, [ebp-30h]
		0xFF, 0x00, 0x00, 0x00, 0x00,              // +0x0043 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_12; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};

#ifdef PLATFORM_64BITS
	constexpr uintptr_t s_CallOff_CPopulationManager_RestorePlayerCurrency = 0x33;
#else
	constexpr uintptr_t s_CallOff_CPopulationManager_RestorePlayerCurrency = 0x43;
#endif

	constexpr uint8_t s_Buf_CollectPlayers_WaveCompleteUpdate[] = {
#ifdef PLATFORM_64BITS
		0x48, 0xc7, 0x85, 0x70, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     [rbp+var_90], 0
		0xbe, 0x02, 0x00, 0x00, 0x00,                                      // +0x000b mov     esi, 2
		0x48, 0x89, 0xc7,                                                  // +0x0010 mov     rdi, rax
		0x48, 0xc7, 0x85, 0x78, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,  // +0x0013 mov     [rbp+var_88], 0
		0x48, 0xc7, 0x45, 0x80, 0x00, 0x00, 0x00, 0x00,                    // +0x001e mov     [rbp+var_80], 0
		0x48, 0xc7, 0x45, 0x88, 0x00, 0x00, 0x00, 0x00,                    // +0x0026 mov     [rbp+var_78], 0
		0x48, 0x89, 0x85, 0x68, 0xff, 0xff, 0xff,                          // +0x002e mov     [rbp+var_98], rax
		0xe8, 0xc8, 0xde, 0xff, 0xff,                                      // +0x0035 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_13; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xc7, 0x45, 0xb0, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     dword ptr [ebp-50h], 0
		0x6a, 0x00,                                // +0x0007 push    0
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0009 mov     edx, 2
		0xc7, 0x45, 0xb4, 0x00, 0x00, 0x00, 0x00,  // +0x000e mov     dword ptr [ebp-4Ch], 0
		0xc7, 0x45, 0xb8, 0x00, 0x00, 0x00, 0x00,  // +0x0015 mov     dword ptr [ebp-48h], 0
		0xc7, 0x45, 0xbc, 0x00, 0x00, 0x00, 0x00,  // +0x001c mov     dword ptr [ebp-44h], 0
		0xc7, 0x45, 0xc0, 0x00, 0x00, 0x00, 0x00,  // +0x0023 mov     dword ptr [ebp-40h], 0
		0xe8, 0xf6, 0xdf, 0xff, 0xff,              // +0x002a call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_13; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};

	constexpr uint8_t s_Mask_CollectPlayers_WaveCompleteUpdate[] = {
#ifdef PLATFORM_64BITS
		0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0000 mov     [rbp+var_90], 0
		0xbe, 0x02, 0x00, 0x00, 0x00,                                      // +0x000b mov     esi, 2
		0x48, 0x89, 0xc7,                                                  // +0x0010 mov     rdi, rax
		0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0013 mov     [rbp+var_88], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,                    // +0x001e mov     [rbp+var_80], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,                    // +0x0026 mov     [rbp+var_78], 0
		0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,                          // +0x002e mov     [rbp+var_98], rax
		0xFF, 0x00, 0x00, 0x00, 0x00,                                      // +0x0035 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_13; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#else
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0000 mov     dword ptr [ebp-50h], 0
		0xFF, 0xFF,                                // +0x0007 push    0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,              // +0x0009 mov     edx, 2
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x000e mov     dword ptr [ebp-4Ch], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0015 mov     dword ptr [ebp-48h], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x001c mov     dword ptr [ebp-44h], 0
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0023 mov     dword ptr [ebp-40h], 0
		0xFF, 0x00, 0x00, 0x00, 0x00,              // +0x002a call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_13; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
#endif
	};

#ifdef PLATFORM_64BITS
	constexpr uintptr_t s_CallOff_CollectPlayers_WaveCompleteUpdate = 0x35;
#else
	constexpr uintptr_t s_CallOff_CollectPlayers_WaveCompleteUpdate = 0x2a;
#endif

#ifdef PLATFORM_64BITS
constexpr uint8_t s_Buf_CollectPlayers_RadiusSpyScan[] = {
		0x48, 0xc7, 0x45, 0xb0, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     [rbp+var_50], 0
		0xbe, 0x03, 0x00, 0x00, 0x00,                    // +0x0008 mov     esi, 3
		0x48, 0x89, 0xc7,                                // +0x000d mov     rdi, rax
		0x48, 0xc7, 0x45, 0xb8, 0x00, 0x00, 0x00, 0x00,  // +0x0010 mov     [rbp+var_48], 0
		0x48, 0xc7, 0x45, 0xc0, 0x00, 0x00, 0x00, 0x00,  // +0x0018 mov     [rbp+var_40], 0
		0x48, 0xc7, 0x45, 0xc8, 0x00, 0x00, 0x00, 0x00,  // +0x0020 mov     [rbp+var_38], 0
		0x48, 0x89, 0x45, 0xa8,                          // +0x0028 mov     [rbp+var_58], rax
		0xe8, 0xa4, 0x8c, 0xfe, 0xff,                    // +0x002c call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
};

constexpr uint8_t s_Mask_CollectPlayers_RadiusSpyScan[] = {
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0000 mov     [rbp+var_50], 0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                    // +0x0008 mov     esi, 3
		0xFF, 0xFF, 0xFF,                                // +0x000d mov     rdi, rax
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0010 mov     [rbp+var_48], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0018 mov     [rbp+var_40], 0
		0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // +0x0020 mov     [rbp+var_38], 0
		0xFF, 0xFF, 0xFF, 0x00,                          // +0x0028 mov     [rbp+var_58], rax
		0xFF, 0x00, 0x00, 0x00, 0x00,                    // +0x002c call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb_isra_0_1; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool) [clone]
};

constexpr uintptr_t s_CallOff_CollectPlayers_RadiusSpyScan = 0x2c;
#endif
	
	constexpr uint8_t s_Buf_RadiusSpyScan[] = {
#ifdef PLATFORM_64BITS
		0x48, 0x8b, 0xbf, 0xc0, 0x01, 0x00, 0x00,  // +0x0000 mov     rdi, [rdi+1C0h]; this
		0xe8, 0x00, 0xbe, 0x20, 0x00,              // +0x0007 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xf8, 0x02,                          // +0x000c cmp     eax, 2
#else
		0xff, 0xb7, 0x8c, 0x01, 0x00, 0x00,  // +0x0000 push    dword ptr [edi+18Ch]
		0xe8, 0x49, 0xf5, 0x1f, 0x00,        // +0x0006 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xc4, 0x10,                    // +0x000b add     esp, 10h
		0x83, 0xf8, 0x02,                    // +0x000e cmp     eax, 2
#endif
	};
	
	struct CPatch_RadiusSpyScan : public CPatch
	{
		CPatch_RadiusSpyScan() : CPatch(sizeof(s_Buf_RadiusSpyScan)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::RadiusSpyScan"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0030; } // @ +0x000c
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_RadiusSpyScan);
			
			int off_CTFPlayerShared_m_pOuter;
			if (!Prop::FindOffset(off_CTFPlayerShared_m_pOuter, "CTFPlayerShared", "m_pOuter")) return false;
#ifdef PLATFORM_64BITS
			buf.SetDword(0x00 + 3, off_CTFPlayerShared_m_pOuter);
			
			mask.SetDword(0x07 + 1, 0x00000000);
#else
			buf.SetDword(0x00 + 2, off_CTFPlayerShared_m_pOuter);
			
			mask.SetDword(0x06 + 1, 0x00000000);
			mask[0x0b + 2] = 0x00;
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
			/* replace 'cmp eax,TF_TEAM_RED' with 'cmp eax,eax; nop' */
			buf[0x0c + 0] = 0x39;
			buf[0x0c + 1] = 0xc0;
			buf[0x0c + 2] = 0x90;
			
			mask.SetRange(0x0c, 3, 0xff);
#else
			/* replace 'cmp eax,TF_TEAM_RED' with 'cmp eax,eax; nop' */
			buf[0x0e + 0] = 0x39;
			buf[0x0e + 1] = 0xc0;
			buf[0x0e + 2] = 0x90;
			
			mask.SetRange(0x0e, 3, 0xff);
#endif
			
			return true;
		}
	};
	
	
	ConVar cvar_max("sig_mvm_jointeam_blue_allow_max", "-1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: max humans to allow on blue team (-1 for no limit)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			Mod::MvM::Player_Limit::RecalculateSlots();
		});
	
	ConVar cvar_flag_pickup("sig_mvm_bluhuman_flag_pickup", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: allow picking up the flag");
	ConVar cvar_flag_capture("sig_mvm_bluhuman_flag_capture", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: allow scoring a flag Capture by carrying it to the capture zone");
	
	ConVar cvar_spawn_protection("sig_mvm_bluhuman_spawn_protection", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: enable spawn protection invulnerability");
	ConVar cvar_spawn_no_shoot("sig_mvm_bluhuman_spawn_noshoot", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: when spawn protection invulnerability is enabled, disallow shooting from spawn");
	
	ConVar cvar_infinite_cloak("sig_mvm_bluhuman_infinite_cloak", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: enable infinite spy cloak meter");
	ConVar cvar_infinite_cloak_deadringer("sig_mvm_bluhuman_infinite_cloak_deadringer", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: enable infinite spy cloak meter (Dead Ringer)");

	ConVar cvar_infinite_ammo("sig_mvm_bluhuman_infinite_ammo", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: infinite ammo");

	ConVar cvar_teleport("sig_mvm_bluhuman_teleport", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: teleport to engiebot teleport");

	ConVar cvar_teleport_player("sig_mvm_bluhuman_teleport_player", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: teleport bots and players to player teleport exit");
		
	ConVar cvar_no_footsteps("sig_mvm_jointeam_blue_no_footsteps", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: No robot footsteps");
		
	ConVar cvar_spectator_is_blu("sig_mvm_jointeam_blue_spectator", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Blue humans in MvM: Force spectators to join blue team");
	
	std::map<CHandle<CTFPlayer>, float> player_deploy_time; 
	// TODO: probably need to add in a check for TF_COND_REPROGRAMMED here and:
	// - exclude humans on TF_TEAM_BLUE who are in TF_COND_REPROGRAMMED
	// - include humans on TF_TEAM_RED who are in TF_COND_REPROGRAMMED
	
	bool IsMvMBlueHuman(CTFPlayer *player)
	{
		if (player == nullptr)                       return false;
		if (player->GetTeamNumber() != TF_TEAM_BLUE) return false;
		if (player->IsBot())                         return false;
		if (!TFGameRules()->IsMannVsMachineMode())   return false;
		
		return true;
	}
	
	int GetMvMBlueHumanCount()
	{
		int count = 0;
		
		ForEachTFPlayer([&](CTFPlayer *player){
			if (IsMvMBlueHuman(player)) {
				++count;
			}
		});
		
		return count;
	}
	
	bool IsInBlueSpawnRoom(CTFPlayer *player)
	{
		player->UpdateLastKnownArea();
		auto area = static_cast<CTFNavArea *>(player->GetLastKnownArea());
		return (area != nullptr && area->HasTFAttributes(BLUE_SPAWN_ROOM));
	}
	
	__gcc_regcall static int CollectPlayers_EnemyTeam(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		extern ConVar cvar_force;
		ClientMsgAll("see enemy team %d\n", cvar_force.GetBool());
		return CollectPlayers(playerVector, cvar_force.GetBool() ? TF_TEAM_RED : TF_TEAM_BLUE,  isAlive, shouldAppend);
	}

	__gcc_regcall static int CollectPlayers_RedAndBlue(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		(void) CollectPlayers(playerVector, TF_TEAM_RED,  isAlive, shouldAppend);
		return CollectPlayers(playerVector, TF_TEAM_BLUE, isAlive, true);
	}
	
	__gcc_regcall static int CollectPlayers_RedAndBlue_IsBot(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		CUtlVector<CTFPlayer *> tempVector;
		CollectPlayers(&tempVector, TF_TEAM_RED,  isAlive, true);
		CollectPlayers(&tempVector, TF_TEAM_BLUE, isAlive, true);
		
		if (!shouldAppend) {
			playerVector->RemoveAll();
		}
		
		for (auto player : tempVector) {
			if (player->IsBot()) {
				playerVector->AddToTail(player);
			}
		}
		
		return playerVector->Count();
	}
	
	__gcc_regcall static int CollectPlayers_RedAndBlue_NotBot(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		CUtlVector<CTFPlayer *> tempVector;
		CollectPlayers(&tempVector, TF_TEAM_RED,  isAlive, true);
		CollectPlayers(&tempVector, TF_TEAM_BLUE, isAlive, true);
		
		if (!shouldAppend) {
			playerVector->RemoveAll();
		}
		
		for (auto player : tempVector) {
			if (!player->IsBot()) {
				playerVector->AddToTail(player);
			}
		}
		
		return playerVector->Count();
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFGameRules_GetTeamAssignmentOverride, CTFPlayer *pPlayer, int iWantedTeam, bool b1)
	{
		/* it's important to let the call happen, because pPlayer->m_nCurrency
		 * is set to its proper value in the call (stupid, but whatever) */
		auto iResult = DETOUR_MEMBER_CALL(pPlayer, iWantedTeam, b1);
		
		// debug message for the "sometimes bots don't get put on TEAM_SPECTATOR properly at wave end" situation
		if (TFGameRules()->IsMannVsMachineMode() && pPlayer->IsBot() && iResult != iWantedTeam) {
			DevMsg("[CTFGameRules::GetTeamAssignmentOverride] Bot [ent:%d userid:%d name:\"%s\"]: on team %d, wanted %d, forced onto %d!\n",
				ENTINDEX(pPlayer), pPlayer->GetUserID(), pPlayer->GetPlayerName(), pPlayer->GetTeamNumber(), iWantedTeam, iResult);
			BACKTRACE();
		}
		
		extern ConVar cvar_force;
		extern ConVar cvar_adminonly;
		bool allowJoinTeamBluNoAdmin = !cvar_adminonly.GetBool() || Mod::Pop::PopMgr_Extensions::PopFileIsOverridingJoinTeamBlueConVarOn() || cvar_force.GetBool();
		//DevMsg("Get team assignment %d %d %d\n", iWantedTeam, cvar_force.GetBool());
		if (TFGameRules()->IsMannVsMachineMode() && pPlayer->IsRealPlayer()
			&& ((iWantedTeam == TF_TEAM_BLUE && iResult != iWantedTeam) || (iWantedTeam != TEAM_SPECTATOR && iResult == TEAM_SPECTATOR && allowJoinTeamBluNoAdmin) 
				|| (iWantedTeam == TEAM_SPECTATOR && cvar_spectator_is_blu.GetBool() && !cvar_force.GetBool() && allowJoinTeamBluNoAdmin) || (iWantedTeam == TF_TEAM_RED && cvar_force.GetBool())) ) {
			// NOTE: if the pop file had custom param 'AllowJoinTeamBlue 1', then disregard admin-only restrictions
			if (allowJoinTeamBluNoAdmin || PlayerIsSMAdmin(pPlayer)) {
				if (cvar_max.GetInt() < 0 || GetMvMBlueHumanCount() < cvar_max.GetInt()) {
					DevMsg("Player #%d \"%s\" requested team %d but was forced onto team %d; overriding to allow them to join team %d.\n",
						ENTINDEX(pPlayer), pPlayer->GetPlayerName(), iWantedTeam, iResult, iWantedTeam);
					iResult = TF_TEAM_BLUE;
				} else {
					if (cvar_force.GetBool())
						iResult = TEAM_SPECTATOR;
					DevMsg("Player #%d \"%s\" requested team %d but was forced onto team %d; would have overridden to allow joining team %d but limit has been met.\n",
						ENTINDEX(pPlayer), pPlayer->GetPlayerName(), iWantedTeam, iResult, iWantedTeam);
					ClientMsg(pPlayer, "Cannot join team blue: the maximum number of human players on blue team has already been met.\n");
				}
			} else {
				ClientMsg(pPlayer, "You are not authorized to use this command because you are not a SourceMod admin. Sorry.\n");
			}
		}
		
		//DevMsg("Get team assignment result %d\n", iResult);
		
		return iResult;
	}
	
	ConVar cvar_allow_reanimators("sig_mvm_jointeam_blue_allow_revive", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL, "Allow reanimators for the blue players");
	DETOUR_DECL_STATIC(CTFReviveMarker *, CTFReviveMarker_Create, CTFPlayer *player)
	{
		if (!cvar_allow_reanimators.GetBool() && IsMvMBlueHuman(player)) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(player);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_OnNavAreaChanged, CNavArea *enteredArea, CNavArea *leftArea)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		DETOUR_MEMBER_CALL(enteredArea, leftArea);
		
		if (IsMvMBlueHuman(player) &&
			(enteredArea == nullptr ||  static_cast<CTFNavArea *>(enteredArea)->HasTFAttributes(BLUE_SPAWN_ROOM)) &&
			(leftArea    == nullptr || !static_cast<CTFNavArea *>(leftArea)   ->HasTFAttributes(BLUE_SPAWN_ROOM))) {
			player->m_Shared->m_bInUpgradeZone = false;
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_ClientCommandKeyValues, edict_t *pEntity, KeyValues *pKeyValues)
	{
		if (FStrEq(pKeyValues->GetName(), "MvM_UpgradesDone")) {
			CTFPlayer *player = ToTFPlayer(GetContainingEntity(pEntity));
			if (IsMvMBlueHuman(player)) {
				player->m_Shared->m_bInUpgradeZone = false;
			}
		}
		
		DETOUR_MEMBER_CALL(pEntity, pKeyValues);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsAllowedToPickUpFlag)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (IsMvMBlueHuman(player) && !cvar_flag_pickup.GetBool()) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CCaptureZone_ShimTouch, CBaseEntity *pOther)
	{
		auto zone = reinterpret_cast<CCaptureZone *>(this);
		
		[&]{
			if (zone->IsDisabled()) return;
			
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player == nullptr)       return;
			if (!IsMvMBlueHuman(player)) return;
			
			if (!cvar_flag_capture.GetBool()) return;
			
			if (!player->HasItem()) return;
			CTFItem *item = player->GetItem();
			
			if (item->GetItemID() != TF_ITEM_CAPTURE_FLAG) return;
			
			auto flag = dynamic_cast<CCaptureFlag *>(item);
			if (flag == nullptr) return;
			
			// skipping flag->m_nType check out of laziness
			
			if (!TFGameRules()->FlagsMayBeCapped()) return;
			if (zone->GetTeamNumber() == TEAM_UNASSIGNED || player->GetTeamNumber() == TEAM_UNASSIGNED || zone->GetTeamNumber() == player->GetTeamNumber()) {
				
				bool wasTaunting = player->m_Shared->InCond(TF_COND_FREEZE_INPUT);
				//if (!wasTaunting) {
				//	 //Taunt(TAUNT_BASE_WEAPON,0);
				//}
				player->m_Shared->AddCond(TF_COND_FREEZE_INPUT, 0.7f);
				if (player_deploy_time.find(player) == player_deploy_time.end() || !wasTaunting || gpGlobals->curtime - player_deploy_time[player] > 2.2f) {
					player_deploy_time[player] = gpGlobals->curtime;
					player->SetAbsVelocity(vec3_origin);
					player->PlaySpecificSequence("primary_deploybomb");
				}
				else {
					if (!wasTaunting)
						player_deploy_time.erase(player);
					else if (gpGlobals->curtime - player_deploy_time[player] >= 1.9f) {
						#warning Should have flag->IsCaptured() check in here
						//	if (flag->IsCaptured() || zone->GetTeamNumber() == TEAM_UNASSIGNED || player->GetTeamNumber() == TEAM_UNASSIGNED || zone->GetTeamNumber() == player->GetTeamNumber()) {
						zone->Capture(player);
						player_deploy_time.erase(player);
					}
				}
			}
			//player->m_Shared->StunPlayer(1.0f, 1.0f, TF_STUNFLAGS_CONTROL | TF_STUNFLAGS_CONTROL, nullptr);
		}();
		
		DETOUR_MEMBER_CALL(pOther);
	}
	
	
//	DETOUR_DECL_MEMBER(void, CPlayerMove_StartCommand, CBasePlayer *player, CUserCmd *ucmd)
//	{
//		DETOUR_MEMBER_CALL(player, ucmd);
//		
//		DevMsg("CPlayerMove::StartCommand(#%d): buttons = %08x\n", ENTINDEX(player), ucmd->buttons);
//		
//		/* ideally we'd either do this or not do this based on the value of
//		 * CanBotsAttackWhileInSpawnRoom in g_pPopulationManager, but tracking
//		 * down the offset of that boolean is more work than it's worth */
//		CTFPlayer *tfplayer = ToTFPlayer(player);
//		if (cvar_spawn_protection.GetBool() && cvar_spawn_no_shoot.GetBool() && IsMvMBlueHuman(tfplayer) && IsInBlueSpawnRoom(tfplayer)) {
//			ucmd->buttons &= ~(IN_ATTACK | IN_ATTACK2 | IN_ATTACK3);
//			DevMsg("- stripped attack buttons: %08x\n", ucmd->buttons);
//		}
//	}
	
	RefCount rc_CTFGameRules_FireGameEvent__teamplay_round_start;
	DETOUR_DECL_MEMBER(void, CTFGameRules_FireGameEvent, IGameEvent *event)
	{
		SCOPED_INCREMENT_IF(rc_CTFGameRules_FireGameEvent__teamplay_round_start,
			(event != nullptr && strcmp(event->GetName(), "teamplay_round_start") == 0));
		DETOUR_MEMBER_CALL(event);
	}
	
	DETOUR_DECL_STATIC(int, CollectPlayers_CTFPlayer, CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		if (rc_CTFGameRules_FireGameEvent__teamplay_round_start > 0 && (team == TF_TEAM_BLUE && !isAlive && !shouldAppend)) {
			/* collect players on BOTH teams */
			return CollectPlayers_RedAndBlue_IsBot(playerVector, team, isAlive, shouldAppend);
		}
		
		return DETOUR_STATIC_CALL(playerVector, team, isAlive, shouldAppend);
	}
	
	
	RefCount rc_CTFPlayerShared_RadiusSpyScan;
	int radius_spy_scan_teamnum = TEAM_INVALID;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_RadiusSpyScan)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		
		/* instead of restricting the ability to team red, restrict it to human players */
		if (player->IsBot() || !player->IsAlive()) {
			return;
		}
		
		SCOPED_INCREMENT(rc_CTFPlayerShared_RadiusSpyScan);
		radius_spy_scan_teamnum = player->GetTeamNumber();
		
		DETOUR_MEMBER_CALL();
	}
	
	__gcc_regcall static int CollectPlayers_RadiusSpyScan(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		/* sanity checks */
		assert(rc_CTFPlayerShared_RadiusSpyScan > 0);
		
		/* rather than always affecting blue players, affect players on the opposite team of the player with the ability */
		return CollectPlayers(playerVector, GetEnemyTeam(radius_spy_scan_teamnum), isAlive, shouldAppend);
	}
	
	
	/* log cases where bots are spawning at weird times (not while the wave is running) */
	DETOUR_DECL_MEMBER(void, CTFBot_Spawn)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		DETOUR_MEMBER_CALL();
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			// ========= MANN VS MACHINE MODE ROUND STATE TRANSITIONS ==========
			// [CPopulationManager::JumpToWave]                     --> PREROUND
			// [CPopulationManager::StartCurrentWave]               --> RND_RUNNING
			// [CPopulationManager::WaveEnd]                        --> BETWEEN_RNDS
			// [CPopulationManager::WaveEnd]                        --> GAME_OVER
			// -----------------------------------------------------------------
			// [CTeamplayRoundBasedRules::CTeamplayRoundBasedRules] --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_INIT]         --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_RND_RUNNING]  --> PREGAME
			// [CTeamplayRoundBasedRules::State_Think_PREGAME]      --> STARTGAME
			// [CTeamplayRoundBasedRules::CheckReadyRestart]        --> RESTART
			// [CTeamplayRoundBasedRules::State_Enter_RESTART]      --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_STARTGAME]    --> PREROUND
			// [CTeamplayRoundBasedRules::CheckWaitingForPlayers]   --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_PREROUND]     --> RND_RUNNING
			// [CTeamplayRoundBasedRules::State_Enter_PREROUND]     --> BETWEEN_RNDS
			// [CTeamplayRoundBasedRules::State_Think_TEAM_WIN]     --> PREROUND
			// [CTeamplayRoundBasedRules::State_Think_TEAM_WIN]     --> GAME_OVER
			// =================================================================
			
			bool sketchy;
			switch (TFGameRules()->State_Get()) {
				case GR_STATE_INIT:         sketchy = true;  break;
				case GR_STATE_PREGAME:      sketchy = true;  break;
				case GR_STATE_STARTGAME:    sketchy = false; break;
				case GR_STATE_PREROUND:     sketchy = true;  break;
				case GR_STATE_RND_RUNNING:  sketchy = false; break;
				case GR_STATE_TEAM_WIN:     sketchy = true;  break;
				case GR_STATE_RESTART:      sketchy = true;  break;
				case GR_STATE_STALEMATE:    sketchy = true;  break;
				case GR_STATE_GAME_OVER:    sketchy = true;  break;
				case GR_STATE_BONUS:        sketchy = true;  break;
				case GR_STATE_BETWEEN_RNDS: sketchy = true;  break;
				default:                    sketchy = true;  break;
			}
			
			if (sketchy) {
				DevMsg("[CTFBot::Spawn] Bot [ent:%d userid:%d name:\"%s\"]: spawned during game state %s!\n",
					ENTINDEX(bot), bot->GetUserID(), bot->GetPlayerName(), GetRoundStateName(TFGameRules()->State_Get()));
				BACKTRACE();
			}
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_PlayerReadyStatus_ShouldStartCountdown)
	{
		bool ret = DETOUR_MEMBER_CALL();
		if (TFGameRules()->IsMannVsMachineMode()) {
			bool notReadyPlayerBlue = false;
			bool notReadyPlayerRed = false;
			bool hasPlayer = false;
			ForEachTFPlayer([&](CTFPlayer *player){
				if (notReadyPlayerRed || notReadyPlayerBlue) return;
				if (player->GetTeamNumber() < 2) return;
				if (player->IsBot()) return;
				if (player->IsFakeClient()) return;
				hasPlayer = true;
				if (!TFGameRules()->IsPlayerReady(ENTINDEX(player))) {
					if (player->GetTeamNumber() == TF_TEAM_BLUE)
						notReadyPlayerBlue = true;
					else
						notReadyPlayerRed = true;
					return;
				}
			});
			
			return hasPlayer && !notReadyPlayerRed && !notReadyPlayerBlue;
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFBaseBoss_OnTakeDamage, CTakeDamageInfo &info)
	{
		float damage = DETOUR_MEMBER_CALL(info);
		if (info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == reinterpret_cast<CBaseEntity *>(this)->GetTeamNumber())
			return 0;
		return damage;
	}

	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		static ConVarRef sig_mvm_teleporter_aggro("sig_mvm_teleporter_aggro");
		auto vision = reinterpret_cast<IVision *>(this);
		CTFBot *bot = static_cast<CTFBot *>(vision->GetBot()->GetEntity());
		if (bot->GetTeamNumber() == TF_TEAM_RED && (!sig_mvm_teleporter_aggro.GetBool() || rtti_cast<CTFWeaponBaseMelee *>(bot->GetActiveWeapon()) != nullptr)) {
			if (bot != nullptr && (bot->GetActiveWeapon()) != nullptr && ent->IsBaseObject()) {
				CBaseObject *obj = ToBaseObject(ent);
				if (obj != nullptr && obj->GetType() == OBJ_TELEPORTER) {
					return true;
				}
			}
		}
		return DETOUR_MEMBER_CALL(ent);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAmmo, int count, const char *name)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		bool change = player->GetTeamNumber() == TF_TEAM_BLUE && !cvar_infinite_ammo.GetBool() && !player->IsBot();

		if (change)
			player->SetTeamNumber(TF_TEAM_RED);
		DETOUR_MEMBER_CALL(count, name);

		if (change)
			player->SetTeamNumber(TF_TEAM_BLUE);
	}

	int sentryAmmoCurrent;
	int sentryRocketAmmoCurrent;
	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SentryThink)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		
		int maxAmmo = obj->m_iMaxAmmoShells;
		int maxAmmoRockets = obj->m_iMaxAmmoRockets;
		sentryAmmoCurrent = obj->m_iAmmoShells;
		sentryRocketAmmoCurrent = obj->m_iAmmoRockets;
		bool stopammo = TFGameRules()->IsMannVsMachineMode() && !cvar_infinite_ammo.GetBool() && IsMvMBlueHuman(owner);

		DETOUR_MEMBER_CALL();

		if (stopammo) {
			obj->m_iMaxAmmoShells = maxAmmo;
			obj->m_iMaxAmmoRockets = maxAmmoRockets;
			obj->m_iAmmoShells = sentryAmmoCurrent;
			obj->m_iAmmoRockets = sentryRocketAmmoCurrent;
		}
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_Attack)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);

		DETOUR_MEMBER_CALL();
		sentryAmmoCurrent = obj->m_iAmmoShells;
		sentryRocketAmmoCurrent = obj->m_iAmmoRockets;
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_ShouldQuickBuild)
	{
		CBaseObject *obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *builder = ToTFPlayer(obj->GetBuilder());
		bool changedteam = builder != nullptr && IsMvMBlueHuman(builder);
		
		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_RED);

		bool ret = DETOUR_MEMBER_CALL();

		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_BLUE);

		return ret;
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_DeterminePlaybackRate)
	{
		CObjectTeleporter *obj = reinterpret_cast<CObjectTeleporter *>(this);
		CTFPlayer *builder = ToTFPlayer(obj->GetBuilder());
		bool changedteam = !cvar_teleport_player.GetBool() && builder != nullptr && IsMvMBlueHuman(builder);
		
		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_RED);

		DETOUR_MEMBER_CALL();
		if (obj->m_iState == 1) {
			StopParticleEffects(obj);
		}
		if (changedteam)
			obj->SetTeamNumber(TF_TEAM_BLUE);

	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ShouldRespawnQuickly, CTFPlayer *player)
	{
		bool ret = DETOUR_MEMBER_CALL(player);
		ret |= TFGameRules()->IsMannVsMachineMode() && player->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT && !player->IsBot();
		return ret;
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CTFKnife_CanPerformBackstabAgainstTarget, CTFPlayer *target )
	{
		bool ret = DETOUR_MEMBER_CALL(target);

		if ( !ret && TFGameRules() && TFGameRules()->IsMannVsMachineMode() && target->GetTeamNumber() == TF_TEAM_RED )
		{
			if ( target->m_Shared->InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
				return true;

			if ( target->m_Shared->InCond( TF_COND_SAPPED ) && !target->IsMiniBoss() )
				return true;
		}
		return ret;
	}

	void TeleportAfterSpawn(CTFPlayer *player)
	{
		float distanceToBomb = std::numeric_limits<float>::max();
		CObjectTeleporter *teleOut = nullptr;
		CCaptureZone *zone = nullptr;

		for (auto elem : ICaptureZoneAutoList::AutoList()) {
			zone = rtti_scast<CCaptureZone *>(elem);
			if (zone != nullptr)
				break;
		}
		if (zone == nullptr)
			return;
			
		CTFPlayer *playerbot = ToTFBot(player);
		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			auto tele = rtti_cast<CObjectTeleporter *>(IBaseObjectAutoList::AutoList()[i]);
			if (tele != nullptr && tele->GetTeamNumber() == player->GetTeamNumber() && tele->IsFunctional()) {
				if (((cvar_teleport_player.GetBool() && ToTFBot(tele->GetBuilder()) == nullptr) || 
						(playerbot == nullptr && (tele->GetBuilder() == nullptr || ToTFBot(tele->GetBuilder()) != nullptr))) ) {
					float dist = tele->WorldSpaceCenter().DistToSqr(zone->WorldSpaceCenter());
					if ( dist < distanceToBomb) {
						teleOut = tele;
						distanceToBomb = dist;
					}
				}
			}
		}
		if (teleOut != nullptr) {
			auto vec = teleOut->WorldSpaceCenter();
			vec.z += teleOut->CollisionProp()->OBBMaxs().z;
			bool is_space_to_spawn = IsSpaceToSpawnHere(vec);
			if (!is_space_to_spawn)
				vec.z += 50.0f;
			if (is_space_to_spawn || IsSpaceToSpawnHere(vec)){
				player->Teleport(&(vec),&(teleOut->GetAbsAngles()),&(player->GetAbsVelocity()));
				static ConVarRef uberDuration("tf_mvm_engineer_teleporter_uber_duration");
				if (uberDuration.GetFloat() != 5.0f) {
					player->m_Shared->AddCond(TF_COND_INVULNERABLE, uberDuration.GetFloat());
					player->m_Shared->AddCond(TF_COND_INVULNERABLE_WEARINGOFF, uberDuration.GetFloat());
				}
				player->EmitSound("MVM.Robot_Teleporter_Deliver");
			}
		}
	}
	
	THINK_FUNC_DECL(TeleportAfterSpawnThink)
	{
		TeleportAfterSpawn(reinterpret_cast<CTFPlayer *>(this));
	}

	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(player);
		bool bluhuman = IsMvMBlueHuman(player);
		CTFPlayer *playerbot = ToTFBot(player);
		if ((bluhuman && cvar_teleport.GetBool()) || cvar_teleport_player.GetBool()) {

			if (player->IsBot()) {
				// Bots need a delay in telein in case they have reprogrammed mode on, to prevent teleports from teleporting opposite team bots
				THINK_FUNC_SET(player, TeleportAfterSpawnThink, gpGlobals->curtime + 1.0f);
			}
			else {
				TeleportAfterSpawn(player);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Touch, CBaseEntity *toucher)
	{
		DETOUR_MEMBER_CALL(toucher);
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->IsMiniBoss() && toucher->IsBaseObject()) {
			CBaseObject *obj = ToBaseObject(toucher);
			if (obj != nullptr && (obj->GetType() != OBJ_SENTRYGUN || obj->m_bDisposableBuilding || obj->m_bMiniBuilding)) {

				CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, 4.0f * std::max(obj->GetMaxHealth(), obj->GetHealth()), DMG_BLAST);
				obj->TakeDamage(info);
			}
		}
	}
	GlobalThunk<SendTable> DT_TFPlayerResource_g_SendTable("DT_TFPlayerResource::g_SendTable");
	GlobalThunk<SendTable> DT_PlayerResource_g_SendTable("DT_PlayerResource::g_SendTable");
	unsigned int team_num_prop_index = 0;
	SendTable *team_num_table = nullptr;

	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		bool prevIsGiant[MAX_PLAYERS + 1];
		if (cvar_no_footsteps.GetBool()) {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (IsMvMBlueHuman(player) && ENTINDEX(player) < MAX_PLAYERS + 1) {
					bool isBoss = player->IsMiniBoss();
					prevIsGiant[ENTINDEX(player)] = isBoss;
					player->SetMiniBoss(true);
					if (!isBoss && player->GetActiveTFWeapon() != nullptr) {
						player->GetActiveTFWeapon()->SetTeamNumber(TF_TEAM_RED);
					}
				}
			});
		}
		DETOUR_STATIC_CALL(clientCount, clients, snapshot);
		
		if (cvar_no_footsteps.GetBool()) {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (IsMvMBlueHuman(player) && ENTINDEX(player) < MAX_PLAYERS + 1) {
					bool isBoss = prevIsGiant[ENTINDEX(player)];
					player->SetMiniBoss(isBoss);
					if (!isBoss && player->GetActiveTFWeapon() != nullptr) {
						player->GetActiveTFWeapon()->SetTeamNumber(TF_TEAM_BLUE);
					}
				}
			});
		}
	}
	
	CBaseEntity *laser_dot = nullptr;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_CreateSniperDot)
	{
		if (IsMvMBlueHuman(reinterpret_cast<CTFSniperRifle *>(this)->GetTFPlayerOwner())) {
			return;
		}
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_STATIC(CBaseEntity *, CSniperDot_Create, Vector &origin, CBaseEntity *owner, bool visibleDot)
	{
		return laser_dot = DETOUR_STATIC_CALL(origin, owner, visibleDot);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAllOwnedEntitiesFromWorld, bool explode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool blueHuman = IsMvMBlueHuman(player);
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		DETOUR_MEMBER_CALL(explode);
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
	}
	
	struct ScoreStats
	{
		int m_iPrevDamage;
		int m_iPrevDamageAssist;
		int m_iPrevDamageBoss;
		int m_iPrevHealing;
		int m_iPrevHealingAssist;
		int m_iPrevDamageBlocked;
		int m_iPrevCurrencyCollected;
		int m_iPrevBonusPoints;

		void Reset()
		{
			m_iPrevDamage = 0;
			m_iPrevDamageAssist = 0;
			m_iPrevDamageBoss = 0;
			m_iPrevHealing = 0;
			m_iPrevHealingAssist = 0;
			m_iPrevDamageBlocked = 0;
			m_iPrevCurrencyCollected = 0;
			m_iPrevBonusPoints = 0;
		}
	};
	ScoreStats score_stats[MAX_PLAYERS + 1];

	void DisplayScoreboard(CTFPlayer *playerShow);

	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr && strcmp(event->GetName(), "player_death") == 0 && (event->GetInt( "death_flags" ) & 0x0200 /*TF_DEATH_MINIBOSS*/) == 0) {
			auto player = UTIL_PlayerByUserId(event->GetInt("userid"));
			if (IsMvMBlueHuman(ToTFPlayer(player)))
				event->SetInt("death_flags", (event->GetInt( "death_flags" ) | 0x0200));
		}

		if (event != nullptr && strcmp(event->GetName(), "mvm_reset_stats") == 0) {
			for (auto &stats : score_stats) {
				stats.Reset();
			}
		}

		if (event != nullptr && strcmp(event->GetName(), "player_connect_client") == 0) {
			int index = event->GetInt("index") + 1;
			if (index < (int)ARRAYSIZE(score_stats))
				score_stats[index].Reset();
		}

		if (event != nullptr && strcmp(event->GetName(), "mvm_wave_complete") == 0) {
			CTFPlayerResource *tfres = TFPlayerResource();
			if (tfres != nullptr) {
				for (uint index = 0; index < ARRAYSIZE(score_stats); index++) {
					score_stats[index].m_iPrevDamage += tfres->m_iDamage[index];
					score_stats[index].m_iPrevDamageAssist += tfres->m_iDamageAssist[index];
					score_stats[index].m_iPrevDamageBoss += tfres->m_iDamageBoss[index];
					score_stats[index].m_iPrevDamageBlocked += tfres->m_iDamageBlocked[index];
					score_stats[index].m_iPrevHealing += tfres->m_iHealing[index];
					score_stats[index].m_iPrevHealingAssist += tfres->m_iHealingAssist[index];
					score_stats[index].m_iPrevCurrencyCollected += tfres->m_iCurrencyCollected[index];
					score_stats[index].m_iPrevBonusPoints += tfres->m_iBonusPoints[index];
				}
			}
		}

		// Switch team victory panels so it no longer shows you lost a wave after capturing it
		extern ConVar cvar_force;
		if (event != nullptr && cvar_force.GetBool() && strcmp(event->GetName(), "pve_win_panel") == 0) {
			int teamnum = event->GetInt("winning_team");
			event->SetInt("winning_team", teamnum == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED);
		}
		
		return DETOUR_MEMBER_CALL(event, bDontBroadcast);
	}

	RefCount rc_CTFBot_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		DETOUR_MEMBER_CALL(info);
		
		extern ConVar cvar_force;
		if (cvar_force.GetBool() && bot->GetTeamNumber() == TF_TEAM_RED) {
			bool foundMoreSpies = false;
			ForEachTFBot([&](CTFBot *bot2) {
				if (bot2->GetTeamNumber() == TF_TEAM_RED && bot2->IsAlive() && bot2->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY) {
					foundMoreSpies = true;
					return false;
				}
				return true;
			});
			if (!foundMoreSpies) {
				IGameEvent *event = gameeventmanager->CreateEvent("mvm_mission_update");
				if ( event )
				{
					event->SetInt("class", TF_CLASS_SPY);
					event->SetInt("count", 0);
					gameeventmanager->FireEvent(event);
				}
			}
		}
	}

	class ScoreboardHandler : public IMenuHandler
    {
    public:
        ScoreboardHandler() : IMenuHandler() {
		}
		
        virtual void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }
    };
	ScoreboardHandler scoreboard_handler_def;
	
	void DisplayScoreboard(CTFPlayer *playerShow)
	{
		ScoreboardHandler *handler = new ScoreboardHandler();
		static IBaseMenu *menu = nullptr;
		if (menu != nullptr) {
			menu->Destroy();
		}
        menu = menus->GetDefaultStyle()->CreateMenu(handler, g_Ext.GetIdentity());
        
        menu->SetDefaultTitle("Name                        |Score|Damage|Tank|Healing|Support|Cash");
        menu->SetMenuOptionFlags(MENUFLAG_BUTTON_EXIT | MENUFLAG_NO_SOUND);

		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->IsBot() || player->GetTeamNumber() != TF_TEAM_BLUE) return;
			int index = ENTINDEX(player);
			char buf[256];
			CTFPlayerResource *tfres = TFPlayerResource();
			int support = tfres->m_iDamageAssist[index] + score_stats[index].m_iPrevDamageAssist + tfres->m_iHealingAssist[index] + score_stats[index].m_iPrevHealingAssist + tfres->m_iDamageBlocked[index] + score_stats[index].m_iPrevDamageBlocked + ((tfres->m_iBonusPoints[index] + score_stats[index].m_iPrevBonusPoints) * 25);

			snprintf(buf, sizeof(buf), "%24.24s|%6d|%6d|%6d|%6d|%6d|%4d", player->GetPlayerName(), tfres->m_iTotalScore[index], tfres->m_iDamage[index] + score_stats[index].m_iPrevDamage, tfres->m_iDamageBoss[index] + score_stats[index].m_iPrevDamageBoss, tfres->m_iHealing[index] + score_stats[index].m_iPrevHealing, support, tfres->m_iCurrencyCollected[index] + score_stats[index].m_iPrevCurrencyCollected);
			ItemDrawInfo info1(buf, ITEMDRAW_DEFAULT);
			menu->AppendItem("", info1);
		});
		
		if (playerShow != nullptr) {
        	menu->Display(ENTINDEX(playerShow), 60);
		}
		else {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (player->IsBot()) return;
        		menu->Display(ENTINDEX(player), 60);
			});
		}
	}

	// this argument optimized out, CTFPlayer becomes this
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CVoteController_IsValidVoter)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool blueHuman = IsMvMBlueHuman(ToTFPlayer(player));
		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		
		bool ret = DETOUR_MEMBER_CALL();

		if (blueHuman) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
		return ret;
	}

	CBasePlayer *killed = nullptr;
	bool team_change_back = false;
	DETOUR_DECL_MEMBER(void, CTFGameRules_PlayerKilled, CBasePlayer *pVictim, const CTakeDamageInfo& info)
	{
	//	DevMsg("CTFGameRules::PlayerKilled\n");
		killed = IsMvMBlueHuman(ToTFPlayer(info.GetAttacker())) && pVictim->GetTeamNumber() == TF_TEAM_RED ? pVictim : nullptr;
		team_change_back = false;
		DETOUR_MEMBER_CALL(pVictim, info);
	}

	DETOUR_DECL_MEMBER(void, CTeamplayRules_PlayerKilled, CBasePlayer *pVictim, const CTakeDamageInfo& info)
	{
		if (killed != nullptr && team_change_back) 
			killed->SetTeamNumber(TF_TEAM_RED);
		DETOUR_MEMBER_CALL(pVictim, info);
		killed = nullptr;
	}
	
    DETOUR_DECL_STATIC(CTFWeaponBase *, GetKilleaterWeaponFromDamageInfo, CTakeDamageInfo &info)
	{
		if (killed != nullptr) {
			killed->SetTeamNumber(TF_TEAM_BLUE);
			
			team_change_back = true;
		}

        return DETOUR_STATIC_CALL(info);
    }

	DETOUR_DECL_MEMBER(void, CMultiplayRules_HaveAllPlayersSpeakConceptIfAllowed, int iConcept, int iTeam /* = TEAM_UNASSIGNED */, const char *modifiers)
	{
		if (iConcept == TF_TEAM_RED) {
			iTeam = TF_TEAM_BLUE;
		}
		DETOUR_MEMBER_CALL(iConcept, iTeam, modifiers);
	}
	
	void CallbackScoreboard(CBaseEntity *entity, int clientIndex, DVariant &value, int callbackIndex, SendProp *prop, uintptr_t data) {
		auto player = UTIL_PlayerByIndex(clientIndex);
		if (player != nullptr && ((player->GetTeamNumber() == TF_TEAM_BLUE && (player->m_nButtons & IN_SCORE)) || (player->IsRealPlayer() && player->GetTeamNumber() == TEAM_SPECTATOR))) {
			auto playerTarget = UTIL_PlayerByIndex(data);
			if (playerTarget != nullptr && !playerTarget->IsBot() && playerTarget->GetTeamNumber() == TF_TEAM_BLUE) {
				value.m_Int = TF_TEAM_RED;
			}
			if (playerTarget != nullptr && playerTarget->IsBot() && playerTarget->GetTeamNumber() == TF_TEAM_RED) {		
				value.m_Int = TF_TEAM_BLUE;
			}
		}
	}

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("MvM:JoinTeam_Blue_Allow")
		{
			/* change the team assignment rules */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetTeamAssignmentOverride, "CTFGameRules::GetTeamAssignmentOverride");
			
			/* don't drop reanimators for blue humans */
			MOD_ADD_DETOUR_STATIC(CTFReviveMarker_Create, "CTFReviveMarker::Create");
			
			/* enable upgrading in blue spawn zones via "upgrade" client command */
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand,                "CTFPlayer::ClientCommand");
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnNavAreaChanged,             "CTFPlayer::OnNavAreaChanged");
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientCommandKeyValues,    "CTFGameRules::ClientCommandKeyValues");
			
			/* allow flag pickup and capture depending on convar values */
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToPickUpFlag, "CTFPlayer::IsAllowedToPickUpFlag");
			MOD_ADD_DETOUR_MEMBER(CCaptureZone_ShimTouch,          "CCaptureZone::ShimTouch");
			
			/* disallow attacking while in the blue spawn room */
		//	MOD_ADD_DETOUR_MEMBER(CPlayerMove_StartCommand, "CPlayerMove::StartCommand");
			
			/* fix hardcoded teamnum check when forcing bots to move to team spec at round change */
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent, "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_STATIC(CollectPlayers_CTFPlayer,   "CollectPlayers<CTFPlayer>");
			
			// Allow blue players to ready up
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerReadyStatus_ShouldStartCountdown,   "CTFGameRules::PlayerReadyStatus_ShouldStartCountdown");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBoss_OnTakeDamage,   "CTFBaseBoss::OnTakeDamage");
			// Ignore blue teleporters
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");

			// Disable infinite ammo for players unless toggled
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAmmo, "CTFPlayer::RemoveAmmo");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SentryThink,                  "CObjectSentrygun::SentryThink");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_Attack,                  "CObjectSentrygun::Attack");

			// Stop blue player teleporters from spinning when disabled
			MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_DeterminePlaybackRate, "CObjectTeleporter::DeterminePlaybackRate");

			// Enable fast redeploy for engineers
			MOD_ADD_DETOUR_MEMBER(CBaseObject_ShouldQuickBuild, "CBaseObject::ShouldQuickBuild");

			// Allow scouts to spawn faster
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ShouldRespawnQuickly, "CTFGameRules::ShouldRespawnQuickly");

			// Red sapped robots can be backstabbed from any range
			MOD_ADD_DETOUR_MEMBER(CTFKnife_CanPerformBackstabAgainstTarget, "CTFKnife::CanPerformBackstabAgainstTarget [clone]");

			// Stop blue team sniper dot
			// MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_CreateSniperDot, "CTFSniperRifle::CreateSniperDot");
			//MOD_ADD_DETOUR_STATIC(CSniperDot_Create, "CSniperDot::Create");

			// Forcibly remove blue human buildings when changing classes
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAllOwnedEntitiesFromWorld, "CTFPlayer::RemoveAllOwnedEntitiesFromWorld");

			// Display blue player death notice
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");

			// Show red team spy death message
			//MOD_ADD_DETOUR_MEMBER(CTFBot_Event_Killed, "IGameEventManager2::FireEvent");

			// Make voting work properly for blue players
			MOD_ADD_DETOUR_MEMBER(CVoteController_IsValidVoter, "CVoteController::IsValidVoter [clone]");

			/* fix hardcoded teamnum check when clearing MvM checkpoints */
			this->AddPatch(new CPatch_CollectPlayers_Caller_Regcall<0x0000, 0x0200, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::ClearCheckpoint",
				sizeof(s_Buf_CollectPlayers_Common_Regcall), s_Buf_CollectPlayers_Common_Regcall, s_Mask_CollectPlayers_Common_Regcall, s_CallOff_CollectPlayers_Common_Regcall));
			
			/* fix hardcoded teamnum check when restoring MvM checkpoints */
			this->AddPatch(new CPatch_CollectPlayers_Caller_Regcall<0x0000, 0x0200, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::RestoreCheckpoint",
				sizeof(s_Buf_CollectPlayers_Common_Regcall), s_Buf_CollectPlayers_Common_Regcall, s_Mask_CollectPlayers_Common_Regcall, s_CallOff_CollectPlayers_Common_Regcall));
			
			/* fix hardcoded teamnum check when restoring player currency */
			this->AddPatch(new CPatch_CollectPlayers_Caller_Regcall<0x0000, 0x0100, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue>("CPopulationManager::RestorePlayerCurrency",
				sizeof(s_Buf_CPopulationManager_RestorePlayerCurrency), s_Buf_CPopulationManager_RestorePlayerCurrency, s_Mask_CPopulationManager_RestorePlayerCurrency, s_CallOff_CPopulationManager_RestorePlayerCurrency));
			
			/* fix hardcoded teamnum check when respawning dead players and resetting their sentry stats at wave end */
			this->AddPatch(new CPatch_CollectPlayers_Caller_Regcall<0x0000, 0x0400, TF_TEAM_RED, false, false, CollectPlayers_RedAndBlue_NotBot>("CWave::WaveCompleteUpdate", 
				sizeof(s_Buf_CollectPlayers_WaveCompleteUpdate), s_Buf_CollectPlayers_WaveCompleteUpdate, s_Mask_CollectPlayers_WaveCompleteUpdate, s_CallOff_CollectPlayers_WaveCompleteUpdate));

			// Show only spy bots on the enemy team
			// this->AddPatch(new CPatch_CollectPlayers_Caller1<0x0500, 0x0930, TF_TEAM_BLUE, true, false, CollectPlayers_RedAndBlue>("CTFBot::Event_Killed"));
			
			/* fix hardcoded teamnum checks in the radius spy scan ability */
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_RadiusSpyScan, "CTFPlayerShared::RadiusSpyScan");
			this->AddPatch(new CPatch_CollectPlayers_Caller_Regcall<0x0000, 0x0100, TF_TEAM_BLUE, true, false, CollectPlayers_RadiusSpyScan>("CTFPlayerShared::RadiusSpyScan",
#ifdef PLATFORM_64BITS
				sizeof(s_Buf_CollectPlayers_RadiusSpyScan), s_Buf_CollectPlayers_RadiusSpyScan, s_Mask_CollectPlayers_RadiusSpyScan, s_CallOff_CollectPlayers_RadiusSpyScan));
#else
				sizeof(s_Buf_CollectPlayers_Common_Regcall), s_Buf_CollectPlayers_Common_Regcall, s_Mask_CollectPlayers_Common_Regcall, s_CallOff_CollectPlayers_Common_Regcall));
#endif			
			this->AddPatch(new CPatch_RadiusSpyScan());
			
			/* this is purely for debugging the blue-robots-spawning-between-waves situation */
			MOD_ADD_DETOUR_MEMBER(CTFBot_Spawn, "CTFBot::Spawn");
			// Teleport to bot logic
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned,                  "CTFGameRules::OnPlayerSpawned");

			// Player minigiant stomp logic
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Touch,                  "CTFPlayer::Touch");
			
			// Allow blue players to appear on the scoreboard
			MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks,           "SV_ComputeClientPacks");
			// MOD_ADD_DETOUR_STATIC_PRIORITY(SendTable_WritePropList,"SendTable_WritePropList", HIGH);
			// MOD_ADD_DETOUR_MEMBER(CBaseServer_WriteDeltaEntities,  "CBaseServer::WriteDeltaEntities");
			
			// Robot part fix?
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerKilled, "CTFGameRules::PlayerKilled");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRules_PlayerKilled, "CTeamplayRules::PlayerKilled");
			MOD_ADD_DETOUR_STATIC(GetKilleaterWeaponFromDamageInfo, "GetKilleaterWeaponFromDamageInfo");

			//MOD_ADD_DETOUR_MEMBER(CMultiplayRules_HaveAllPlayersSpeakConceptIfAllowed, "CMultiplayRules::HaveAllPlayersSpeakConceptIfAllowed");
			
			//MOD_ADD_DETOUR_MEMBER(CBaseEntity_DispatchUpdateTransmitState,     "CBaseEntity::DispatchUpdateTransmitState");
			//MOD_ADD_DETOUR_MEMBER(CBaseEntity_ShouldTransmit,                  "CBaseEntity::ShouldTransmit");
			
		}
		virtual bool OnLoad() override
		{
			auto precalc = DT_TFPlayerResource_g_SendTable.GetRef().m_pPrecalc;
			auto &table = DT_PlayerResource_g_SendTable.GetRef();
			SendProp *teamNumProp = nullptr;
			for (int i = 0; i < table.GetNumProps(); i++) {
				if (strcmp(table.GetProp(i)->GetName(), "m_iTeam") == 0) {
					teamNumProp = table.GetProp(i)->GetDataTable()->GetProp(0);
					team_num_table = table.GetProp(i)->GetDataTable();
					break;
				}
			}
			if (teamNumProp != nullptr) {
				for (int i = 0; i < precalc->m_Props.Count(); i++) {
					if (precalc->m_Props[i] == teamNumProp) {
						team_num_prop_index = i;
						break;
					}
				}
			}
			return true;
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void OnDisable() override
		{
			if (TFGameRules()->IsMannVsMachineMode()) {
				ForEachTFPlayer([](CTFPlayer *player){
					if (player->GetTeamNumber() != TF_TEAM_BLUE) return;
					if (player->IsBot())                         return;
					player->ForceChangeTeam(TF_TEAM_RED, true);
				});
			}
		}

		virtual std::vector<std::string> GetRequiredMods() { return {"Etc:SendProp_Override"};}

		virtual void FrameUpdatePostEntityThink() override
		{
			static bool prevPressedScore[MAX_PLAYERS + 1] {};
			static int propOverrideIds[MAX_PLAYERS + 1] {};
			static bool someonePressedScorePre = false;
			bool someonePressedScore = false;
			if (TFGameRules()->IsMannVsMachineMode()) {
				ForEachTFPlayer([&someonePressedScore](CTFPlayer *player){
					
					int plIndex = player->entindex();
					if (!someonePressedScore && player->IsRealPlayer() && player->GetTeamNumber() == TEAM_SPECTATOR) {
						someonePressedScore = true;
					}
					if (player->GetTeamNumber() != TF_TEAM_BLUE) return;
					if (player->IsBot())                         return;
					if (plIndex >= (int)ARRAYSIZE(prevPressedScore)) return;
					
					// if(player->m_nButtons & IN_SCORE && !prevPressedScore[plIndex]) {
					// 	IBaseMenu *menu = nullptr;
					// 	menus->GetDefaultStyle()->GetClientMenu(plIndex, (void **)&menu);
					// 	if (menu != nullptr && menu->GetDefaultTitle() != nullptr && strcmp(menu->GetDefaultTitle(), "Name                        |Score|Damage|Tank|Healing|Support|Cash") == 0) {
					// 		auto panel = menus->GetDefaultStyle()->CreatePanel();
					// 		ItemDrawInfo info1("", ITEMDRAW_RAWLINE);
					// 		panel->DrawItem(info1);
					// 		ItemDrawInfo info2("", ITEMDRAW_RAWLINE);
					// 		panel->DrawItem(info2);
					// 		panel->SetSelectableKeys(255);
					// 		panel->SendDisplay(plIndex, &scoreboard_handler_def, 1);
					// 	}
					// 	else {
					// 		DisplayScoreboard(player);
					// 	}
						
					// }
					if (player->m_nButtons & IN_SCORE) {
						someonePressedScore = true;
					}

					if (gpGlobals->tickcount % 3 == 1 && cvar_spawn_protection.GetBool() ) {
						if (PointInRespawnRoom(player, player->WorldSpaceCenter(), true)) {
							player->m_Shared->AddCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED, 0.500f);
							
							if (cvar_spawn_no_shoot.GetBool()) {
								player->m_Shared->m_flStealthNoAttackExpire = gpGlobals->curtime + 0.500f;
								// alternative method: set m_Shared->m_bFeignDeathReady to true
							}
						}
						else if (player->m_Shared->m_flStealthNoAttackExpire <= gpGlobals->curtime){
							player->m_Shared->RemoveCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED);
						}

					}

					// Add self blast penalty for flag bearing players
					if (!player->IsMiniBoss() && player->GetActiveTFWeapon() != nullptr) {
						static ConVarRef intel_speed("tf_mvm_bot_flag_carrier_movement_penalty");
						CEconItemAttributeDefinition *attr_def = nullptr;
						if (attr_def == nullptr)
							attr_def = GetItemSchema()->GetAttributeDefinitionByName("self dmg push force decreased");
						
						auto attr = player->GetActiveTFWeapon()->GetItem()->GetAttributeList().GetAttributeByID(attr_def->GetIndex());
						float desiredSpeed = (intel_speed.GetFloat() + 1.0f) * 0.5f;
						if (attr == nullptr && player->HasItem())
							player->GetActiveTFWeapon()->GetItem()->GetAttributeList().SetRuntimeAttributeValue(attr_def, desiredSpeed);
						else if (attr != nullptr && !player->HasItem() && attr->GetValuePtr()->m_Float == desiredSpeed)
							player->GetActiveTFWeapon()->GetItem()->GetAttributeList().RemoveAttribute(attr_def);
					}

					if (cvar_infinite_cloak.GetBool()) {
						bool should_refill_cloak = true;
						
						if (!cvar_infinite_cloak_deadringer.GetBool()) {
							/* check for attribute "set cloak is feign death" */
							auto invis = rtti_cast<CTFWeaponInvis *>(player->Weapon_GetWeaponByType(TF_WPN_TYPE_ITEM1));
							if (invis != nullptr && CAttributeManager::AttribHookValue<int>(0, "set_weapon_mode", invis) == 1) {
								should_refill_cloak = false;
							}
						}
						
						if (should_refill_cloak) {
							player->m_Shared->m_flCloakMeter = 100.0f;
						}
					}

					if (gpGlobals->tickcount % 5 == 0 && !(player->m_nButtons & IN_SCORE)) {
						hudtextparms_t textparam;
						textparam.channel = 0;
						textparam.x = 0.042f;
						textparam.y = 0.93f;
						textparam.effect = 0;
						textparam.r1 = 255;
						textparam.r2 = 255;
						textparam.b1 = 255;
						textparam.b2 = 255;
						textparam.g1 = 255;
						textparam.g2 = 255;
						textparam.a1 = 0;
						textparam.a2 = 0; 
						textparam.fadeinTime = 0.f;
						textparam.fadeoutTime = 0.f;
						textparam.holdTime = 0.5f;
						textparam.fxTime = 1.0f;
						DisplayHudMessageAutoChannel(player, textparam, CFmtStr("$%d", player->GetCurrency()), 3);
					}
				});

				if (someonePressedScore && !someonePressedScorePre) {
					auto mod = PlayerResource()->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");// m_iTeam.SetIndex(gpGlobals->tickcount % 2, i);

					for (int i = 1; i <= gpGlobals->maxClients && i <= MAX_PLAYERS; i++) {
						propOverrideIds[i] = mod->AddOverride(CallbackScoreboard, team_num_prop_index + i, team_num_table->GetProp(i), i);
					}
				}
				else if (!someonePressedScore && someonePressedScorePre) {
					auto mod = PlayerResource()->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
					for (int i = 1; i <= gpGlobals->maxClients && i <= MAX_PLAYERS; i++) {
						mod->RemoveOverride(propOverrideIds[i]);
					}
				}
				someonePressedScorePre = someonePressedScore;
			}
		}
	};
	CMod s_Mod;
	
	void ToggleModActive();

	/* by way of incredibly annoying persistent requests from Hell-met,
	 * I've acquiesced and made this mod convar non-notifying (sigh) */
	ConVar cvar_enable("sig_mvm_jointeam_blue_allow", "0", /*FCVAR_NOTIFY*/FCVAR_NONE | FCVAR_GAMEDLL,
		"Mod: permit client command 'jointeam blue' from human players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			ToggleModActive();
			
			Mod::MvM::Player_Limit::RecalculateSlots();
		});
	
	/* default: admin-only mode ENABLED */
	ConVar cvar_adminonly("sig_mvm_jointeam_blue_allow_adminonly", "1", /*FCVAR_NOTIFY*/FCVAR_NONE | FCVAR_GAMEDLL,
		"Mod: restrict this mod's functionality to SM admins only"
		" [NOTE: missions with WaveSchedule param AllowJoinTeamBlue 1 will OVERRIDE this and allow non-admins for the duration of the mission]",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			Mod::MvM::Player_Limit::RecalculateSlots();
		});

	// force blue team joining, also loads the mod if not loaded
	ConVar cvar_force("sig_mvm_jointeam_blue_allow_force", "0", /*FCVAR_NOTIFY*/FCVAR_NONE | FCVAR_GAMEDLL,
		"Mod: force players to join blue team",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			ToggleModActive();
			Mod::MvM::Player_Limit::RecalculateSlots();
			if (cvar_force.GetBool()) {
				ForEachTFPlayer([&](CTFPlayer *player) {
					if (player->GetTeamNumber() == TF_TEAM_RED && player->IsRealPlayer()) {
						player->HandleCommand_JoinTeam("blue");
					}
				});
			}
		});
	
	void ToggleModActive()
	{
		s_Mod.Toggle(cvar_enable.GetBool() || cvar_force.GetBool());
	}

	bool PlayersCanJoinBlueTeam()
	{
		return s_Mod.IsEnabled() && (cvar_force.GetBool() || (cvar_max.GetInt() != 0 && !cvar_adminonly.GetBool()));
	}
}
