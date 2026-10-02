// riff.cpp - Un fichier RIFF comme arbre de chunks
#include "riff.h"

#include <algorithm>

namespace riff {
namespace {

void putFourCC(std::vector<uint8_t> &out, const std::string &cc) {
    for (size_t i = 0; i < 4; ++i) out.push_back(i < cc.size() ? (uint8_t)cc[i] : (uint8_t)' ');
}

void putU32LE(std::vector<uint8_t> &out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back((uint8_t)(v >> (8 * i)));
}

void writeChunk(std::vector<uint8_t> &out, const Chunk &c) {
    std::vector<uint8_t> body;
    if (c.container)
        for (const Chunk &child : c.children) writeChunk(body, child);
    else
        body = c.data;
    putFourCC(out, c.id);
    putU32LE(out, (uint32_t)body.size());
    out.insert(out.end(), body.begin(), body.end());
    if (body.size() % 2 != 0) out.push_back(0);
}

uint32_t u32LE(const std::vector<uint8_t> &b, size_t off) {
    return (uint32_t)b[off] | ((uint32_t)b[off + 1] << 8) | ((uint32_t)b[off + 2] << 16) |
           ((uint32_t)b[off + 3] << 24);
}

bool readChunks(const std::vector<uint8_t> &b, size_t pos, size_t end,
                const std::function<bool(const std::string &)> &isContainer,
                std::vector<Chunk> &out, std::string &error) {
    while (pos < end) {
        if (pos + 8 > end) {
            error = "chunk tronque";
            return false;
        }
        Chunk c;
        c.id.assign(b.begin() + (long)pos, b.begin() + (long)pos + 4);
        const uint32_t size = u32LE(b, pos + 4);
        const size_t start = pos + 8;
        if (size > end - start) {
            error = "chunk '" + c.id + "' tronque";
            return false;
        }
        c.container = isContainer(c.id);
        if (c.container) {
            if (!readChunks(b, start, start + size, isContainer, c.children, error)) return false;
        } else {
            c.data.assign(b.begin() + (long)start, b.begin() + (long)(start + size));
        }
        out.push_back(std::move(c));
        // Le padding d'une taille impaire peut manquer au tout dernier octet.
        pos = std::min(end, start + size + (size % 2));
    }
    return true;
}

} // namespace

bool read(const std::vector<uint8_t> &bytes, const std::string &form,
          const std::function<bool(const std::string &)> &isContainer,
          std::vector<Chunk> &out, std::string &error) {
    out.clear();
    error.clear();
    if (bytes.size() < 12 || std::string(bytes.begin(), bytes.begin() + 4) != "RIFF" ||
        std::string(bytes.begin() + 8, bytes.begin() + 12) != form) {
        error = "ce n'est pas un fichier RIFF de forme '" + form + "'";
        return false;
    }
    const uint32_t size = u32LE(bytes, 4);
    if (size < 4 || size > bytes.size() - 8) {
        error = "fichier RIFF tronque";
        return false;
    }
    return readChunks(bytes, 12, 8 + (size_t)size, isContainer, out, error);
}

std::vector<uint8_t> write(const std::string &form, const std::vector<Chunk> &chunks) {
    std::vector<uint8_t> body;
    putFourCC(body, form);
    for (const Chunk &c : chunks) writeChunk(body, c);
    std::vector<uint8_t> out;
    putFourCC(out, "RIFF");
    putU32LE(out, (uint32_t)body.size());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

} // namespace riff
