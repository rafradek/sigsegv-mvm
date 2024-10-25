#include "mem/detour.h"
#include "addr/addr.h"
#include "mem/alloc.h"
#include "mem/protect.h"
#include "mem/opcode.h"
#include "mem/wrapper.h"
#include "mem/func_copy.h"
#include "util/backtrace.h"
#include "util/demangle.h"
#include "util/misc.h"
#include "stub/server.h"

#include <udis86.h>

#include <regex>


#if !(defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) )
#error Architecture must be IA32/64
#endif


//#define DETOUR_DEBUG 1


#if DETOUR_DEBUG
#define TRACE_ENABLE 1
#define TRACE_TERSE  1
#endif
#include "util/trace.h"

char registerPushCaller[] {0xE8, 0x00, 0x00, 0x00, 0x00, 0xC3};
intptr_t registerPushCallAddrOffset = 0x1;

/* analogous to asm.c copy_bytes() when dest == nullptr */
static size_t Trampoline_CalcNumBytesToCopy(size_t len_min, const uint8_t *func)
{
	ud_t ud;
	ud_init(&ud);
#ifdef PLATFORM_64BITS
	ud_set_mode(&ud, 64);
#else
	ud_set_mode(&ud, 32);
#endif
	ud_set_pc(&ud, (uint64_t)func);
	ud_set_input_buffer(&ud, func, 0x100);
	
	size_t len_actual = 0;
	while (len_actual < len_min) {
		size_t len_decoded = ud_decode(&ud);
		assert(len_decoded != 0);

		len_actual += len_decoded;
	}
	return len_actual;
}

static bool Jump_ShouldUseRelativeJump(intptr_t from, intptr_t target)
{
#ifndef PLATFORM_64BITS
	return true;
#endif
	auto offset = (intptr_t)target - ((intptr_t)from + (intptr_t)JmpRelImm32::Size());
	return offset < INT_MAX && offset > INT_MIN;
}

static size_t Jump_CalculateSize(intptr_t from, intptr_t target)
{
	auto size = Jump_ShouldUseRelativeJump(from, target) ? JmpRelImm32::Size() : JmpIndirectMem32::Size() + sizeof(intptr_t);
	return size;
}

static void Jump_WriteJump(uint8_t *from, uintptr_t target, size_t padSize)
{
	if (Jump_ShouldUseRelativeJump((intptr_t) from, (intptr_t) target)) {
		auto jmp = JmpRelImm32(from, target);
		if (padSize > 0) 
			jmp.WritePadded(padSize);
		else 
			jmp.Write();
	}
	else {
		auto pointerAddress = (uintptr_t)from + JmpIndirectMem32::Size();
		auto jmp = JmpIndirectMem32(from, pointerAddress - ((uintptr_t)from + JmpIndirectMem32::Size()));
		if (padSize > 0) 
			jmp.WritePadded(padSize);
		else 
			jmp.Write();
		
		*(uintptr_t *)pointerAddress = target;
	}
}

#warning ALL OF THIS NEEDS TESTING!


bool IDetour::Load()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	if (this->DoLoad()) {
		this->m_bLoaded = true;
		return true;
	}
	
	return false;
}

void IDetour::Unload()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	this->Disable();
	this->DoUnload();
}


void IDetour::Toggle(bool enable)
{
	if (enable) {
		this->Enable();
	} else {
		this->Disable();
	}
}

void IDetour::Enable()
{
	TRACE("[this: %08x \"%s\"] loaded:%c active:%c", (uintptr_t)this, this->GetName(),
		(this->IsLoaded() ? 'Y' : 'N'),
		(this->IsActive() ? 'Y' : 'N'));
	
	if (this->IsLoaded() && !this->IsActive()) {
		this->DoEnable();
		this->m_bActive = true;
	}
}

void IDetour::Disable()
{
	TRACE("[this: %08x \"%s\"] loaded:%c active:%c", (uintptr_t)this, this->GetName(),
		(this->IsLoaded() ? 'Y' : 'N'),
		(this->IsActive() ? 'Y' : 'N'));
	
	if (this->IsLoaded() && this->IsActive()) {
		this->DoDisable();
		this->m_bActive = false;
	}
}


void IDetour::DoEnable()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	ITrace *trace   = nullptr;
	CDetour *detour = nullptr;
	
	if ((trace = dynamic_cast<ITrace *>(this)) != nullptr) {
		CDetouredFunc::Find(this->GetFuncPtr()).AddTrace(trace);
	} else if ((detour = dynamic_cast<CDetour *>(this)) != nullptr) {
		CDetouredFunc::Find(this->GetFuncPtr()).AddDetour(detour);
	} else {
		/* don't know how to deal with this kind of IDetour */
		assert(false);
	}
}

void IDetour::DoDisable()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	ITrace *trace   = nullptr;
	CDetour *detour = nullptr;
	
	if ((trace = dynamic_cast<ITrace *>(this)) != nullptr) {
		CDetouredFunc::Find(this->GetFuncPtr()).RemoveTrace(trace);
	} else if ((detour = dynamic_cast<CDetour *>(this)) != nullptr) {
		CDetouredFunc::Find(this->GetFuncPtr()).RemoveDetour(detour);
	} else {
		/* don't know how to deal with this kind of IDetour */
		assert(false);
	}
}


bool IDetour_SymNormal::DoLoad()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	if (this->m_bFuncByName) {
		addrLoadingDetour = true;
		this->m_pFunc = reinterpret_cast<uint8_t *>(AddrManager::GetAddr(this->m_strFuncName.c_str()));
		addrLoadingDetour = false;
		if (this->m_pFunc == nullptr) {
			Warning("IDetour_SymNormal::DoLoad: \"%s\": addr lookup failed for \"%s\"\n", this->GetName(), this->m_strFuncName.c_str());
			return false;
		}
	} else {
		if (this->m_pFunc == nullptr) {
			Warning("IDetour_SymNormal::DoLoad: \"%s\": func ptr provided is nullptr\n", this->GetName());
			return false;
		}
	}

	CDetouredFunc::Find(this->m_pFunc);
	
	return true;
}

void IDetour_SymNormal::DoUnload()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
}


const char *IDetour_SymRegex::GetName() const
{
	if (this->IsLoaded()) {
		return this->m_strDemangled.c_str();
	} else {
		return this->m_strPattern.c_str();
	}
}


bool IDetour_SymRegex::DoLoad()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
	
	if (this->m_Library == Library::INVALID) {
		Warning("IDetour_SymRegex::DoLoad: \"%s\": invalid library\n", this->GetName());
		return false;
	}
	
	if (!LibMgr::HaveLib(this->m_Library)) {
		Warning("IDetour_SymRegex::DoLoad: \"%s\": library not available: %s\n", this->GetName(), LibMgr::Lib_ToString(this->m_Library));
		return false;
	}
	
	const SegInfo& info_seg_text = LibMgr::GetInfo(this->m_Library).GetSeg(Segment::TEXT);
	auto text_begin = reinterpret_cast<const void *>(info_seg_text.AddrBegin());
	auto text_end   = reinterpret_cast<const void *>(info_seg_text.AddrEnd());
	
#ifndef _MSC_VER
	#warning NEED try/catch for std::regex ctor!
#endif
	std::regex filter(this->m_strPattern, std::regex_constants::ECMAScript);
	std::vector<Symbol> syms;
	LibMgr::ForEachSym(this->m_Library, [&](const Symbol& sym){
		if (std::regex_search(sym.name, filter, std::regex_constants::match_any)) {
			if (sym.addr >= text_begin && sym.addr < text_end) {
				syms.push_back(sym);
			}
		}
		
		return true;
	});
	
	if (syms.size() != 1) {
		Warning("IDetour_SymRegex::DoLoad: \"%s\": symbol lookup failed (%zu matches):\n", this->GetName(), syms.size());
		for (const auto& sym : syms) {
			Warning("  %s\n", sym.name.c_str());
		}
		return false;
	}
	
	this->m_strSymbol = syms[0].name;
	this->m_pFunc     = reinterpret_cast<uint8_t *>(syms[0].addr);
	
	DemangleName(this->m_strSymbol.c_str(), this->m_strDemangled);
	
	return true;
}

void IDetour_SymRegex::DoUnload()
{
	TRACE("[this: %08x \"%s\"]", (uintptr_t)this, this->GetName());
}


bool CDetour::DoLoad()
{
	if (!IDetour_SymNormal::DoLoad()) {
		return false;
	}
	
	if (!this->EnsureUniqueInnerPtrs()) {
		return false;
	}

	s_LoadedDetours.push_back(this);
	return true;
}

void CDetour::DoUnload()
{
	IDetour_SymNormal::DoUnload();
	
	s_LoadedDetours.erase(std::remove(s_LoadedDetours.begin(), s_LoadedDetours.end(), this), s_LoadedDetours.end());
}


void CDetour::DoEnable()
{
	IDetour::DoEnable();
	
	s_ActiveDetours.push_back(this);
}

void CDetour::DoDisable()
{
	IDetour::DoDisable();
	
	s_ActiveDetours.erase(std::remove(s_ActiveDetours.begin(), s_ActiveDetours.end(), this), s_ActiveDetours.end());
}


/* ensure that no two loaded CDetour instances have the same m_pInner value:
 * this is important because it may seem like two different function detours can
 * share the same callback; but in fact, that's completely unsafe */
bool CDetour::EnsureUniqueInnerPtrs()
{
	TRACE("[this: %08x \"%s\"] %08x", (uintptr_t)this, this->GetName(), (uintptr_t)this->m_pInner);
	
	for (auto detour : s_LoadedDetours) {
		if (detour == this) continue;
		if (detour->m_pInner == this->m_pInner) {
			Warning("Found two CDetour instances with the same inner function pointer!\n"
				"this: %08x \"%s\"\n"
				"that: %08x \"%s\"\n",
				(uintptr_t)this, this->GetName(),
				(uintptr_t)detour, detour->GetName());
			return false;
		}
	}
	
	return true;
}


void CFuncCount::TracePre()
{
	++this->m_RefCount;
}

void CFuncCount::TracePost()
{
	--this->m_RefCount;
}


void CFuncCallback::TracePre()
{
	if (this->m_pCBPre != nullptr) {
		this->m_pCBPre();
	}
}

void CFuncCallback::TracePost()
{
	if (this->m_pCBPost != nullptr) {
		this->m_pCBPost();
	}
}


void CFuncTrace::TracePre()
{
	ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "[%7d] %*s%s ", gpGlobals->tickcount, 2 * TraceLevel::Get(), "", "{");
	ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "%s\n", this->GetName());
	TraceLevel::Increment();
}

void CFuncTrace::TracePost()
{
	TraceLevel::Decrement();
	ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "[%7d] %*s%s ", gpGlobals->tickcount, 2 * TraceLevel::Get(), "", "}");
	ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "%s\n", this->GetName());
}


void CFuncBacktrace::TracePre()
{
	BACKTRACE();
}

void CFuncBacktrace::TracePost()
{
}


void CFuncVProf::TracePre()
{
	if (g_VProfCurrentProfile.IsEnabled()) {
		g_VProfCurrentProfile.EnterScope(this->m_strVProfName.c_str(), 0,
			this->m_strVProfGroup.c_str(), false, BUDGETFLAG_OTHER);
	}
}

void CFuncVProf::TracePost()
{
	if (g_VProfCurrentProfile.IsEnabled()) {
		g_VProfCurrentProfile.ExitScope();
	}
}

constexpr bool s_bGameHasOptimizedVirtuals = SOURCE_ENGINE == SE_TF2;

CDetouredFunc::CDetouredFunc(void *func_ptr) :
	m_pFunc(reinterpret_cast<uint8_t *>(func_ptr))
{
	TRACE("[this: %08x] [func: %08x]", (uintptr_t)this, (uintptr_t)func_ptr);
	size_t len_prologue = Trampoline_CalcNumBytesToCopy(JmpRelImm32::Size(), this->m_pFunc);
	this->m_TrueOriginalPrologue.resize(len_prologue);
	memcpy(this->m_TrueOriginalPrologue.data(), this->m_pFunc, len_prologue);

	// For TF2, virtual calls can be optimized to be not called if the function is not overridden. Because of this detours also have to modify the vtables with matching functions
	if (s_bGameHasOptimizedVirtuals) {
		bool found = false;
		for (auto &[vtname, pVTInfo] : RTTI::GetAllVTableInfo()) {
			auto pVT = pVTInfo.vtable;
			auto size = pVTInfo.size / sizeof(void *);
			for (int i = 0; i < size; ++i) {
				if (pVT[i] == func_ptr) {
					this->m_FoundFuncPtrAndVTablePtr.emplace(const_cast<void **>(pVT + i), pVT);
					found = true;
				}
			}
		}

		if (found) {
			this->m_VirtualHookOptional = CVirtualHookBase(nullptr, CVirtualHookBase::DETOUR_HOOK);
		}
	}
}
CDetouredFunc::~CDetouredFunc()
{
	TRACE("[this: %08x] [func: %08x]", (uintptr_t)this, (uintptr_t)this->m_pFunc);
	
	this->RemoveAllDetours();
	this->DestroyWrapper();
	this->DestroyTrampoline();
}


CDetouredFunc& CDetouredFunc::Find(void *func_ptr)
{
//	TRACE("%08x", (uintptr_t)func_ptr);
	
	auto it = s_FuncMap.find(func_ptr);
	if (it == s_FuncMap.end()) {
		/* oh god C++11 why do you do this to me */
		auto result = s_FuncMap.emplace(std::piecewise_construct, std::forward_as_tuple(func_ptr), std::forward_as_tuple(func_ptr));
		it = result.first;
	}
	
	return (*it).second;
	
//	TRACE_EXIT("%s", (result.second ? "inserted" : "existing"));
}

CDetouredFunc *CDetouredFunc::FindOptional(void *func_ptr)
{
//	TRACE("%08x", (uintptr_t)func_ptr);
	
	auto it = s_FuncMap.find(func_ptr);
	if (it == s_FuncMap.end()) {
		return nullptr;
	}
	
	return &(*it).second;
	
//	TRACE_EXIT("%s", (result.second ? "inserted" : "existing"));
}



void CDetouredFunc::CleanUp()
{
	for (auto it = s_FuncMap.begin(); it != s_FuncMap.end(); ) {
		const CDetouredFunc& df = (*it).second;
		if (df.m_Detours.empty() && df.m_Traces.empty()) {
			it = s_FuncMap.erase(it);
		} else {
			++it;
		}
	}
}


void CDetouredFunc::AddDetour(CDetour *detour)
{
	TRACE("[this: %08x] [detour: %08x \"%s\"]", (uintptr_t)this, (uintptr_t)detour, detour->GetName());
	
	assert(std::count(this->m_Detours.begin(), this->m_Detours.end(), detour) == 0);

	this->m_Detours.push_back(detour);
	std::sort(this->m_Detours.begin(), this->m_Detours.end(), [](CDetour *&a, CDetour *&b) {
		return a->GetPriority() > b->GetPriority();
	});

	this->Reconfigure();
}

void CDetouredFunc::RemoveDetour(CDetour *detour)
{
	TRACE("[this: %08x] [detour: %08x \"%s\"]", (uintptr_t)this, (uintptr_t)detour, detour->GetName());
	
	assert(std::count(this->m_Detours.begin(), this->m_Detours.end(), detour) == 1);
	this->m_Detours.erase(std::remove(this->m_Detours.begin(), this->m_Detours.end(), detour), this->m_Detours.end());
	
	this->Reconfigure();
}


void CDetouredFunc::AddTrace(ITrace *trace)
{
	TRACE("[this: %08x] [trace: %08x \"%s\"]", (uintptr_t)this, (uintptr_t)trace, trace->GetName());
	
	assert(std::count(this->m_Traces.begin(), this->m_Traces.end(), trace) == 0);
	this->m_Traces.push_back(trace);
	
	this->Reconfigure();
}

void CDetouredFunc::RemoveTrace(ITrace *trace)
{
	TRACE("[this: %08x] [trace: %08x \"%s\"]", (uintptr_t)this, (uintptr_t)trace, trace->GetName());
	
	assert(std::count(this->m_Traces.begin(), this->m_Traces.end(), trace) == 1);
	this->m_Traces.erase(std::remove(this->m_Traces.begin(), this->m_Traces.end(), trace), this->m_Traces.end());
	
	this->Reconfigure();
}


void CDetouredFunc::RemoveAllDetours()
{
	TRACE("[this: %08x]", (uintptr_t)this);
	
	this->m_Detours.clear();
	this->m_Traces.clear();
	
	this->Reconfigure();
}


void CDetouredFunc::CreateWrapper()
{
	TRACE("[this: %08x]", (uintptr_t)this);
	
#if !defined _WINDOWS && defined TRACE_DETOUR_ENABLED
	this->m_pWrapper = TheExecMemManager()->AllocWrapper();
	{
		MemProtModifier_RX_RWX(this->m_pWrapper, Wrapper::Size());
		
		memcpy(this->m_pWrapper, Wrapper::Base(), Wrapper::Size());
		
		auto p_mov_funcaddr_1 = this->m_pWrapper + Wrapper::Offset_MOV_FuncAddr_1();
		auto p_call_pre       = this->m_pWrapper + Wrapper::Offset_CALL_Pre();
		auto p_call_inner     = this->m_pWrapper + Wrapper::Offset_CALL_Inner();
		auto p_mov_funcaddr_2 = this->m_pWrapper + Wrapper::Offset_MOV_FuncAddr_2();
		auto p_call_post      = this->m_pWrapper + Wrapper::Offset_CALL_Post();
		
		MovRegImm32      (p_mov_funcaddr_1, REG_CX, (uint32_t)this->m_pFunc)         .Write();
		CallIndirectMem32(p_call_pre,               (uint32_t)&this->m_pWrapperPre)  .Write();
		CallIndirectMem32(p_call_inner,             (uint32_t)&this->m_pWrapperInner).Write();
		MovRegImm32      (p_mov_funcaddr_2, REG_CX, (uint32_t)this->m_pFunc)         .Write();
		CallIndirectMem32(p_call_post,              (uint32_t)&this->m_pWrapperPost) .Write();
	}
	
	assert(this->m_WrapperCheck.empty());
	this->m_WrapperCheck.resize(Wrapper::Size());
	memcpy(this->m_WrapperCheck.data(), this->m_pWrapper, Wrapper::Size());
#endif
}

void CDetouredFunc::DestroyWrapper()
{
	TRACE("[this: %08x]", (uintptr_t)this);
	
#if !defined _WINDOWS && defined TRACE_DETOUR_ENABLED
	if (this->m_pWrapper != nullptr) {
		this->ValidateWrapper();
		
		TheExecMemManager()->FreeWrapper(this->m_pWrapper);
		this->m_pWrapper = nullptr;
	}
#endif
}

void CDetouredFunc::CreateTrampoline()
{
	TRACE("[this: %08x]", (uintptr_t)this);

	size_t len_prologue = Trampoline_CalcNumBytesToCopy(this->m_zJumpSize, this->m_pFunc);
	TRACE_MSG("len_prologue = %zu\n", len_prologue);

	size_t len_trampoline_alloc = len_prologue + 4 + (IsPlatform64Bits() ? JmpIndirectMem32::Size() + sizeof(uintptr_t) : JmpRelImm32::Size());
	
	this->m_pTrampoline = TheExecMemManager()->AllocTrampoline(len_trampoline_alloc);
	TRACE_MSG("trampoline @ %08x\n", (uintptr_t)this->m_pTrampoline);
	
	bool jumpInTrampolineRelative = Jump_ShouldUseRelativeJump((intptr_t)this->m_pTrampoline + len_prologue, (intptr_t)this->m_pFunc + len_prologue);
	size_t jumpInTrampolineSize = Jump_CalculateSize((intptr_t)this->m_pTrampoline + len_prologue, (intptr_t)this->m_pFunc + len_prologue);

	assert(this->m_OriginalPrologue.empty());
	this->m_OriginalPrologue.resize(len_prologue);
	memcpy(this->m_OriginalPrologue.data(), this->m_pFunc, len_prologue);
	size_t len_trampoline;
	{
		MemProtModifier_RX_RWX(this->m_pTrampoline, len_trampoline_alloc);
		len_trampoline = CopyAndFixUpFuncBytes(len_prologue, this->m_pFunc, this->m_pTrampoline);
		//Msg("trampoline addr %p len trampoline %zu alloc %zu jumps %zu prologue %zu\n", this->m_pTrampoline, len_trampoline, len_trampoline_alloc, jumpInTrampolineSize, len_prologue);
		TRACE_MSG("len_trampoline = %zu\n", len_trampoline);

		assert(len_trampoline + jumpInTrampolineSize <= len_trampoline_alloc);
		Jump_WriteJump(this->m_pTrampoline + len_trampoline, (uintptr_t)this->m_pFunc + len_prologue, 0);
	}
	assert(this->m_TrampolineCheck.empty());
	this->m_TrampolineCheck.resize(len_trampoline + jumpInTrampolineSize);
	memcpy(this->m_TrampolineCheck.data(), this->m_pTrampoline, len_trampoline + jumpInTrampolineSize);
}

void CDetouredFunc::DestroyTrampoline()
{
	TRACE("[this: %08x]", (uintptr_t)this);
	
	if (this->m_pTrampoline != nullptr) {
		this->ValidateTrampoline();
		
		TheExecMemManager()->FreeTrampoline(this->m_pTrampoline);
		this->m_pTrampoline = nullptr;
	}
	this->m_OriginalPrologue.clear();
	this->m_TrampolineCheck.clear();
}


void CDetouredFunc::Reconfigure()
{
	for (auto &[pVFuncPtr, pVT] : this->m_FoundFuncPtrAndVTablePtr) {
		auto vhook = CVirtualHookFunc::FindOptional(pVFuncPtr);
		if (vhook != nullptr) {
			vhook->RemoveVirtualHook(&this->m_VirtualHookOptional);
		}
	}

	TRACE("[this: %08x] with %zu detour(s)", (uintptr_t)this, this->m_Detours.size());
	
	this->DestroyWrapper();

	this->UninstallJump();
	this->DestroyTrampoline();
	
	void *jump_to = nullptr;
	
	if (!this->m_Detours.empty()) {
		//Msg("Installing detour %s\n", AddrManager::ReverseLookup(this->m_pFunc));
		CDetour *first = this->m_Detours.front();
		CDetour *last  = this->m_Detours.back();
		this->m_bJumpIsRelative = Jump_ShouldUseRelativeJump((intptr_t)this->m_pFunc, (intptr_t)first->m_pCallback);
		this->m_zJumpSize = Jump_CalculateSize((intptr_t)this->m_pFunc, (intptr_t)first->m_pCallback);
		this->CreateTrampoline();
		TRACE_MSG("detour[\"%s\"].inner [%08x] -> trampoline [%08x]\n",
			last->GetName(), (uintptr_t)last->m_pInner, (uintptr_t)this->m_pTrampoline);
		*last->m_pInner = this->m_pTrampoline;
		
		for (int i = this->m_Detours.size() - 2; i >= 0; --i) {
			CDetour *d1 = this->m_Detours[i];
			CDetour *d2 = this->m_Detours[i + 1];
			
			TRACE_MSG("detour[\"%s\"].inner [%08x] -> detour[\"%s\"].callback [%08x]\n",
				d1->GetName(), (uintptr_t)d1->m_pInner,
				d2->GetName(), (uintptr_t)d2->m_pCallback);
			*d1->m_pInner = d2->m_pCallback;
		}
		
		/* just to make sure... */
		for (auto detour : this->m_Detours) {
			assert(detour->EnsureUniqueInnerPtrs());
		}
		
		this->m_VirtualHookOptional = CVirtualHookBase(first->m_pCallback, CVirtualHookBase::DETOUR_HOOK, first->GetName());
		for (auto &[pVFuncPtr, pVT] : this->m_FoundFuncPtrAndVTablePtr) {
			CVirtualHookFunc::Find(pVFuncPtr, pVT).AddVirtualHook(&this->m_VirtualHookOptional);
		}
		jump_to = first->m_pCallback;
		
		/*TRACE_MSG("func jump -> detour[\"%s\"].callback [%08x]\n",
			first->GetName(), (uintptr_t)first->m_pCallback);
		this->InstallJump(first->m_pCallback);*/
	}
	
#if !defined _WINDOWS && defined TRACE_DETOUR_ENABLED
	if (!this->m_Traces.empty()) {

		this->CreateWrapper();
		if (jump_to != nullptr) {
			this->m_pWrapperInner = jump_to;
		} else {
			this->m_pWrapperInner = this->m_pTrampoline;
		}
		
		jump_to = this->m_pWrapper;
	}
#endif
	
	if (jump_to != nullptr) {
		this->InstallJump(jump_to);
	}
}


void CDetouredFunc::InstallJump(void *target)
{
	TRACE("[this: %08x] [target: %08x]", (uintptr_t)this, (uintptr_t)target);
	
	/* already installed */
	if (this->m_bJumpInstalled) {
		this->ValidateCurrentPrologue();
		return;
	}
	
	this->ValidateOriginalPrologue();

	if (!this->m_bModifiedByPatch && !this->Validate(this->m_pFunc, this->m_TrueOriginalPrologue, "ValidateTrueOriginalPrologue")) {
		const char *func_name = AddrManager::ReverseLookup(this->m_pFunc);
		if (func_name == nullptr) func_name = "???";
		ConColorMsg(Color(0xff, 0xff, 0x60), "\"%s\" is probably already detoured by another plugin/extension\nThis might be fine, but to be safe you could enable sigsegv convars before other plugins\n", func_name);
		ConColorMsg(Color(0xff, 0xff, 0x60), "Convars in sigsegv_convars.cfg are always loaded before sourcemod plugins\n");
	}
	
	assert(!this->m_OriginalPrologue.empty());
	
	{
		MemProtModifier_RX_RWX(this->m_pFunc, this->m_OriginalPrologue.size());
		
		Jump_WriteJump(this->m_pFunc, (uintptr_t)target, this->m_OriginalPrologue.size() > this->m_zJumpSize ? this->m_OriginalPrologue.size() : 0);
	}
	
	this->m_CurrentPrologue.resize(this->m_OriginalPrologue.size());
	memcpy(this->m_CurrentPrologue.data(), this->m_pFunc, this->m_CurrentPrologue.size());
	
	this->m_bJumpInstalled = true;
}

void CDetouredFunc::TemporaryDisable()
{
	this->m_bModifiedByPatch = true;
	if (this->m_bJumpInstalled) {
		this->UninstallJump();
	}
}

void CDetouredFunc::TemporaryEnable()
{
	this->Reconfigure();
}

void CDetouredFunc::UninstallJump()
{
	TRACE("[this: %08x]", (uintptr_t)this);
	
	/* already uninstalled */
	if (!this->m_bJumpInstalled) {
		this->ValidateOriginalPrologue();
		return;
	}
	
	// Check if some extension had overriden our jump, but not when the game is in shutdown/restart state as it does not matter by then
	int state = g_HostState.GetRef().m_currentState;
	if (!this->ValidateCurrentPrologue() && !(state == 6 || state == 7)) {
		const char *func_name = AddrManager::ReverseLookup(this->m_pFunc);
		if (func_name == nullptr) func_name = "???";
		
		ConColorMsg(Color(0xff, 0x60, 0x60), "\"%s\" detour validation failure!\n"
		"This means that another plugin or extension had detoured this function, and those detours are now disabled\n", func_name);
		ConColorMsg(Color(0xff, 0x60, 0x60), "You can avoid this problem by disabling the conflicting plugin/extension before changing sigsegv convars or unloading sigsegv, and enable the plugin afterwards\n");
	}
	
	assert(!this->m_OriginalPrologue.empty());
	
	{
		MemProtModifier_RX_RWX(this->m_pFunc, this->m_OriginalPrologue.size());
		memcpy(this->m_pFunc, this->m_OriginalPrologue.data(), this->m_OriginalPrologue.size());
	}
	
	this->m_bJumpInstalled = false;
}


bool CDetouredFunc::Validate(const uint8_t *ptr, const std::vector<uint8_t>& vec, const char *caller)
{
	const uint8_t *check_ptr = vec.data();
	size_t         check_len = vec.size();
	
	return memcmp(ptr, check_ptr, check_len) == 0;

	// if (memcmp(ptr, check_ptr, check_len) != 0) {
	// 	const char *func_name = AddrManager::ReverseLookup(this->m_pFunc);
	// 	if (func_name == nullptr) func_name = "???";

	// 	Warning("CDetouredFunc::%s [func: \"%s\"]: validation failure!\n"
	// 		"Expected:\n%s"
	// 		"Actual:\n%s",
	// 		caller, func_name,
	// 		HexDump(check_ptr, check_len).c_str(),
	// 		HexDump(      ptr, check_len).c_str());

	// 	return false;
	// }
	// return true;
}


#if !defined _WINDOWS && defined TRACE_DETOUR_ENABLED

void CDetouredFunc::FuncPre()
{
	for (auto trace : this->m_Traces) {
		trace->TracePre();
	}
}

void CDetouredFunc::FuncPost()
{
	for (auto trace : this->m_Traces) {
		trace->TracePost();
	}
}


__fastcall void CDetouredFunc::WrapperPre(void *func_ptr, const uint32_t *retaddr_save)
{
//	DevMsg("WrapperPre [func: %08x] [retaddr_save: %08x]\n", (uintptr_t)func_ptr, (uintptr_t)retaddr_save);
	
	/* use the func_ptr arg to locate the correct CDetouredFunc instance */
	auto it = s_FuncMap.find(func_ptr);
	assert(it != s_FuncMap.end());
	CDetouredFunc& inst = (*it).second;
	
	/* do the pre-function stuff for this function (traces etc) */
	inst.FuncPre();
	
	/* save the wrapper's return address, for restoration later */
	s_WrapperCallerRetAddrs.push(*retaddr_save);
}

__fastcall void CDetouredFunc::WrapperPost(void *func_ptr, uint32_t *retaddr_restore)
{
//	DevMsg("WrapperPost [func: %08x] [retaddr_restore: %08x]\n", (uintptr_t)func_ptr, (uintptr_t)retaddr_restore);
	
	/* restore the wrapper's return address, which we saved earlier */
	assert(!s_WrapperCallerRetAddrs.empty());
	*retaddr_restore = s_WrapperCallerRetAddrs.top();
	s_WrapperCallerRetAddrs.pop();
	
	/* use the func_ptr arg to locate the correct CDetouredFunc instance */
	auto it = s_FuncMap.find(func_ptr);
	assert(it != s_FuncMap.end());
	CDetouredFunc& inst = (*it).second;
	
	/* do the post-function stuff for this function (traces etc) */
	inst.FuncPost();
}

#endif
