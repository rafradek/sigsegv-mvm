#include "mem/alloc.h"
#include "mem/wrapper.h"
#include "mem/protect.h"
#include "mem/opcode.h"
#include "util/misc.h"
#include <bitset>


#if 0
#define TRACE_ENABLE       1
#define TRACE_TERSE        1
#define TRACE_FNAME_MANUAL 1
#endif
#include "util/trace.h"


/* we haven't actually calculated what the max hypothetical size of a trampoline might need to be,
 * so we've set a reasonably generous size here and will always check that it is not exceeded */
constexpr size_t CHUNK_SIZE_TRAMPOLINE = 0x20;

/* we know the size of the wrapper function with certainty and can verify that this is big enough */
constexpr size_t CHUNK_SIZE_WRAPPER = 0x80;


#define TRACE_ExecMemBlockAllocator(func, ...) TRACE(TR_FNAME("ExecMemBlockAllocator<0x%zX,0x%zX>::" #func, BLOCK_SIZE, CHUNK_SIZE), ##__VA_ARGS__)
template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
class ExecMemBlockAllocator
{
public:
	ExecMemBlockAllocator() = default;
	~ExecMemBlockAllocator()
	{
		/* leak check! */
		assert(this->m_Blocks.empty());
		assert(this->m_BlocksByUsage.empty());
	}
	
	[[nodiscard]] uint8_t *Alloc();
	void Free(const uint8_t *chunk_ptr);
	
private:
	static_assert(IsMultipleOf(BLOCK_SIZE,      4096U));
	static_assert(IsMultipleOf(CHUNK_SIZE,         4U));
	static_assert(IsMultipleOf(BLOCK_SIZE, CHUNK_SIZE));
	
	static constexpr size_t CHUNKS_PER_BLOCK = (BLOCK_SIZE / CHUNK_SIZE);
	
	class Block
	{
	public:
		Block();
		~Block();
		
		bool operator< (const Block& rhs) const noexcept { return (this->BaseAddr() <  rhs.BaseAddr()); }
		bool operator==(const Block& rhs) const noexcept { return (this->BaseAddr() == rhs.BaseAddr()); }
		
		[[nodiscard]] uint8_t *Alloc();
		void Free(const uint8_t *chunk_ptr);
		
		const uint8_t *BaseAddr() const { return this->m_Base; }
		
		size_t FreeSlots() const             { return this->m_FreeSlots; }
		static constexpr size_t TotalSlots() { return CHUNKS_PER_BLOCK;  }
		
	private:
		void Map();
		void UnMap();
		
		constexpr void CheckIdx(size_t chunk_idx) const;
		constexpr void CheckPtr(const uint8_t *chunk_ptr) const;
		
		constexpr uint8_t *IdxToPtr(size_t chunk_idx) const;
		constexpr size_t PtrToIdx(const uint8_t *chunk_ptr) const;
		
		uint8_t *m_Base = nullptr;
		size_t m_FreeSlots = TotalSlots();
		std::bitset<CHUNKS_PER_BLOCK> m_AllocMask;
	};
	
	struct BlockCompare
	{
		using is_transparent = void;
		
		bool operator()(const Block&   lhs, const Block&   rhs) const noexcept { return (lhs.BaseAddr() < rhs.BaseAddr()); }
		bool operator()(const Block&   lhs, const uint8_t *rhs) const noexcept { return (lhs.BaseAddr() < rhs           ); }
		bool operator()(const uint8_t *lhs, const Block&   rhs) const noexcept { return (lhs            < rhs.BaseAddr()); }
	};
	
	void PostUpdate();
	
	std::set<Block, BlockCompare> m_Blocks; // sorted by block base address
	std::vector<Block *> m_BlocksByUsage;   // sorted by usage level (filled --> empty)
};


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
[[nodiscard]] uint8_t *ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Alloc()
{
	TRACE_ExecMemBlockAllocator(Alloc);
	
	Block *block = nullptr;
	
	/* find the most-filled existing block that is not entirely full */
	for (auto it = this->m_BlocksByUsage.begin(); it != this->m_BlocksByUsage.end(); ++it) {
		if ((*it)->FreeSlots() > 0) {
			block = *it;
			break;
		}
	}
	
	/* all existing blocks are full, so map a new block */
	if (block == nullptr) {
		auto result = m_Blocks.emplace();
		assert(result.second);
		
		block = const_cast<Block *>(&(*result.first));
		m_BlocksByUsage.push_back(block);
	}
	
	uint8_t *chunk_ptr = block->Alloc();
	{
		MemProtModifier_RX_RWX(chunk_ptr, CHUNK_SIZE);
		std::fill_n(chunk_ptr, CHUNK_SIZE, OPCODE_INT3);
	}
	
	this->PostUpdate();
	return chunk_ptr;
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Free(const uint8_t *chunk_ptr)
{
	TRACE_ExecMemBlockAllocator(Free);
	
	auto block_ptr = (const uint8_t *)((uintptr_t)chunk_ptr & ~(BLOCK_SIZE - 1));
	auto it = this->m_Blocks.find(block_ptr);
	assert(it != this->m_Blocks.end());
	
	Block *block = const_cast<Block *>(&(*it));
	block->Free(chunk_ptr);
	
	/* if this block is now completely free, unmap it */
	if (block->FreeSlots() == Block::TotalSlots()) {
		for (auto it = this->m_BlocksByUsage.begin(); it != this->m_BlocksByUsage.end(); ++it) {
			if ((*it)->BaseAddr() == block_ptr) {
				this->m_BlocksByUsage.erase(it);
				break;
			}
		}
		this->m_Blocks.erase(it);
	}
	
	this->PostUpdate();
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::PostUpdate()
{
	TRACE_ExecMemBlockAllocator(PostUpdate, "[%zu block(s)]", this->m_Blocks.size());
#if TRACE_ENABLE
	for (Block *block : this->m_BlocksByUsage) {
		TRACE_MSG("[0x%08X] %*zu/%*zu\n", (uintptr_t)block->BaseAddr(),
			NumDigits(Block::TotalSlots()), (Block::TotalSlots() - block->FreeSlots()),
			NumDigits(Block::TotalSlots()), Block::TotalSlots());
	}
#endif
	
	assert(this->m_Blocks.size() == this->m_BlocksByUsage.size());
	
	std::stable_sort(this->m_BlocksByUsage.begin(), this->m_BlocksByUsage.end(),
		[](const Block *lhs, const Block *rhs){
		//	if (lhs->FreeSlots() != rhs->FreeSlots()) {
				return (lhs->FreeSlots() < rhs->FreeSlots());
		//	} else {
		//		return (lhs->BaseAddr() < rhs->BaseAddr());
		//	}
		});
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::Block()
{
	TRACE_ExecMemBlockAllocator(Block::Block);
	
	this->Map();
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::~Block()
{
	TRACE_ExecMemBlockAllocator(Block::~Block);
	
	/* leak check! */
	assert(this->FreeSlots() == TotalSlots());
	assert(this->m_AllocMask.none());
	this->UnMap();
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
[[nodiscard]] uint8_t *ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::Alloc()
{
	TRACE_ExecMemBlockAllocator(Block::Alloc);
	
	for (size_t chunk_idx = 0; chunk_idx < TotalSlots(); ++chunk_idx) {
		if (!this->m_AllocMask.test(chunk_idx)) {
			this->m_AllocMask.set(chunk_idx);
			--this->m_FreeSlots;
			
			return this->IdxToPtr(chunk_idx);
		}
	}
	
	/* this should never happen */
	assert(false);
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::Free(const uint8_t *chunk_ptr)
{
	TRACE_ExecMemBlockAllocator(Block::Free);
	
	size_t chunk_idx = this->PtrToIdx(chunk_ptr);
	
	assert(this->m_AllocMask.test(chunk_idx));
	assert(this->m_FreeSlots < TotalSlots());
	
	this->m_AllocMask.reset(chunk_idx);
	++this->m_FreeSlots;
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::Map()
{
	TRACE_ExecMemBlockAllocator(Block::Map);
	
	assert(this->m_Base == nullptr);
	
#if defined _WINDOWS
	this->m_Base = reinterpret_cast<uint8_t *>(VirtualAlloc(nullptr, BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ));
	assert(this->m_Base != nullptr);
#else
	this->m_Base = reinterpret_cast<uint8_t *>(mmap(nullptr, BLOCK_SIZE, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	assert(this->m_Base != MAP_FAILED);
#endif
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::UnMap()
{
	TRACE_ExecMemBlockAllocator(Block::UnMap);
	
	assert(this->m_Base != nullptr);
	
#if defined _WINDOWS
	assert(VirtualFree(this->m_Base, 0, MEM_RELEASE));
#else
	assert(munmap(this->m_Base, BLOCK_SIZE) == 0);
#endif
	
	this->m_Base = nullptr;
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
constexpr void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::CheckIdx(size_t chunk_idx) const
{
	assert(chunk_idx >= 0 && chunk_idx < TotalSlots());
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
constexpr void ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::CheckPtr(const uint8_t *chunk_ptr) const
{
	assert(chunk_ptr >= this->m_Base && chunk_ptr < (this->m_Base + BLOCK_SIZE));
	assert(((uintptr_t)chunk_ptr & (CHUNK_SIZE - 1)) == 0);
}


template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
constexpr uint8_t *ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::IdxToPtr(size_t chunk_idx) const
{
	this->CheckIdx(chunk_idx);
	return (this->m_Base + (chunk_idx * CHUNK_SIZE));
}

template<size_t BLOCK_SIZE, size_t CHUNK_SIZE>
constexpr size_t ExecMemBlockAllocator<BLOCK_SIZE, CHUNK_SIZE>::Block::PtrToIdx(const uint8_t *chunk_ptr) const
{
	this->CheckPtr(chunk_ptr);
	return ((chunk_ptr - this->m_Base) / CHUNK_SIZE);
}


#define TRACE_IExecMemManager(func, ...) TRACE("IExecMemManager::" #func, ##__VA_ARGS__)
#define TRACE_CExecMemManager(func, ...) TRACE("CExecMemManager::" #func, ##__VA_ARGS__)
class CExecMemManager : public IExecMemManager
{
public:
	CExecMemManager();
	virtual ~CExecMemManager() override;
	
	[[nodiscard]] virtual uint8_t *AllocTrampoline(size_t len) override;
	virtual void FreeTrampoline(const uint8_t *ptr) override;
	
	[[nodiscard]] virtual uint8_t *AllocWrapper() override;
	virtual void FreeWrapper(const uint8_t *ptr) override;
	
private:
	static inline std::atomic_flag s_SingleInstance = ATOMIC_FLAG_INIT;
	
	size_t m_nAllocsTrampoline = 0;
	size_t m_nAllocsWrapper    = 0;
	
	ExecMemBlockAllocator<4096, CHUNK_SIZE_TRAMPOLINE> m_AllocatorTrampoline;
	ExecMemBlockAllocator<4096, CHUNK_SIZE_WRAPPER>    m_AllocatorWrapper;
};


IExecMemManager *pTheExecMemManager = nullptr;
IExecMemManager *TheExecMemManager()
{
	assert(pTheExecMemManager != nullptr);
	return pTheExecMemManager;
}


void IExecMemManager::Load()
{
	TRACE_IExecMemManager(Load);
	
	assert(pTheExecMemManager == nullptr);
	pTheExecMemManager = new CExecMemManager();
}

void IExecMemManager::Unload()
{
	TRACE_IExecMemManager(Unload);
	
	assert(pTheExecMemManager != nullptr);
	delete pTheExecMemManager;
	pTheExecMemManager = nullptr;
}


CExecMemManager::CExecMemManager()
{
	TRACE_CExecMemManager(CExecMemManager);
	
	TRACE_MSG("Base 0x%08X\n", (uint32_t)Wrapper::Base());
	TRACE_MSG("Size 0x%08X\n", (uint32_t)Wrapper::Size());
	TRACE_MSG("OffA 0x%08X\n", (uint32_t)Wrapper::Offset_MOV_FuncAddr_1());
	TRACE_MSG("OffB 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Pre());
	TRACE_MSG("OffC 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Inner());
	TRACE_MSG("OffD 0x%08X\n", (uint32_t)Wrapper::Offset_MOV_FuncAddr_2());
	TRACE_MSG("OffE 0x%08X\n", (uint32_t)Wrapper::Offset_CALL_Post());
	
	/* only permit one instance to exist at any given time */
	assert(!s_SingleInstance.test_and_set());
}

CExecMemManager::~CExecMemManager()
{
	TRACE_CExecMemManager(~CExecMemManager, "[m_nAllocsTrampoline: %zu] [m_nAllocsWrapper: %zu]", this->m_nAllocsTrampoline, this->m_nAllocsWrapper);
	
	/* leak check! */
	assert(this->m_nAllocsTrampoline == 0);
	assert(this->m_nAllocsWrapper    == 0);
	
	s_SingleInstance.clear();
}


[[nodiscard]] uint8_t *CExecMemManager::AllocTrampoline(size_t len)
{
	TRACE_CExecMemManager(AllocTrampoline, "[CHUNK_SIZE_TRAMPOLINE: %zu] [len: %zu]", CHUNK_SIZE_TRAMPOLINE, len);
	
	/* ensure that our arbitrary upper limit on possible trampoline sizes is always appropriate */
	assert(len <= CHUNK_SIZE_TRAMPOLINE);
	
	++this->m_nAllocsTrampoline;
	
	uint8_t *chunk_ptr = this->m_AllocatorTrampoline.Alloc();
	assert(chunk_ptr != nullptr);
	
	return chunk_ptr;
}

void CExecMemManager::FreeTrampoline(const uint8_t *ptr)
{
	TRACE_CExecMemManager(FreeTrampoline, "[ptr: 0x%08x] [m_nAllocsTrampoline: %zu]", (uintptr_t)ptr, this->m_nAllocsTrampoline);
	
	assert(this->m_nAllocsTrampoline > 0);
	--this->m_nAllocsTrampoline;
	
	this->m_AllocatorTrampoline.Free(ptr);
}


[[nodiscard]] uint8_t *CExecMemManager::AllocWrapper()
{
	TRACE_CExecMemManager(AllocWrapper, "[CHUNK_SIZE_WRAPPER: %zu]", CHUNK_SIZE_WRAPPER);
	
	++this->m_nAllocsWrapper;
	
	uint8_t *chunk_ptr = this->m_AllocatorWrapper.Alloc();
	assert(chunk_ptr != nullptr);
	
	return chunk_ptr;
}

void CExecMemManager::FreeWrapper(const uint8_t *ptr)
{
	TRACE_CExecMemManager(FreeWrapper, "[ptr: 0x%08x] [m_nAllocsWrapper: %zu]", (uintptr_t)ptr, this->m_nAllocsWrapper);
	
	assert(this->m_nAllocsWrapper > 0);
	--this->m_nAllocsWrapper;
	
	this->m_AllocatorWrapper.Free(ptr);
}
