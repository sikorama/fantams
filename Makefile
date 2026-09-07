# Makefile fantams — build simple sans dépendance (cmake optionnel, cf. CMakeLists.txt)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

.PHONY: all test clean

CORE = z80.cpp expr.cpp keywords.cpp parser.cpp pp.cpp asm.cpp link.cpp fo.cpp beautify.cpp sna.cpp sym.cpp

# Les tests vivent dans tests/, binaire compris : la racine ne porte que le code
# et les deux outils. Leurs « #include "asm.h" » se résolvent par -I. — la
# compilation part toujours de la racine.
T     = tests
TESTS = $(T)/z80_test $(T)/expr_test $(T)/pp_test $(T)/parser_test \
        $(T)/asm_test $(T)/link_test $(T)/fo_test $(T)/beautify_test $(T)/sna_test
TCXX  = $(CXX) $(CXXFLAGS) -I.

all: $(TESTS) ppdump fantams

$(T)/z80_test: z80.cpp $(T)/z80_test.cpp z80.h
	$(TCXX) z80.cpp $(T)/z80_test.cpp -o $@

# expr lit les littéraux de chaîne par kw::readLiteral (ADR 0010) : la règle
# « où finit un littéral » vit une seule fois, dans keywords.
$(T)/expr_test: expr.cpp keywords.cpp z80.cpp $(T)/expr_test.cpp expr.h keywords.h
	$(TCXX) expr.cpp keywords.cpp z80.cpp $(T)/expr_test.cpp -o $@

$(T)/pp_test: pp.cpp expr.cpp z80.cpp keywords.cpp $(T)/pp_test.cpp pp.h expr.h z80.h keywords.h
	$(TCXX) pp.cpp expr.cpp z80.cpp keywords.cpp $(T)/pp_test.cpp -o $@

$(T)/parser_test: parser.cpp z80.cpp keywords.cpp $(T)/parser_test.cpp parser.h z80.h keywords.h
	$(TCXX) parser.cpp z80.cpp keywords.cpp $(T)/parser_test.cpp -o $@

$(T)/asm_test: asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/asm_test.cpp asm.h link.h sym.h keywords.h
	$(TCXX) asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/asm_test.cpp -o $@

# Le linkage se teste au point le plus haut : des objets fabriques A LA MAIN,
# et l'image qui en sort. Verifier un recouvrement ne demande plus d'ecrire un
# source Z80 qui le provoque, mais deux structures de dix lignes.
$(T)/link_test: link.cpp asm.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/link_test.cpp link.h asm.h
	$(TCXX) link.cpp asm.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/link_test.cpp -o $@

# Le fichier objet : l'aller-retour se teste par CHAINES, ce qui est justement
# la raison de ne pas le faire compact.
$(T)/fo_test: fo.cpp asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/fo_test.cpp fo.h asm.h
	$(TCXX) fo.cpp asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp $(T)/fo_test.cpp -o $@

# Le beautify n'a besoin que du parseur et du vocabulaire réservé : ni adresse,
# ni octet, ni assemblage (ADR 0013).
# beautify.cpp lui-meme n'a besoin que de keywords + z80 ; l'assembleur n'est la
# que pour l'invariant d'octets, verifie par les tests.
$(T)/beautify_test: beautify.cpp keywords.cpp z80.cpp asm.cpp link.cpp parser.cpp expr.cpp pp.cpp $(T)/beautify_test.cpp beautify.h keywords.h
	$(TCXX) beautify.cpp keywords.cpp z80.cpp asm.cpp link.cpp parser.cpp expr.cpp pp.cpp $(T)/beautify_test.cpp -o $@

$(T)/sna_test: sna.cpp $(T)/sna_test.cpp sna.h
	$(TCXX) sna.cpp $(T)/sna_test.cpp -o $@

ppdump: pp.cpp expr.cpp z80.cpp keywords.cpp pp_main.cpp pp.h expr.h z80.h keywords.h
	$(CXX) $(CXXFLAGS) pp.cpp expr.cpp z80.cpp keywords.cpp pp_main.cpp -o $@

fantams: $(CORE) asm_main.cpp asm.h pp.h sym.h
	$(CXX) $(CXXFLAGS) $(CORE) asm_main.cpp -o $@

test: $(TESTS) fantams
	@for t in $(TESTS); do ./$$t || exit 1; done
	@$(T)/accept_separate.sh

clean:
	rm -f $(TESTS) ppdump fantams
	rm -rf build
