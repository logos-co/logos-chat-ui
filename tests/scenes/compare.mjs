// Lay out renders of the scene catalog (`nix run .#scenes`) as a static page.
//
//   node tests/scenes/compare.mjs <site-dir> <head-dir> [<base-dir>]
//
// Scenes pair up by name between the base and the head: each is changed (its
// pixels differ; a diff image marks where), new, removed or unchanged. Without a
// base the page is the head's catalog. Writes <site-dir>/index.html, the images
// it shows under head/, base/ and diff/, and <site-dir>/summary.json with the
// scene names by kind, for the PR comment. Needs ImageMagick's `compare`.
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

const [siteDir, headDir, baseDir] = process.argv.slice(2);
if (!siteDir || !headDir) {
  console.error("usage: compare.mjs <site-dir> <head-dir> [<base-dir>]");
  process.exit(2);
}
const hasBase = Boolean(baseDir) && fs.existsSync(baseDir);

const scenesIn = (dir) => fs.readdirSync(dir).filter((f) => f.endsWith(".png")).map((f) => f.slice(0, -4)).sort();
const png = (dir, name) => path.join(dir, `${name}.png`);
for (const side of ["head", "base", "diff"]) fs.mkdirSync(path.join(siteDir, side), { recursive: true });

// A PNG's width and height, from its IHDR chunk.
const sizeOf = (file) => {
  const header = fs.readFileSync(file).subarray(16, 24);
  return `${header.readUInt32BE(0)}x${header.readUInt32BE(4)}`;
};

// compare exits 0 when the pixels match, 1 when they differ, having written the
// diff image, and 2 on an error. It pads the smaller image with its edge pixels,
// so a size change whose new area matches that edge needs the sizes compared.
function differs(name) {
  const diff = png(path.join(siteDir, "diff"), name);
  const r = spawnSync("compare", ["-metric", "AE", png(baseDir, name), png(headDir, name), diff], { encoding: "utf-8" });
  if (r.error) throw r.error;
  if (r.status !== 0 && r.status !== 1) throw new Error(`compare ${name}: ${r.stderr.trim()}`);
  if (r.status === 1 || sizeOf(png(baseDir, name)) !== sizeOf(png(headDir, name))) return true;
  fs.rmSync(diff, { force: true });
  return false;
}

const head = scenesIn(headDir);
const base = hasBase ? scenesIn(baseDir) : [];
const summary = { base: hasBase, scenes: head, changed: [], added: [], removed: [] };
if (hasBase) {
  for (const name of head) {
    if (!base.includes(name)) summary.added.push(name);
    else if (differs(name)) summary.changed.push(name);
  }
  summary.removed = base.filter((name) => !head.includes(name));
}
const unchanged = head.filter((name) => !summary.changed.includes(name) && !summary.added.includes(name));

for (const name of head) fs.copyFileSync(png(headDir, name), png(path.join(siteDir, "head"), name));
for (const name of [...summary.changed, ...summary.removed]) fs.copyFileSync(png(baseDir, name), png(path.join(siteDir, "base"), name));

const image = (side, name, label) => {
  const src = `${side}/${name}.png`;
  return `<div><p class="label">${label}</p><a href="${src}"><img src="${src}" alt="${name}, ${label}"></a></div>`;
};
const section = (title, names, sides) => names.length === 0 ? "" : `
<h2>${title} (${names.length})</h2>
${names.map((name) => `<figure id="${name}"><figcaption><a href="#${name}">${name}</a></figcaption>
<div class="row">${sides.map(([side, label]) => image(side, name, label)).join("")}</div></figure>`).join("\n")}`;

const body = !hasBase
  ? `<p>${head.length} scenes, with no base to compare them with.</p>
${section("Scenes", head, [["head", "scene"]])}`
  : `<p>${head.length} scenes: ${summary.changed.length} changed, ${summary.added.length} new and ${summary.removed.length} removed against the base.</p>
${section("Changed", summary.changed, [["base", "before"], ["head", "after"], ["diff", "diff"]])}
${section("New", summary.added, [["head", "after"]])}
${section("Removed", summary.removed, [["base", "before"]])}
${section("Unchanged", unchanged, [["head", "scene"]])}`;
fs.writeFileSync(path.join(siteDir, "index.html"), `<!doctype html><meta charset=utf-8>
<title>logos-chat-ui scenes</title>
<style>
body{font:15px system-ui;margin:32px;color:#1f2328;background:#fff}
h2{margin-top:40px}figure{margin:0 0 40px}figcaption{font-family:monospace;margin-bottom:8px}
figcaption a{color:inherit}.row{display:flex;gap:16px;flex-wrap:wrap}.row>div{flex:1 1 320px;min-width:0}
.label{font-size:13px;color:#59636e;margin:0 0 4px}img{max-width:100%;border:1px solid #d0d7de}
</style>
<h1>logos-chat-ui scenes</h1>
${body}
`);
fs.writeFileSync(path.join(siteDir, "summary.json"), JSON.stringify(summary, null, 2) + "\n");
console.log(hasBase
  ? `${head.length} scenes: ${summary.changed.length} changed, ${summary.added.length} new, ${summary.removed.length} removed`
  : `${head.length} scenes, no base`);
