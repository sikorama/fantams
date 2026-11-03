Status: resolved
Blocked by: —

# 05: Décision : modules imbriqués, ENDMODULE ferme, MODULE OFF retiré

Décision à acter dans l'ADR du ticket 08 (ou une ADR dédiée) :

- `MODULE x` ouvre un module, `ENDMODULE` ou `END` le ferme.
- La forme commutateur (deux `MODULE` pour un `ENDMODULE`) est abandonnée.
- `MODULE OFF` est retiré.
- Justification : syntaxe régulière, fermeture par bloc comme les autres
  blocs. Le commentaire `pp.cpp:1103` (« deux MODULE pour un ENDMODULE est la
  forme normale ») devient faux et doit être retiré.

**Conséquences à traiter dans les tickets 06 et 03** : `tests/pp_test.cpp:525-536`
(MODULE OFF, forme commutateur), `examples/struct_module.asm`,
`docs/syntax.md:967-974`, `CONTEXT.md:144-145`.

Pas de code dans ce ticket : c'est la décision seule.

## Réponse

Décision confirmée telle qu'écrite ci-dessus (voir `spec.md`, « Décisions
acquises »). Elle sera consignée dans l'ADR du ticket 08 ; l'implémentation et
les conséquences (tests, exemple, doc, commentaire `pp.cpp:1103`) sont portées
par les tickets 06 et 03.
