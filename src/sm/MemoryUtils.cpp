/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2011 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

// WARNING: VERY HEAVILY MODIFIED FROM ORIGINAL SOURCEMOD CODE!

#include "MemoryUtils.h"
#ifdef PLATFORM_LINUX
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>

#define PAGE_SIZE			4096
#define PAGE_ALIGN_UP(x)	((x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#endif

#ifdef PLATFORM_APPLE
#include <AvailabilityMacros.h>
#include <mach/task.h>
#include <mach-o/dyld_images.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#endif // PLATFORM_APPLE

MemoryUtils g_MemUtils;

MemoryUtils::MemoryUtils()
{
#ifdef PLATFORM_APPLE

	task_dyld_info_data_t dyld_info;
	mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
	task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&dyld_info, &count);
	m_ImageList = (struct dyld_all_image_infos *)dyld_info.all_image_info_addr;

#endif
}

void *MemoryUtils::ResolveSymbol(void *handle, const char *symbol)
{
#ifdef PLATFORM_WINDOWS

	return GetProcAddress((HMODULE)handle, symbol);
	
#elif defined PLATFORM_LINUX

#ifdef PLATFORM_X86
	typedef Elf32_Ehdr ElfHeader;
	typedef Elf32_Shdr ElfSHeader;
	typedef Elf32_Sym ElfSymbol;
	#define ELF_SYM_TYPE ELF32_ST_TYPE
#else
	typedef Elf64_Ehdr ElfHeader;
	typedef Elf64_Shdr ElfSHeader;
	typedef Elf64_Sym ElfSymbol;
	#define ELF_SYM_TYPE ELF64_ST_TYPE
#endif

	struct link_map *dlmap = (struct link_map *)handle;
	struct stat dlstat;
	int dlfile;
	uintptr_t map_base;
	ElfHeader *file_hdr;
	ElfSHeader *sections, *shstrtab_hdr, *symtab_hdr = nullptr, *strtab_hdr = nullptr;
	ElfSymbol *symtab;
	const char *shstrtab, *strtab;
	uint16_t section_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table = nullptr;
	
	/* See if we already have a symbol table for this library */
	for (LibSymbolTable& libtable2 : m_SymTables)
	{
		if (libtable2.lib_base == dlmap->l_addr)
		{
			libtable = &libtable2;
			table = &libtable2.table;
			break;
		}
	}

	/* If we don't have a symbol table for this library, then create one */
	if (table == nullptr)
	{
		libtable = &(m_SymTables.emplace_back(dlmap->l_addr));
		table = &libtable->table;
	}

	/* See if the symbol is already cached in our table */
	auto it = table->find(symbol);
	if (it != table->end())
	{
		return (*it).second;
	}

	/* If symbol isn't in our table, then we have open the actual library */
	dlfile = open(dlmap->l_name, O_RDONLY);
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
	{
		close(dlfile);
		return nullptr;
	}

	/* Map library file into memory */
	file_hdr = (ElfHeader *)mmap(nullptr, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	map_base = (uintptr_t)file_hdr;
	if (file_hdr == MAP_FAILED)
	{
		close(dlfile);
		return nullptr;
	}
	close(dlfile);

	if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
	{
		munmap(file_hdr, dlstat.st_size);
		return nullptr;
	}

	sections = (ElfSHeader *)(map_base + file_hdr->e_shoff);
	section_count = file_hdr->e_shnum;
	/* Get ELF section header string table */
	shstrtab_hdr = &sections[file_hdr->e_shstrndx];
	shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

	/* Iterate sections while looking for ELF symbol table and string table */
	for (uint16_t i = 0; i < section_count; i++)
	{
		ElfSHeader &hdr = sections[i];
		const char *section_name = shstrtab + hdr.sh_name;

		if (strcmp(section_name, ".symtab") == 0)
		{
			symtab_hdr = &hdr;
		}
		else if (strcmp(section_name, ".strtab") == 0)
		{
			strtab_hdr = &hdr;
		}
	}

	/* Uh oh, we don't have a symbol table or a string table */
	if (symtab_hdr == nullptr || strtab_hdr == nullptr)
	{
		munmap(file_hdr, dlstat.st_size);
		return nullptr;
	}

	symtab = (ElfSymbol *)(map_base + symtab_hdr->sh_offset);
	strtab = (const char *)(map_base + strtab_hdr->sh_offset);
	symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

	void *sym_addr = nullptr;

	/* Iterate symbol table starting from the position we were at last time */
	for (size_t i = libtable->last_pos; i < symbol_count; i++)
	{
		ElfSymbol &sym = symtab[i];
		unsigned char sym_type = ELF_SYM_TYPE(sym.st_info);
		const char *sym_name = strtab + sym.st_name;

		/* Skip symbols that are undefined or do not refer to functions or objects */
		if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
		{
			continue;
		}

		/* Caching symbols as we go along */
		table->emplace(sym_name, (void *)(dlmap->l_addr + sym.st_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			sym_addr = (void *)(dlmap->l_addr + sym.st_value);
			libtable->last_pos = ++i;
			break;
		}
	}

	munmap(file_hdr, dlstat.st_size);
	return sym_addr;

#elif defined PLATFORM_APPLE

#ifdef PLATFORM_X86
	typedef struct mach_header MachHeader;
	typedef struct segment_command MachSegment;
	typedef struct nlist MachSymbol;
	const uint32_t MACH_LOADCMD_SEGMENT = LC_SEGMENT;
#else
	typedef struct mach_header_64 MachHeader;
	typedef struct segment_command_64 MachSegment;
	typedef struct nlist_64 MachSymbol;
	const uint32_t MACH_LOADCMD_SEGMENT = LC_SEGMENT_64;
#endif

	typedef struct load_command MachLoadCmd;
	typedef struct symtab_command MachSymHeader;

	uintptr_t dlbase = 0, linkedit_addr;
	uint32_t image_count = m_ImageList->infoArrayCount;
	MachHeader *file_hdr;
	MachLoadCmd *loadcmds;
	MachSegment *linkedit_hdr = nullptr;
	MachSymHeader *symtab_hdr = nullptr;
	MachSymbol *symtab;
	const char *strtab;
	uint32_t loadcmd_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table = nullptr;
	
	/* Loop through mach-o images in process.
	 * We can skip index 0 since that is just the executable.
	 */
	for (uint32_t i = 1; i < image_count; i++)
	{
		const struct dyld_image_info &info = m_ImageList->infoArray[i];
		
		/* "Load" each one until we get a matching handle */
		void *h = dlopen(info.imageFilePath, RTLD_NOLOAD);
		if (h == handle)
		{
			dlbase = (uintptr_t)info.imageLoadAddress;
			dlclose(h);
			break;
		}
		
		dlclose(h);
	}
	
	if (!dlbase)
	{
		/* Uh oh, we couldn't find a matching handle */
		return nullptr;
	}
	
	/* See if we already have a symbol table for this library */
	for (LibSymbolTable& libtable2 : m_SymTables)
	{
		if (libtable2.lib_base == dlbase)
		{
			libtable = &libtable2;
			table = &libtable2.table;
			break;
		}
	}
	
	/* If we don't have a symbol table for this library, then create one */
	if (table == nullptr)
	{
		libtable = &(m_SymTables.emplace_back(dlbase));
		table = &libtable->table;
	}
	
	/* See if the symbol is already cached in our table */
	auto it = table->find(symbol);
	if (it != table->end())
	{
		return (*it).second;
	}
	
	/* If symbol isn't in our table, then we have to locate it in memory */
	
	file_hdr = (MachHeader *)dlbase;
	loadcmds = (MachLoadCmd *)(dlbase + sizeof(MachHeader));
	loadcmd_count = file_hdr->ncmds;
	
	/* Loop through load commands until we find the ones for the symbol table */
	for (uint32_t i = 0; i < loadcmd_count; i++)
	{
		if (loadcmds->cmd == MACH_LOADCMD_SEGMENT && !linkedit_hdr)
		{
			MachSegment *seg = (MachSegment *)loadcmds;
			if (strcmp(seg->segname, "__LINKEDIT") == 0)
			{
				linkedit_hdr = seg;
				if (symtab_hdr)
				{
					break;
				}
			}
		}
		else if (loadcmds->cmd == LC_SYMTAB)
		{
			symtab_hdr = (MachSymHeader *)loadcmds;
			if (linkedit_hdr)
			{
				break;
			}
		}

		/* Load commands are not of a fixed size which is why we add the size */
		loadcmds = (MachLoadCmd *)((uintptr_t)loadcmds + loadcmds->cmdsize);
	}
	
	if (!linkedit_hdr || !symtab_hdr || !symtab_hdr->symoff || !symtab_hdr->stroff)
	{
		/* Uh oh, no symbol table */
		return nullptr;
	}

	linkedit_addr = dlbase + linkedit_hdr->vmaddr;
	symtab = (MachSymbol *)(linkedit_addr + symtab_hdr->symoff - linkedit_hdr->fileoff);
	strtab = (const char *)(linkedit_addr + symtab_hdr->stroff - linkedit_hdr->fileoff);
	symbol_count = symtab_hdr->nsyms;
	
	void *sym_addr = nullptr;
	
	/* Iterate symbol table starting from the position we were at last time */
	for (size_t i = libtable->last_pos; i < symbol_count; i++)
	{
		MachSymbol &sym = symtab[i];
		/* Ignore the prepended underscore on all symbols, so +1 here */
		const char *sym_name = strtab + sym.n_un.n_strx + 1;
		
		/* Skip symbols that are undefined */
		if (sym.n_sect == NO_SECT)
		{
			continue;
		}
		
		/* Caching symbols as we go along */
		table->emplace(sym_name, (void *)(dlbase + sym.n_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			sym_addr = (void *)(dlbase + sym.n_value);
			libtable->last_pos = ++i;
			break;
		}
	}
	
	return sym_addr;

#endif
}

bool MemoryUtils::GetLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	uintptr_t baseAddr;

	if (libPtr == nullptr)
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS

#ifdef PLATFORM_X86
	const WORD PE_FILE_MACHINE = IMAGE_FILE_MACHINE_I386;
#else
	const WORD PE_FILE_MACHINE = IMAGE_FILE_MACHINE_AMD64;
#endif

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<uintptr_t>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture */
	if (file->Machine != PE_FILE_MACHINE)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

#elif defined PLATFORM_LINUX


#ifdef PLATFORM_X86
	typedef Elf32_Ehdr ElfHeader;
	typedef Elf32_Phdr ElfPHeader;
	const unsigned char ELF_CLASS = ELFCLASS32;
	const uint16_t ELF_MACHINE = EM_386;
	DevMsg("32 bit\n");
#else
	typedef Elf64_Ehdr ElfHeader;
	typedef Elf64_Phdr ElfPHeader;
	const unsigned char ELF_CLASS = ELFCLASS64;
	const uint16_t ELF_MACHINE = EM_X86_64;
	DevMsg("64 bit\n");
#endif

	Dl_info info;
	ElfHeader *file;
	ElfPHeader *phdr;
	uint16_t phdrCount;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<uintptr_t>(info.dli_fbase);
	file = reinterpret_cast<ElfHeader *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	/* Check ELF endianness */
	if (file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* Check ELF architecture */
	if (file->e_ident[EI_CLASS] != ELF_CLASS || file->e_machine != ELF_MACHINE)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<ElfPHeader *>(baseAddr + file->e_phoff);

	for (uint16_t i = 0; i < phdrCount; i++)
	{
		ElfPHeader &hdr = phdr[i];

		/* We only really care about the segment with executable code */
		if (hdr.p_type == PT_LOAD && hdr.p_flags == (PF_X|PF_R))
		{
			/* From glibc, elf/dl-load.c:
			 * c->mapend = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1) 
			 *             & ~(GLRO(dl_pagesize) - 1));
			 *
			 * In glibc, the segment file size is aligned up to the nearest page size and
			 * added to the virtual address of the segment. We just want the size here.
			 */
			lib.memorySize = PAGE_ALIGN_UP(hdr.p_filesz);
			break;
		}
	}

#elif defined PLATFORM_APPLE

#ifdef PLATFORM_X86
	typedef struct mach_header MachHeader;
	typedef struct segment_command MachSegment;
	const uint32_t MACH_MAGIC = MH_MAGIC;
	const uint32_t MACH_LOADCMD_SEGMENT = LC_SEGMENT;
	const cpu_type_t MACH_CPU_TYPE = CPU_TYPE_I386;
	const cpu_subtype_t MACH_CPU_SUBTYPE = CPU_SUBTYPE_I386_ALL;
#else
	typedef struct mach_header_64 MachHeader;
	typedef struct segment_command_64 MachSegment;
	const uint32_t MACH_MAGIC = MH_MAGIC_64;
	const uint32_t MACH_LOADCMD_SEGMENT = LC_SEGMENT_64;
	const cpu_type_t MACH_CPU_TYPE = CPU_TYPE_X86_64;
	const cpu_subtype_t MACH_CPU_SUBTYPE = CPU_SUBTYPE_X86_64_ALL;
#endif

	Dl_info info;
	MachHeader *file;
	MachSegment *seg;
	uint32_t cmd_count;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = (uintptr_t)info.dli_fbase;
	file = (MachHeader *)baseAddr;

	/* Check Mach-O magic */
	if (file->magic != MACH_MAGIC)
	{
		return false;
	}

	/* Check architecture */
	if (file->cputype != MACH_CPU_TYPE || file->cpusubtype != MACH_CPU_SUBTYPE)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if (file->filetype != MH_DYLIB)
	{
		return false;
	}

	cmd_count = file->ncmds;
	seg = (MachSegment *)(baseAddr + sizeof(MachHeader));
	
	/* Add up memory sizes of mapped segments */
	for (uint32_t i = 0; i < cmd_count; i++)
	{		
		if (seg->cmd == MACH_LOADCMD_SEGMENT)
		{
			lib.memorySize += seg->vmsize;
		}
		
		seg = (MachSegment *)((uintptr_t)seg + seg->cmdsize);
	}

#endif

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}

void MemoryUtils::ForEachSymbol(void *handle, const std::function<bool(const Symbol&)>& functor)
{
	/* do a bogus symbol lookup to force everything into the symbol table cache */
	assert(ResolveSymbol(handle, "________________") == nullptr);
	
	uintptr_t dlbase = 0;
	SymbolTable *table = nullptr;
	
#ifdef PLATFORM_LINUX
	struct link_map *dlmap = (struct link_map *)handle;
	dlbase = dlmap->l_addr;
#elif defined PLATFORM_APPLE
	uint32_t image_count = m_ImageList->infoArrayCount;
	
	/* Loop through mach-o images in process.
	 * We can skip index 0 since that is just the executable.
	 */
	for (uint32_t i = 1; i < image_count; i++)
	{
		const struct dyld_image_info &info = m_ImageList->infoArray[i];
		
		/* "Load" each one until we get a matching handle */
		void *h = dlopen(info.imageFilePath, RTLD_NOLOAD);
		if (h == handle)
		{
			dlbase = (uintptr_t)info.imageLoadAddress;
			dlclose(h);
			break;
		}
		
		dlclose(h);
	}
	
	if (!dlbase)
	{
		/* Uh oh, we couldn't find a matching handle */
		return;
	}
#endif
	
	/* See if we already have a symbol table for this library */
	for (LibSymbolTable& libtable : m_SymTables)
	{
		if (libtable.lib_base == dlbase)
		{
			table = &libtable.table;
			break;
		}
	}
	
	/* If we don't have a symbol table for this library, then create one */
	if (table == nullptr)
	{
		table = &m_SymTables.emplace_back(dlbase).table;
	}
	
	for (auto it = table->begin(); it != table->end(); ++it) {
		const std::string& sym_name = (*it).first;
		void *sym_addr              = (*it).second;
		
	//	if (sym_addr == nullptr) continue; // ?
		
		if (!functor(Symbol{sym_name, sym_addr})) break;
	}
}

/* TODO: maybe update ForEachSection for SourceMod's 64-bit compatibility changes (20161009..20180405) */
#ifndef PLATFORM_X86
#error
#endif

#if defined PLATFORM_LINUX
void MemoryUtils::ForEachSection(void *handle, const std::function<void(const Elf32_Shdr *, const char *)>& functor)
{
	auto dlmap = (struct link_map *)handle;
	
	int dlfile = open(dlmap->l_name, O_RDONLY);
	struct stat dlstat;
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1) {
		close(dlfile);
		return;
	}
	
	auto ehdr = (Elf32_Ehdr *)mmap(nullptr, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	if (ehdr == MAP_FAILED) {
		close(dlfile);
		return;
	}
	
	close(dlfile);
	
	if (ehdr->e_shoff == 0 || ehdr->e_shstrndx == SHN_UNDEF) {
		munmap(ehdr, dlstat.st_size);
		return;
	}
	
	auto shdrs = (Elf32_Shdr *)((uintptr_t)ehdr + ehdr->e_shoff);
	uint16_t n_shdrs = ehdr->e_shnum;
	
	auto shstrtab = (const char *)((uintptr_t)ehdr + shdrs[ehdr->e_shstrndx].sh_offset);
	
	for (uint16_t i = 0; i < n_shdrs; ++i) {
		Elf32_Shdr *shdr = shdrs + i;
		
		if (shdr->sh_type == SHT_NULL) continue;
		
		const char *name = shstrtab + shdr->sh_name;
		functor(shdr, name);
	}
	
	munmap(ehdr, dlstat.st_size);
}
#elif defined PLATFORM_WINDOWS
void MemoryUtils::ForEachSection(void *handle, const std::function<void(const IMAGE_SECTION_HEADER *)>& functor)
{
	IMAGE_NT_HEADERS *pNtHdr = ImageNtHeader(handle);
	auto pSectHdr = (IMAGE_SECTION_HEADER *)(pNtHdr + 1);
	
	int n_sect = pNtHdr->FileHeader.NumberOfSections;
	for (int i = 0; i < n_sect; ++i) {
		functor(pSectHdr);
		++pSectHdr;
	}
}
#endif
