# The `.fo` object format

A `.fo` file is the object produced by assembling one source unit. `fantams a.asm -o a.fo`
writes it without linking; `fantams a.fo b.fo -o prog.bin` reads several back and links them.

The format is **plain text**, on purpose: a wrong object can be read by eye, tests compare
strings, and a human can correct a file by hand. It is deterministic: the same object always
produces the same text, so a write–read–write round trip yields identical output.

The reference implementation is `fo.h` / `fo.cpp`. This document describes what that code
writes and accepts; if the two ever disagree, the code is the authority and this page is the bug.

## File layout

```
fantams-object 2
; comments run from ';' to the end of the line
section ...        ; zero or more
site ...           ; zero or more, indexed in order
fragment ...       ; zero or more, each followed by data/gap lines and 'end'
  data ...
  gap ...
end
reloc ...          ; zero or more
access ...         ; zero or more
symbol ...         ; zero or more
entry ...          ; at most one
```

The writer always emits blocks in this order. The reader accepts the same order; `site`
and `fragment` lines must appear with indices `0, 1, 2, …` in sequence, and nothing else
is order-sensitive. Blank lines are ignored.

The first line is mandatory and must be exactly:

```
fantams-object <version>
```

The current version is **2**. A reader rejects any other version with an error. The version was
bumped when `place` and `window` were added to sections, because an older reader would silently
link such sections to wrong addresses.

## Lexical rules

- A line is a sequence of tokens separated by whitespace.
- A `;` outside quotes starts a comment that runs to the end of the line.
- A token may be a **quoted string**: `"..."`. Inside quotes, `\"` and `\\` are the only escapes.
  Quotes are required for any name or path that may contain a space.
- A token of the form `key=value` is a **pair**. The value may be quoted, as in `file="my dir/a.asm"`.
- A bare word without `=` is a **flag** (for example `reloc`, `placed`, `public`).
- **Numbers** are integers, decimal or with a `0x` prefix. The writer uses uppercase hex with a
  `0x` prefix, and a negative value as `-0x…`. The reader parses both forms with C `strtoll`
  base 0.
- **Hex bytes** in `data` lines are exactly two hex digits each.
- A missing optional key takes the default listed below. A key that is present but
  not a number is an error.

## Errors

When reading fails, the reader returns an error of the form:

```
line N: message
```

where `N` is the 1-based line number. The reader stops at the first error.

## Blocks

### Header

```
fantams-object 2
```

### `section`

```
section "<name>" id=<n> type=<kind|-> reloc|abs size=<n> [max=<n>] [place="<config>"] [window="<window>"] [at="<file>" line=<n>]
```

Declares a section as the source wrote it.

| Field | Meaning |
|---|---|
| `"<name>"` | Section name (required). |
| `id=` | Stable identity. Relocations refer to sections by this number. Default `-1`. |
| `type=` | Section type as the source gave it (`section name, "ro"` gives `RO`). `-` when there is none. |
| `reloc` / `abs` | A **relocatable** section has no `org` of its own: the linker places it. An `abs` section sits where its `org` says. The reader sets `relocatable` only on the flag `reloc`. |
| `size=` | Number of bytes the section occupies. Default `0`. |
| `max=` | Optional upper bound on the size, when the source declared one. |
| `place=` | The configuration the source asked for, as an **opaque string**. The assembler does not interpret it; the linker resolves it against the profile. |
| `window=` | The named window inside `place`, if the source gave one. Also an opaque string. |
| `at=` / `line=` | File and line of the first declaration. Used to name the source of a type or size conflict between two objects. Optional. |

`place` and `window` come from `IN <config>` and `IN <window> OF <config>`. The first form sets
only `place`; the second sets both.

### `site`

```
site <index> "<file>" <line>
```

A source location that wrote bytes. Fragment bytes refer to sites by index (see `data`). Sites
are numbered from `0`, in order.

### `fragment` … `end`

```
fragment <index> section="<name>" placed|unplaced reloc=<n> addr=<n> logical=<n> bank=<n>
  data <site|-> <XX> [<XX> …]
  gap <count>
end
```

A **fragment** is a contiguous block of bytes belonging to one section. It is the unit the
linker places. Fragments are numbered from `0`, in the order they were written, and placement
replays them in that order.

| Field | Meaning |
|---|---|
| `section=""` | Section the fragment belongs to. Empty when the bytes sit outside any section. |
| `placed` / `unplaced` | `placed` means a source `org` gave the fragment its address. |
| `reloc=` | If not `-1`, the **id** of the section whose base this fragment waits for. In that case `addr` and `logical` are **offsets inside that section**, not absolute addresses. `-1` when it waits for nothing. |
| `addr=` | Storage address of byte 0. Default `0`. |
| `logical=` | Logical address of byte 0. Equal to `addr` unless the fragment is in a moved block. Default `0`. |
| `bank=` | Bank forced by an `org b<n>:` prefix, or `-1`. Default `-1`. |

The body contains bytes in **runs**, each with its provenance:

- `data <site> <XX> …` gives bytes written by source site `<site>`. Each line carries at most
  16 bytes. `-` in place of the site index means the provenance is unknown.
- `gap <count>` gives `count` bytes that were reserved but not written (for example by `ds`).
  They have no site and no value; the reader fills them with zero.

Coverage is therefore "everything that is not a gap". Bytes are listed in order, and the
concatenation of all runs is the fragment's content.

### `reloc`

```
reloc frag=<n> offset=<n> abs16|rel8|high8|low8|bankof (symbol="<name>" | section=<n>) addend=<n>
```

A value the assembler could not know, to be written by the linker at the fragment byte `offset`.

- `abs16` — a 16-bit absolute address.
- `rel8` — an 8-bit relative displacement (`jr`, `djnz`).
- `high8` / `low8` — the high or low byte of an address (`high()`, `low()`).
- `bankof` — the bank of an address.

The target is **either** a symbol declared `EXTERN` (`symbol="…"`), **or** the base of a section of
this object (`section=<id>`). Never both. The final address is the target's base plus `addend`.

### `access`

```
access frag=<n> offset=<n> write (symbol="<name>" | section=<n>) addend=<n>
```

Records a write to a literal address (for example `ld (nn),a`), so that a later check can flag an
instruction whose write could be affected by bank switching. The entry is advisory: it is not
interpreted here. Only the `write` kind exists in this version; the reader rejects anything else.
The target is written the same way as a relocation target.

### `symbol`

```
symbol "<name>" const|label public|local section="<name>" value=<n> frag=<n> offset=<n> file="<file>" line=<n>
```

A symbol in the object's table.

- `label` or `const` — required. `const` is an `EQU` value and normally has `frag=-1`.
- `public` exports the symbol to other objects. Without it the symbol is `local`, so two files may
  use the same internal label name. Default: local.
- `section=""` names the section that holds the symbol, or is empty for a constant.
- `value=` is the logical address, or the value of a constant.
- `frag=` and `offset=` give where the symbol sits in its fragment. `frag=-1` for constants.
- `file=` and `line=` give the declaration site.

The symbol table is also the subset of `--sym` output that other objects may need. Variables are
not part of it.

### `entry`

```
entry symbol="<name>" value=<n> file="<file>" line=<n>
```

The entry point requested with `run`. Present only when one was requested. It names a symbol;
the linker resolves the name, so `value=` is the value the source gave at the time of writing.

## Example

This is the real output of `fantams main.asm -o main.fo` for the following source:

```asm
    extern start
    section code, "ro"
    ld a, 0
    jp start
```

The object, with the writer's comment lines left out:

```
fantams-object 2
section "code" id=0 type=RO reloc size=0x5 at="main.asm" line=2
site 0 "main.asm" 3
site 1 "main.asm" 4
fragment 0 section="code" unplaced reloc=0 addr=0x0 logical=0x0 bank=-1
  data 0 3E 00
  data 1 C3 00 00
end
reloc frag=0 offset=0x3 abs16 symbol="start" addend=0x0
```

Notes on the example:

- `3E 00` is written by site 0 (`ld a, 0`), `C3 00 00` by site 1 (`jp start`). The two `00 00`
  bytes are placeholders; the `reloc` line says the linker must overwrite them at offset `0x3`.
- The section is relocatable (`reloc`), so the fragment waits for the base of section `0`:
  that is why it has `reloc=0`, and why its `addr` and `logical` are offsets, not addresses.
- `start` is `extern`, so the object has a `reloc` naming it and no `symbol` entry for it.
- The assembler wrote `0 symbols`: the object holds no `symbol` lines.
