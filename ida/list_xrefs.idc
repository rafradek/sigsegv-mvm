/*
Lists each function and its xref counts, saves the results to xrefcounts.txt file
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
        fprintf(f, "%s|%s|%d\n",get_func_name(ea), demangle_name(get_func_name(ea),get_inf_attr(INF_SHORT_DN)), crefcount);
    }
    fclose(f);
}