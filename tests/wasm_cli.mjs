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

const ins = [], outs = [];
let rest = [];
while (argv.length) {
  const a = argv.shift();
  if (a === '--in') ins.push(argv.shift());
  else if (a === '--out') outs.push(argv.shift());
  else if (a === '--') { rest = argv.splice(0); }
  else { console.error(`wasm_cli.mjs: argument inattendu: ${a}`); process.exit(2); }
}

// Le separateur est le DERNIER deux-points : un chemin hote peut en porter.
const split = (s) => { const i = s.lastIndexOf(':'); return [s.slice(0, i), s.slice(i + 1)]; };

const createFantams = (await import(pathToFileURL(modPath).href)).default;

let out = '', err = '';
const mod = await createFantams({
  print: (s) => { out += s + '\n'; },
  printErr: (s) => { err += s + '\n'; },
  noExitRuntime: true,
});

for (const spec of ins) {
  const [host, vfs] = split(spec);
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
  for (const spec of outs) {
    const [vfs, host] = split(spec);
    writeFileSync(host, Buffer.from(mod.FS.readFile(vfs)));
  }
}
process.exit(code);
