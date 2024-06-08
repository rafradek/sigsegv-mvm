#ifndef _INCLUDE_SIGSEGV_MEM_FUNC_COPY_H_
#define _INCLUDE_SIGSEGV_MEM_FUNC_COPY_H_

// Copy function opcodes from source to destination, fixing up relative addresses.
// @param len_min Target amount of bytes to copy. Ideally this many bytes will be copied
// @param len_max Maximum amount of bytes to copy. Nothing will be copied if the copying operation would result in more bytes being written
// @param source Address of the function to copy
// @param destination_address Destination address of the copied function
// @param buffer Buffer to write the copied function to. If `buffer != destination_address`, the copied function need to be moved to `destination_address` manually before calling it. If buffer is null, nothing will be written
// @param stop_at_nop if the copying should stop after reading nop and int3 bytes. Use `false` if copying entire function
// @return amount of bytes copied
size_t CopyAndFixUpFuncBytes(size_t len_min, size_t len_max, const uint8_t *source, const uint8_t *destination_address, uint8_t *buffer, bool stop_at_nop = true);

// Copy function opcodes from source to destination, fixing up relative addresses.
// @param len_min Minimum amount of bytes to copy. Ideally this many bytes will be copied
// @param source Address of the function to copy
// @param destination_address Destination address to where the function should be copied
// @param stop_at_nop if the copying should stop after reading nop and int3 bytes. Use `false` if copying entire function
// @return amount of bytes copied
inline size_t CopyAndFixUpFuncBytes(size_t len_min, const uint8_t *source, uint8_t *destination_address, bool stop_at_nop = true) {
    return CopyAndFixUpFuncBytes(len_min, SIZE_MAX, source, destination_address, destination_address, stop_at_nop);
}

#endif