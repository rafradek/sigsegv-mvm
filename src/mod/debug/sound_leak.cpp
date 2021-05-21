#include "mod.h"
#include "util/scope.h"
#include "link/link.h"


#define LOGMEM(...) \
	do { \
		std::lock_guard<std::mutex> f_lock(m_file); \
		if (f != nullptr) { \
			fprintf(f, __VA_ARGS__); \
			fflush(f); \
		} \
	} while (false)


namespace Mod::Debug::Sound_Leak
{
	// 1000 x "play" => ~14MB increase in memory usage
	// so roughly 14000 bytes leaked per "play"
	
	
	ConVar cvar_minimum("sig_debug_sound_leak_minimum", "128", FCVAR_NOTIFY, "Debug: minimum allocation size to track");
	
	
//	ConVar cvar_alloc  ("sig_debug_sound_leak_alloc",   "1", FCVAR_NOTIFY, "Debug: enable logging for IMemAlloc::Alloc");
//	ConVar cvar_realloc("sig_debug_sound_leak_realloc", "1", FCVAR_NOTIFY, "Debug: enable logging for IMemAlloc::Realloc");
//	ConVar cvar_free   ("sig_debug_sound_leak_free",    "1", FCVAR_NOTIFY, "Debug: enable logging for IMemAlloc::Free");
	
	
	FILE *f = nullptr;
	std::mutex m_file;
	
	
	std::unordered_map<void *, size_t> mems;
	
	
	bool mem_tracking = false;
	void Mem_Begin()
	{
		mems.clear();
		mem_tracking = true;
		
		LOGMEM(
			"================================================================================\n"
			"Memory tracking started\n"
			"================================================================================\n");
	}
	void Mem_End()
	{
		if (!mem_tracking) return;
		mem_tracking = false;
		
		size_t total = 0;
		LOGMEM(
			"================================================================================\n"
			"Memory leak report:\n"
			"================================================================================\n");
		for (const auto& pair : mems) {
			const void *ptr = pair.first;
			size_t size     = pair.second;
			
			LOGMEM("0x%08x: %u bytes\n", (uintptr_t)ptr, size);
			
			total += size;
		}
		LOGMEM(
			"================================================================================\n"
			"Total memory leaked: %u bytes\n"
			"================================================================================\n",
			total);
	}
	
	
	RefCount rc_S_Play;
	std::mutex m_S_Play;
	DETOUR_DECL_STATIC(void, S_Play, const char *pszName, bool flush)
	{
		std::lock_guard<std::mutex> lock(m_S_Play);
		
		LOGMEM("[%12.7f] BEGIN S_Play\n", Plat_FloatTime());
		{
			SCOPED_INCREMENT(rc_S_Play);
			DETOUR_STATIC_CALL(S_Play)(pszName, flush);
		}
		LOGMEM("[%12.7f] END   S_Play\n\n", Plat_FloatTime());
	}
	
	
	RefCount rc_Alloc;
	std::mutex m_Alloc;
	DETOUR_DECL_MEMBER(void *, IMemAlloc_Alloc, size_t nSize)
	{
		std::lock_guard<std::mutex> lock(m_Alloc);
		SCOPED_INCREMENT(rc_Alloc);
		
		auto result = DETOUR_MEMBER_CALL(IMemAlloc_Alloc)(nSize);
		
		if (mem_tracking && rc_Alloc <= 1 && nSize >= (size_t)cvar_minimum.GetInt()) {
			if (mems.find(result) == mems.end()) {
				LOGMEM("[%12.7f] Alloc   %7u @ 0x%08x\n", Plat_FloatTime(), nSize, (uintptr_t)result);
				mems[result] = nSize;
			} else {
				Warning("IMemAlloc::Alloc(0x%08x, %u): pointer already in map!\n", (uintptr_t)result, nSize);
			}
			
		//	if (cvar_alloc.GetBool()) {
		//		LOGMEM("[%12.7f] Alloc   %7u @ 0x%08x\n", Plat_FloatTime(), nSize, (uintptr_t)result);
		//	}
		}
		
		return result;
	}
	
	RefCount rc_Realloc;
	std::mutex m_Realloc;
	DETOUR_DECL_MEMBER(void *, IMemAlloc_Realloc, void *pMem, size_t nSize)
	{
		std::lock_guard<std::mutex> lock(m_Realloc);
		SCOPED_INCREMENT(rc_Realloc);
		
		auto result = DETOUR_MEMBER_CALL(IMemAlloc_Realloc)(pMem, nSize);
		
		if (mem_tracking && rc_Realloc <= 1 && nSize >= (size_t)cvar_minimum.GetInt()) {
			auto it = mems.find(pMem);
			if (it != mems.end()) {
				mems.erase(it);
				
				if (mems.find(result) == mems.end()) {
					LOGMEM("[%12.7f] Realloc %7u   0x%08x => 0x%08x\n", Plat_FloatTime(), nSize, (uintptr_t)pMem, (uintptr_t)result);
					mems[result] = nSize;
				} else {
					Warning("IMemAlloc::Realloc: pointer already in map!\n");
				}
			} else {
			//	Warning("IMemAlloc::Realloc: reallocation of untracked pointer!\n");
			}
			
		//	if (cvar_realloc.GetBool()) {
		//		LOGMEM("[%12.7f] Realloc %7u   0x%08x => 0x%08x\n", Plat_FloatTime(), nSize, (uintptr_t)pMem, (uintptr_t)result);
		//	}
		}
		
		return result;
	}
	
	RefCount rc_Free;
	std::mutex m_Free;
	DETOUR_DECL_MEMBER(void, IMemAlloc_Free, void *pMem)
	{
		std::lock_guard<std::mutex> lock(m_Free);
		SCOPED_INCREMENT(rc_Free);
		
		if (pMem == nullptr) return;
		
		if (mem_tracking && rc_Free <= 1) {
			auto it = mems.find(pMem);
			if (it != mems.end()) {
				LOGMEM("[%12.7f] Free    %7u @ 0x%08x\n", Plat_FloatTime(), (*it).second, (uintptr_t)pMem);
				mems.erase(it);
			} else {
			//	Warning("IMemAlloc::Free: deallocation of untracked pointer!\n");
			}
			
		//	if (cvar_free.GetBool()) {
		//		LOGMEM("[%12.7f] Free              0x%08x\n", Plat_FloatTime(), (uintptr_t)pMem);
		//	}
		}
		
		DETOUR_MEMBER_CALL(IMemAlloc_Free)(pMem);
	}
	
	
	std::mutex m_CAudioMixerWaveMP3;
	int num_CAudioMixerWaveMP3 = 0;
	
	DETOUR_DECL_MEMBER(void, CAudioMixerWaveMP3_ctor, uint32_t i1)
	{
		std::lock_guard<std::mutex> lock(m_CAudioMixerWaveMP3);
		
		++num_CAudioMixerWaveMP3;
		LOGMEM("+++ CAudioMixerWaveMP3 @ 0x%08x [total: %d]\n", (uintptr_t)this, num_CAudioMixerWaveMP3);
		DETOUR_MEMBER_CALL(CAudioMixerWaveMP3_ctor)(i1);
	}
	
	DETOUR_DECL_MEMBER(void, CAudioMixerWaveMP3_dtor, uint32_t i1)
	{
		std::lock_guard<std::mutex> lock(m_CAudioMixerWaveMP3);
		
		--num_CAudioMixerWaveMP3;
		LOGMEM("--- CAudioMixerWaveMP3 @ 0x%08x [total: %d]\n", (uintptr_t)this, num_CAudioMixerWaveMP3);
		DETOUR_MEMBER_CALL(CAudioMixerWaveMP3_dtor)(i1);
	}
	
	
	std::mutex m_CWaveDataStreamAsync;
	int num_CWaveDataStreamAsync = 0;
	
	DETOUR_DECL_MEMBER(void, CWaveDataStreamAsync_ctor, uint32_t r1, uint32_t p1, const char *s1, int i1, int i2, uint32_t p2, int i3)
	{
		std::lock_guard<std::mutex> lock(m_CWaveDataStreamAsync);
		
		++num_CWaveDataStreamAsync;
		LOGMEM("+++ CWaveDataStreamAsync @ 0x%08x [total: %d]\n", (uintptr_t)this, num_CWaveDataStreamAsync);
		DETOUR_MEMBER_CALL(CWaveDataStreamAsync_ctor)(r1, p1, s1, i1, i2, p2, i3);
	}
	
	DETOUR_DECL_MEMBER(void, CWaveDataStreamAsync_dtor, uint32_t i1)
	{
		std::lock_guard<std::mutex> lock(m_CWaveDataStreamAsync);
		
		--num_CWaveDataStreamAsync;
		LOGMEM("--- CWaveDataStreamAsync @ 0x%08x [total: %d]\n", (uintptr_t)this, num_CWaveDataStreamAsync);
		DETOUR_MEMBER_CALL(CWaveDataStreamAsync_dtor)(i1);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Sound_Leak")
		{
			MOD_ADD_DETOUR_STATIC(S_Play, "[client] S_Play");
			
		//	MOD_ADD_DETOUR_MEMBER(IMemAlloc_Alloc,   "IMemAlloc::Alloc");
		//	MOD_ADD_DETOUR_MEMBER(IMemAlloc_Realloc, "IMemAlloc::Realloc");
		//	MOD_ADD_DETOUR_MEMBER(IMemAlloc_Free,    "IMemAlloc::Free");
			
			/* these detours seem to remove the 'janking' and 'failed to create decoder' messages... seems we're messing with something here... */
		//	MOD_ADD_DETOUR_MEMBER(CAudioMixerWaveMP3_ctor, "[client] CAudioMixerWaveMP3::CAudioMixerWaveMP3");
		//	MOD_ADD_DETOUR_MEMBER(CAudioMixerWaveMP3_dtor, "[client] CAudioMixerWaveMP3::~CAudioMixerWaveMP3");
			
			MOD_ADD_DETOUR_MEMBER(CWaveDataStreamAsync_ctor, "[client] CWaveDataStreamAsync::CWaveDataStreamAsync");
			MOD_ADD_DETOUR_MEMBER(CWaveDataStreamAsync_dtor, "[client] CWaveDataStreamAsync::~CWaveDataStreamAsync");
		}
		
		virtual void OnEnable() override
		{
			if (f == nullptr) {
				std::lock_guard<std::mutex> f_lock(m_file);
				
				f = fopen("soundmem.log", "w");
				if (f == nullptr) {
					Warning("Failed to open soundmem.log: errno = %d:\n%s\n", errno, strerror(errno));
				} else {
					Msg("Opened soundmem.log\n");
				}
			}
		}
		virtual void OnDisable() override
		{
			if (f != nullptr) {
				std::lock_guard<std::mutex> f_lock(m_file);
				
				fflush(f);
				fclose(f);
				f = nullptr;
			}
		}
		
		virtual bool ShouldReceiveCallbacks() const { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink()
		{
			LOGMEM("FrameUpdatePreEntityThink\n");
		//	Mem_End();
		}
		
		virtual void FrameUpdatePostEntityThink()
		{
			LOGMEM("FrameUpdatePostEntityThink\n");
			Mem_End();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_sound_leak", "0", FCVAR_NOTIFY,
		"Debug: determine source of MP3-related memory leak",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	/* REMOVE ME!!! */
	StaticFuncThunk<void, const char *, bool> S_Play("[client] S_Play");
	
	
	CON_COMMAND(sig_debug_sound_leak_test, "")
	{
		if (args.ArgC() < 4) return;
		const char *filename = args[1];
		int count = atoi(args[2]);
		bool flush = (atoi(args[3]) != 0);
		
		DevMsg("Playing %d nonexistent MP3 files.\n", count);
		Mem_Begin();
		for (int i = 0; i < count; ++i) {
			S_Play(filename, flush);
		}
	//	Mem_End();
	}
}
