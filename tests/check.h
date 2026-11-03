// check.h - Le harnais commun des tests unitaires : compteurs, `ok`, bilan.
//
// Un test garde ses assertions spécialisées (attendu/obtenu, refus attendu) ;
// elles comptent dans `g_pass` / `g_fail`, et `summary` imprime un bilan au même
// format pour tous.
#pragma once

#include <cstdio>

inline int g_pass = 0, g_fail = 0;

inline void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s\n", desc);
    }
}

// Le bilan, et le code de sortie du test.
inline int summary() {
    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
