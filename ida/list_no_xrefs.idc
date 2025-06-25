/*
Lists each function that has no xrefs, saves the results to xrefcounts.txt file
*/

static main() {
    auto ea = 0;
    auto f = fopen("xrefcounts.txt", "w");
    while (ea >= 0) {
        ea = get_next_func(ea);
        if (ea == -1) break;
        auto cref = 0;
        auto crefcount = 0;
        for (cref = get_first_fcref_to(ea); cref != -1; cref = get_next_fcref_to(ea, cref)) {
            crefcount = crefcount + 1;
        }
        if (crefcount == 0) {
            fprintf(f, "%s\n",get_func_name(ea));
        }
    }
    fclose(f);
}