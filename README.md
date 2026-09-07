# fantams

A small Z80 assembler with a **full preprocessor** and a **source formatter**,
targeting Amstrad CPC `.sna` snapshots and raw binaries. Written in portable
C++17, it builds as a native CLI or as a WebAssembly module that runs in a
browser.

```bash
make fantams                          # native CLI
./fantams game.asm -o game.sna        # assemble to a CPC snapshot
./fantams game.asm -E -o game.pp.asm  # see what the preprocessor produced
./fantams game.asm --beautify -o game.asm
```

---

## What it does that most Z80 assemblers don't

**A real preprocessor, and you can read its output.** Macros, `repeat` / `while`
/ `for`, `if` / `elseif`, `struct`, `module`, `include`, per-expansion label
scoping — and `-E`, which writes the **unrolled source**: every macro expanded,
every loop unrolled, every include inserted, every scoped label renamed. It is
plain assembly you can read, diff, and re-assemble. When a macro misbehaves you
look at what it actually generated instead of guessing.

**A formatter with no options to argue about.** `--beautify` puts labels in
column 1, indents block bodies one step, detaches `label: instruction`, and adds
call parentheses to the macros it knows. Nothing is configurable and no
formatting directive exists in the source, so two people's files come out the
same.

**Snapshot output that survives firmware calls.** `--base` lays the assembled
bytes onto a captured post-boot machine state, so `call &BB5A` works instead of
jumping into zeros.

**A machine-readable symbol table.** `--sym` writes a CSV — one line per label
and constant, with type, owning section, logical address, storage bank, and origin
file and line
— for a disassembler or an emulator.

---

## Quick start

```bash
git clone https://github.com/sikorama/fantams
cd fantams
make            # builds the CLI and the test binaries
make test       # 7 suites, 675 assertions
```

A first source:

```asm
        org #8000
        run start

macro poke(addr,val)
        ld a,val
        ld (addr),a
end

start:
        poke(#C000,42)
        ret
```

```bash
./fantams hello.asm -o hello.sna     # snapshot
./fantams hello.asm -o hello.bin     # raw binary
./fantams hello.asm -o hello.bin -s  # ... and print the symbol table
```

The output format is chosen by the extension of `-o`: `.sna` gives a snapshot,
anything else a raw binary.

---

## The preprocessor

Everything below happens before the assembler sees a single opcode, and `-E`
shows you the result.

| Feature | |
|---|---|
| Macros | `macro name(p1,p2)` … `end`, called `name(a,b)` |
| Loops | `repeat n[,var]`, `while cond`, `for var = low to high` |
| Conditionals | `if` / `ifdef` / `ifndef`, `elseif`, `else` |
| Structures | `struct name` … `end`, instances, `sizeof(name)` |
| Namespaces | `module gfx` prefixes every label with `gfx.` |
| Inclusion | `include "file"` |
| PP variables | `LET n = 4`, substituted with `{n}`, computed with `{=n*n}` |
| Scoped labels | `@loop` is renamed once per expansion; `@@export` opts out |

### Loops

```asm
repeat 4,idx
        db idx          ; -> db 0 / db 1 / db 2 / db 3
end

for k = 1 to 4
        db {=k*k}       ; -> db 1 / db 4 / db 9 / db 16
end
```

`for` takes `to` for an inclusive bound and `until` for an exclusive one. The
bounds are written down, so there is nothing to guess.

### Macro arguments: value or text

A bare argument is the **value**, captured at the call site. An argument in
**braces** is the **text**, resolved where it is emitted. The difference is
visible:

```asm
n = 5
macro m(x)
    repeat 3
        db x            ; value -> 5, 5, 5
        db {x}          ; text  -> 5, 4, 3
        n = n - 1
    end
end
        m(n)
```

Arguments are evaluated, never pasted: `m(1+1)` in a body doing `db x*2` gives
4, not 3.

### Per-expansion labels

A label prefixed with `@` is made unique per expansion, so a macro can loop
without colliding with itself. A label without the prefix is left alone — reused
across two expansions it is a genuine duplicate symbol, and you get told.

```asm
macro wait(n)
@loop:  dec n
        jr nz,@loop
end
```

---

## Reading the preprocessor's output

```bash
./fantams src.asm -E -o src.pp.asm
```

`-E` is a first-class output, not a debug artifact: it comes out formatted, one
instruction per line, with the extended notations already canonicalized. This
source:

```asm
LET COUNT = 4
        org #8000
        run start

macro wait(n)
@loop:  dec n
        jr nz,@loop
end

start:
        ld b,{COUNT}
        wait(b)
        wait(b)

tbl:
for k = 1 to COUNT
        db {=k*k}
end
```

comes out as:

```asm
    org #8000
    run start
start:
    ld b,4
@loop__1:
    dec b
    jr nz,@loop__1
@loop__2:
    dec b
    jr nz,@loop__2
tbl:
    db 1
    db 4
    db 9
    db 16
```

---

## The formatter

```bash
./fantams src.asm --beautify -o src.asm
```

Four rules, and no way to add a fifth from the source:

1. a label alone on its line gets its colon;
2. labels sit in column 1, everything else is indented;
3. block bodies get one extra step of indentation;
4. `label: instruction` is split in two, so all opcodes align.

Before:

```asm
wait MACRO n
  dec n
  jr nz,wait
MEND
start
     ld b,4
     wait b
  repeat 3
  nop
  rend
```

After:

```asm
    MACRO wait n
        dec n
        jr nz,wait
    MEND
start:
    ld b,4
    wait(b)
    repeat 3
        nop
    rend
```

`--no-detach-labels` and `--no-indent-blocks` opt out of the last two. The
formatter never canonicalizes — `--normalize` does that, separately, and
deliberately changes the line count.

A construct it cannot read without guessing is left alone. `sprite 4,12` might
be a macro call or a label followed by a directive; if the macro is unknown, the
line comes back untouched.

---

## Snapshots and the base state

An assembler's snapshot contains the assembled bytes and nothing else. A
`call &BB5A` needs the firmware jumpblock above `&B000`, the system variables,
the RST vectors, and lower ROM enabled — all of it installed by the ROM at boot,
none of it produced by assembling.

```bash
./fantams src.asm -o out.sna --base bases/cpc6128-en.sna
```

Every address the source actually wrote wins; every other one keeps the base's
byte. The base's header is authoritative (`SP`, gate array, `I`, `IM`, CRTC,
palette) and only `PC` is patched.

To capture a base: boot the emulator, wait for `Ready`, save a **version 2**
snapshot (256-byte header plus a flat 64 K dump). The **ROM** has to match, not
just the model.

The exported `.sna` carries 64 KB if the source stays within banks 0–3, and
128 KB as soon as it writes to banks 4–7.

---

## Differences from other assemblers

* **`repeat` index starts at 0**: indexing begins at 0 (rather than 1), matching standard index arithmetic (`db idx*8`).
* **Counted loops**: provides `for k = 1 to n` (and `until` for exclusive bounds) as a native construct.
* **Parenthesized macro calls**: supports parenthesized syntax `sprite(4, 12)` alongside bare calls `sprite 4, 12`. This makes macro calls readable without prior knowledge of the symbol list (e.g., macros coming from an `include`). Bare calls trigger a single warning per macro, and `--beautify` automatically adds parentheses.
* **Universal block closure**: the `end` keyword closes any innermost block (`if`, `macro`, `repeat`), alongside standard closing keywords (`endif`, `endm`, etc.). A mismatched closing tag produces an explicit error.
* **Trigonometry in radians**: `sin` and `cos` operate on radians instead of degrees. Degree inputs require explicit conversion (`sin(a * 3.14159265 / 180)`).
* **Floating-point division**: the `/` operator performs floating-point division, while `div` is used for integer division.
* **Preprocessor output (`-E`)**: a `-E` flag is available to output the fully unrolled source code for debugging.

---

## Command line

```
fantams file.asm [-o out] [-s] [-E] [--beautify] [--normalize] [--strict]
                 [--no-detach-labels] [--no-indent-blocks]
                 [--base base.sna] [--sym[=out.sym]]
```

| Option | Effect |
|---|---|
| `-o file` | output; `.sna` selects the snapshot backend, anything else a raw binary |
| `-s` | print the symbol table |
| `-E` | write the unrolled source instead of assembling |
| `--beautify` | format only — no preprocessing, no assembling |
| `--normalize` | canonicalize without unrolling |
| `--strict` | refuse anything that is not canonical Z80 |
| `--no-detach-labels` | keep `label: instruction` on one line |
| `--no-indent-blocks` | do not indent block bodies |
| `--base f.sna` | lay the assembled bytes onto a captured machine state (`.sna` output only) |
| `--sym[=file]` | write the symbol table as CSV; the default path derives from `-o` |

`ppdump` is the preprocessor alone, equivalent to `-E`.

---

## WebAssembly build

Goes through the `emscripten/emsdk` image under podman or docker, so no local
`emcc` is needed:

```bash
./build-wasm.sh                             # -> dist/fantams.mjs + fantams.wasm
FANTAMS_OUT_DIR=../myapp/wasm ./build-wasm.sh
```

The result is an ES6 module (`export default createFantams`) exposing `callMain`
and `FS`, with no auto-run — usable from Node and from a browser.
`FANTAMS_OUT_DIR` and `FANTAMS_PUB_DIR` say where to drop the two files; relative
paths resolve from the calling directory. Nothing is recompiled unless a source
is newer than the `.wasm`, and `--force` overrides that.

Two flags matter: `-fexceptions`, without which every `throw` becomes `abort()`,
and `-sSTACK_SIZE=8388608`, because the parser recurses.

---

## Documentation

- [`docs/syntax.md`](docs/syntax.md) — the full syntax reference
- [`docs/principes.md`](docs/principes.md) — design principles
- [`docs/adr/`](docs/adr) — architecture decision records
- [`CONTEXT.md`](CONTEXT.md) — the codebase, module by module
