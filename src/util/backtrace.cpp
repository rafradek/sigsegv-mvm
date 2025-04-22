#include "util/backtrace.h"
#include "util/demangle.h"
#include "library.h"


#if defined _LINUX || defined _OSX

#ifdef DEBUG 

static void sym_get_proc_name(unw_cursor_t *cp, char *bufp, size_t len, unw_word_t *offp)
{
	snprintf(bufp, len, "???");
	*offp = 0;
	
	unw_word_t r_ip;
	unw_get_reg(cp, UNW_REG_IP, &r_ip);
	
	Library lib = LibMgr::WhichLibAtAddr((void *)r_ip);
	if (lib == Library::INVALID) {
		return;
	}
	
	const Symbol *best = nullptr;
	LibMgr::ForEachSym(lib, [&r_ip, &best](const Symbol& sym) {
		if (r_ip > (uintptr_t)sym.addr) {
			if (best == nullptr || (r_ip - (uintptr_t)best->addr) > (r_ip - (uintptr_t)sym.addr)) {
				best = &sym;
			}
		}
		
		return true;
	});
	
	if (best != nullptr) {
		snprintf(bufp, len, "%*s", best->name.size(), best->name.c_str());
		*offp = r_ip - (uintptr_t)best->addr;
	}
}


static void cached_get_proc_name(unw_cursor_t *cp, const char **p_name, unw_word_t *p_off)
{
	struct CacheEntry
	{
		std::string name;
		unw_word_t off;
	};
	
	static std::unordered_map<unw_word_t, CacheEntry> cache;
	static constexpr size_t MAX_ENTRIES = 4096;
	
	unw_word_t r_ip;
	unw_get_reg(cp, UNW_REG_IP, &r_ip);
	
	auto it = cache.find(r_ip);
	if (it != cache.end()) {
		/* cache hit */
		*p_name = (*it).second.name.c_str();
		*p_off  = (*it).second.off;
		return;
	}
	
	/* evict if necessary */
	while (cache.size() >= MAX_ENTRIES) {
		auto it = cache.begin();
		std::advance(it, RandomInt(0, cache.size() - 1));
		
		cache.erase(it);
	}
	
	/* create new cache entry */
	char buf[0x1000];
	buf[0] = '\0';
	unw_word_t off = 0;
	if (unw_get_proc_name(cp, buf, sizeof(buf), &off) == -UNW_ENOINFO) {
		sym_get_proc_name(cp, buf, sizeof(buf), &off);
	}
	
	std::string demangled = "???";
	if (buf[0] != '\0') {
		DemangleName(buf, demangled);
	}
	
	auto& entry = cache[r_ip];
	entry.name = demangled;
	entry.off  = off;
	
	*p_name = entry.name.c_str();
	*p_off  = entry.off;
}


void print_backtrace()
{
	unw_context_t ctx;
	unw_getcontext(&ctx);
	
	unw_cursor_t cur;
	unw_init_local(&cur, &ctx);
	
	DevMsg("%-3s  %-10s%-10s%s\n",
		"FRM", "ESP", "EIP", "FUNC");
	
	int f_idx = 1;
	while (unw_step(&cur) > 0) {
		unw_word_t r_ip;
		unw_get_reg(&cur, UNW_REG_IP, &r_ip);
		
		unw_word_t r_sp;
		unw_get_reg(&cur, UNW_REG_SP, &r_sp);
		
		const char *f_name = nullptr;
		unw_word_t off = 0;
		
		cached_get_proc_name(&cur, &f_name, &off);
		
		DevMsg("%3d  %08x  %08x  %s+0x%x\n", f_idx, r_sp, r_ip, f_name, off);
		
		++f_idx;
		
		/* sanity check to prevent unending log-filling backtraces when
		 * libunwind gets confused about where the end of the stack is */
		if (f_idx >= 1000) break;
	}
}

void sig_handler(int signo)
{
	print_backtrace();
}

CON_COMMAND(sig_crash_handler, "")
{
	signal(SIGSEGV, sig_handler);
}

#endif

extern "C"
{
	// TODO:
	// implement handlers for std::terminate and pure virtual
	
	
	[[noreturn]] void __real___assert_fail(const char *__assertion, const char *__file, unsigned int __line, const char *__function);
	
	[[noreturn]] void __wrap___assert_fail(const char *__assertion, const char *__file, unsigned int __line, const char *__function)
	{
		BACKTRACE();
		__real___assert_fail(__assertion, __file, __line, __function);
	}
	
//	[[noreturn]] void __real_abort();
//
//	[[noreturn]] void __wrap_abort()
//	{
//		BACKTRACE();
//		__real_abort();
//	}
}


#endif
