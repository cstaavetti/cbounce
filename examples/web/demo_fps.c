#include "demo_shared.h"

#include <math.h>
#include <string.h>

#define DEMO_PLAYER_RADIUS 0.34f
#define DEMO_PLAYER_HEIGHT 1.72f
#define DEMO_PLAYER_SPEED 6.2f
#define DEMO_PLAYER_CAST_SKIN 0.018f
#define DEMO_PLAYER_PUSH_IMPULSE 1.25f
#define DEMO_PLAYER_SLIDE_PASSES 5
#define DEMO_PLAYER_MIN_X (-DEMO_ARENA_HALF_X + DEMO_PLAYER_RADIUS + 0.28f)
#define DEMO_PLAYER_MAX_X (DEMO_ARENA_HALF_X - DEMO_PLAYER_RADIUS - 0.28f)
#define DEMO_PLAYER_MIN_Z (-DEMO_ARENA_HALF_Z + DEMO_PLAYER_RADIUS + 0.28f)
#define DEMO_PLAYER_MAX_Z (DEMO_ARENA_HALF_Z - DEMO_PLAYER_RADIUS - 0.28f)
#define DEMO_PROJECTILE_RADIUS 0.18f
#define DEMO_PROJECTILE_SPEED 42.0f

static uint32 demo_projectile_slots[DEMO_MAX_PROJECTILES];
static uint32 demo_projectile_slot_count;

static bool demo_is_projectile(const DemoRenderBody *entry) {
  return entry != NULL && entry->kind == DEMO_KIND_PROJECTILE;
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
  if (entry->kind == DEMO_KIND_FLOOR || entry->kind == DEMO_KIND_WALL ||
      entry->kind == DEMO_KIND_TERRAIN || entry->kind == DEMO_KIND_PROJECTILE) {
    return false;
  }
  return true;
}

static b3Vec3 demo_clamp_player_to_arena(b3Vec3 position) {
  position.x = demo_clamp(position.x, DEMO_PLAYER_MIN_X, DEMO_PLAYER_MAX_X);
  position.z = demo_clamp(position.z, DEMO_PLAYER_MIN_Z, DEMO_PLAYER_MAX_Z);
  return position;
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

static void demo_push_hit_body(const b3ShapeCastOutput *hit,
                               b3Vec3 desired_move) {
  if (hit == NULL || hit->body == NULL ||
      b3Body_GetType(hit->body) != e_dynamicBody) {
    return;
  }

  b3Vec3 direction = demo_vec3(desired_move.x, 0.0f, desired_move.z);
  if (demo_vec3_len_sq(direction) <= 0.000001f) {
    direction = demo_vec3(-hit->normal.x, 0.0f, -hit->normal.z);
  }
  if (demo_vec3_len_sq(direction) <= 0.000001f) {
    return;
  }

  direction = demo_vec3_normalize(direction);
  scalar mass = b3Body_GetMass(hit->body);
  scalar impulse = demo_clamp(mass * DEMO_PLAYER_PUSH_IMPULSE, 1.5f, 18.0f);
  b3Body_ApplyLinearImpulseToCenter((b3Body *)hit->body,
                                    demo_vec3_mul(direction, impulse), true);
}

static void demo_apply_player_displacement(b3Vec3 displacement) {
  if (demo_world == NULL || demo_vec3_len_sq(displacement) <= 0.0000001f) {
    return;
  }

  b3Vec3 current =
      demo_vec3(demo_player_x_value, demo_player_y_value, demo_player_z_value);
  b3Vec3 remaining = displacement;

  for (int pass = 0; pass < DEMO_PLAYER_SLIDE_PASSES; ++pass) {
    if (demo_vec3_len_sq(remaining) <= 0.0000001f) {
      break;
    }

    b3ShapeCastOutput hit;
    memset(&hit, 0, sizeof(hit));
    if (!b3World_SweepCapsuleClosestWithFilter(
            demo_world, current, DEMO_PLAYER_RADIUS, DEMO_PLAYER_HEIGHT,
            remaining, NULL, demo_player_filter, NULL, &hit)) {
      current = demo_vec3_add(current, remaining);
      current = demo_clamp_player_to_arena(current);
      break;
    }

    scalar fraction = demo_clamp(hit.fraction, 0.0f, 1.0f);
    scalar travel = fraction - DEMO_PLAYER_CAST_SKIN;
    if (travel < 0.0f) {
      travel = 0.0f;
    }
    current = demo_vec3_add(current, demo_vec3_mul(remaining, travel));
    demo_push_hit_body(&hit, remaining);

    b3Vec3 normal = demo_vec3(hit.normal.x, 0.0f, hit.normal.z);
    if (demo_vec3_len_sq(normal) <= 0.000001f) {
      break;
    }
    normal = demo_vec3_normalize(normal);
    b3Vec3 leftover = demo_vec3_mul(remaining, 1.0f - fraction);
    scalar into_surface = demo_vec3_dot(leftover, normal);
    if (fraction <= 0.001f && into_surface >= -0.00001f) {
      current = demo_vec3_add(current, leftover);
      break;
    }
    if (into_surface < 0.0f) {
      leftover = demo_vec3_sub(leftover, demo_vec3_mul(normal, into_surface));
    }
    remaining = leftover;
    current = demo_clamp_player_to_arena(current);
  }

  current = demo_clamp_player_to_arena(current);
  demo_player_x_value = current.x;
  demo_player_z_value = current.z;
}

void demo_fps_init_projectiles(void) {
  memset(demo_projectile_slots, 0, sizeof(demo_projectile_slots));
  demo_projectile_slot_count = 0u;
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

void demo_fps_update_projectiles(scalar dt) {
  for (uint32 i = 0u; i < demo_projectile_slot_count; ++i) {
    DemoRenderBody *entry = &demo_entries[demo_projectile_slots[i]];
    if (!entry->active || entry->body == NULL) {
      continue;
    }
    entry->ttl -= dt;
    b3Vec3 pos = b3Body_GetPosition(entry->body);
    if (entry->ttl <= 0.0f || pos.y < -8.0f ||
        demo_abs(pos.x) > DEMO_ARENA_HALF_X + 5.0f ||
        demo_abs(pos.z) > DEMO_ARENA_HALF_Z + 5.0f) {
      demo_destroy_projectile(entry);
    }
  }
}

void demo_fps_move_player(scalar right, scalar forward, scalar yaw, scalar dt) {
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

  demo_apply_player_displacement(demo_vec3(dx, 0.0f, dz));
}

int demo_fps_shoot_projectile(scalar x, scalar y, scalar z, scalar dx,
                              scalar dy, scalar dz) {
  if (demo_world == NULL || demo_projectile_slot_count == 0u) {
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

int demo_fps_active_projectiles(void) {
  int count = 0;
  for (uint32 i = 0u; i < demo_projectile_slot_count; ++i) {
    if (demo_entries[demo_projectile_slots[i]].active) {
      ++count;
    }
  }
  return count;
}
