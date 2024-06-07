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

#ifndef _INCLUDE_SOURCEMOD_MEMORYUTILS_H_
#define _INCLUDE_SOURCEMOD_MEMORYUTILS_H_


//#include "common_logic.h"
#include <am-hashmap.h>
#include <memory>

#ifdef PLATFORM_APPLE
#include <CoreServices/CoreServices.h>
#endif

struct DynLibInfo
{
	void *baseAddress;
	size_t memorySize;
	std::unique_ptr<char[]> originalCopy;
};

using SymbolTable = std::unordered_map<std::string, void *>;

struct Symbol
{
	const std::string& name;
	void *addr;
};

#if defined _LINUX || defined PLATFORM_APPLE
struct LibSymbolTable
{
	LibSymbolTable(uintptr_t lib_base) : lib_base(lib_base) {}

	SymbolTable table;
	uintptr_t lib_base;
	uint32_t last_pos = 0;
};
#endif

#ifdef _LINUX
#ifndef PLATFORM_64BITS
	using ElfHeader = Elf32_Ehdr;
	using ElfSHeader = Elf32_Shdr;
	using ElfSymbol = Elf32_Sym;
#else
    using ElfHeader = Elf64_Ehdr;
	using ElfSHeader = Elf64_Shdr;
	using ElfSymbol = Elf64_Sym;
#endif
#endif

class MemoryUtils
{
public:
	MemoryUtils();
public: // IMemoryUtils
	void *FindPattern(const void *libPtr, const char *pattern, size_t len);
	void *ResolveSymbol(void *handle, const char *symbol);
public:
	bool GetLibraryInfo(const void *libPtr, DynLibInfo &lib);
	void ForEachSymbol(void *handle, const std::function<bool(const Symbol&)>& functor);
#if defined _LINUX
	void ForEachSection(void *handle, const std::function<void(const ElfSHeader *, const char *)>& functor);
#elif defined PLATFORM_WINDOWS
	void ForEachSection(void *handle, const std::function<void(const IMAGE_SECTION_HEADER *)>& functor);
#endif
#if defined _LINUX || defined PLATFORM_APPLE
private:
	std::vector<LibSymbolTable> m_SymTables;
#ifdef PLATFORM_APPLE
	struct dyld_all_image_infos *m_ImageList;
	SInt32 m_OSXMajor;
	SInt32 m_OSXMinor;
#endif
#endif
	typedef ke::HashMap<void *, DynLibInfo, ke::PointerPolicy<void> > LibraryInfoMap;
	LibraryInfoMap m_InfoMap;
};

extern MemoryUtils g_MemUtils;

#endif // _INCLUDE_SOURCEMOD_MEMORYUTILS_H_