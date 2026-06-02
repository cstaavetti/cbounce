import * as THREE from "https://cdn.jsdelivr.net/npm/three@0.184.0/build/three.module.js";
import createCbounceDemo from "./cbounce_demo.mjs";

const canvas = document.getElementById("viewport");
const statsEl = document.getElementById("stats");
const playButton = document.getElementById("play");

const KIND_BOX = 1;
const KIND_WOOD = 2;
const KIND_PROJECTILE = 3;
const KIND_FLOOR = 4;
const KIND_WALL = 5;
const KIND_CHAIN_BALL = 6;
const KIND_CHAIN_LINK = 7;
const KIND_CHAIN_FRAME = 8;
const KIND_TUMBLER_WALL = 9;
const KIND_TUMBLER_BALL = 10;
const KIND_TUMBLER_BOX = 11;
const KIND_TERRAIN = 12;
const KIND_CAR_CHASSIS = 13;
const KIND_CAR_WHEEL = 14;
const KIND_BARREL = 15;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0xa8bcc5);
scene.fog = new THREE.Fog(0xa8bcc5, 40, 105);

const camera = new THREE.PerspectiveCamera(70, 1, 0.05, 150);
camera.rotation.order = "YXZ";

const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  powerPreference: "high-performance",
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;
renderer.outputColorSpace = THREE.SRGBColorSpace;

const hemi = new THREE.HemisphereLight(0xe8f5ff, 0x66513e, 1.6);
scene.add(hemi);

const sun = new THREE.DirectionalLight(0xfff0d2, 3.2);
sun.position.set(7, 12, 8);
sun.castShadow = true;
sun.shadow.mapSize.set(2048, 2048);
sun.shadow.camera.left = -44;
sun.shadow.camera.right = 44;
sun.shadow.camera.top = 44;
sun.shadow.camera.bottom = -44;
sun.shadow.camera.near = 0.5;
sun.shadow.camera.far = 95;
scene.add(sun);

const meshes = [];
const keys = new Set();
const shotDirection = new THREE.Vector3();
const carPosition = new THREE.Vector3();
const carQuaternion = new THREE.Quaternion();
const carForward = new THREE.Vector3();
const carLookTarget = new THREE.Vector3();
const carCameraTarget = new THREE.Vector3();
const carLookLift = new THREE.Vector3(0, 0.9, 0);
const carCameraLift = new THREE.Vector3(0, 2.9, 0);
const carEnterCameraLift = new THREE.Vector3(0, 2.8, 0);
const carWheelLocalOffsets = [
  new THREE.Vector3(-1.14, -0.43, -1.34),
  new THREE.Vector3(1.14, -0.43, -1.34),
  new THREE.Vector3(-1.14, -0.43, 1.34),
  new THREE.Vector3(1.14, -0.43, 1.34),
];
const carWheelWorldOffset = new THREE.Vector3();
const carWheelBaseQuaternion = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 0, 1), Math.PI * 0.5);
const carWheelSteerQuaternion = new THREE.Quaternion();
const carWheelRollQuaternion = new THREE.Quaternion();
const localYAxis = new THREE.Vector3(0, 1, 0);
let carSteerAngle = 0;
let carWheelRoll = 0;
let wasm = null;
let renderStride = 13;
let yaw = 0;
let pitch = 0;
let mouseDown = false;
let driving = false;
let lastShotAt = 0;
let lastFrameAt = performance.now();
let smoothedFps = 60;

function makeCanvasTexture(draw) {
  const textureCanvas = document.createElement("canvas");
  textureCanvas.width = 256;
  textureCanvas.height = 256;
  const ctx = textureCanvas.getContext("2d");
  draw(ctx, textureCanvas.width, textureCanvas.height);
  const texture = new THREE.CanvasTexture(textureCanvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.wrapS = THREE.RepeatWrapping;
  texture.wrapT = THREE.RepeatWrapping;
  return texture;
}

function noise(ctx, width, height, alpha) {
  const image = ctx.getImageData(0, 0, width, height);
  for (let i = 0; i < image.data.length; i += 4) {
    const value = (Math.random() * 255) | 0;
    image.data[i] = value;
    image.data[i + 1] = value;
    image.data[i + 2] = value;
    image.data[i + 3] = alpha;
  }
  ctx.putImageData(image, 0, 0);
}

const floorTexture = makeCanvasTexture((ctx, width, height) => {
  ctx.fillStyle = "#68776f";
  ctx.fillRect(0, 0, width, height);
  ctx.globalAlpha = 0.18;
  noise(ctx, width, height, 34);
  ctx.globalAlpha = 1;
  ctx.strokeStyle = "rgba(245, 240, 220, 0.16)";
  ctx.lineWidth = 2;
  for (let i = 0; i <= width; i += 32) {
    ctx.beginPath();
    ctx.moveTo(i, 0);
    ctx.lineTo(i, height);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(0, i);
    ctx.lineTo(width, i);
    ctx.stroke();
  }
});
floorTexture.repeat.set(28, 28);

const terrainTexture = makeCanvasTexture((ctx, width, height) => {
  ctx.fillStyle = "#566b58";
  ctx.fillRect(0, 0, width, height);
  ctx.fillStyle = "rgba(213, 203, 155, 0.14)";
  for (let i = 0; i < 90; i += 1) {
    const x = Math.random() * width;
    const y = Math.random() * height;
    ctx.beginPath();
    ctx.ellipse(x, y, 4 + Math.random() * 18, 1 + Math.random() * 5, Math.random() * Math.PI, 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.globalAlpha = 0.16;
  noise(ctx, width, height, 38);
  ctx.globalAlpha = 1;
});
terrainTexture.repeat.set(4, 4);

const woodTexture = makeCanvasTexture((ctx, width, height) => {
  ctx.fillStyle = "#b87b45";
  ctx.fillRect(0, 0, width, height);
  for (let y = 0; y < height; y += 9) {
    ctx.strokeStyle = `rgba(${105 + Math.random() * 45}, ${55 + Math.random() * 24}, 24, 0.34)`;
    ctx.lineWidth = 1 + Math.random() * 2;
    ctx.beginPath();
    ctx.moveTo(0, y + Math.random() * 5);
    for (let x = 0; x <= width; x += 24) {
      ctx.lineTo(x, y + Math.sin(x * 0.04 + y) * 4 + Math.random() * 3);
    }
    ctx.stroke();
  }
});
woodTexture.repeat.set(1.5, 0.8);

const boxTexture = makeCanvasTexture((ctx, width, height) => {
  ctx.fillStyle = "#546e84";
  ctx.fillRect(0, 0, width, height);
  ctx.strokeStyle = "rgba(222, 232, 235, 0.23)";
  ctx.lineWidth = 12;
  ctx.strokeRect(18, 18, width - 36, height - 36);
  ctx.strokeStyle = "rgba(23, 31, 36, 0.28)";
  ctx.lineWidth = 3;
  ctx.strokeRect(22, 22, width - 44, height - 44);
});

const wallTexture = makeCanvasTexture((ctx, width, height) => {
  ctx.fillStyle = "#4e595c";
  ctx.fillRect(0, 0, width, height);
  ctx.fillStyle = "rgba(236, 231, 211, 0.08)";
  for (let y = 0; y < height; y += 36) {
    for (let x = (y / 36) % 2 ? -38 : 0; x < width; x += 76) {
      ctx.fillRect(x + 3, y + 3, 70, 30);
    }
  }
});
wallTexture.repeat.set(8, 1);

const carCabinMaterial = new THREE.MeshStandardMaterial({ color: 0x25313b, roughness: 0.46, metalness: 0.12 });

const materials = new Map([
  [KIND_BOX, new THREE.MeshStandardMaterial({ map: boxTexture, roughness: 0.76, metalness: 0.04 })],
  [KIND_WOOD, new THREE.MeshStandardMaterial({ map: woodTexture, roughness: 0.68, metalness: 0.02 })],
  [KIND_PROJECTILE, new THREE.MeshStandardMaterial({ color: 0xffe6a3, roughness: 0.38, metalness: 0.22, emissive: 0x4b3512, emissiveIntensity: 0.25 })],
  [KIND_FLOOR, new THREE.MeshStandardMaterial({ map: floorTexture, roughness: 0.9, metalness: 0 })],
  [KIND_WALL, new THREE.MeshStandardMaterial({ map: wallTexture, roughness: 0.82, metalness: 0 })],
  [KIND_CHAIN_BALL, new THREE.MeshStandardMaterial({ color: 0x9d2f28, roughness: 0.42, metalness: 0.38 })],
  [KIND_CHAIN_LINK, new THREE.MeshStandardMaterial({ color: 0x46515a, roughness: 0.36, metalness: 0.58 })],
  [KIND_CHAIN_FRAME, new THREE.MeshStandardMaterial({ color: 0x30383d, roughness: 0.62, metalness: 0.34 })],
  [KIND_TUMBLER_WALL, new THREE.MeshStandardMaterial({ color: 0x6aa7ad, roughness: 0.34, metalness: 0.18, transparent: true, opacity: 0.48, depthWrite: false })],
  [KIND_TUMBLER_BALL, new THREE.MeshStandardMaterial({ color: 0xd8ba55, roughness: 0.48, metalness: 0.08 })],
  [KIND_TUMBLER_BOX, new THREE.MeshStandardMaterial({ color: 0x8f6ec7, roughness: 0.6, metalness: 0.06 })],
  [KIND_TERRAIN, new THREE.MeshStandardMaterial({ map: terrainTexture, roughness: 0.94, metalness: 0 })],
  [KIND_CAR_CHASSIS, new THREE.MeshStandardMaterial({ color: 0xb7352d, roughness: 0.42, metalness: 0.18 })],
  [KIND_CAR_WHEEL, new THREE.MeshStandardMaterial({ color: 0x191b1d, roughness: 0.78, metalness: 0.08 })],
  [KIND_BARREL, new THREE.MeshStandardMaterial({ map: woodTexture, color: 0x8e6240, roughness: 0.74, metalness: 0.04 })],
]);

function resize() {
  const width = Math.max(1, window.innerWidth);
  const height = Math.max(1, window.innerHeight);
  camera.aspect = width / height;
  camera.updateProjectionMatrix();
  renderer.setSize(width, height, false);
}

function requestPlay() {
  const lockRequest = canvas.requestPointerLock();
  if (lockRequest && typeof lockRequest.catch === "function") {
    lockRequest.catch(() => {});
  }
}

function resetDemo() {
  if (!wasm) {
    return;
  }
  wasm._demo_reset();
  setDriving(false);
  yaw = 0;
  pitch = 0;
  camera.rotation.set(pitch, yaw, 0);
  lastShotAt = 0;
}

function isSphereKind(kind) {
  return kind === KIND_PROJECTILE || kind === KIND_CHAIN_BALL || kind === KIND_CHAIN_LINK || kind === KIND_TUMBLER_BALL;
}

function isCylinderKind(kind) {
  return kind === KIND_CAR_WHEEL || kind === KIND_BARREL;
}

function geometryFor(kind, sx, sy, sz) {
  if (isSphereKind(kind)) {
    return new THREE.SphereGeometry(Math.max(0.02, sx), 18, 12);
  }
  if (isCylinderKind(kind)) {
    return new THREE.CylinderGeometry(Math.max(0.02, sx), Math.max(0.02, sx), Math.max(0.04, sy * 2), 24, 1);
  }
  return new THREE.BoxGeometry(Math.max(0.02, sx), Math.max(0.02, sy), Math.max(0.02, sz));
}

function disposeObject(object) {
  object.traverse((child) => {
    if (child.geometry) {
      child.geometry.dispose();
    }
  });
}

function decorateMesh(mesh, kind) {
  if (kind === KIND_CAR_CHASSIS) {
    const cabin = new THREE.Mesh(new THREE.BoxGeometry(1.25, 0.62, 1.14), carCabinMaterial);
    cabin.position.set(0, 0.53, -0.18);
    cabin.castShadow = true;
    cabin.receiveShadow = true;
    mesh.add(cabin);
  }
}

function ensureMesh(index, kind, sx, sy, sz) {
  const signature = `${kind}:${sx.toFixed(3)}:${sy.toFixed(3)}:${sz.toFixed(3)}`;
  let mesh = meshes[index];
  if (mesh && mesh.userData.signature === signature) {
    return mesh;
  }

  if (mesh) {
    scene.remove(mesh);
    disposeObject(mesh);
  }

  mesh = new THREE.Mesh(geometryFor(kind, sx, sy, sz), materials.get(kind) || materials.get(KIND_BOX));
  mesh.castShadow = kind !== KIND_FLOOR;
  mesh.receiveShadow = true;
  mesh.userData.signature = signature;
  decorateMesh(mesh, kind);
  meshes[index] = mesh;
  scene.add(mesh);
  return mesh;
}

function updateMeshes() {
  const count = wasm._demo_render_count();
  const ptr = wasm._demo_render_data();
  const data = wasm.HEAPF32.subarray(ptr >> 2, (ptr >> 2) + count * renderStride);
  let carWheelIndex = 0;
  let carTransformFresh = false;
  for (let i = 0; i < count; i += 1) {
    const base = i * renderStride;
    const active = data[base] > 0.5;
    const kind = data[base + 1] | 0;
    const sx = data[base + 9];
    const sy = data[base + 10];
    const sz = data[base + 11];
    const mesh = ensureMesh(i, kind, sx, sy, sz);
    mesh.visible = active;
    if (!active) {
      continue;
    }
    mesh.position.set(data[base + 2], data[base + 3], data[base + 4]);
    mesh.quaternion.set(data[base + 5], data[base + 6], data[base + 7], data[base + 8]);
    if (kind === KIND_CAR_WHEEL) {
      if (!carTransformFresh) {
        readCarTransform();
        carTransformFresh = true;
      }
      applyCarWheelVisual(mesh, carWheelIndex);
      carWheelIndex += 1;
    }
  }
  for (let i = count; i < meshes.length; i += 1) {
    if (meshes[i]) {
      meshes[i].visible = false;
    }
  }
}

function movementInput() {
  let right = 0;
  let forward = 0;
  if (keys.has("KeyA")) right -= 1;
  if (keys.has("KeyD")) right += 1;
  if (keys.has("KeyW")) forward += 1;
  if (keys.has("KeyS")) forward -= 1;
  return { right, forward };
}

function vehicleInput() {
  let throttle = 0;
  let steer = 0;
  if (keys.has("ArrowUp")) throttle += 1;
  if (keys.has("ArrowDown")) throttle -= 1;
  if (keys.has("ArrowLeft")) steer += 1;
  if (keys.has("ArrowRight")) steer -= 1;
  return { throttle, steer };
}

function setDriving(value) {
  driving = value;
  document.body.classList.toggle("driving", driving);
  mouseDown = false;
}

function readCarTransform() {
  carPosition.set(wasm._demo_car_x(), wasm._demo_car_y(), wasm._demo_car_z());
  carQuaternion.set(wasm._demo_car_qx(), wasm._demo_car_qy(), wasm._demo_car_qz(), wasm._demo_car_qw());
  carSteerAngle = wasm._demo_car_steer();
  carWheelRoll = wasm._demo_car_wheel_roll();
  carForward.set(0, 0, -1).applyQuaternion(carQuaternion);
  carForward.y = 0;
  if (carForward.lengthSq() < 0.000001) {
    carForward.set(0, 0, -1);
  } else {
    carForward.normalize();
  }
}

function applyCarWheelVisual(mesh, wheelIndex) {
  const offset = carWheelLocalOffsets[wheelIndex];
  if (!offset) {
    return;
  }

  carWheelWorldOffset.copy(offset).applyQuaternion(carQuaternion);
  mesh.position.copy(carPosition).add(carWheelWorldOffset);
  carWheelRollQuaternion.setFromAxisAngle(localYAxis, carWheelRoll);
  mesh.quaternion.copy(carQuaternion);
  if (wheelIndex < 2) {
    carWheelSteerQuaternion.setFromAxisAngle(localYAxis, carSteerAngle);
    mesh.quaternion.multiply(carWheelSteerQuaternion);
  }
  mesh.quaternion.multiply(carWheelBaseQuaternion).multiply(carWheelRollQuaternion);
}

function syncYawToCar() {
  readCarTransform();
  yaw = Math.atan2(-carForward.x, -carForward.z);
  pitch = 0;
  camera.rotation.set(pitch, yaw, 0);
}

function toggleCar() {
  if (!wasm) {
    return;
  }
  if (driving) {
    wasm._demo_car_exit();
    syncYawToCar();
    setDriving(false);
    return;
  }

  const canEnter = wasm._demo_car_can_enter(
    wasm._demo_player_x(),
    wasm._demo_player_y(),
    wasm._demo_player_z(),
  );
  if (canEnter) {
    readCarTransform();
    camera.position.copy(carPosition).addScaledVector(carForward, -6.0).add(carEnterCameraLift);
    setDriving(true);
  }
}

function shoot(now) {
  if (!wasm || driving || now - lastShotAt < 105) {
    return;
  }
  lastShotAt = now;
  camera.getWorldDirection(shotDirection);
  wasm._demo_shoot_projectile(
    camera.position.x,
    camera.position.y,
    camera.position.z,
    shotDirection.x,
    shotDirection.y,
    shotDirection.z,
  );
}

function shootFrom(x, y, z, dx, dy, dz) {
  if (!wasm) {
    return false;
  }
  return Boolean(wasm._demo_shoot_projectile(x, y, z, dx, dy, dz));
}

function updateCamera(dt) {
  if (driving) {
    const { throttle, steer } = vehicleInput();
    wasm._demo_drive_car(throttle, steer, dt);
    readCarTransform();
    carLookTarget.copy(carPosition).addScaledVector(carForward, 3.0).add(carLookLift);
    carCameraTarget.copy(carPosition).addScaledVector(carForward, -6.2).add(carCameraLift);
    const follow = 1 - Math.pow(0.001, Math.min(0.05, dt) * 4.5);
    camera.position.lerp(carCameraTarget, follow);
    camera.lookAt(carLookTarget);
    return;
  }

  const { right, forward } = movementInput();
  if (document.pointerLockElement === canvas) {
    wasm._demo_move_player(right, forward, yaw, dt);
  }
  camera.position.set(wasm._demo_player_x(), wasm._demo_player_y() + 1.55, wasm._demo_player_z());
  camera.rotation.set(pitch, yaw, 0);
}

function updateStats(dt) {
  const instantFps = dt > 0 ? 1 / dt : 60;
  smoothedFps = smoothedFps * 0.92 + instantFps * 0.08;
  const mode = driving ? "Drive" : "Walk";
  statsEl.textContent = `FPS ${Math.round(smoothedFps)} | Mode ${mode} | Targets ${wasm._demo_toppled_targets()}/${wasm._demo_total_targets()} | Shots ${wasm._demo_active_projectiles()} | Bodies ${wasm._demo_body_count()}`;
}

function stepSimulation(seconds) {
  if (!wasm) {
    return;
  }
  const steps = Math.max(0, Math.min(600, Math.ceil(seconds * 120)));
  for (let i = 0; i < steps; i += 1) {
    wasm._demo_step(1 / 120);
  }
  updateMeshes();
  updateCamera(0);
  updateStats(1 / 60);
}

function targetSnapshot() {
  const count = wasm._demo_render_count();
  const ptr = wasm._demo_render_data();
  const data = wasm.HEAPF32.subarray(ptr >> 2, (ptr >> 2) + count * renderStride);
  const targets = [];
  for (let i = 0; i < count; i += 1) {
    const base = i * renderStride;
    const active = data[base] > 0.5;
    const kind = data[base + 1] | 0;
    if (active && (kind === KIND_BOX || kind === KIND_WOOD)) {
      targets.push([i, data[base + 2], data[base + 3], data[base + 4]]);
    }
  }
  return targets;
}

function targetMovement(before, after) {
  const afterByIndex = new Map(after.map((target) => [target[0], target]));
  let movement = 0;
  for (const target of before) {
    const next = afterByIndex.get(target[0]);
    if (!next) {
      continue;
    }
    movement += Math.abs(next[1] - target[1]);
    movement += Math.abs(next[2] - target[2]);
    movement += Math.abs(next[3] - target[3]);
  }
  return movement;
}

function runAutotestIfRequested() {
  const params = new URLSearchParams(window.location.search);
  if (!params.has("autotest")) {
    return;
  }
  const beforeTargets = targetSnapshot();
  const before = {
    activeProjectiles: wasm._demo_active_projectiles(),
    toppledTargets: wasm._demo_toppled_targets(),
    totalTargets: wasm._demo_total_targets(),
    bodyCount: wasm._demo_body_count(),
  };
  shootFrom(-5.0, 1.7, 4.2, 0.0, -0.02, -1.0);
  stepSimulation(2.2);
  const after = {
    activeProjectiles: wasm._demo_active_projectiles(),
    toppledTargets: wasm._demo_toppled_targets(),
    totalTargets: wasm._demo_total_targets(),
    bodyCount: wasm._demo_body_count(),
    targetMovement: targetMovement(beforeTargets, targetSnapshot()),
  };
  document.body.dataset.demoAutotest = JSON.stringify({ before, after });
}

function frame(now) {
  const dt = Math.min(0.05, Math.max(0, (now - lastFrameAt) / 1000));
  lastFrameAt = now;

  if (wasm) {
    if (document.pointerLockElement === canvas && mouseDown) {
      shoot(now);
    }
    updateCamera(dt);
    wasm._demo_step(dt);
    updateMeshes();
    updateStats(dt);
  }

  renderer.render(scene, camera);
  requestAnimationFrame(frame);
}

window.addEventListener("resize", resize);
playButton.addEventListener("click", requestPlay);
canvas.addEventListener("mousedown", (event) => {
  if (event.button !== 0) {
    return;
  }
  if (document.pointerLockElement !== canvas) {
    requestPlay();
    return;
  }
  mouseDown = true;
  shoot(performance.now());
});
window.addEventListener("mouseup", () => {
  mouseDown = false;
});
window.addEventListener("keydown", (event) => {
  if (event.code.startsWith("Arrow")) {
    event.preventDefault();
  }
  keys.add(event.code);
  if (event.code === "KeyE" && !event.repeat) {
    toggleCar();
  }
  if (event.code === "KeyR") {
    resetDemo();
  }
});
window.addEventListener("keyup", (event) => {
  if (event.code.startsWith("Arrow")) {
    event.preventDefault();
  }
  keys.delete(event.code);
});
window.addEventListener("mousemove", (event) => {
  if (document.pointerLockElement !== canvas) {
    return;
  }
  yaw -= event.movementX * 0.0022;
  pitch -= event.movementY * 0.0022;
  pitch = Math.max(-1.45, Math.min(1.45, pitch));
});
document.addEventListener("pointerlockchange", () => {
  document.body.classList.toggle("playing", document.pointerLockElement === canvas);
  mouseDown = false;
});

async function boot() {
  resize();
  try {
    wasm = await createCbounceDemo({
      locateFile(path) {
        return new URL(path, import.meta.url).href;
      },
    });
    renderStride = wasm._demo_render_stride();
    resetDemo();
    updateMeshes();
    updateStats(1 / 60);
    runAutotestIfRequested();
  } catch (error) {
    console.error(error);
    statsEl.textContent = "Load failed";
  }
  requestAnimationFrame(frame);
}

boot();
