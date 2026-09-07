// expr.h - Évaluateur d'expressions réutilisable (fantams)
//
// Autonome, sans état global. Le resolver de symboles est injecté : le
// préprocesseur l'utilise avec les seules variables PP ; l'assembleur le
// réutilise avec un resolver connaissant les labels.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace expr {

// Une section, vue de l'évaluateur : un identifiant OPAQUE. `expr` n'en sait
// rien d'autre que « c'est la même » ou « ce n'est pas la même » — la table des
// sections vit chez l'assembleur, et la faire connaître ici rendrait
// l'évaluateur intestable seul.
using SectionId = int;
constexpr SectionId NoSection = -1;

// L'octet retenu d'une valeur relocalisable — ce que produisent `high()` et
// `low()`. Sur une valeur ABSOLUE ces deux fonctions calculent tout de suite et
// rendent `Whole` : il n'y a rien à reporter au linkage.
enum class Byte { Whole, High, Low };

// Une VALEUR d'expression : une partie connue, plus le coefficient d'une base
// de section qu'on ne connaîtra qu'au linkage.
//
//   coeff == 0 -> ABSOLUE       : `real` est toute la valeur
//   coeff != 0 -> RELOCALISABLE : coeff * base(section) + real
//
// Le coefficient est borné à {-1, 0, +1} et une SEULE section est citable à la
// fois : c'est ce qui rend « fin - debut » absolu dans une même section et
// « label * 2 » refusable. L'affinité est une propriété de l'ARBRE, pas du
// symbole — elle ne se calcule qu'ici.
//
// L'ADR 0008 n'est pas contredit, il est délimité : il régit l'arithmétique
// ABSOLUE, où le calcul reste réel de bout en bout. Une valeur relocalisable
// est entière par construction.
struct Value {
    double real = 0;
    SectionId section = NoSection;
    int coeff = 0;
    Byte byte = Byte::Whole;

    bool absolute() const { return coeff == 0; }
    bool relocatable() const { return coeff != 0; }
};

// Renvoie true et remplit `out` si le symbole est connu, false sinon.
// Le résolveur rend une VALEUR : un réel — une constante ou une variable peut
// en valoir un, et l'arrondir au passage perdrait l'information avant même que
// l'expression soit calculée (ADR 0008) — et, pour un label d'une section
// relocalisable, la section et le coefficient qui vont avec. Un résolveur qui
// ne connaît que des nombres, comme celui du préprocesseur, laisse le
// coefficient nul et n'a rien d'autre à faire.
using Resolver = std::function<bool(const std::string &name, Value &out)>;

// Le résultat : une valeur, plus l'issue de son calcul. `value` est la partie
// CONNUE arrondie — la valeur entière tout court quand elle est absolue, et
// l'addend de la relocalisation quand elle ne l'est pas.
struct Result : Value {
    bool ok = false;
    int64_t value = 0;
    std::string error;
};

// Évalue une expression. Opérateurs (précédence C) :
//   || && | ^ &  == !=  < <= > >=  << >>  + -  * / %  unaires - ~ !
// Nombres : décimal, 0x.. / $.. / #.. (hexa), %.. (binaire), et un littéral de
// chaîne d'UN octet — 'c' ou "c" indifféremment (ADR 0010). Tout littéral d'une
// autre longueur est une erreur : une expression rend un nombre, et une suite
// d'octets n'en est pas un.
//
// Sur une valeur relocalisable, seuls `+`, `-` et `high()` / `low()` sont
// acceptés ; tout le reste est refusé, parce qu'il demanderait une adresse que
// personne ne connaît encore.
Result eval(const std::string &text, const Resolver &resolver);

} // namespace expr
