// riff.h - Un fichier RIFF comme arbre de chunks
//
// RIFF ne dit rien du sens des données, seulement de leur rangement : un
// en-tête `RIFF <taille> <forme>`, puis des chunks `<id> <taille> <données>`,
// complétés d'un octet nul quand la taille est impaire. Certains chunks en
// contiennent d'autres ; lesquels, c'est la forme qui le sait, pas RIFF.
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace riff {

struct Chunk {
    std::string id;                // quatre caractères
    // Un chunk CONTENEUR a des enfants et aucune donnée propre : son corps est
    // la suite de ses enfants.
    bool container = false;
    std::vector<uint8_t> data;
    std::vector<Chunk> children;
};

// Le fichier complet de forme `form` (quatre caractères).
std::vector<uint8_t> write(const std::string &form, const std::vector<Chunk> &chunks);

// Relit un fichier de forme `form`. `isContainer(id)` dit quels chunks en
// contiennent d'autres ; tous les autres sont gardés tels quels, qu'on les
// connaisse ou non. `false` avec `error` si l'en-tête n'est pas `RIFF <form>`
// ou si un chunk déborde de son parent.
bool read(const std::vector<uint8_t> &bytes, const std::string &form,
          const std::function<bool(const std::string &)> &isContainer,
          std::vector<Chunk> &out, std::string &error);

} // namespace riff
