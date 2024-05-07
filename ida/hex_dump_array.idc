/* 
Prints c-style array hex dump from selected instructions to console output. Instructions are included in the dumped array as comments

Example output:
{
	0xe8, 0xa0, 0x0e, 0xbd, 0xff,  // +0x0000 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
	0x83, 0xc4, 0x10,              // +0x0005 add     esp, 10h
	0x83, 0xf8, 0x03,              // +0x0008 cmp     eax, 3
}

*/
static main() {
    auto cur_addr = read_selection_start();
    auto end_addr = read_selection_end();
    
    auto pos = 0;
    auto longestInstr = 0;
    
    while (cur_addr < end_addr) {
        auto size = get_item_size(cur_addr);
        if (size == BADADDR) break;
        if (size > longestInstr) {
            longestInstr = size;
        }
        
        cur_addr = cur_addr + size;
    }
    cur_addr = read_selection_start();
    msg("{\n");
    while (cur_addr < end_addr) {
        auto instr = generate_disasm_line(cur_addr, GENDSM_FORCE_CODE);
        auto hexStr = "";
        size = get_item_size(cur_addr);
        if (size == BADADDR) break;
        
        auto i = 0;
        while (i < size) {
            hexStr = hexStr + sprintf("0x%02x, ", byte(cur_addr + i));
            i = i + 1;
        }
        while (i < longestInstr) {
            hexStr = hexStr + "      ";
            i = i + 1;
        }
        
        msg("\t%s // +0x%04x %s\n",  hexStr,pos, instr);
        
        pos = pos + size;
        cur_addr = cur_addr + size;
    }
    msg("};\n");
}