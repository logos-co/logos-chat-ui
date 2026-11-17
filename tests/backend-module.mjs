// The backend runs as the core module chat_ui_backend ("backend": "module"):
// it is loaded and restricted to this app, it reaches chat_module, and the
// view shows what it answers. Run with `nix run .#ui-tests`.
import { readdirSync, readFileSync } from "node:fs";
import { join, resolve } from "node:path";

const root = process.env.LOGOS_QT_MCP || new URL("../result-mcp", import.meta.url).pathname;
const { test, run } = await import(resolve(root, "test-framework/framework.mjs"));

const APP = "chat_ui";
const BACKEND = `${APP}_backend`;

function sessionLog() {
  const logs = join(process.env.BASECAMP_TEST_USER_DIR, "logs");
  const file = readdirSync(logs).filter((f) => /^basecamp_.*\.log$/.test(f)).sort().pop();
  if (!file) throw new Error(`no session log in ${logs}`);
  return readFileSync(join(logs, file), "utf8");
}

// Evaluated in the context of the object, so the view's ids resolve. The view's
// root is found by its own property: it sets no objectName.
async function evalOn(app, property, expression) {
  const match = ((await app.findByProperty(property)).matches ?? [])[0];
  if (!match) throw new Error(`no object with ${property}`);
  const res = await app.inspector.send("evaluate", { objectId: match.id, expression });
  if (res.error) throw new Error(res.error);
  return res.result;
}

test("backend module: the view shows the backend's answer from chat_module", async (app) => {
  let v;
  await app.waitFor(async () => {
    v = await evalOn(app, "pendingSelection", "store.backend.myAddress");
    if (!(v && v.length > 0)) throw new Error(`store.backend.myAddress: ${v}`);
  }, { timeout: 60000, interval: 500, description: "the backend to reach chat_module" });
});

test("backend module: loaded as a core module, restricted to the app", async () => {
  const log = sessionLog();
  const restricted = log.indexOf(`Restricted ${BACKEND} to ${APP}`);
  const loaded = log.indexOf(`Module loaded: ${BACKEND}`);
  if (restricted < 0) throw new Error("no restriction line in the session log");
  if (loaded < 0) throw new Error("no load line for the backend in the session log");
  if (restricted > loaded) throw new Error("the backend was loaded before its restriction");
});

test("backend module: no ui-host deprecation for the app", async () => {
  const hit = sessionLog().split("\n").find((l) => l.includes("DEPRECATED") && l.includes(APP));
  if (hit) throw new Error(`deprecation logged: ${hit}`);
});

test("backend module: no stopped panel", async (app) => {
  const res = await app.findByProperty("objectName", "appStopped.root");
  const shown = (res.matches ?? []).filter((m) => m.visible !== false);
  if (shown.length) throw new Error("the app's stopped panel is shown");
});

// Two clients for one dependency each mint a token, and the dependency keeps
// one per caller: the other's calls are refused.
test("backend module: no call refused for a stale token", async () => {
  const hit = sessionLog().split("\n").find((l) => /auth token not recognized|rejecting unauthorized call/.test(l));
  if (hit) throw new Error(`refused: ${hit}`);
});

run();
