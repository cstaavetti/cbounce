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

function snapshotTargets() {
  const count = wasm._demo_render_count();
  const ptr = wasm._demo_render_data();
  const data = wasm.HEAPF32.subarray(ptr >> 2, (ptr >> 2) + count * stride);
  const targets = [];
  for (let i = 0; i < count; i += 1) {
    const base = i * stride;
    const active = data[base] > 0.5;
    const kind = data[base + 1] | 0;
    if (active && (kind === 1 || kind === 2)) {
      targets.push({
        index: i,
        x: data[base + 2],
        y: data[base + 3],
        z: data[base + 4],
      });
    }
  }
  return targets;
}

const before = snapshotTargets();
if (before.length < 80) {
  throw new Error(`expected target boxes, got ${before.length}`);
}

if (!wasm._demo_shoot_projectile(-5.0, 1.7, 4.2, 0.0, -0.02, -1.0)) {
  throw new Error("demo_shoot_projectile failed");
}

for (let i = 0; i < 360; i += 1) {
  wasm._demo_step(1 / 120);
}

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

console.log(`web smoke passed: ${before.length} targets, movement=${movement.toFixed(3)}`);
