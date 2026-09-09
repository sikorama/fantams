# Makefile fantams — build simple sans dépendance (cmake optionnel, cf. CMakeLists.txt)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

.PHONY: all test clean

# Les modules du coeur sont declares une seule fois, dans sources.manifest, que
# les trois points d'entree de construction LISENT (cf. l'en-tete du manifeste).
# On n'y prend que la seconde colonne : le groupe ne sert qu'a CMake.
MANIFEST = sources.manifest
# Tous les en-tetes de la racine, pas trois d'entre eux. La liste nominative
# etait fausse des que le coeur en gagnait un — version.h en a fait la
# demonstration : « touch version.h && make » repondait « up to date », et le
# binaire continuait d'annoncer l'ancienne date de version. C'est exactement la
# classe de peremption que ce chantier ferme.
HDRS := $(wildcard *.h)
# (le « \# » est pour make, qui couperait la ligne sur un dièse nu ; awk le
# recoit par -v, donc sans echappement.)
HASH := \#
CORE := $(shell awk -v h='$(HASH)' 'NF && substr($$1,1,1) != h { print $$2 }' $(MANIFEST))

# Les tests vivent dans tests/, binaire compris : la racine ne porte que le code
# et les deux outils. Leurs « #include "asm.h" » se résolvent par -I. — la
# compilation part toujours de la racine.
T     = tests
TESTS = $(T)/z80_test $(T)/expr_test $(T)/pp_test $(T)/parser_test \
        $(T)/asm_test $(T)/link_test $(T)/fo_test $(T)/script_test \
        $(T)/profile_test $(T)/beautify_test $(T)/sna_test
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

$(T)/asm_test: asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp script.cpp lex.cpp $(T)/asm_test.cpp asm.h link.h sym.h keywords.h script.h
	$(TCXX) asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp script.cpp lex.cpp $(T)/asm_test.cpp -o $@

# Le linkage se teste au point le plus haut : des objets fabriques A LA MAIN,
# et l'image qui en sort. Verifier un recouvrement ne demande plus d'ecrire un
# source Z80 qui le provoque, mais deux structures de dix lignes.
# Le profil et le script y sont ANALYSES depuis du texte, et non fabriques a la
# main : c'est le chemin reel, et un test qui echoue nomme le bon maillon.
$(T)/link_test: link.cpp asm.cpp parser.cpp z80.cpp expr.cpp keywords.cpp lex.cpp script.cpp profile.cpp $(T)/link_test.cpp link.h asm.h script.h profile.h
	$(TCXX) link.cpp asm.cpp parser.cpp z80.cpp expr.cpp keywords.cpp lex.cpp script.cpp profile.cpp $(T)/link_test.cpp -o $@

# Le fichier objet : l'aller-retour se teste par CHAINES, ce qui est justement
# la raison de ne pas le faire compact.
$(T)/fo_test: fo.cpp asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp script.cpp lex.cpp $(T)/fo_test.cpp fo.h asm.h script.h
	$(TCXX) fo.cpp asm.cpp link.cpp sym.cpp parser.cpp z80.cpp expr.cpp keywords.cpp script.cpp lex.cpp $(T)/fo_test.cpp -o $@

# Le script de linkage : un TEXTE entre, une valeur sort. Aucun profil, aucun
# objet, aucun octet — c'est ce qui le rend testable seul, et c'est pour cela
# que ces deux suites ne se lient qu'a leur analyseur et au lexeur partage.
$(T)/script_test: lex.cpp script.cpp $(T)/script_test.cpp script.h lex.h asm.h
	$(TCXX) lex.cpp script.cpp $(T)/script_test.cpp -o $@

# Le profil de cible, et les profils LIVRES : la suite charge chaque profil
# embarque et verifie qu'il se lit, sans qu'aucun code de linker connaisse son
# nom (§13.2, test d'acceptation n°2 du modele).
$(T)/profile_test: lex.cpp profile.cpp profiles.cpp $(T)/profile_test.cpp profile.h lex.h asm.h
	$(TCXX) lex.cpp profile.cpp profiles.cpp $(T)/profile_test.cpp -o $@

# Le beautify n'a besoin que du parseur et du vocabulaire réservé : ni adresse,
# ni octet, ni assemblage (ADR 0013).
# beautify.cpp lui-meme n'a besoin que de keywords + z80 ; l'assembleur n'est la
# que pour l'invariant d'octets, verifie par les tests.
$(T)/beautify_test: beautify.cpp keywords.cpp z80.cpp asm.cpp link.cpp parser.cpp expr.cpp pp.cpp script.cpp lex.cpp $(T)/beautify_test.cpp beautify.h keywords.h script.h
	$(TCXX) beautify.cpp keywords.cpp z80.cpp asm.cpp link.cpp parser.cpp expr.cpp pp.cpp script.cpp lex.cpp $(T)/beautify_test.cpp -o $@

$(T)/sna_test: sna.cpp $(T)/sna_test.cpp sna.h
	$(TCXX) sna.cpp $(T)/sna_test.cpp -o $@

ppdump: pp.cpp expr.cpp z80.cpp keywords.cpp pp_main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) pp.cpp expr.cpp z80.cpp keywords.cpp pp_main.cpp -o $@

fantams: $(MANIFEST) $(CORE) asm_main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CORE) asm_main.cpp -o $@

test: $(TESTS) fantams
	@for t in $(TESTS); do ./$$t || exit 1; done
	@$(T)/accept_separate.sh
	@$(T)/accept_profile.sh
	@$(T)/accept_banked.sh
	@$(T)/accept_aliased.sh
	@$(T)/no_machine_names.sh
	@# Le verrou natif == WASM. Il se SAUTE (code 77) quand l'artefact WASM ou
	@# node manquent : ici comme sous ctest, un saut est bruyant et n'est pas un
	@# succes.
	@$(T)/accept_wasm_equiv.sh; rc=$$?; \
	 if [ $$rc = 77 ]; then echo "   (saute — la suite n'a PAS verifie natif == WASM)"; \
	 elif [ $$rc != 0 ]; then exit $$rc; fi
	@# L'epreuve du snapshot. Meme discipline : absente l'une de ses
	@# dependances, elle se saute — et le dit.
	@$(T)/epreuve_snapshot.sh; rc=$$?; \
	 if [ $$rc = 77 ]; then echo "   (saute — l'artefact n'a PAS ete eprouve sur machine)"; \
	 elif [ $$rc != 0 ]; then exit $$rc; fi

clean:
	rm -f $(TESTS) ppdump fantams
	rm -rf build
