# The fantams syntax

This document describes what fantams accepts **today**, verified against the
binary and not against intentions. Standard Z80 mnemonics are not included here:
they are the same as everyone else's. What follows covers directives, expressions,
macros, and extended notations.

Here, only the facts: what to write, what it does, and where fantams is
unusual.

---

## 1. The line

```
label:  instruction operands   ; comment
```

- The **colon** of a label is optional, but its absence is flagged.
  A label starts in column 1; the rest is indented (`--beautify` establishes
  this form).
- **Comments** run from `;` or `//` to the end of the line, and `/* … */`
  can span multiple lines.
- The **colon also separates instructions**: `ld a,1:inc a` contains
  two. This is a writing convenience, canonicalized to two lines by the
  preprocessor, and refused by `--strict`.
- A reserved word before that colon is **never** a label: `nop:nop:nop`
  assembles to three `nop`, because a mnemonic cannot name a label.
  It is flagged all the same — `nop:` *reads* like a label — so write
  `nop : nop : nop` to say plainly what is meant.
- A `label:` attached to its instruction is detached by `--beautify`
  (`--no-detach-labels` disables it).

---

## 2. Numbers

| Notation | Base | Example |
|---|---|---|
| `123` | 10 | |
| `#FF` `$FF` `0xFF` | 16 | |
| `%1010` | 2 | |
| `3.14` | 10, float | the dot only makes sense in base 10 |

All values are **reals**; bitwise operators convert to integer. `$` alone
is the current address — `$FF` remains a number, the distinction is made by what
follows.

---

## 3. Strings

`'text'` and `"text"` are **two notations for the same object**: delimiters
are interchangeable, and `'A'` is in no way different from `"A"`.

- A string of **one byte** has a value in an expression: the code of its
  character. Any other length has none.
- **Offset string**: `db 'hello'-'a'` applies the arithmetic tail to each
  byte and emits five. This is an **emission** construct, valid where a
  sequence of bytes is expected, never where a value is.
- A `;` in a string does not open a comment.

---

## 4. Expressions

### Operators, from least to most binding

| Level | Operators |
|---|---|
| logical or | `\|\|` |
| logical and | `&&` |
| bitwise or | `\|` `or` |
| bitwise xor | `^` `xor` |
| bitwise and | `&` `and` |
| equality | `==` `!=` |
| comparison | `<` `<=` `>` `>=` |
| shift | `<<` `shl` · `>>` `shr` |
| addition | `+` `-` |
| multiplication | `*` · `/` · `%` `mod` · `div` |
| unary | `-` `+` `~` `not` `!` |
| power | `**` — right-associative, more binding than unary |

- **`/` is floating-point division**, `div` is integer division. `7/2` equals 3.5,
  `7 div 2` equals 3.
- Text aliases designate **bitwise** forms. There is deliberately
  no text alias for `&&`, `||`, and `!`.
- `//` is **not** an operator: it is a comment.

### Functions

| Function | Arguments | Note |
|---|---|---|
| `sin` `cos` | 1 | angles in **radians**, never degrees |
| `abs` | 1 | |
| `high` `low` (`hi` `lo`) | 1 | high / low byte of the integer value |
| `floor` `ceil` `int` `round` | 1 | toward −∞ / +∞ / zero / nearest |
| `min` `max` | 2 | |
| `sizeof` | 1 | size of a `struct` |

Halves round **up** (`3.5 → 4`, `-3.5 → -3`).

`sin` and `cos` take **radians**, which is what a maths library takes. Write
`sin(a*3.14159265/180)` for a degree argument.

`high` and `low` are the explicit way to take one byte of a value; `hi` and `lo`
are spellings of the same two functions. They are also the **only** functions
that accept a relocatable address — see below.

### Relocatable values

A section that carries **no `org` of its own** is placed by the linker. Its
labels have no address while assembling: they are a section base plus an offset.
Arithmetic on such a value is limited to what stays meaningful once the linker
has chosen where the section goes.

| Written | Result |
|---|---|
| `label`, `label + 4`, `label - 4` | relocatable |
| `end - start`, **same** section | a plain number — this is how a table is measured |
| `end - start`, **different** sections | **refused**: the distance is the linker's to decide |
| `label + label`, `label * 2`, `label / 2` | **refused**: no meaning |
| `label >> 8` | **refused**, naming `high()` |
| `label & 255` | **refused**, naming `low()` |
| `high(label)`, `low(label)` | the byte, resolved at link time |
| `label + 0.5` | **refused**: an address is an integer |
| `db label` | **refused**: an address does not fit in one byte — use `high()` / `low()` |

Comparisons, boolean operators and the real functions (`sin`, `abs`, `min`, …)
are refused on a relocatable value for the same reason. So is an index
displacement, a bit number, an `rst` vector and an `im` mode: none of them is
ever an address.

`$` behaves as a label of the current section, so the idioms built on it cross
relocation unchanged.

The `>> 8` and `& 255` forms stay legal on a plain number. They are **not**
recognised as patterns on a relocatable one: `label >> 9` looks like the pattern
without being it, and someone whose `>> 9` failed while `>> 8` worked could not
guess what separated them.

**Relative jumps stay doubly checked.** A `jr` or `djnz` whose target is in the
same section is measured while assembling, and refused there if it leaves
[-128, 127]. One that crosses sections is measured by the linker, and refused
there. No guard byte is ever emitted without its relocation, so an out-of-range
jump is never silent.

### Separate assembly

| Command | Effect |
|---|---|
| `fantams a.asm -o a.fo` | assembles **only**, and writes the object |
| `fantams a.fo b.fo -o prog.bin` | links objects into a binary |

The output kind is read from the extension, as `.sna` already was: `-o x.fo`
stops after assembling. An input ending in `.fo` is an object already assembled,
so it is read back rather than reassembled.

A `.fo` is **text**, on purpose: a wrong object is read by eye, which is worth
more than anything at the stage that introduces relocation. It carries the
sections and their fragments, the symbols, the relocations, and the bytes in
hexadecimal — one object line per source line that wrote bytes, so provenance
travels without a number per byte. Writing it, reading it back and writing it
again gives the **same text**. A malformed one is refused with a diagnostic that
names the faulty line.

The `--sym` table is a **subset** of the `symbol` block: same names, plus the
scope and the fragment each one is anchored in.

### Scope between objects

| Directive | Effect |
|---|---|
| `public name[, name…]` | exports the name, so another object can refer to it |
| `extern name[, name…]` | declares the name defined in another object |

A symbol is **local to its object by default**, so two files can use the same
internal label name without colliding. `include` is a *preprocessor* thing, so a
multi-file source of today still assembles as **one** object and sees no
difference.

A name that is **neither defined nor declared `extern`** stays an assembly
error, at its own line. An implicit `extern` would turn a typo into an
unresolved relocation reported two links further along, when the assembler can
say it where you wrote it.

Refused, each at its line: exporting a name nothing defines, exporting a name
declared `extern`, and declaring `extern` a name this object also defines — in
either order, since it is the same mistake both ways.

`public`, `extern`, `high` and `low` are **reserved at every phase** (ADR 0015):
they cannot name a label or a symbol, and formatting knows them — a `public` at
the start of a line is a directive, never a label to be given a colon.

**An `org` above a section does not place it.** It applies to the bytes outside
any section; the section itself still has no `org` of its own, so the linker
places it — and says so, once per section. To place a section yourself, write
the `org` **inside** it.

### What doesn't exist

There is **no ternary `? :`**, and there won't be: the `:` is already the
instruction separator as much as it is a label suffix, so
`1 ? 2 : 3` is split in two before reaching the evaluator. An `if` says the same
thing more clearly.

---

## 5. Names and values

| Form | Object | Resolved |
|---|---|---|
| `name:` or `name` at line start | **label** — an address | assembly time |
| `name EQU value` | **constant**, non-reassignable | assembly |
| `name = value` | **variable**, reassignable, sequential | assembly |
| `LET name = value` | **preprocessor variable**, resolution **required** at PP time | preprocessor |

A definition (`EQU`, `=`) never receives a colon: that is its canonical form.

### Local labels

A label starting with `.` belongs to the last global label encountered:

```
plot:
.x      ld a,0
        ld (plot.x+1),a     ; reference from outside
```

A **definition** interleaved (`delta equ 4`) does not change the owner.

A label coming out of a **macro expansion** is a global label like any other, so it
becomes the owner of the `.locals` that follow it: after `poke(…)` whose body
defines `@retry`, a `.local` is qualified as `@retry__2.local`. The symbol table
(`--sym`) is where this becomes visible.

### Reserved words

Registers, pairs, and conditions (`a`, `hl`, `i`, `p`, `nz`, `pc`…) cannot
name a label, a macro parameter, or a loop index. The refusal names what
the word is — hence the failure of `for i = …` or a
parameter named `p`.

---

## 6. Emission directives

| Directive | Alias | Effect |
|---|---|---|
| `db` | `defb` `dm` `defm` | bytes, strings, offset strings |
| `dw` | `defw` | 16-bit words (little-endian) |
| `ds` | `defs` `rmb` | reserve `n` bytes with value `v` |

`ds` accepts **multiple pairs** on one line: `ds 3,1,3,2` reserves three
bytes with value 1 then three with value 2.

A single `ds` cannot exceed **`#10000`** bytes — the whole address space. Past
that the bytes would come back over themselves, so it is refused rather than
wrapped in silence.

## 7. Placement

| Directive | Effect |
|---|---|
| `org [b<n>:]address` | sets the assembly address and storage bank |
| `org logical,[b<n>:]storage` | assembles for one address, stores at another |
| `align n` | aligns to a multiple of `n` |
| `run address` | entry point |
| `boundary n` … `end_boundary` | a block that must not straddle an `n`-byte boundary |
| `section name, "type"[, max]` | opens a logical unit of assembly (`"ro"`, `"rw"`, `"uninit"`) |
| `assert_size n` … `end_assert_size` | a block that must not exceed `n` bytes |

### Boundary blocks

A `boundary` block is an **allocation contract**, not an alignment: this structure
must not straddle a boundary of `n` bytes — 256, typically, so that `H` does not
change while walking the table.

```asm
        boundary 256
my_table:
        dw label_1
        dw label_2
        db #FF
        end_boundary
```

The assembler **measures the block itself** — here 5 bytes — looks at the current
address, and applies one rule:

> Emit in place if the block fits entirely within the current `n`-byte page;
> otherwise skip to the start of the next one.

At `&2FFE` the 5 bytes do not fit in the two remaining, so the block goes to
`&3000`. At `&2F00` it is emitted in place, with no skip at all. Nothing to
compute by hand, and no magic number to pass.

- The skip **emits nothing**: like `align`, it advances the address, and the
  skipped bytes stay outside the coverage.
- A block **larger** than its boundary can never satisfy the rule, and is an
  assembly error naming the block and both sizes.
- The first label inside the block **names** it, for that diagnostic.
- A missing `end_boundary`, an `end_boundary` with no block open, and a block
  **nested** in another are all refused. Nesting is refused rather than
  mis-measured: the outer block's measurement would stop at the inner
  `end_boundary`.

### Sections

A **section** is a logical unit of assembly — the unit a linker will one day place
as a whole. Declaring one is what lets the assembler say *where a symbol lives*,
and refuse a write into read-only memory.

```asm
        section tables_data, "ro"
mon_tableau:
        db 1, 2, 3, 4

        section execution, "ro"
        ld a, 5
        ld (mon_tableau), a     ; refused: writes into a "ro" section
```

Three types, and they are the only hardware semantics the assembler knows:

| Type | Content | Emits bytes |
|---|---|---|
| `"ro"` | executable code and constants | yes |
| `"rw"` | initialized, modifiable data | yes |
| `"uninit"` | reserved space, not initialized | no |

The type is **mandatory**, and a fourth one is refused, naming the three.

- A section **reopens** — that is how code and data alternate — but it keeps the
  type of its **first** declaration. Changing it is refused: `"ro"` then `"rw"`
  under one name would silently disarm the check below.
- Placement stays **absolute**: `org` inside a section still decides addresses.
  A section names and classifies; it does not yet relocate.
- The section that owns each symbol appears in the symbol table (`--sym`). A
  constant carries none: it lives nowhere.

### A declared maximum size

The third argument caps a section, and the overflow is reported **at assembly
time**, without waiting for a linker:

```asm
        section audio, "ro", 0x2000
```

```
Section 'audio' exceeds maximum declared size (0x2140 > 0x2000 bytes)
```

- The size is the **sum of the bytes emitted**, cumulated over every reopening —
  not the `max − min` span of the addresses. What a section costs is the room it
  asks for, the room a linker will place as one block.
- `align` and `boundary` do **not** count: they move the address without emitting,
  and at this stage padding does not exist.
- The maximum is **frozen at the first declaration**, like the type. A reopening
  may leave it out — that is the normal form — but not raise it, lower it, or
  introduce one that the first declaration did not carry.
- It must be resolvable in **pass 1**, since it decides a refusal while the bytes
  are being counted. A forward `equ` is refused.
- A size exactly equal to the maximum is accepted: a cap is a permitted size, not
  the first refused one.

### `"uninit"`: reserved, not written

An `"uninit"` section is a **reserved location**, and the table above is enforced:
it emits no bytes. `ds` is its whole vocabulary — reserving is exactly what it is
for:

```asm
        section vars, "uninit"
        org #C000
buffer: ds 16
flag:   ds 1

        section code, "ro"
        org #8000
        ld a, (flag)            ; the address is known: 0xC010
```

- `ds` **advances the address without writing**: the labels are placed, the
  symbol table carries them with their section, and nothing enters the binary.
  The image above is four bytes at `0x8000`, not sixteen kilobytes.
- `db`, `dw`, a string and an instruction are **refused** there, naming the type
  of the section. A reserved area has nowhere to put bytes — a linker would have
  no file to write them to.
- `ds` takes **no fill value** there: `ds 16,#FF` would suggest an initialized
  area, so it is refused rather than ignored.
- Reserved space **counts** towards the declared maximum. It is the only thing an
  `"uninit"` section tells a linker.

### `assert_size`: a cap on a sub-area

A section's maximum covers the unit a linker will place. `assert_size` covers a
**sub-area inside it** — a jump table, a descriptor, whatever its author delimits:

```asm
        assert_size 8
jump_table:
        dw draw, move, hide, kill
        end_assert_size
```

```
Block 'jump_table' exceeds its asserted size (0xA > 0x8 bytes)
```

- The area is **explicit**, on the `boundary` model. Measuring "from the last
  label" would read just as well, but a label inserted in the middle would change
  what is measured without anyone asking for it.
- The **first label** of the block names it in the diagnostic; without one, the
  block is designated by its address. The error is reported on the `assert_size`
  line — the one that carries the number to fix.
- Blocks **nest**, unlike `boundary`: the area is assembled normally and measured
  by difference of addresses, so there is no prior measurement that an inner block
  could cut short.
- It measures, it does not move: nothing is aligned, nothing is padded, and the
  bytes are the same with or without it.
- A missing `end_assert_size`, and an `end_assert_size` with no block open, are
  both refused.

### Writes into `"ro"`, refused statically

Knowing the type of the section that owns each symbol, the assembler refuses a
write whose destination is a **literal address**:

```
"ld (nn), a" writes into read-only section 'tables_data'
```

The check applies to `ld (nn),a` and `ld (nn),hl/bc/de/sp/ix/iy`, and follows an
expression: `ld (mon_tableau+1),hl` is caught too. Reads are untouched — a `"ro"`
section exists to be read — and so is a hard-coded address, which owns no symbol.

**The limit is written rather than discovered**: a `ld (hl),a` whose `HL` is
computed does not appear here and will **never** be caught. This refusal covers
addresses written in the clear, and nothing else.

### Displaced blocks

`org #A600,#100` assembles for `#A600` — labels take that value — and **stores** the
bytes at `#100`. It is code meant to be **copied** to its logical address before it
runs; a loader elsewhere does the copying.

- `run` takes the **logical** address, because `run label` must equal `label`. The
  `PC` therefore lands on memory nothing has loaded yet: this **warns**.
- `align` aligns the **logical** address, the one the code will run at. The storage
  address shifts by the same amount and is not aligned.
- The displacement is **not persistent**: a bare `org` resets it.
- `loadAddress` and the binary's extent describe the **storage** address, so a raw
  binary no longer loads at the address of its labels.
- The bank prefix qualifies the **storage** address, so it goes on the **last**
  parameter: `org #4000,b4:#100`. On the first parameter of a two-parameter form it
  is **refused**, naming the replacement.

### Banks

`org b4:#4000` stores bytes in **bank 4**; `#4000` remains the **logical
address**, the one labels take. The offset within the bank equals
`address & 0x3FFF`.

Nothing is deduced: a bank has no natural slot, the gate array RAM
configurations paging any extra bank into slot 1.

- Banks **0 to 3** form the base 64 K; without a prefix, the bank follows
  the address (`#8000` is in bank 2).
- The bank is **persistent** from one `org` to the next. A bare `org` that inherits a
  bank outside the base 64 K **warns** — a forgotten prefix would move the block
  without any diagnosis.
- Masking has an assumed cost: `b4:#0000`, `b4:#4000`, `b4:#8000`, and `b4:#C000`
  store in the **same place**. A typo in the slot shows up as
  an overlap, which the assembler reports.

The `.sna` exported carries **64 KB** if the source stays within banks 0-3, and
**128 KB** as soon as it writes to banks 4-7 — a flat dump in both cases, read by
anything that reads an ordinary `.sna`. Beyond **bank 7**, assembly
works but export refuses, naming the banks: it would need the `MEM` chunks
of v3.

A **raw binary** (`-o x.bin`) cannot carry banks — it is a
contiguous interval of logical addresses. A banked source exported this way receives
a warning.

`R:` is reserved for cartridge ROMs, without being implemented.

## 8. Diagnostics in the source

| Directive | Effect |
|---|---|
| `assert condition[,"message"]` | fails in pass 2 if the condition is false |
| `print value[,…]` | displays at assembly |

`assert` expects a comparison operator, so `==` and not `=` (`=` is an
assignment). `print` **swallows expression errors** and displays `0` — a known
flaw, not a rule.

---

## 9. Macros

### Definition

```
macro name p1,p2      |   macro name(p1,p2)      |   name MACRO p1,p2
    …                 |       …                 |       …
endmacro              |   endmacro              |   endmacro
```

Closures: `endmacro` (canonical), `endm`, `mend`, or `end`.

The third notation, `name MACRO p,q`, is **inherited and warns** once per macro;
`--beautify` rewrites it to `macro name p,q`.

### Call

```
name arg1,arg2        bare form, inherited — warns once per macro
name(arg1,arg2)       parenthesized form
name()                without argument
```

The parenthesis is **attached** to the name and the closing one is the **last** character.
This is the only form that reads without knowing the macros — hence the only one worth
using for a macro from an `include`, or not yet written. A first argument in parentheses
is written `name((4),12)`.

`--beautify` adds parentheses to macros it knows.

### Arguments

The **bare** form is the **value**, captured at the call site. **Braces**
are the **text**, which the assembler resolves at the emission point. This is a call
by value against a call by name, and the difference is observable:

```
n = 5
macro m x
    repeat 3
        db x        ; bare      -> 5, 5, 5
        db {x}      ; braces    -> 5, 4, 3
        n = n - 1
    endrepeat
endmacro
    m(n)
```

The argument is **evaluated**, never substituted textually: `m(1+1)` in a body
doing `db x*2` gives 4, and not `1+1*2` — which is 3.

**When the value doesn't exist**, the bare form falls back to text:

| The argument is… | Fallback | Warning |
|---|---|---|
| a register, `(ix+2)`, a string of more than one byte | text | **no** — `push reg` is an idiom |
| an expression depending on a label (`buffer+2`) | text | **yes** — the fallback moves the moment of resolution |

The warning is anchored on the line of the **body**, where the correction is written, and
names the **call site**, where the fact comes from. It only appears once per
(body line, call site) pair: a call in a loop does not warn each time.

A string of **one byte** has a value: `m('A')` passes 65.

Braces are never a format prefix.

### Scope

A label prefixed with **`@`** is made **unique to each expansion**. A label
without the prefix is **not renamed**: reused across two
expansions, it stays a real collision, and the assembler says so
(`duplicate symbol`).

```
macro poke addr,val
    ld a,val
@retry:                 ; -> @retry__1, @retry__2, … one per expansion
    ld (addr),a
    jr nz,@retry
endmacro
```

The rule is the same for the iterations of `repeat` and `while`. `module`, on the
other hand, renames **every** label of its body by prefixing it (`M.plain`).

`@@export name` takes a label out of the renaming, so that every expansion shares
one name. It therefore only concerns `@`-prefixed labels — on a plain label it has
nothing to exempt:

```
macro m
@@export @glob
@glob:  nop            ; -> @glob in every expansion, hence a shared name
endmacro
```

---

## 10. Blocks

| Opener | Closures | Note |
|---|---|---|
| `if` `ifdef` `ifndef` | `endif` | `else`, `elseif` |
| `repeat n[,var]` | `endrepeat` `rend` | the index starts at **0** |
| `while cond` | `endwhile` `wend` | |
| `for var = low to high` | `endfor` | `until` for an exclusive bound |
| `macro` | `endmacro` `endm` `mend` | |
| `struct name` | `endstruct` `ends` | `sizeof(name)` |

**`end` closes any block**, the innermost one. A named closure that does not
match is an error stating so. Any block `X` closes with `endX` or
with `end`, without exception — `endr` does not exist.

The `repeat` index starts at 0, which is what the index arithmetic written next
to it expects (`db idx*8`).

A block can open and close on one line: `repeat 3 : dw a,b : rend`.

### Modules

`MODULE name` **switches** the active module — it does not nest. `MODULE`,
`MODULE OFF`, and `ENDMODULE` disable it. Labels in it are prefixed:
`gfx.plot`.

---

## 11. Extended notations

### One-to-many — canonicalized by the preprocessor

The unrolled source shows one instruction per line, so these are expanded
before the assembler sees them.

| Notation | Equals |
|---|---|
| `push hl,de` · `pop af,bc` | one `push`/`pop` per register |
| `ld a,1:inc a` | two lines |
| `ld de,hl` | `ld d,h` · `ld e,l` |
| `ld hl,(ix+2)` | `ld h,(ix+3)` · `ld l,(ix+2)` |
| `ld (iy-1),de` | `ld (iy+0),d` · `ld (iy-1),e` |

`ld rr,rr'` works across **BC, DE, HL, IX, IY** in both directions, the index
halves being the undocumented `hx`/`lx`/`hy`/`ly`. `ld hl,ix` and `ld ix,iy`
do **not** exist — the DD prefix makes `h` the half of IX, so no instruction
names H and IXH at once. Neither does `ld hl,sp`.

`ld rr,(ix+d)` and `ld (ix+d),rr` cover **BC, DE, HL**. The low byte sits at the
low address, so the **high** half takes `d+1` — the detail one writes backwards
half the time by hand.

### One-for-one — orthographies

The assembler tolerates them; `--normalize` rewrites them; `--strict` refuses
them; `--beautify` leaves them alone (it formats, it does not canonicalize).

| Written | Canon |
|---|---|
| `ld pc,hl` · `jp hl` (idem `ix`, `iy`) | `jp (hl)` |
| `ex hl,de` | `ex de,hl` |
| `ex hl,(sp)` · `ex ix,(sp)` | `ex (sp),hl` · `ex (sp),ix` |
| `ex af,af` | `ex af,af'` — **warned** |
| `defb` `dm` `defm` → `db` · `defw` → `dw` · `defs` `rmb` → `ds` | |
| `endm` `mend` → `endmacro` · `rend` → `endrepeat` · `wend` → `endwhile` · `ends` → `endstruct` | |

`ex af,af` is the **only** tolerance that warns. Its literal reading denotes a
*different* operation — exchanging AF with itself, which is a no-op — where every
other one is merely unfashionable. The rule generalizes: fantams warns when the
text lies, not when it is out of style.

`jp (hl)` remains the **canon**, despite parentheses suggesting a
non-existent indirection: `ld pc,hl` is non-standard Z80 for everyone, and a
canon that other assemblers refuse would lose what makes its value.

### Repetition

A mnemonic that takes **no operand** may carry a count: `nop 32`, `ldi 16`,
`halt 2`. It is shorthand for `repeat n : <mnemonic> : endrepeat`, and it follows
that reading exactly.

- This is **unrolling**, not canonicalization: `-E` expands it fully — all
  1024 lines of `nop 1024` — while `--normalize`, which canonicalizes *without*
  unrolling, leaves `nop 32` intact. `--strict` refuses it.
- The count is a **preprocessor value**: a variable or an expression is fine,
  an expression touching a **label** is not — at preprocessor time no address
  exists. To reserve space measured on labels defined *above*, use `ds`.
- `nop 0` is legal and emits nothing; a negative count is an error.
- The rule covers **every** operand-less mnemonic, with no blocklist: a rule with
  exceptions costs more to remember than it saves. `ret` and `im` are not
  operand-less, so a count on them is not a count.

### What is refused, though other assemblers accept it

| Form | Why |
|---|---|
| `ld hl,sp` | it is usually rendered as `ld hl,0 : add hl,sp` — 4 bytes, and the carry is clobbered |
| `rlc hl` · `rr de` · `srl8 de` | 2 to 4 `cb` operations: a routine, not an orthography — write a macro |
| `rst z,#38` | 2 bytes that **overlap** — the `jr` displacement is itself the `rst` opcode — so no pair of canonical Z80 lines expresses it |
| `inc hl,de` · `dec bc,de` | no idiom behind it, and it collides with `add hl,de`; multi-register lists stay on `push`/`pop`, which are a sequence by nature |

`add a,b` and `add b` are **both** accepted, as are `and a,b`, `or a,b` and
`xor a,b`. Neither is elected canon: both are one opcode in a
standard spelling, so the difference is a taste, not a structure. `--normalize`
leaves them, `--strict` takes both.

---

## 12. Inclusion

`include "file"` inserts a source. `incbin` and `read` are **reserved but not
implemented** — using them produces a message about a reserved word, not
about a lack.

---

## 13. What is recognized to be refused

fantams keeps these reserved words rather than reading them as labels, to
fail by naming the replacement:

| Word | What the refusal says |
|---|---|
| `BANK` | write `org b<n>:<address>` — bank and address go on the same line |
| `SNASET` `SETCPC` | describes the OUTPUT format, not the program: pass it to invocation |
| `CHARSET` | a character set permutation is an asset encoding: generate the `db` with a script |
| `TICKER` | counting cycles is a control flow analysis, not a directive |
| `STR` | not yet implemented: use `db` (`STR` sets bit 7 of the last character) |

`BUILDSNA`, `BANKSET`, `NOLIST`, and `LIST` are **accepted and ignored**: they
are output-format or listing headers, which have no place in a source and no
effect here.

---

## 14. Where fantams is unusual

These are the points a reader coming from another Z80 assembler is most likely to
get wrong. None of them is negotiable, and each is argued in an ADR.

| Point | fantams |
|---|---|
| `repeat` index | starts at **0** |
| `sin` / `cos` | **radians**, never degrees |
| modules | **switch**, they do not nest — labels get a `gfx.` prefix |
| macro call | bare **or parenthesized** |
| block closing | `endX` or **`end`** for any block; `endr` does not exist |
| `/` | floating-point; `div` is the integer one |

---

## 15. The tool's modes

| Option | Effect |
|---|---|
| `-E` | writes the **unrolled source**: macros expanded, loops unrolled, canonicalized |
| `--normalize` | canonicalizes **without** unrolling |
| `--beautify` | formats only (labels in column 1, blocks indented, call parentheses) |
| `--strict` | refuses anything not canonical Z80 |
| `--no-detach-labels` | keeps `label: instruction` on one line |
| `--no-indent-blocks` | does not indent block bodies |
| `--sym[=file]` | writes the **symbol table** (CSV) for a disassembler or emulator |

`--sym` writes one line per **label and constant** — name, type, owning section,
logical value, storage bank and address, origin file and line. Not a listing: one line
per *name*, and no bytes. Variables (`=`) are left out. The default path derives
from `-o`, so the file travels next to the binary it describes. It refuses to
combine with `--beautify` and `--normalize`, which never reach the assembler, and
cohabits with `-E`. For a human reading a terminal, `-s` prints the table instead.
