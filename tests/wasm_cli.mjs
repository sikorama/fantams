// wasm_cli.mjs — invoquer l'adaptateur WASM comme on invoque l'adaptateur CLI.
//
// L'adaptateur WASM n'exporte AUCUNE fonction du coeur : son contrat EST le
// contrat CLI — un argv et un systeme de fichiers. Ce pilote ne fait donc rien
// d'autre que poser les fichiers d'entree dans le systeme de fichiers virtuel,
// appeler `callMain` avec l'argv qu'on lui donne, et ressortir les fichiers
// demandes. C'est ce qui rend l'equivalence entre les deux adaptateurs
// verifiable sans rien construire de neuf.
//
//   node tests/wasm_cli.mjs <fantams.mjs> [--in hote:vfs]... [--out vfs:hote]... -- argv...
//
// La sortie standard et la sortie d'erreur du module sont reemises telles
// quelles ; le code de sortie est celui que rend `main`.
import { pathToFileURL } from 'node:url';
import { readFileSync, writeFileSync } from 'node:fs';

const argv = process.argv.slice(2);
const modPath = argv.shift();
if (!modPath) { console.error('usage: wasm_cli.mjs <fantams.mjs> [--in h:v] [--out v:h] -- argv...'); process.exit(2); }

const die = (m) => { console.error(`wasm_cli.mjs: ${m}`); process.exit(2); };

const ins = [], outs = [];
let rest = [];
while (argv.length) {
  const a = argv.shift();
  if (a === '--in' || a === '--out') {
    const spec = argv.shift();
    if (spec === undefined) die(`${a} attend « chemin:nom »`);
    (a === '--in' ? ins : outs).push(spec);
  } else if (a === '--') { rest = argv.splice(0); }
  else die(`argument inattendu: ${a}`);
}

// Le separateur est le DERNIER deux-points : un chemin hote peut en porter.
// Sans deux-points du tout, « lastIndexOf » rendrait -1 et decouperait la
// chaine en silence, pour finir sur un ENOENT qui n'expliquerait rien.
const split = (s) => {
  const i = s.lastIndexOf(':');
  if (i <= 0 || i === s.length - 1) die(`« ${s} » n'est pas de la forme « chemin:nom »`);
  return [s.slice(0, i), s.slice(i + 1)];
};

// Les specifications sont validees AVANT d'instancier le module : une erreur
// d'invocation doit se lire comme une erreur d'invocation, pas se cacher
// derriere une trace d'import ou un ENOENT tardif.
const inPairs = ins.map(split);
const outPairs = outs.map(split);

const createFantams = (await import(pathToFileURL(modPath).href)).default;

let out = '', err = '';
const mod = await createFantams({
  print: (s) => { out += s + '\n'; },
  printErr: (s) => { err += s + '\n'; },
  noExitRuntime: true,
});

for (const [host, vfs] of inPairs) {
  mod.FS.writeFile(vfs, new Uint8Array(readFileSync(host)));
}

let code = 0;
try {
  code = mod.callMain(rest) ?? 0;
} catch (e) {
  // ExitStatus : `main` a rendu par exit(). Tout le reste est une vraie panne.
  if (e && typeof e.status === 'number') code = e.status;
  else { process.stdout.write(out); process.stderr.write(err); console.error(`wasm_cli.mjs: ${e}`); process.exit(70); }
}

process.stdout.write(out);
process.stderr.write(err);

if (code === 0) {
  for (const [vfs, host] of outPairs) {
    writeFileSync(host, Buffer.from(mod.FS.readFile(vfs)));
  }
}
process.exit(code);
