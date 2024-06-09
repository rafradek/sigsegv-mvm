#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#ifdef SE_IS_TF2
#include "stub/tfbot.h"
#include "stub/tfweaponbase.h"
#else
#include "stub/baseplayer.h"
#include "stub/baseweapon.h"
#endif
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "sdk2013/mempool.h"
#include "mem/protect.h"
#include "util/prop_helper.h"
#include "util/iterate.h"

#define STATICPROP_EHANDLE_MASK 0x40000000
namespace Mod::Perf::Virtual_Call_Optimize
{
    struct CPatch_ReplaceFunction : public CPatch
	{
		CPatch_ReplaceFunction(const std::string &function, IProp *add, IProp *deref) : CPatch(9), func(function), data(9), propAdd(add), propDeref(deref) {
            this->data.CopyFrom(data);
        }
		
		virtual const char *GetFuncName() const override { return func.c_str(); }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
        IProp *propAdd;
        IProp *propDeref;

        std::string func;
        ByteBuf data;
	};

    uint8_t s_Buf_CBaseEntity_GetNetworkable[] {
        0x8b, 0x44, 0x24, 0x04,
        0x83, 0xc0, 0x14,
        0xc3, 0x90, 0x90
    };

    constexpr uint8_t s_Buf_CBaseEntity_GetTeamNumber[] = {
        /*0:*/  0x8b, 0x44, 0x24, 0x04,          
        /*4:*/  0x8b, 0x80, 0x18, 0x02, 0x00, 0x00,
        /*10:*/ 0xc3                     
	};
        
    struct CPatch_CBaseEntity_GetTeamNumber : public CPatch
	{
		CPatch_CBaseEntity_GetTeamNumber() : CPatch(sizeof(s_Buf_CBaseEntity_GetTeamNumber)) {}
		
		virtual const char *GetFuncName() const override { return "CBaseEntity::GetTeamNumber"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CBaseEntity_GetTeamNumber);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            buf.SetDword(0x4 + 2, CBaseEntity::s_prop_m_iTeamNum.GetOffsetDirect());
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
            buf.SetDword(0x4 + 2, CBaseEntity::s_prop_m_iTeamNum.GetOffsetDirect());
			return true;
		}
	};
    
    constexpr uint8_t s_Buf_CBasePlayer_IsPlayer[] = {
        /*0:*/  0xb8, 0x01, 0x00, 0x00, 0x00,          //mov    eax,0x1
        /*5:*/  0xc3                      //ret
	};
        
    struct CPatch_CBasePlayer_IsPlayer : public CPatch
	{
		CPatch_CBasePlayer_IsPlayer() : CPatch(sizeof(s_Buf_CBasePlayer_IsPlayer)) {}
		
		virtual const char *GetFuncName() const override { return "CBasePlayer::IsPlayer"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CBasePlayer_IsPlayer);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
	};
    
    constexpr uint8_t s_Buf_CBaseEntity_GetBaseEntity[] = {
        0x8b, 0x44, 0x24, 0x04,             //mov    eax,DWORD PTR [esp+0x4]
        0xc3                      //ret
	};
        
    struct CPatch_CBaseEntity_GetBaseEntity : public CPatch
	{
		CPatch_CBaseEntity_GetBaseEntity() : CPatch(sizeof(s_Buf_CBaseEntity_GetBaseEntity)) {}
		
		virtual const char *GetFuncName() const override { return "CBaseEntity::GetBaseEntity"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CBaseEntity_GetBaseEntity);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
	};



    constexpr uint8_t s_Buf_CStaticPropMgr_IsStaticProp[] = {
        // return !pHandleEntity || (size_t)(*pHandleEntity) == 0x44444444(44444444);
#ifdef PLATFORM_64BITS
        // mov     eax, 1
        // test    rsi, rsi
        // je      .L1
        // movabs  rax, 4919131752989213764
        // cmp     QWORD PTR [rsi], rax
        // sete    al
        // .L1:
        // ret

        0xb8, 0x01, 0x00, 0x00, 0x00, 0x48, 0x85, 0xf6, 
        0x74, 0x10, 0x48, 0xb8, 0x44, 0x44, 0x44, 0x44, 
        0x44, 0x44, 0x44, 0x44, 0x48, 0x39, 0x06, 0x0f, 
        0x94, 0xc0, 0xc3, 
#else
        0x52,
        0x8b, 0x54, 0x24, 0x0c,             //mov    edx,DWORD PTR [esp+0x8 + 0x04] 1
        0xb8, 0x01, 0x00, 0x00, 0x00,          //mov    eax,0x1 5
        0x85, 0xd2,                   //test   edx,edx a
        0x74, 0x09,                   //je     16 <L1> c
        0x81, 0x3a, 0xba, 0x7e, 0x87, 0x02,       //cmp    DWORD PTR [edx],0x2877eba e
        0x0f, 0x94, 0xc0,                //sete   al
        // 00000017 <L1>:
        0x5a,
        0xc3                      //ret
#endif
	};
        
    struct CPatch_CStaticPropMgr_IsStaticProp : public CPatch
	{
		CPatch_CStaticPropMgr_IsStaticProp() : CPatch(sizeof(s_Buf_CStaticPropMgr_IsStaticProp)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticPropMgr::IsStaticProp"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CStaticPropMgr_IsStaticProp);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
            buf.SetQword(0xc, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#else
            buf.SetDword(0xe + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#endif
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
#ifdef PLATFORM_64BITS
            buf.SetQword(0xc, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#else
            buf.SetDword(0xe + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#endif
			return true;
		}
	};

    constexpr uint8_t s_Buf_CStaticPropMgr_GetStaticProp[] = {
        // if (!pHandleEntity) return nullptr;
        // if ((size_t)(*pHandleEntity) != 0x44444444(44444444)) return nullptr;
        // return pHandleEntity+2;
#ifdef PLATFORM_64BITS
        // xor     eax, eax
        // test    rsi, rsi
        // je      .L6
        // movabs  rdx, 4919131752989213764
        // cmp     QWORD PTR [rsi], rdx
        // jne     .L6
        // lea     rax, [rsi+16]
        // ret
        // .L6:
        // ret

        0x31, 0xc0, 0x48, 0x85, 0xf6, 0x74, 0x14, 0x48, 
        0xba, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 
        0x44, 0x48, 0x39, 0x16, 0x75, 0x05, 0x48, 0x8d, 
        0x46, 0x10, 0xc3, 0xc3, 
#else
        0x52,
        0x8b, 0x44, 0x24, 0x0c,       //      mov    eax,DWORD PTR [esp+0x8 + 0x4]
        0x31, 0xd2,                   //xor    edx,edx
        0x85, 0xc0,                   //test   eax,eax
        0x74, 0x0b,                   //je     15 <L7>
        0x81, 0x38, 0xe4, 0x19, 0x86, 0x02,       //cmp    DWORD PTR [eax],0x28619e4
        0x75, 0x03,                   //jne    15 <L7>
        0x8d, 0x50, 0x08,                //lea    edx,[eax+0x08]
        //00000016 <L7>:
        0x89, 0xd0,                  // mov    eax,edx
        0x5a,
        0xc3,                      //ret
#endif
	};
        
    struct CPatch_CStaticPropMgr_GetStaticProp : public CPatch
	{
		CPatch_CStaticPropMgr_GetStaticProp() : CPatch(sizeof(s_Buf_CStaticPropMgr_GetStaticProp)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticPropMgr::GetStaticProp"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CStaticPropMgr_GetStaticProp);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
            buf.SetQword(0x9, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#else
            buf.SetDword(0xb + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#endif
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
#ifdef PLATFORM_64BITS
            buf.SetQword(0x9, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#else
            buf.SetDword(0xb + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
#endif
			return true;
		}
	};

    constexpr uint8_t s_Buf_CStaticPropMgr_GetStaticPropIndex[] = {
        0x55, 
        0x89, 0xE5,
        0x8B, 0x45, 0x0C, 
        0x85, 0xC0, 
        0x74, 0x0E, 
        0x8B, 0x40, 0x34,
        0x5D, 
        0x25, 0xFF, 0x0F, 0x00, 0x00,
        0xC3, 
        0x8D, 0x74, 0x26, 0x00,
        0x31, 0xC0,
        0x5D, 
        0xC3
        // /*0:  */0x8b, 0x44, 0x24, 0x0c,             //mov    eax,DWORD PTR [esp+0xc]
        // /*4:  */0x85, 0xc0,                   //test   eax,eax
        // /*6:  */0x74, 0x09,                   //je     11 <L13>
        // /*8:  */0x8b, 0x40, 0x34,                //mov    eax,DWORD PTR [eax+0x34]
        // /*b:  */0x25, 0xff, 0x0f, 0x00, 0x00,          //and    eax,0xfff
        // /*10: */0xc3,                      //ret
        // //00000011 <L13>:
        // /*11: */0x31, 0xc0,                   //xor    eax,eax
        // /*13: */0xc3                      //ret

	};
        
    struct CPatch_CStaticPropMgr_GetStaticPropIndex : public CPatch
	{
		CPatch_CStaticPropMgr_GetStaticPropIndex() : CPatch(sizeof(s_Buf_CStaticPropMgr_GetStaticPropIndex)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticPropMgr::GetStaticPropIndex"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CStaticPropMgr_GetStaticPropIndex);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            //buf.SetDword(0xa + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
            //buf.SetDword(0xa + 2, (uintptr_t)RTTI::GetVTable("11CStaticProp"));
			return true;
		}
	};

    constexpr uint8_t s_Buf_CStaticProp_GetEntityHandle[] = {
        0x31, 0xc0,                      //push   ebp 0
        0xc3
	};
        
    struct CPatch_CStaticProp_GetEntityHandle : public CPatch
	{
		CPatch_CStaticProp_GetEntityHandle() : CPatch(sizeof(s_Buf_CStaticProp_GetEntityHandle)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticProp::GetEntityHandle"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CStaticProp_GetEntityHandle);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
	};

    constexpr uint8_t s_Buf_CStaticProp_GetRefEHandle[] = {
        0x31, 0xc0,                      //push   ebp 0
        0xc3
	};
        
    struct CPatch_CStaticProp_GetRefEHandle : public CPatch
	{
		CPatch_CStaticProp_GetRefEHandle() : CPatch(sizeof(s_Buf_CStaticProp_GetRefEHandle)) {}
		
		virtual const char *GetFuncName() const override { return "CStaticProp::GetRefEHandle"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
            // auto addr = (uint8_t *)AddrManager::GetAddr("CEconItemView::GetStaticData");
            // Msg("Pre enable:");
            // for (int i = 0; i<48;i++) {
            //     Msg(" %02hhX", *(addr+i));
            // }
            // Msg("\n");
			buf.CopyFrom(s_Buf_CStaticProp_GetRefEHandle);
			
			mask.SetAll(0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			mask.SetAll(0xFF);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			return true;
		}
	};

    std::set<uint8_t *> addresses;

    enum DerefMode
    {
        DEREF_INT,
        DEREF_SHORT,
        DEREF_BYTE
    };
    bool ReplaceMemory(uint8_t *buf, int callSize, int add, int deref1 = INT_MIN, int deref2 = INT_MIN, DerefMode derefMode = DEREF_INT)
    {
        size_t size = callSize;
        DevMsg("Size %d\n", size);
        auto callReg = buf[-2] & 7;
        uint8_t thisReg = 255;
        uint8_t derefDestReg = 255;
        uint8_t derefSourceReg = 255;
        bool needToMov = true;
        std::vector<uint8_t> carryOverPostEsp;
        std::vector<uint8_t> carryOverPreEsp;
        //Skip some mov operations if they are 
        while (true) {
            int toSkip = 0;
            if (buf[-size-6] == 0x8D && (buf[-size-5] & 0x80) == 0x80){
                toSkip += 6;
            }
            if (buf[-size-6] == 0x89 && (buf[-size-5] & 0xf8) == 0x80){
                toSkip += 6;
            }
            else if (buf[-size-5] == 0xBF){
                toSkip += 5;
            }
            else if (buf[-size-3] == 0x89 && buf[-size-1] != 0x24) {
                toSkip += 3;
            }
            else if (buf[-size-3] == 0x8d && (buf[-size-2] & 0x80) == 0) {
                toSkip += 3;
            }
            else if (buf[-size-2] == 0x89) {
                toSkip += 2;
            }
            if (toSkip > 0) {
                for (int i = 1; i <= toSkip; i++) {
                    carryOverPostEsp.insert(carryOverPostEsp.begin(),buf[-size-i]);
                }
                size += toSkip;
            }
            else {
                break;
            }
        }
        if (buf[-size-3] == 0x89 && buf[-size-1] == 0x24) {
            thisReg = (buf[-size-2] & 0x38) >> 3;
            size+=3;

            int pos = -size;
            // while (buf[pos - 3] == 0x89 && (buf[pos - 4] & 0xC7) == 0x45) {
            //     carryOver.push_back(buf[pos - 3]);
            //     carryOver.push_back(buf[pos - 2]);
            //     carryOver.push_back(buf[pos - 1]);
            //     pos -= 3;
            // }
            if (buf[pos - 2] == 0x8B) {
                derefDestReg = (buf[pos - 1] & 0x38) >> 3;
                derefSourceReg = buf[pos - 1] & 0x7;
            }
        }
        else {
            return false;
        }
        if (thisReg == 0 /*EAX*/) {
            needToMov = false;
        }

        if (thisReg != 255/*memcmp(buf-size, pattern.find.data(), size - 1) == 0*/) {
            std::vector<uint8_t> replace;
            DevMsg("Pre-Fix:");
            for (int i = -16; i<16;i++) {
                DevMsg(" %02hhX", *(buf+i));
            }
            DevMsg("\n");
            
            for (auto byte : carryOverPostEsp) {
                replace.push_back(byte);
            }
            if (needToMov) {
                replace.push_back(0x89);
                replace.push_back(0xC0 + (thisReg << 3));
            }
            if (add != 0) {
                if (add <= 0x7F) {
                    replace.push_back(0x83);
                    replace.push_back(0xc0);
                    replace.push_back((uint8_t)add);
                }
                else {
                    replace.push_back(0x05);
                    replace.push_back((uint8_t)(add & 0xFF));
                    replace.push_back((uint8_t)((add >> 8) & 0xFF));
                    replace.push_back((uint8_t)((add >> 16) & 0xFF));
                    replace.push_back((uint8_t)((add >> 24) & 0xFF));
                }
            }
            if (deref1 != INT_MIN) {
                
                if (derefMode == DEREF_INT) {
                    replace.push_back(0x8b);
                }
                else if (derefMode == DEREF_SHORT) {
                    replace.push_back(0x0f);
                    replace.push_back(0xb7);
                }
                else {
                    replace.push_back(0x0f);
                    replace.push_back(0xb6);
                }
                if (deref1 <= 0x7f) {
                    replace.push_back(0x40);
                    replace.push_back((uint8_t)(deref1 & 0xFF));
                }
                else {
                    
                    replace.push_back(0x80);
                    replace.push_back((uint8_t)(deref1 & 0xFF));
                    replace.push_back((uint8_t)((deref1 >> 8) & 0xFF));
                    replace.push_back((uint8_t)((deref1 >> 16) & 0xFF));
                    replace.push_back((uint8_t)((deref1 >> 24) & 0xFF));
                }
            }
            if (deref2 != INT_MIN) {
                replace.push_back(0x8b);
                
                if (deref1 <= 0x7f) {
                    replace.push_back(0x40);
                    replace.push_back((uint8_t)(deref2 & 0xFF));
                }
                else {
                    replace.push_back(0x80);
                    
                    replace.push_back((uint8_t)(deref2 & 0xFF));
                    replace.push_back((uint8_t)((deref2 >> 8) & 0xFF));
                    replace.push_back((uint8_t)((deref2 >> 16) & 0xFF));
                    replace.push_back((uint8_t)((deref2 >> 24) & 0xFF));
                }
            }
            
            // Dereference destination register is eax, can be removed
            if (derefDestReg == 0 /*EAX*/ && carryOverPostEsp.empty() && replace.size() > size) {
                size += 2;
            }

            // Skip if cannot fit replace data
            if (replace.size() > size) {
                DevMsg("Cannot fit replace data of size %d into %d\n Replace Data: ", replace.size(), size);
                    for (size_t i = 0; i<replace.size();i++) {
                    DevMsg(" %02hhX", replace[i]);
                }
                DevMsg("\n");
                return false;
            }
            MemProtModifier_RX_RWX(buf-size, size);
            replace.insert(replace.begin(), size-replace.size(), 0x90);
            memcpy(buf-size, replace.data(), size);
            DevMsg("Post-Fix:");
            for (int i = -16; i<16;i++) {
                DevMsg(" %02hhX", *(buf+i));
            }
            DevMsg("\n");
            return true;
        }
        return false;
    }
    bool OptimizeVirtual(uint8_t *buf, int vtableOffset, int add = 0, int deref1 = INT_MIN, int deref2 = INT_MIN, DerefMode derefMode = DEREF_INT)
    {
        bool success = false;
        if (!addresses.contains(buf)) {
            // Call register+offset
            if ((vtableOffset != 0 && *(buf-3) == (uint8_t) 0xFF && *(buf-1) == (uint8_t) vtableOffset) || 
                (vtableOffset == 0 && *(buf-2) == (uint8_t) 0xFF)) {

                int i = 0;
                //for (auto &pattern : replacePatterns) {
                    i++;
                    success = ReplaceMemory(buf, vtableOffset == 0 ? 2 : 3, add, deref1, deref2, derefMode);
                    if (success) {
                        DevMsg("Optimized function %p of offset %d with pattern %d\n", buf, vtableOffset, i);
                        //break;
                    }
                //}
                
                if (!success) {
                    DevMsg("\nFailed optimizing function %p of offset %d with add = %d, deref1 = %d\n", buf, vtableOffset, add, deref1);
                    for (int i = -16; i<16;i++) {
                        DevMsg(" %02hhX", *(buf+i));
                    }
                    DevMsg("\n");
                    //raise(SIGTRAP);
                }
            }
            addresses.insert(buf);
        }
        return success;
    }

    std::vector<void *> getbaseenity_callers;
    DETOUR_DECL_MEMBER(CBaseEntity *, CBaseEntity_GetBaseEntity)
	{
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x18)
            OptimizeVirtual(addr, 0x18, 0);
        return reinterpret_cast<CBaseEntity *>(this);
    }
    DETOUR_DECL_MEMBER(void *, CBaseEntity_GetNetworkable)
	{
        void *result = DETOUR_MEMBER_CALL();
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x14) {
            static const uintptr_t offset = (uintptr_t) result - (uintptr_t)this;
            OptimizeVirtual(addr, 0x14, (int) offset);
        }
        return result;
    }
    DETOUR_DECL_MEMBER(void *, CBaseEntity_GetCollideable)
	{
        void *result = DETOUR_MEMBER_CALL();
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x10) {
            static const uintptr_t offset = (uintptr_t) result - (uintptr_t)this;
            OptimizeVirtual(addr, 0x10, (int) offset);
        }
        return result;
    }
    DETOUR_DECL_MEMBER(void *, CBaseEntity_GetRefEHandle)
	{
        void *result = DETOUR_MEMBER_CALL();
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x0c) {
            static const uintptr_t offset = (uintptr_t) result - (uintptr_t)this;
            //Msg("Offset %d %d\n", offset, CBaseEntity::s_prop_m_RefEHandle.GetOffsetDirect());
            OptimizeVirtual(addr, 0x0c, (int) offset);
        }
        
        return result;
    }
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetEdict)
	{
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x08) {
            OptimizeVirtual(addr, 0x08, 0, 0x0C);
        }
        return DETOUR_MEMBER_CALL();
    }
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetEntityHandle)
	{
        
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x00) {
            OptimizeVirtual(addr, 0x00, 0, 0x08);
        }
        return DETOUR_MEMBER_CALL();
    }
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetServerClass)
	{
        //Not optimizable
        return DETOUR_MEMBER_CALL();
    }
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetBaseEntity)
	{
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x1c) {
            OptimizeVirtual(addr, 0x1c, 0, 0x08);
        }
        return DETOUR_MEMBER_CALL();
    }
    CProp_DataMap prop_classname("CBaseEntity", "m_iClassname", nullptr);
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetClassName)
	{
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x0c) {
            OptimizeVirtual(addr, 0x0c, 0, 0x08, prop_classname.GetOffsetDirect());
        }
        return DETOUR_MEMBER_CALL();
    }
    DETOUR_DECL_MEMBER(void *, CServerNetworkProperty_GetPVSInfo)
	{
        
        void *result = DETOUR_MEMBER_CALL();
        
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x20) {
            static const uintptr_t offset = (uintptr_t) result - (uintptr_t)this;
            OptimizeVirtual(addr, 0x20, offset);
        }
        

        return result;
    }
    CProp_SendProp prop_model_index("CBaseEntity", "m_nModelIndex", nullptr, "CBaseEntity", nullptr);
    DETOUR_DECL_MEMBER(void *, CBaseEntity_GetModelIndex)
	{
        auto addr = (uint8_t*)__builtin_return_address(0);
        if (addr[-3] == 0xFF && addr[-1] == 0x1c) {
            OptimizeVirtual(addr, 0x1C, 0, prop_model_index.GetOffsetDirect(), INT_MIN, DEREF_SHORT);
        }
        return DETOUR_MEMBER_CALL();
    }
    
    DETOUR_DECL_MEMBER(void *, CCollisionProperty_GetEntityHandle)
	{
        OptimizeVirtual((uint8_t*)__builtin_return_address(0), 0x00, 0, 0x04);
        void *result = DETOUR_MEMBER_CALL();
        return result;
    }

    DETOUR_DECL_MEMBER(int, CCollisionProperty_GetSolid)
	{
        OptimizeVirtual((uint8_t*)__builtin_return_address(0), 0x34, 0, 0x41, INT_MIN, DEREF_BYTE);
        return DETOUR_MEMBER_CALL();
    }

    DETOUR_DECL_MEMBER(int, CCollisionProperty_GetSolidFlags)
	{
        OptimizeVirtual((uint8_t*)__builtin_return_address(0), 0x38, 0, 0x3c, INT_MIN, DEREF_SHORT);
        return DETOUR_MEMBER_CALL();
    }


    DETOUR_DECL_MEMBER(bool, CStaticPropMgr_IsStaticProp, IHandleEntity *pHandleEntity)
	{
        return pHandleEntity == nullptr || *((uintptr_t *)pHandleEntity) == (uintptr_t)RTTI::GetVTable("11CStaticProp");
    }

    DETOUR_DECL_MEMBER(ICollideable *, CStaticPropMgr_GetStaticProp, IHandleEntity *pHandleEntity)
	{
        if (pHandleEntity == nullptr) return nullptr;

        if (*((uintptr_t *)pHandleEntity) == 3341244/*(uintptr_t)RTTI::GetVTable("11CStaticProp")*/) {
            return (ICollideable *)((uintptr_t)pHandleEntity + 0x08);
        }
        return nullptr;
    }

    DETOUR_DECL_MEMBER(int, CStaticPropMgr_GetStaticPropIndex, IHandleEntity *pHandleEntity)
	{
        if (pHandleEntity == nullptr) return 0;
        int offset = 0x34;
        return ((CBaseHandle *)((uintptr_t)pHandleEntity + offset))->GetEntryIndex();
    }

    MemberFuncThunk<IStaticPropMgr *, void> CStaticPropMgr_LevelInit("CStaticPropMgr::LevelInit");
    MemberFuncThunk<IStaticPropMgr *, void> CStaticPropMgr_LevelShutdown("CStaticPropMgr::LevelShutdown");
    GlobalThunk<IStaticPropMgr> s_StaticPropMgr("s_StaticPropMgr");
    
    VHOOK_DECL(bool, CBasePlayer_IsBotOfType, int type)
    {
        return false;
    }
    VHOOK_DECL(bool, CTFBot_IsBotOfType, int type)
    {
        return true;
    }

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf::Virtual_Call_Optimize")
		{
            //MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetServerClass, "CServerNetworkProperty::GetServerClass");
            //MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetBaseNetworkable, "CServerNetworkProperty::GetBaseNetworkable");
            //MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_AreaNum, "CServerNetworkProperty::AreaNum");
            //MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetTeamNumber, "CBaseEntity::GetTeamNumber");
            //MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetModelName, "CBaseEntity::GetModelName");
            //MOD_ADD_DETOUR_MEMBER(CCollisionProperty_GetEntityHandle, "CCollisionProperty::GetEntityHandle");
            //MOD_ADD_DETOUR_MEMBER(CCollisionProperty_GetSolid, "CCollisionProperty::GetSolid");
            //MOD_ADD_DETOUR_MEMBER(CCollisionProperty_GetSolidFlags, "CCollisionProperty::GetSolidFlags");
            
            //MOD_ADD_DETOUR_MEMBER(CStaticPropMgr_IsStaticProp, "CStaticPropMgr::IsStaticProp");
            //MOD_ADD_DETOUR_MEMBER(CStaticPropMgr_GetStaticProp, "CStaticPropMgr::GetStaticProp");
            //MOD_ADD_DETOUR_MEMBER(CStaticPropMgr_GetStaticPropIndex, "CStaticPropMgr::GetStaticPropIndex");

            // MOD_ADD_DETOUR_MEMBER(CStaticProp_Init, "CStaticProp::Init");
            // MOD_ADD_DETOUR_MEMBER(CSpatialPartition_CreateHandle, "CSpatialPartition::CreateHandle");

            // Rewrite those functions with assumption that an entity handle is static prop if vtable is of static prop
            this->AddPatch(new CPatch_CStaticPropMgr_IsStaticProp());
            this->AddPatch(new CPatch_CStaticPropMgr_GetStaticProp());
//          this->AddPatch(new CPatch_CStaticPropMgr_GetStaticPropIndex());
            
            // this->AddPatch(new CPatch_CStaticProp_GetEntityHandle());
            // this->AddPatch(new CPatch_CStaticProp_GetRefEHandle());
            
            // Rewrite those functions but with less instructions (no prologue) therefore faster
#ifndef SE_IS_TF2
            this->AddPatch(new CPatch_CBasePlayer_IsPlayer());
#endif
            //this->AddPatch(new CPatch_CBaseEntity_GetBaseEntity());
#ifndef PLATFORM_64BITS
            this->AddPatch(new CPatch_CBaseEntity_GetTeamNumber());
#endif
            
#ifdef SE_IS_TF2
            // Automatically assume that IsBotOfType is asking for bots of type 1337 (CTFBot)
            MOD_ADD_VHOOK(CBasePlayer_IsBotOfType, TypeName<CBasePlayer>(), "CBasePlayer::IsBotOfType");
            MOD_ADD_VHOOK(CBasePlayer_IsBotOfType, TypeName<CTFPlayer>(), "CBasePlayer::IsBotOfType");
            MOD_ADD_VHOOK(CTFBot_IsBotOfType, TypeName<CTFBot>(), "CBasePlayer::IsBotOfType");
#endif
		}
	};
	CMod s_Mod;
	
    
	class CModInlineVirtuals : public IMod
	{
	public:
		CModInlineVirtuals() : IMod("Perf::Virtual_Call_Optimize_Inline_Virtuals")
		{
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetBaseEntity, "CBaseEntity::GetBaseEntity");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetNetworkable, "CBaseEntity::GetNetworkable");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetCollideable, "CBaseEntity::GetCollideable");
            MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetClassName, "CServerNetworkProperty::GetClassName");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetModelIndex, "CBaseEntity::GetModelIndex");

            // uint8_t buf_CBaseEntity_GetBaseEntity[] {
            //     0x8b, 0x44, 0x24, 0x04,
            //     0xc3
            // };
            // this->AddPatch(new CPatch_ReplaceFunction("CBaseEntity::GetBaseEntity", buf_CBaseEntity_GetBaseEntity, sizeof(buf_CBaseEntity_GetBaseEntity)));


            // Non desirable
            // MOD_ADD_DETOUR_MEMBER(CBaseEntity_GetRefEHandle, "CBaseEntity::GetRefEHandle");
            // MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetEdict, "CServerNetworkProperty::GetEdict");
            // MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetBaseEntity, "CServerNetworkProperty::GetBaseEntity");
            // MOD_ADD_DETOUR_MEMBER(CServerNetworkProperty_GetPVSInfo, "CServerNetworkProperty::GetPVSInfo");
        }
	};
	CModInlineVirtuals s_ModInlineVirtuals;

	ConVar cvar_enable("sig_perf_virtual_call_optimize", "0", FCVAR_NOTIFY,
		"Mod: Optimize common virtual calls",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

    ConVar cvar_enable_inline("sig_perf_virtual_call_optimize_inline_virtuals", "0", FCVAR_NOTIFY,
		"Mod: Optimize common virtual calls with experimental function inline",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_ModInlineVirtuals.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
    // ConVar cvar_enable_virtuals("sig_perf_virtual_call_optimize_virtuals", "0", FCVAR_NOTIFY,
	// 	"Mod: Optimize common virtual calls",
	// 	[](IConVar *pConVar, const char *pOldValue, float flOldValue){
	// 		s_ModMini.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
	// 	});
}