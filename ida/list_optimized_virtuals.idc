/*
Lists all cases of optimized virtual calls, and writes them to optimizedvirts.txt file. The output file has to be then processed to sort and remove duplicates because IDC lacks such functionality
*/

static main() {
    auto ea = 0;
    auto f = fopen("optimizedvirts.txt", "w");
    while (ea >= 0) {
        ea = find_text(ea, SEARCH_DOWN | SEARCH_REGEX, 0, 0, "cmp.*, offset _ZN");
        auto instr = generate_disasm_line(ea, GENDSM_FORCE_CODE);
        msg("%x %s\n", ea, instr);
        fprintf(f, "%s\n", instr);
        if (ea == -1) break;
        ea = ea + get_item_size(ea);
    }
    fclose(f);
}