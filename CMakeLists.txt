cmake_minimum_required(VERSION 3.16)
project(fantams CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

# Les modules du coeur sont declares UNE SEULE FOIS, dans sources.manifest, que
# les trois points d'entree de construction lisent. La ligne « groupe fichier »
# y nomme la bibliotheque qui porte le module ; les aretes entre bibliotheques
# restent ici, parce qu'elles sont un fait de CMake et non une liste de sources.
#
# Le manifeste est declare en dependance de la reconfiguration : ajouter un
# module y suffit, sans autre edition et sans « cmake » relance a la main.
set(FANTAMS_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/sources.manifest)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FANTAMS_MANIFEST})

# ENCODING UTF-8 : sans lui, file(STRINGS) coupe les lignes sur les octets non
# ASCII, et un fragment de commentaire accentue arrive ici sans son diese.
file(STRINGS ${FANTAMS_MANIFEST} _manifest_lines ENCODING UTF-8)
foreach(_line IN LISTS _manifest_lines)
  string(STRIP "${_line}" _line)
  if(_line STREQUAL "" OR _line MATCHES "^#")
    continue()
  endif()
  if(NOT _line MATCHES "^([A-Za-z0-9_]+)[ \t]+([^ \t]+)$")
    message(FATAL_ERROR "sources.manifest : ligne illisible : ${_line}")
  endif()
  list(APPEND FANTAMS_GROUPS ${CMAKE_MATCH_1})
  list(APPEND FANTAMS_SRC_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
endforeach()
if(NOT FANTAMS_GROUPS)
  message(FATAL_ERROR "sources.manifest : aucun module lu")
endif()

# Bibliothèques autonomes (portables WASM/JS)
add_library(z80 STATIC ${FANTAMS_SRC_z80})
target_include_directories(z80 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Vocabulaire réservé partagé (ADR 0013) : « ce mot est-il un label ? », une
# seule fois, avec la phase pour paramètre.
add_library(keywords STATIC ${FANTAMS_SRC_keywords})
target_include_directories(keywords PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(keywords PUBLIC z80)

# expr.cpp depend de keywords (kw::readLiteral, ADR 0010).
add_library(pp STATIC ${FANTAMS_SRC_pp})
target_include_directories(pp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pp PUBLIC keywords)

add_library(parser STATIC ${FANTAMS_SRC_parser})
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser PUBLIC z80 keywords)

# Mise en forme du source (ADR 0013) : texte -> texte, sans adresse ni octet.
add_library(beautify STATIC ${FANTAMS_SRC_beautify})
target_include_directories(beautify PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(beautify PUBLIC keywords)

add_library(sna STATIC ${FANTAMS_SRC_sna})
target_include_directories(sna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# sym.cpp : le format de la table des symboles (ADR 0019), fonction pure —
# il vit avec l'assembleur parce qu'il ne rend que son Object.
# asm.cpp rend un OBJET, link.cpp en fait une IMAGE : les deux moities de la
# meme chaine, et la seule facon d'obtenir un octet.
add_library(asmlib STATIC ${FANTAMS_SRC_asmlib})
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

# LE VERROU : le meme argv, a travers l'adaptateur natif et a travers
# l'adaptateur WASM, rend les memes octets. C'est ce qu'un artefact perime
# viole, et c'est ce que le critere precedent — signature du snapshot plus
# drapeau de succes — laissait passer sans broncher.
#
# SKIP_RETURN_CODE : deux dependances vivent hors de l'outillage de construction
# de fantams — l'artefact WASM, produit par un conteneur emsdk, et node. Le
# script rend 77 quand l'une manque, et ctest le rapporte « Skipped », distinct
# d'un « Passed ». Un test saute n'est pas un test reussi.
add_test(NAME accept_wasm_equiv
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/accept_wasm_equiv.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(accept_wasm_equiv PROPERTIES
                     ENVIRONMENT "FANTAMS=$<TARGET_FILE:fantams>"
                     SKIP_RETURN_CODE 77)
