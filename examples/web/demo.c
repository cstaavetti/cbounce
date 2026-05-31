#define CBOUNCE_IMPLEMENTATION
#include "cbounce.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define DEMO_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define DEMO_EXPORT
#endif

#include <math.h>
#include <stdint.h>
#include <string.h>

#define DEMO_MAX_RENDER 160u
#define DEMO_MAX_PROJECTILES 24u
#define DEMO_RENDER_STRIDE 13u
#define DEMO_FIXED_DT (1.0f / 120.0f)
#define DEMO_PLAYER_RADIUS 0.34f
#define DEMO_PLAYER_HEIGHT 1.72f
#define DEMO_PLAYER_SPEED 6.2f
#define DEMO_PROJECTILE_RADIUS 0.18f
#define DEMO_PROJECTILE_SPEED 52.0f

enum DemoKind {
  DEMO_KIND_BOX = 1,
  DEMO_KIND_WOOD = 2,
  DEMO_KIND_PROJECTILE = 3,
  DEMO_KIND_FLOOR = 4,
  DEMO_KIND_WALL = 5,
};

typedef struct DemoRenderBody {
  b3Body *body;
  scalar sx;
  scalar sy;
  scalar sz;
  scalar ttl;
  int kind;
  bool active;
  bool target;
} DemoRenderBody;

static b3World *demo_world;
static DemoRenderBody demo_entries[DEMO_MAX_RENDER];
static uint32 demo_entry_count;
static uint32 demo_projectile_slots[DEMO_MAX_PROJECTILES];
static uint32 demo_projectile_slot_count;
static scalar demo_render_data_values[DEMO_MAX_RENDER * DEMO_RENDER_STRIDE];
static scalar demo_player_x_value;
static scalar demo_player_y_value;
static scalar demo_player_z_value;
static scalar demo_time_accumulator;

static scalar demo_abs(scalar value) { return value < 0.0f ? -value : value; }

static scalar demo_sqrt(scalar value) {
  return value > 0.0f ? sqrtf(value) : 0.0f;
}

static b3Vec3 demo_vec3(scalar x, scalar y, scalar z) {
  return b3Vec3_Make(x, y, z);
}

static b3Vec3 demo_vec3_add(b3Vec3 a, b3Vec3 b) {
  return b3Vec3_Make(a.x + b.x, a.y + b.y, a.z + b.z);
}

static b3Vec3 demo_vec3_mul(b3Vec3 a, scalar value) {
  return b3Vec3_Make(a.x * value, a.y * value, a.z * value);
}

static scalar demo_vec3_len_sq(b3Vec3 value) {
  return value.x * value.x + value.y * value.y + value.z * value.z;
}

static b3Vec3 demo_vec3_normalize(b3Vec3 value) {
  scalar len_sq = demo_vec3_len_sq(value);
  if (len_sq <= 0.000001f) {
    return b3Vec3_Make(0.0f, 0.0f, -1.0f);
  }
  return demo_vec3_mul(value, 1.0f / sqrtf(len_sq));
}

static bool demo_is_projectile(const DemoRenderBody *entry) {
  return entry != NULL && entry->kind == DEMO_KIND_PROJECTILE;
}

static void demo_zero_state(void) {
  demo_world = NULL;
  memset(demo_entries, 0, sizeof(demo_entries));
  memset(demo_projectile_slots, 0, sizeof(demo_projectile_slots));
  memset(demo_render_data_values, 0, sizeof(demo_render_data_values));
  demo_entry_count = 0u;
  demo_projectile_slot_count = 0u;
  demo_player_x_value = 0.0f;
  demo_player_y_value = 0.0f;
  demo_player_z_value = 7.5f;
  demo_time_accumulator = 0.0f;
}

static void demo_destroy_world(void) {
  if (demo_world != NULL) {
    b3World_Destroy(demo_world);
  }
  demo_zero_state();
}

static DemoRenderBody *demo_alloc_entry(void) {
  if (demo_entry_count >= DEMO_MAX_RENDER) {
    return NULL;
  }
  DemoRenderBody *entry = &demo_entries[demo_entry_count++];
  memset(entry, 0, sizeof(*entry));
  return entry;
}

static b3Body *demo_create_box(int kind, bool dynamic, bool target,
                               b3Vec3 position, b3Quat orientation,
                               scalar sx, scalar sy, scalar sz,
                               scalar density, scalar friction,
                               scalar restitution) {
  DemoRenderBody *entry = demo_alloc_entry();
  if (entry == NULL || demo_world == NULL) {
    return NULL;
  }

  entry->sx = sx;
  entry->sy = sy;
  entry->sz = sz;
  entry->kind = kind;
  entry->active = true;
  entry->target = target;

  b3BodyDef body_def;
  b3BodyDef_Default(&body_def);
  body_def.type = dynamic ? e_dynamicBody : e_staticBody;
  body_def.position = position;
  body_def.orientation = orientation;
  body_def.userData = entry;
  body_def.linearDamping = demo_vec3(0.015f, 0.015f, 0.015f);
  body_def.angularDamping = demo_vec3(0.015f, 0.015f, 0.015f);

  b3Body *body = b3World_CreateBody(demo_world, &body_def);
  if (body == NULL) {
    entry->active = false;
    return NULL;
  }

  b3BoxHull box_hull;
  b3BoxHull_SetExtents(&box_hull, 0.5f * sx, 0.5f * sy, 0.5f * sz);

  b3HullShape box_shape;
  b3HullShape_Init(&box_shape, &box_hull);

  b3FixtureDef fixture_def;
  b3FixtureDef_Default(&fixture_def);
  fixture_def.shape = &box_shape.base;
  fixture_def.density = dynamic ? density : 0.0f;
  fixture_def.friction = friction;
  fixture_def.restitution = restitution;
  b3Fixture *fixture = b3Body_CreateFixture(body, &fixture_def);
  if (fixture != NULL) {
    b3Fixture_SetUserData(fixture, entry);
  }

  entry->body = body;
  return body;
}

static void demo_create_projectile_slots(void) {
  for (uint32 i = 0u; i < DEMO_MAX_PROJECTILES; ++i) {
    DemoRenderBody *entry = demo_alloc_entry();
    if (entry == NULL) {
      return;
    }
    entry->sx = DEMO_PROJECTILE_RADIUS;
    entry->sy = DEMO_PROJECTILE_RADIUS;
    entry->sz = DEMO_PROJECTILE_RADIUS;
    entry->kind = DEMO_KIND_PROJECTILE;
    entry->active = false;
    entry->target = false;
    demo_projectile_slots[demo_projectile_slot_count++] =
        (uint32)(entry - demo_entries);
  }
}

static void demo_destroy_projectile(DemoRenderBody *entry) {
  if (entry == NULL || !demo_is_projectile(entry)) {
    return;
  }
  if (demo_world != NULL && entry->body != NULL) {
    b3World_DestroyBody(demo_world, entry->body);
  }
  entry->body = NULL;
  entry->active = false;
  entry->ttl = 0.0f;
}

static void demo_add_scene(void) {
  demo_create_box(DEMO_KIND_FLOOR, false, false, demo_vec3(0.0f, -0.25f, -3.0f),
                  b3Quat_identity, 32.0f, 0.5f, 32.0f, 0.0f, 0.85f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(-15.75f, 1.0f, -3.0f),
                  b3Quat_identity, 0.5f, 2.0f, 32.0f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(15.75f, 1.0f, -3.0f),
                  b3Quat_identity, 0.5f, 2.0f, 32.0f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(0.0f, 1.0f, -18.75f),
                  b3Quat_identity, 32.0f, 2.0f, 0.5f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(0.0f, 1.0f, 12.75f),
                  b3Quat_identity, 32.0f, 2.0f, 0.5f, 0.0f, 0.7f, 0.02f);

  const scalar cube = 0.88f;
  const scalar gap = 0.91f;
  for (uint32 y = 0u; y < 7u; ++y) {
    for (uint32 x = 0u; x < 9u; ++x) {
      scalar px = -8.2f + (scalar)x * gap;
      scalar py = 0.5f * cube + (scalar)y * gap;
      scalar pz = -8.8f;
      demo_create_box(DEMO_KIND_BOX, true, true, demo_vec3(px, py, pz),
                      b3Quat_identity, cube, cube, cube, 0.85f, 0.62f, 0.03f);
    }
  }

  const scalar block_h = 0.30f;
  const scalar block_w = 0.46f;
  const scalar block_l = 1.62f;
  for (uint32 layer = 0u; layer < 14u; ++layer) {
    bool x_axis = (layer % 2u) == 0u;
    scalar py = 0.5f * block_h + (scalar)layer * (block_h + 0.018f);
    for (int i = -1; i <= 1; ++i) {
      scalar offset = (scalar)i * (block_w + 0.035f);
      scalar px = x_axis ? 5.1f : 5.1f + offset;
      scalar pz = x_axis ? -8.3f + offset : -8.3f;
      scalar sx = x_axis ? block_l : block_w;
      scalar sz = x_axis ? block_w : block_l;
      demo_create_box(DEMO_KIND_WOOD, true, true, demo_vec3(px, py, pz),
                      b3Quat_identity, sx, block_h, sz, 0.75f, 0.72f, 0.02f);
    }
  }

  demo_create_projectile_slots();
}

static bool demo_player_filter(const b3Body *body, const b3Fixture *fixture,
                               void *context) {
  (void)fixture;
  (void)context;
  if (body == NULL) {
    return false;
  }
  DemoRenderBody *entry = (DemoRenderBody *)b3Body_GetUserData(body);
  if (entry == NULL || !entry->active) {
    return true;
  }
  if (entry->kind == DEMO_KIND_FLOOR || entry->kind == DEMO_KIND_PROJECTILE) {
    return false;
  }
  return true;
}

static void demo_apply_player_axis(b3Vec3 displacement) {
  if (demo_world == NULL || demo_vec3_len_sq(displacement) <= 0.0000001f) {
    return;
  }

  b3Vec3 origin =
      demo_vec3(demo_player_x_value, demo_player_y_value, demo_player_z_value);
  b3ShapeCastOutput hit;
  memset(&hit, 0, sizeof(hit));
  if (b3World_SweepCapsuleClosestWithFilter(
          demo_world, origin, DEMO_PLAYER_RADIUS, DEMO_PLAYER_HEIGHT,
          displacement, NULL, demo_player_filter, NULL, &hit)) {
    scalar fraction = hit.fraction - 0.015f;
    if (fraction < 0.0f) {
      fraction = 0.0f;
    }
    displacement = demo_vec3_mul(displacement, fraction);
  }

  demo_player_x_value += displacement.x;
  demo_player_z_value += displacement.z;
}

static void demo_update_projectiles(scalar dt) {
  for (uint32 i = 0u; i < demo_projectile_slot_count; ++i) {
    DemoRenderBody *entry = &demo_entries[demo_projectile_slots[i]];
    if (!entry->active || entry->body == NULL) {
      continue;
    }
    entry->ttl -= dt;
    b3Vec3 pos = b3Body_GetPosition(entry->body);
    if (entry->ttl <= 0.0f || pos.y < -4.0f || demo_abs(pos.x) > 18.0f ||
        pos.z < -21.0f || pos.z > 15.0f) {
      demo_destroy_projectile(entry);
    }
  }
}

static void demo_update_render_data(void) {
  for (uint32 i = 0u; i < demo_entry_count; ++i) {
    DemoRenderBody *entry = &demo_entries[i];
    uint32 base = i * DEMO_RENDER_STRIDE;
    demo_render_data_values[base + 0u] = entry->active ? 1.0f : 0.0f;
    demo_render_data_values[base + 1u] = (scalar)entry->kind;
    demo_render_data_values[base + 2u] = 0.0f;
    demo_render_data_values[base + 3u] = 0.0f;
    demo_render_data_values[base + 4u] = 0.0f;
    demo_render_data_values[base + 5u] = 0.0f;
    demo_render_data_values[base + 6u] = 0.0f;
    demo_render_data_values[base + 7u] = 0.0f;
    demo_render_data_values[base + 8u] = 1.0f;
    demo_render_data_values[base + 9u] = entry->sx;
    demo_render_data_values[base + 10u] = entry->sy;
    demo_render_data_values[base + 11u] = entry->sz;
    demo_render_data_values[base + 12u] = (scalar)i;
    if (!entry->active || entry->body == NULL) {
      continue;
    }
    b3Vec3 pos = b3Body_GetPosition(entry->body);
    b3Quat q = b3Body_GetOrientation(entry->body);
    demo_render_data_values[base + 2u] = pos.x;
    demo_render_data_values[base + 3u] = pos.y;
    demo_render_data_values[base + 4u] = pos.z;
    demo_render_data_values[base + 5u] = q.v.x;
    demo_render_data_values[base + 6u] = q.v.y;
    demo_render_data_values[base + 7u] = q.v.z;
    demo_render_data_values[base + 8u] = q.s;
  }
}

DEMO_EXPORT int demo_reset(void) {
  demo_destroy_world();
  demo_world = b3World_Create();
  if (demo_world == NULL) {
    demo_zero_state();
    return 0;
  }

  b3World_SetGravity(demo_world, demo_vec3(0.0f, -9.8f, 0.0f));
  b3World_SetContinuousCollision(demo_world, true);
  b3World_SetSleeping(demo_world, true);

  demo_player_x_value = 0.0f;
  demo_player_y_value = 0.03f;
  demo_player_z_value = 8.0f;
  demo_add_scene();
  demo_update_render_data();
  return 1;
}

DEMO_EXPORT void demo_step(scalar dt) {
  if (demo_world == NULL) {
    if (!demo_reset()) {
      return;
    }
  }
  if (dt < 0.0f) {
    dt = 0.0f;
  }
  if (dt > 0.05f) {
    dt = 0.05f;
  }

  demo_time_accumulator += dt;
  uint32 steps = 0u;
  while (demo_time_accumulator >= DEMO_FIXED_DT && steps < 8u) {
    b3World_Step(demo_world, DEMO_FIXED_DT, 8u, 3u);
    demo_update_projectiles(DEMO_FIXED_DT);
    demo_time_accumulator -= DEMO_FIXED_DT;
    ++steps;
  }
  if (steps == 8u && demo_time_accumulator > DEMO_FIXED_DT) {
    demo_time_accumulator = DEMO_FIXED_DT;
  }
  demo_update_render_data();
}

DEMO_EXPORT void demo_move_player(scalar right, scalar forward, scalar yaw,
                                  scalar dt) {
  if (demo_world == NULL) {
    return;
  }
  if (dt < 0.0f) {
    dt = 0.0f;
  }
  if (dt > 0.05f) {
    dt = 0.05f;
  }

  scalar input_len = demo_sqrt(right * right + forward * forward);
  if (input_len > 1.0f) {
    right /= input_len;
    forward /= input_len;
  }

  scalar sin_yaw = sinf(yaw);
  scalar cos_yaw = cosf(yaw);
  scalar dx = (right * cos_yaw - forward * sin_yaw) * DEMO_PLAYER_SPEED * dt;
  scalar dz = (-right * sin_yaw - forward * cos_yaw) * DEMO_PLAYER_SPEED * dt;

  demo_apply_player_axis(demo_vec3(dx, 0.0f, 0.0f));
  demo_apply_player_axis(demo_vec3(0.0f, 0.0f, dz));
}

DEMO_EXPORT int demo_shoot_projectile(scalar x, scalar y, scalar z, scalar dx,
                                      scalar dy, scalar dz) {
  if (demo_world == NULL) {
    return 0;
  }

  DemoRenderBody *entry = NULL;
  for (uint32 i = 0u; i < demo_projectile_slot_count; ++i) {
    DemoRenderBody *candidate = &demo_entries[demo_projectile_slots[i]];
    if (!candidate->active) {
      entry = candidate;
      break;
    }
  }
  if (entry == NULL) {
    entry = &demo_entries[demo_projectile_slots[0u]];
    demo_destroy_projectile(entry);
  }

  b3Vec3 direction = demo_vec3_normalize(demo_vec3(dx, dy, dz));
  b3Vec3 position =
      demo_vec3_add(demo_vec3(x, y, z), demo_vec3_mul(direction, 0.45f));

  b3BodyDef body_def;
  b3BodyDef_Default(&body_def);
  body_def.type = e_dynamicBody;
  body_def.position = position;
  body_def.linearVelocity = demo_vec3_mul(direction, DEMO_PROJECTILE_SPEED);
  body_def.userData = entry;
  body_def.allowSleep = false;
  body_def.linearDamping = demo_vec3(0.0f, 0.0f, 0.0f);
  body_def.angularDamping = demo_vec3(0.0f, 0.0f, 0.0f);

  b3Body *body = b3World_CreateBody(demo_world, &body_def);
  if (body == NULL) {
    return 0;
  }
  b3Body_SetGravityScale(body, demo_vec3(0.0f, 0.0f, 0.0f));
  b3Body_SetMaxLinearVelocity(body, 90.0f);

  b3SphereShape sphere;
  b3SphereShape_Init(&sphere, DEMO_PROJECTILE_RADIUS);

  b3FixtureDef fixture_def;
  b3FixtureDef_Default(&fixture_def);
  fixture_def.shape = &sphere.base;
  fixture_def.density = 150.0f;
  fixture_def.friction = 0.25f;
  fixture_def.restitution = 0.08f;
  b3Fixture *fixture = b3Body_CreateFixture(body, &fixture_def);
  if (fixture != NULL) {
    b3Fixture_SetUserData(fixture, entry);
  }

  entry->body = body;
  entry->active = true;
  entry->ttl = 3.5f;
  demo_update_render_data();
  return 1;
}

DEMO_EXPORT int demo_render_count(void) {
  demo_update_render_data();
  return (int)demo_entry_count;
}

DEMO_EXPORT int demo_render_stride(void) { return (int)DEMO_RENDER_STRIDE; }

DEMO_EXPORT uintptr_t demo_render_data(void) {
  demo_update_render_data();
  return (uintptr_t)demo_render_data_values;
}

DEMO_EXPORT scalar demo_player_x(void) { return demo_player_x_value; }
DEMO_EXPORT scalar demo_player_y(void) { return demo_player_y_value; }
DEMO_EXPORT scalar demo_player_z(void) { return demo_player_z_value; }

DEMO_EXPORT int demo_active_projectiles(void) {
  int count = 0;
  for (uint32 i = 0u; i < demo_projectile_slot_count; ++i) {
    if (demo_entries[demo_projectile_slots[i]].active) {
      ++count;
    }
  }
  return count;
}

DEMO_EXPORT int demo_total_targets(void) {
  int count = 0;
  for (uint32 i = 0u; i < demo_entry_count; ++i) {
    if (demo_entries[i].target) {
      ++count;
    }
  }
  return count;
}

DEMO_EXPORT int demo_toppled_targets(void) {
  int count = 0;
  for (uint32 i = 0u; i < demo_entry_count; ++i) {
    DemoRenderBody *entry = &demo_entries[i];
    if (!entry->target || !entry->active || entry->body == NULL) {
      continue;
    }
    b3Vec3 pos = b3Body_GetPosition(entry->body);
    b3Quat q = b3Body_GetOrientation(entry->body);
    if (pos.y < -0.10f || demo_abs(q.v.x) > 0.22f ||
        demo_abs(q.v.z) > 0.22f) {
      ++count;
    }
  }
  return count;
}

DEMO_EXPORT int demo_body_count(void) {
  return demo_world != NULL ? (int)b3World_GetBodyCount(demo_world) : 0;
}
