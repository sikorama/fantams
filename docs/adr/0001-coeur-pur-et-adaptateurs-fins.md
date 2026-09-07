---
status: accepted
---

# Cœur pur et adaptateurs fins, plutôt qu'un CLI émulé

Le cœur de fantams est une fonction pure sans état ni entrées-sorties : les neuf
modules (`z80`, `keywords`, `expr`, `pp`, `parser`, `asm`, `beautify`, `sna`,
`sym`) ne contiennent aucun `fstream`/`fopen`, et les fichiers inclus arrivent
par le callback `pp::FileProvider`. Nous en faisons un invariant et exposons ce
cœur à chaque hôte via un adaptateur fin, au lieu de faire imiter à fantams
les interfaces en ligne de commande des assembleurs Z80 existants.

## Contexte

L'intégration WASM initiale reproduisait un CLI — `argv`, `callMain`, système de
fichiers virtuel — pour qu'un hôte JS traite les trois assembleurs par
un chemin unique. Ces assembleurs sont des programmes externes non
modifiables ; fantams ne l'est pas, et payait ce coût d'imitation sans
contrepartie : `-sFORCE_FILESYSTEM`, l'écriture de `/in.asm` puis la relecture du
binaire produit en devinant son extension, et l'inspection d'`ExitStatus` pour
récupérer un code de retour.

Ce n'est pas resté théorique : les deux seules divergences d'octets du corpus ne
venaient pas de l'assembleur mais de `wrapFantams`, dont la regex `hasLiteOrg`
devinait à tort qu'une source portait déjà son `ORG` parce qu'elle en contenait
un 200 lignes plus bas. L'encodeur, lui, était byte-identique à
l'assembleur de référence.

## Ce qui est acquis, et ce qui ne l'est pas

**L'invariant tient.** Aucun des neuf modules ne fait d'entrées-sorties, et
`pp::FileProvider` reste le seul chemin par lequel un `include` atteint un
fichier. C'est ce qui permet au même cœur de servir le CLI natif et le WASM sans
qu'aucun des deux ne contamine l'autre.

**L'adaptateur fin, lui, reste à écrire.** L'intégration WASM est toujours le CLI
émulé que cet ADR voulait remplacer : `build-wasm.sh` exporte `callMain` et `FS`
sous `-sFORCE_FILESYSTEM=1`, et l'hôte fabrique encore un en-tête `org`/`run`
concaténé en tête de source, décidé par la regex `hasLiteOrg` — celle-là même
qui causait les deux divergences d'octets décrites plus haut. Tant que cet
adaptateur n'existe pas, la partie « exposer le cœur directement » de cette
décision est une intention et non un état.

## Conséquences

La règle de tri qui en découle, et qui protège le projet des dérives reprochées à
l'imitation d'un CLI : **une fonctionnalité qui n'a pas de sens pour tous les
hôtes est un
adaptateur, pas le cœur.**
