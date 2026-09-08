cmake_minimum_required(VERSION 3.16)
project(fantams CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

# Bibliothèques autonomes (portables WASM/JS)
add_library(z80 STATIC z80.cpp)
target_include_directories(z80 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Vocabulaire réservé partagé (ADR 0013) : « ce mot est-il un label ? », une
# seule fois, avec la phase pour paramètre.
add_library(keywords STATIC keywords.cpp)
target_include_directories(keywords PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(keywords PUBLIC z80)

# expr.cpp depend de keywords (kw::readLiteral, ADR 0010).
add_library(pp STATIC pp.cpp expr.cpp)
target_include_directories(pp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pp PUBLIC keywords)

add_library(parser STATIC parser.cpp)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser PUBLIC z80 keywords)

# Mise en forme du source (ADR 0013) : texte -> texte, sans adresse ni octet.
add_library(beautify STATIC beautify.cpp)
target_include_directories(beautify PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(beautify PUBLIC keywords)

add_library(sna STATIC sna.cpp)
target_include_directories(sna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# sym.cpp : le format de la table des symboles (ADR 0019), fonction pure —
# il vit avec l'assembleur parce qu'il ne rend que son Object.
# asm.cpp rend un OBJET, link.cpp en fait une IMAGE : les deux moities de la
# meme chaine, et la seule facon d'obtenir un octet.
add_library(asmlib STATIC asm.cpp link.cpp fo.cpp lex.cpp script.cpp
                          profile.cpp profiles.cpp sym.cpp)
target_include_directories(asmlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(asmlib PUBLIC parser z80 pp sna keywords)

# Outil -E : export de la source préprocessée
add_executable(ppdump pp_main.cpp)
target_link_libraries(ppdump PRIVATE pp)

# CLI bout-en-bout : .asm -> .bin
add_executable(fantams asm_main.cpp)
target_link_libraries(fantams PRIVATE asmlib beautify)

# Tests. Les sources vivent dans tests/ ; les cibles gardent leur nom, et les
# répertoires d'inclusion des bibliothèques suffisent à résoudre leurs en-têtes.
add_executable(z80_test tests/z80_test.cpp)
target_link_libraries(z80_test PRIVATE z80)

add_executable(pp_test tests/pp_test.cpp)
target_link_libraries(pp_test PRIVATE pp)

add_executable(parser_test tests/parser_test.cpp)
target_link_libraries(parser_test PRIVATE parser)

add_executable(asm_test tests/asm_test.cpp)
target_link_libraries(asm_test PRIVATE asmlib)

add_executable(link_test tests/link_test.cpp)
target_link_libraries(link_test PRIVATE asmlib)

add_executable(fo_test tests/fo_test.cpp)
target_link_libraries(fo_test PRIVATE asmlib)

add_executable(script_test tests/script_test.cpp)
target_link_libraries(script_test PRIVATE asmlib)

add_executable(profile_test tests/profile_test.cpp)
target_link_libraries(profile_test PRIVATE asmlib)

add_executable(beautify_test tests/beautify_test.cpp)
target_link_libraries(beautify_test PRIVATE beautify asmlib)

add_executable(sna_test tests/sna_test.cpp)
target_link_libraries(sna_test PRIVATE sna)

# expr_test manquait ici alors que le Makefile le construit : deux listes de
# tests, dont une seule complete, est le genre d'ecart qui se paie en CI.
add_executable(expr_test tests/expr_test.cpp)
target_link_libraries(expr_test PRIVATE pp)

enable_testing()
add_test(NAME z80_encoding COMMAND z80_test)
add_test(NAME expressions COMMAND expr_test)
add_test(NAME preprocessor COMMAND pp_test)
add_test(NAME parser COMMAND parser_test)
add_test(NAME assembler COMMAND asm_test)
add_test(NAME linker COMMAND link_test)
add_test(NAME object_file COMMAND fo_test)
add_test(NAME link_script COMMAND script_test)
add_test(NAME target_profile COMMAND profile_test)
# L'acceptation de l'etage B : deux sources assemblees separement puis linkees
# donnent le meme binaire que la version monolithique. Un script, parce que le
# fait porte sur le CLI et sur des FICHIERS.
# Le critere de l'etape C1.3 : le profil embarque, sa copie exportee et aucun
# profil rendent le meme octet — donc le texte embarque n'a aucun privilege.
add_test(NAME accept_profile
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/accept_profile.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(accept_profile PROPERTIES
                     ENVIRONMENT "FANTAMS=$<TARGET_FILE:fantams>")

# Le critere de fin de l'etage C1 : l'exemple du §12.2, sans compression.
add_test(NAME accept_banked
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/accept_banked.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(accept_banked PROPERTIES
                     ENVIRONMENT "FANTAMS=$<TARGET_FILE:fantams>")

# L'invariant mecanique du §13.2 : aucun nom de machine dans le code du linker.
add_test(NAME no_machine_names
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/no_machine_names.sh)

add_test(NAME accept_separate
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/accept_separate.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(accept_separate PROPERTIES
                     ENVIRONMENT "FANTAMS=$<TARGET_FILE:fantams>")
add_test(NAME beautify COMMAND beautify_test)
add_test(NAME sna COMMAND sna_test)
