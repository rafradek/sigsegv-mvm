#ifndef _INCLUDE_SIGSEGV_LIBRARY_H_
#define _INCLUDE_SIGSEGV_LIBRARY_H_


enum class Library : int
{
	INVALID,
	THIS,
	SERVER,
	ENGINE,
	DEDICATED,
	TIER0,
	CLIENT,
	VGUIMATSURFACE,
	MATERIALSYSTEM,
	SOUNDEMITTERSYSTEM,
	DATACACHE,
	VGUI,
	VPHYSICS,
	VSTDLIB,
	SOURCEMODCORE,
	VSCRIPT,
};


enum class Segment : int
{
	INVALID,
	TEXT,
	DATA,
	RODATA,
	BSS,
};


class SegInfo
{
public:
	SegInfo(uintptr_t offset, uintptr_t length) :
		m_Offset(offset), m_Length(length) {}
	
	uintptr_t Offset() const { return this->m_Offset; }
	uintptr_t Length() const { return this->m_Length; }
	
	uintptr_t AddrBegin() const { return this->m_LibBaseAddr + this->m_Offset; }
	uintptr_t AddrEnd() const   { return this->m_LibBaseAddr + this->m_Offset + this->m_Length; }
	
	bool ContainsAddr(uintptr_t addr, uintptr_t len = 0) const
	{
		uintptr_t range_begin = addr;
		uintptr_t range_end   = addr + len;
		
		return (range_begin >= this->AddrBegin()) && (range_end <= this->AddrEnd());
	}
	template<typename T> bool ContainsAddr(const T *addr, uintptr_t len = 0) const
	{
		return this->ContainsAddr((uintptr_t)addr, len);
	}
	
private:
	uintptr_t m_Offset;
	uintptr_t m_Length;
	
	uintptr_t m_LibBaseAddr = 0x00000000;
	
	friend class LibMgr;
};


class LibInfo
{
public:
	LibInfo(uintptr_t baseaddr, uintptr_t length) :
		m_BaseAddr(baseaddr), m_Length(length) {}
	
	uintptr_t BaseAddr() const { return this->m_BaseAddr; }
	uintptr_t Length() const   { return this->m_Length; }
	
	uintptr_t AddrBegin() const { return this->m_BaseAddr; }
	uintptr_t AddrEnd() const   { return this->m_BaseAddr + this->m_Length; }
	
	bool HaveSeg(Segment seg_type) const     { return (this->m_SegmentsByType.count(seg_type) != 0); }
	bool HaveSeg(const char *seg_name) const { return (this->m_SegmentsByName.count(seg_name) != 0); }
	
	const SegInfo& GetSeg(Segment seg_type) const
	{
		auto it = this->m_SegmentsByType.find(seg_type);
		assert(it != this->m_SegmentsByType.end());
		
		return (*it).second;
	}
	const SegInfo& GetSeg(const char *seg_name) const
	{
		auto it = this->m_SegmentsByName.find(seg_name);
		assert(it != this->m_SegmentsByName.end());
		
		return (*it).second;
	}
	
	bool ContainsAddr(uintptr_t addr, uintptr_t len = 0) const
	{
		uintptr_t range_begin = addr;
		uintptr_t range_end   = addr + len;
		
		return (range_begin >= this->AddrBegin()) && (range_end <= this->AddrEnd());
	}
	template<typename T> bool ContainsAddr(const T *addr, uintptr_t len = 0) const
	{
		return this->ContainsAddr((uintptr_t)addr, len);
	}
	
private:
	uintptr_t m_BaseAddr;
	uintptr_t m_Length;
	
	std::map<Segment,     SegInfo> m_SegmentsByType;
	std::map<std::string, SegInfo> m_SegmentsByName;
	
	friend class LibMgr;
};


class LibMgr
{
public:
	static void Load();
	static void Unload();
	
	template<typename T>
	static void SetPtr(Library lib, T *ptr)
	{
		s_LibPtrs[lib] = reinterpret_cast<void *>(ptr);
	}
	
	static bool HaveLib(Library lib);
	
	static const LibInfo& GetInfo(Library lib);
	
	static void *FindSym(Library lib, const char *sym);
	static std::tuple<bool, std::string, void *> FindSymRegex(Library lib, const char *pattern);
	
	template<typename FUNCTOR>
	static void ForEachSym(Library lib, FUNCTOR&& functor)
	{
		if (!HaveLib(lib)) return;
		
		void *handle = s_LibHandles.at(lib);
		assert(handle != nullptr);
		
		g_MemUtils.ForEachSymbol(handle, std::forward<FUNCTOR>(functor));
	}
	
	static Library WhichLibAtAddr(void *ptr);
	
	static Library Lib_FromString(const char *str);
	static const char *Lib_ToString(Library lib);
	static size_t Lib_MaxStringLen();
	
	static Segment Seg_FromString(const char *str);
	static const char *Seg_ToString(Segment lib);
	static size_t Seg_MaxStringLen();
	
private:
	LibMgr() {}
	
	static void *GetPtr(Library lib);
	
	static void FindInfo(Library lib);
	
	static void *OpenLibHandle(Library lib);
	static void CloseLibHandle(void *handle);
	
	static inline std::map<Library, void *>  s_LibPtrs;
	static inline std::map<Library, LibInfo> s_LibInfos;
	static inline std::map<Library, void *>  s_LibHandles;
};


#endif
