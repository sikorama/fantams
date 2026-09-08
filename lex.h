// lex.h - Le découpeur de jetons et le curseur, partagés (fantams)
//
// Deux langages d'entrée les emploient : le **script de linkage** (quelle
// section va où) et le **profil de cible** (ce que la machine sait faire). Ils
// n'ont pas la même grammaire, mais ils ont le même lexique et la même forme à
// blocs, et ils doivent rendre leurs diagnostics de la même façon.
//
// Ce fichier n'est né qu'au SECOND consommateur, et c'est délibéré : un
// adaptateur, c'est une couture hypothétique ; deux, c'est une couture réelle
// (`coutures-de-la-chaine.md` §4). L'extraire à l'arrivée du premier aurait été
// deviner ce que le second demanderait.
#pragma once

#include "asm.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lex {

// Une seule subtilité dans ce découpage : `..` est UN jeton, et non deux points.
// Sans quoi `[0x3F00..0x3FFF]` et `[ext0..ext3]` demanderaient au parseur de
// deviner, et `rom_upper.on` cesserait d'être lisible.
struct Tok {
    enum Kind { End, Name, Number, Text, Punct } kind = End;
    std::string s;        // le texte, pour Name / Text / Punct
    int64_t n = 0;        // la valeur, pour Number
    int line = 0;
};

// Rend false et remplit `err` / `errLine` sur un caractère inattendu ou une
// chaîne non terminée. Les quatre notations de nombre du projet — `0x`, `&`,
// `#`, `%` et le décimal — sont reconnues : en refuser une demanderait à
// l'auteur d'un `.asm` d'écrire ses adresses autrement ailleurs que dans sa
// source. Les deux styles de commentaire aussi : `//` et `;`.
bool tokenize(const std::string &text, std::vector<Tok> &out,
              std::string &err, int &errLine);

// Le curseur : des jetons, une position, et les diagnostics qui vont avec.
// Ce qu'il porte est exactement ce que les deux grammaires ont en commun ;
// chacune garde sa propre reprise sur erreur, qui dépend de ses mots-clés.
struct Cursor {
    std::vector<Tok> t;
    size_t i = 0;
    std::string file;
    bool ok = true;
    std::vector<asmb::Diagnostic> errors;

    const Tok &cur() const { return t[i]; }
    bool atEnd() const { return t[i].kind == Tok::End; }
    bool isName(const char *w) const { return cur().kind == Tok::Name && cur().s == w; }
    bool isPunct(const char *w) const { return cur().kind == Tok::Punct && cur().s == w; }
    void next() { if (!atEnd()) ++i; }

    void err(const std::string &msg) { err(cur().line, msg); }
    void err(int line, const std::string &msg);

    // Ce que le jeton courant est, dit comme un diagnostic doit le dire : citer
    // « end of file » plutôt que rien du tout est ce qui distingue une accolade
    // oubliée d'une faute de frappe.
    std::string got() const;

    bool want(const char *p);
    bool wantNumber(int64_t &v);
    bool wantName(std::string &v);

    // Sauter jusqu'à la fin du bloc courant, accolades comptées. Après une faute
    // DANS un bloc, reprendre à l'instruction suivante du même bloc produirait
    // une cascade dont seul le premier diagnostic est vrai — et, si la reprise
    // retombe sur le jeton fautif, une boucle.
    void skipBlock();
};

} // namespace lex
