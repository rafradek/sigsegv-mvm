#ifdef _LINUX

#include "mod.h"

#include <sys/mman.h>


namespace Mod::Util::Heap_KSM
{
	void ModifyKSM(bool enable)
	{
		static size_t PAGE_SIZE = getpagesize();
		
		auto f = std::unique_ptr<FILE, decltype(&fclose)>(fopen("/proc/self/maps", "r"), &fclose);
		if (f == nullptr) {
			Msg("Failed to open '/proc/self/maps': %s\n", strerror(errno));
			return;
		}
		
		char buf[4096];
		while (fgets(buf, sizeof(buf), f.get()) != nullptr) {
			size_t start = 0x00000000;
			size_t end   = 0x00000000;
			char p_read    = ' ';
			char p_write   = ' ';
			char p_exec    = ' ';
			char p_share   = ' ';
			uint64_t ino   = 0;
			char file[4096];
			
		//	Msg("\n%s", buf);
			int num = sscanf(buf, "%8" SCNx32 "-%8" SCNx32 " %c%c%c%c %*8x %*2x:%*2x %" SCNu64 " %4095s",
				&start, &end, &p_read, &p_write, &p_exec, &p_share, &ino, file);
			
			if (num == 8) {
				if (file == nullptr || strcmp(file, "[heap]") != 0) {
				//	Msg("Entry has a file and isn't \"[heap]\"\n");
					continue;
				}
			} else if (num != 7) {
			//	Msg("Failure in sscanf\n");
				continue;
			}
			
			if (ino != 0) {
			//	Msg("Inode number is nonzero\n");
				continue;
			}
			
			if ((start & (PAGE_SIZE - 1)) != 0 || (end & (PAGE_SIZE - 1)) != 0) {
			//	Msg("The start or end addr isn't a multiple of PAGE_SIZE\n");
				continue;
			}
			
			if (start >= end) {
			//	Msg("The start and end addr are equal, or are out of order\n");
				continue;
			}
			
			if (p_share != 'p') {
			//	Msg("The 'p' share flag isn't set\n");
				continue;
			}
			// TODO: maybe filter out 'x' mappings...? not sure
			
			auto map_addr = (void *)start;
			auto map_len  = (size_t)(end - start);
			
			Msg("madvise(0x%08zx, 0x%08zx, MADV_%sMERGEABLE)\n", map_addr, map_len, (enable ? "" : "UN"));
			if (madvise(map_addr, map_len, (enable ? MADV_MERGEABLE : MADV_UNMERGEABLE)) < 0) {
				Msg("madvise failed: %s\n", strerror(errno));
//				continue;
			}
			
//			Msg("%08x-%08x %c%c%c%c %" PRIu64 "\n",
//				start, end, p_read, p_write, p_exec, p_share, ino);
		}
	}
	
	
	CON_COMMAND(sig_util_heap_ksm_enable, "Utility: enable Kernel Samepage Merging for all private anonymous mappings")
	{
		ModifyKSM(true);
	}
	
	CON_COMMAND(sig_util_heap_ksm_disable, "Utility: disable Kernel Samepage Merging for all private anonymous mappings")
	{
		ModifyKSM(false);
	}
	
	
#if 0
	std::vector<void *> bufs;
	CON_COMMAND(sig_test_malloc, "")
	{
		if (args.ArgC() < 2) {
			Msg("Nope\n");
			return;
		}
		
		unsigned long size = strtoul(args[1], nullptr, 0);
		if (size == 0) {
			Msg("Nope\n");
			return;
		}
		
		void *buf = malloc(size);
		if (buf == nullptr) {
			Msg("Got nullptr from malloc\n");
			return;
		}
		
		Msg("Allocated %ld/0x%lx bytes @ 0x%08x\n", size, size, (uintptr_t)buf);
		
		bufs.emplace_back(buf);
	}
	
	CON_COMMAND(sig_test_free, "")
	{
		for (auto buf : bufs) {
			Msg("Freeing 0x%08x\n", (uintptr_t)buf);
			free(buf);
		}
		bufs.clear();
	}
#endif
}

#endif
