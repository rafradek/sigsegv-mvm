#ifndef _INCLUDE_SIGSEGV_MEM_ALLOC_H_
#define _INCLUDE_SIGSEGV_MEM_ALLOC_H_


class IExecMemManager
{
public:
	virtual ~IExecMemManager() = default;
	
	[[nodiscard]] virtual uint8_t *AllocTrampoline(size_t len) = 0;
	virtual void FreeTrampoline(const uint8_t *ptr) = 0;
	
	[[nodiscard]] virtual uint8_t *AllocWrapper() = 0;
	virtual void FreeWrapper(const uint8_t *ptr) = 0;

	[[nodiscard]] virtual uint8_t *AllocDetourWrapper() { return nullptr; };
	virtual void FreeDetourWrapper(const uint8_t *ptr) {};
	
	static void Load();
	static void Unload();
};

IExecMemManager *TheExecMemManager();


#endif
