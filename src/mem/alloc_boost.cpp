#include "mem/alloc.h"
#include "mem/wrapper.h"
#include "util/misc.h"
#include <boost/pool/pool.hpp>

#if 1
#define TRACE_ENABLE 1
#define TRACE_TERSE  1
#endif
#include "util/trace.h"


/* we haven't actually calculated what the max hypothetical size of a trampoline might need to be,
 * so we've set a reasonably generous size here and will always check that it is not exceeded */
const size_t CHUNK_SIZE_TRAMPOLINE = 0x20;

/* we know the size of the wrapper function with certainty */
const size_t CHUNK_SIZE_WRAPPER = Wrapper::Size();


/* implementation of UserAllocator concept */
class ExecMemBlockAllocator
{
public:
	using size_type       = size_t;
	using difference_type = ptrdiff_t;
	
	static char *malloc(size_t block_size);
	static void free(const char *block_ptr);
	
	static size_t NumActiveMappings() { return s_Mappings.size(); }
	
private:
	static size_t MappingGranularity();
	static size_t MappingSize(size_t block_size);
	
	static void *PlatVMMap(size_t mapping_size);
	static void PlatVMUnmap(void *mapping_ptr, size_t mapping_size);
	
	/* { mapped_ptr --> mapped_size } pairs */
	static inline std::unordered_map<void *, size_t> s_Mappings;
};


char *ExecMemBlockAllocator::malloc(size_t block_size)
{
	TRACE("[block_size: %zu] [mapping_size: %zu]", block_size, MappingSize(block_size));
	
	size_t mapping_size = MappingSize(block_size);
	void *mapping_ptr   = PlatVMMap(mapping_size);
	
	// TODO: do a complete, comprehensive check on ALL existing mappings to ensure none overlap our new mapping
	
	/* ensure that no mapping already existed with the same mapping_ptr key */
	assert(s_Mappings.insert_or_assign(mapping_ptr, mapping_size).second);
	
	return reinterpret_cast<char *>(mapping_ptr);
}

void ExecMemBlockAllocator::free(const char *block_ptr)
{
	TRACE("[block_ptr: 0x%08x]", (uintptr_t)block_ptr);
	
	/* this is stupid */
	auto mapping_ptr = reinterpret_cast<void *>(const_cast<char *>(block_ptr));
	
	/* this had better be a mapping that we know about */
	auto it = s_Mappings.find(mapping_ptr);
	assert(it != s_Mappings.end());
	
	size_t mapping_size = (*it).second;
	
	PlatVMUnmap(mapping_ptr, mapping_size);
	
	s_Mappings.erase(it);
}


size_t ExecMemBlockAllocator::MappingGranularity()
{
	/* 64 KiB: 16 pages; ideal for the Windows VMM */
	constexpr size_t MAPPING_GRANULARITY = (16 * 4096);
	
#if defined _WINDOWS
	return MAPPING_GRANULARITY;
#else
	/* only run this check the first time we're called */
	static size_t posix_granularity = []{
		long sc_pagesize = sysconf(_SC_PAGESIZE);
		assert(sc_pagesize != -1L);
		
		auto posix_pagesize = static_cast<size_t>(sc_pagesize);
		assert(IsMultipleOf(MAPPING_GRANULARITY, posix_pagesize));
		
		return MAPPING_GRANULARITY;
	}();
	
	return posix_granularity;
#endif
}

size_t ExecMemBlockAllocator::MappingSize(size_t block_size)
{
	return RoundUpToPowerOfTwo(block_size, MappingGranularity());
}


void *ExecMemBlockAllocator::PlatVMMap(size_t mapping_size)
{
	TRACE("[mapping_size: %zu (%zux64KiB)]",
		mapping_size, (mapping_size / MappingGranularity()));
	
	assert(mapping_size % MappingGranularity() == 0);
	
#if defined _WINDOWS
	void *mapping_ptr = VirtualAlloc(nullptr, mapping_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);
	assert(mapping_ptr != nullptr);
#else
	void *mapping_ptr = mmap(nullptr, mapping_size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(mapping_ptr != MAP_FAILED);
#endif
	
	/* the mapped memory is guaranteed to be zeroed on both platforms */
	return mapping_ptr;
}

void ExecMemBlockAllocator::PlatVMUnmap(void *mapping_ptr, size_t mapping_size)
{
	TRACE("[mapping_ptr: 0x%08x] [mapping_size: %zu (%zux64KiB)]",
		(uintptr_t)mapping_ptr, mapping_size, (mapping_size / MappingGranularity()));
	
	assert(mapping_size % MappingGranularity() == 0);
	
#if defined _WINDOWS
	assert(VirtualFree(mapping_ptr, 0, MEM_RELEASE));
#else
	assert(munmap(mapping_ptr, mapping_size) == 0);
#endif
}


class CExecMemManager : public IExecMemManager
{
public:
	CExecMemManager();
	virtual ~CExecMemManager() override;
	
	virtual void *AllocTrampoline(size_t len) override;
	virtual void FreeTrampoline(const void *ptr) override;
	
	virtual void *AllocWrapper() override;
	virtual void FreeWrapper(const void *ptr) override;
	
private:
	static inline std::atomic_flag s_SingleInstance = ATOMIC_FLAG_INIT;
	
	size_t m_nAllocsTrampoline = 0;
	size_t m_nAllocsWrapper    = 0;
	
	boost::pool<ExecMemBlockAllocator> *m_PoolTrampoline = nullptr;
	boost::pool<ExecMemBlockAllocator> *m_PoolWrapper    = nullptr;
};


IExecMemManager *pTheExecMemManager = nullptr;
IExecMemManager *TheExecMemManager()
{
	assert(pTheExecMemManager != nullptr);
	return pTheExecMemManager;
}


void IExecMemManager::Load()
{
	TRACE();
	
	assert(pTheExecMemManager == nullptr);
	pTheExecMemManager = new CExecMemManager();
}

void IExecMemManager::Unload()
{
	TRACE();
	
	assert(pTheExecMemManager != nullptr);
	delete pTheExecMemManager;
	pTheExecMemManager = nullptr;
}


CExecMemManager::CExecMemManager()
{
	TRACE();
	
	TRACE_MSG("Base 0x%08X\n", (uint32_t)Wrapper::Base());
	TRACE_MSG("Size 0x%08X\n", (uint32_t)Wrapper::Size());
	TRACE_MSG("OffA 0x%08X\n", (uint32_t)Wrapper::Offset_MOV_FuncAddr_1());
	TRACE_MSG("OffB 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Pre());
	TRACE_MSG("OffC 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Inner());
	TRACE_MSG("OffD 0x%08X\n", (uint32_t)Wrapper::Offset_MOV_FuncAddr_2());
	TRACE_MSG("OffE 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Post());
	
	/* only permit one instance to exist at any given time */
	assert(!s_SingleInstance.test_and_set());
	
	this->m_PoolTrampoline = new boost::pool<ExecMemBlockAllocator>(CHUNK_SIZE_TRAMPOLINE);
	this->m_PoolWrapper    = new boost::pool<ExecMemBlockAllocator>(CHUNK_SIZE_WRAPPER);
}

CExecMemManager::~CExecMemManager()
{
	TRACE("[m_nAllocsTrampoline: %zu] [m_nAllocsWrapper: %zu] [NumActiveMappings: %zu]",
		this->m_nAllocsTrampoline, this->m_nAllocsWrapper, ExecMemBlockAllocator::NumActiveMappings());
	
	/* leak check! */
	assert(this->m_nAllocsTrampoline == 0);
	assert(this->m_nAllocsWrapper    == 0);
	
	delete this->m_PoolTrampoline;
	delete this->m_PoolWrapper;
	
	/* leak check! */
	assert(ExecMemBlockAllocator::NumActiveMappings() == 0);
	
	s_SingleInstance.clear();
}


void *CExecMemManager::AllocTrampoline(size_t len)
{
	TRACE("[CHUNK_SIZE_TRAMPOLINE: %zu] [len: %zu]", CHUNK_SIZE_TRAMPOLINE, len);
	
	/* ensure that our arbitrary upper limit on possible trampoline sizes is always appropriate */
	assert(len <= CHUNK_SIZE_TRAMPOLINE);
	
	void *chunk_ptr = this->m_PoolTrampoline->malloc();
	assert(chunk_ptr != nullptr);
	
	++this->m_nAllocsTrampoline;
	
	return chunk_ptr;
}

void CExecMemManager::FreeTrampoline(const void *ptr)
{
	TRACE("[ptr: 0x%08x] [m_nAllocsTrampoline: %zu]", (uintptr_t)ptr, this->m_nAllocsTrampoline);
	
	assert(this->m_nAllocsTrampoline > 0);
	
	this->m_PoolTrampoline->free(const_cast<void *>(ptr));
	
	--this->m_nAllocsTrampoline;
}


void *CExecMemManager::AllocWrapper()
{
	TRACE("[CHUNK_SIZE_WRAPPER: %zu]", CHUNK_SIZE_WRAPPER);
	
	void *chunk_ptr = this->m_PoolWrapper->malloc();
	assert(chunk_ptr != nullptr);
	
	++this->m_nAllocsWrapper;
	
	return chunk_ptr;
}

void CExecMemManager::FreeWrapper(const void *ptr)
{
	TRACE("[ptr: 0x%08x] [m_nAllocsWrapper: %zu]", (uintptr_t)ptr, this->m_nAllocsWrapper);
	
	assert(this->m_nAllocsWrapper > 0);
	
	this->m_PoolWrapper->free(const_cast<void *>(ptr));
	
	--this->m_nAllocsWrapper;
}
