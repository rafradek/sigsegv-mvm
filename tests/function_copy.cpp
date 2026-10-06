#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>
#include "mem/func_copy.h"

static void Put32(uint8_t *where, uint32_t value)
{
    std::memcpy(where, &value, sizeof(value));
}

int main()
{
    auto *memory = static_cast<uint8_t *>(mmap(nullptr, 4096,
        PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(memory != MAP_FAILED);
    auto *source = memory + 256;
    auto *destination = memory + 1024;
    auto *thunk = memory + 2048;
    auto *data = memory + 3072;
    constexpr uint32_t expected = 0x12345678;
    Put32(data, expected);
    using Function = uint32_t (*)();

#ifndef PLATFORM_64BITS
    // Cover every supported compiler PIC register, including newer GCC's
    // si/di thunks. Preserve the register to obey the caller's ABI.
    for (int reg : {0, 1, 2, 3, 5, 6, 7}) {
        source[0] = 0x50 + reg; // push reg
        source[1] = 0xe8; // call __x86.get_pc_thunk.reg
        Put32(source + 2, static_cast<uint32_t>(thunk - (source + 6)));
        source[6] = 0x81;
        source[7] = 0xc0 + reg; // add reg, data-original_pc
        Put32(source + 8, static_cast<uint32_t>(data - (source + 6)));
        source[12] = 0x8b; // mov eax,[reg]
        source[13] = reg == 5 ? 0x45 : reg;
        size_t end = 14;
        if (reg == 5) source[end++] = 0; // ebp requires disp8
        source[end++] = 0x58 + reg; // pop reg
        source[end++] = 0xc3;
        thunk[0] = 0x8b;
        thunk[1] = 0x04 + (reg << 3);
        thunk[2] = 0x24; // mov reg,[esp]
        thunk[3] = 0xc3;

        // EAX cannot be restored before returning the loaded result.
        if (reg == 0) source[end - 2] = 0x59; // pop ecx instead
        assert(reinterpret_cast<Function>(source)() == expected);
        auto copied = CopyAndFixUpFuncBytes(end, end + 32,
            source, destination, destination, false);
        assert(copied == end);
        assert(destination[1] == 0xb8 + reg);
        assert(reinterpret_cast<Function>(destination)() == expected);
        uint8_t staged[64];
        assert(CopyAndFixUpFuncBytes(end, end + 32,
            source, destination, nullptr, false) == end);
        assert(CopyAndFixUpFuncBytes(end, end + 32,
            source, destination, staged, false) == end);
        std::memcpy(destination, staged, end);
        assert(reinterpret_cast<Function>(destination)() == expected);
    }
    std::puts("x86 PIC relocation: all seven register fixtures passed");
#else
    source[0] = 0x8b;
    source[1] = 0x05; // mov eax,[rip+disp32]
    Put32(source + 2, static_cast<uint32_t>(data - (source + 6)));
    source[6] = 0xc3;
    assert(reinterpret_cast<Function>(source)() == expected);
    assert(CopyAndFixUpFuncBytes(7, 39, source,
        destination, destination, false) == 7);
    assert(reinterpret_cast<Function>(destination)() == expected);
    std::puts("x64 RIP-relative relocation: passed");
#endif
    // A normal external call must remain a call, not be classified as PIC.
    source[0] = 0xe8;
    Put32(source + 1, static_cast<uint32_t>(thunk - (source + 5)));
    source[5] = 0xc3;
    thunk[0] = 0xb8; // mov eax,expected; ret
    Put32(thunk + 1, expected);
    thunk[5] = 0xc3;
    assert(CopyAndFixUpFuncBytes(6, 38, source,
        destination, destination, false) == 6);
    assert(destination[0] == 0xe8);
    assert(reinterpret_cast<Function>(destination)() == expected);
    std::puts("ordinary external-call relocation: passed");
    assert(munmap(memory, 4096) == 0);
}
