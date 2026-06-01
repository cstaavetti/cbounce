import { fileURLToPath } from "node:url";

const siteUrl = new URL("../../build/web/site/", import.meta.url);
const moduleUrl = new URL("cbounce_demo.mjs", siteUrl);
const createCbounceDemo = (await import(moduleUrl.href)).default;

const wasm = await createCbounceDemo({
  locateFile(path) {
    return fileURLToPath(new URL(path, siteUrl));
  },
});

if (!wasm._demo_reset()) {
  throw new Error("demo_reset failed");
}

const stride = wasm._demo_render_stride();

const KIND_BOX = 1;
const KIND_WOOD = 2;
const KIND_PROJECTILE = 3;
const KIND_CHAIN_BALL = 6;
const KIND_TUMBLER_BALL = 10;
const KIND_TUMBLER_BOX = 11;

function snapshotBodies(filterKind) {
  const count = wasm._demo_render_count();
  const ptr = wasm._demo_render_data();
  const data = wasm.HEAPF32.subarray(ptr >> 2, (ptr >> 2) + count * stride);
  const bodies = [];
  for (let i = 0; i < count; i += 1) {
    const base = i * stride;
    const active = data[base] > 0.5;
    const kind = data[base + 1] | 0;
    if (active && filterKind(kind)) {
      bodies.push({
        index: i,
        kind,
        x: data[base + 2],
        y: data[base + 3],
        z: data[base + 4],
      });
    }
  }
  return bodies;
}

function snapshotTargets() {
  return snapshotBodies((kind) => kind === KIND_BOX || kind === KIND_WOOD);
}

function stepFrames(count) {
  for (let i = 0; i < count; i += 1) {
    wasm._demo_step(1 / 120);
  }
}

if (!wasm._demo_shoot_projectile(0.0, 6.0, 0.0, 1.0, 0.0, 0.0)) {
  throw new Error("demo_shoot_projectile failed for gravity check");
}
const projectileStart = snapshotBodies((kind) => kind === KIND_PROJECTILE)[0];
stepFrames(12);
const projectileEnd = snapshotBodies((kind) => kind === KIND_PROJECTILE)[0];
if (!projectileStart || !projectileEnd ||
    projectileEnd.y >= projectileStart.y - 0.015) {
  throw new Error("projectile did not fall under gravity");
}

if (!wasm._demo_reset()) {
  throw new Error("demo_reset failed before target check");
}

const before = snapshotTargets();
if (before.length < 80) {
  throw new Error(`expected target boxes, got ${before.length}`);
}

if (!wasm._demo_shoot_projectile(-5.0, 1.7, 4.2, 0.0, -0.02, -1.0)) {
  throw new Error("demo_shoot_projectile failed");
}

stepFrames(360);

const after = snapshotTargets();
let movement = 0;
for (const initial of before) {
  const next = after.find((target) => target.index === initial.index);
  if (!next) {
    continue;
  }
  movement += Math.abs(next.x - initial.x);
  movement += Math.abs(next.y - initial.y);
  movement += Math.abs(next.z - initial.z);
}

if (movement < 0.25) {
  throw new Error(`projectile did not move target structures enough: ${movement.toFixed(3)}`);
}

if (!wasm._demo_reset()) {
  throw new Error("demo_reset failed before movement check");
}

for (let i = 0; i < 300; i += 1) {
  wasm._demo_move_player(1, 0, 0, 1 / 60);
}
const wallZ = wasm._demo_player_z();
for (let i = 0; i < 90; i += 1) {
  wasm._demo_move_player(0, 1, 0, 1 / 60);
}
const tangentSlideDistance = Math.abs(wasm._demo_player_z() - wallZ);
if (tangentSlideDistance < 1.5) {
  throw new Error(`player did not slide tangentially along wall: ${tangentSlideDistance.toFixed(3)}`);
}
const diagonalWallZ = wasm._demo_player_z();
for (let i = 0; i < 90; i += 1) {
  wasm._demo_move_player(1, 1, 0, 1 / 60);
}
const slideDistance = Math.abs(wasm._demo_player_z() - diagonalWallZ);
if (slideDistance < 1.5) {
  throw new Error(`player did not slide along wall: ${slideDistance.toFixed(3)}`);
}
const wallX = wasm._demo_player_x();
for (let i = 0; i < 60; i += 1) {
  wasm._demo_move_player(-1, 0, 0, 1 / 60);
}
const awayDistance = wallX - wasm._demo_player_x();
if (awayDistance < 1.5) {
  throw new Error(`player did not move away from wall: ${awayDistance.toFixed(3)}`);
}

if (!wasm._demo_reset()) {
  throw new Error("demo_reset failed before chain check");
}
const chainBallStart = snapshotBodies((kind) => kind === KIND_CHAIN_BALL)[0];
if (!chainBallStart) {
  throw new Error("expected chain ball");
}
if (!wasm._demo_shoot_projectile(-9.5, 1.35, 3.0, 1.0, 0.0, 0.0)) {
  throw new Error("demo_shoot_projectile failed for chain check");
}
stepFrames(360);
const chainBallEnd = snapshotBodies((kind) => kind === KIND_CHAIN_BALL)[0];
if (!chainBallEnd || chainBallEnd.y < 0.85) {
  throw new Error(`chain ball did not stay suspended: ${chainBallEnd ? chainBallEnd.y.toFixed(3) : "missing"}`);
}

if (!wasm._demo_reset()) {
  throw new Error("demo_reset failed before tumbler check");
}
stepFrames(720);
const tumblerPieces = snapshotBodies((kind) => kind === KIND_TUMBLER_BALL || kind === KIND_TUMBLER_BOX);
const escapedTumblerPiece = tumblerPieces.find((body) =>
  body.y < -0.50 || body.y > 5.00 ||
  Math.abs(body.x - 8.8) > 3.5 ||
  Math.abs(body.z - 5.0) > 1.55
);
if (escapedTumblerPiece) {
  throw new Error(`tumbler piece escaped: ${JSON.stringify(escapedTumblerPiece)}`);
}

console.log(`web smoke passed: ${before.length} targets, movement=${movement.toFixed(3)}, slide=${slideDistance.toFixed(3)}, tangent=${tangentSlideDistance.toFixed(3)}, away=${awayDistance.toFixed(3)}`);
