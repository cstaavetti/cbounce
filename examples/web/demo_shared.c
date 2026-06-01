#include "demo_shared.h"

#include <math.h>
#include <string.h>

b3World *demo_world;
DemoRenderBody demo_entries[DEMO_MAX_RENDER];
uint32 demo_entry_count;
scalar demo_render_data_values[DEMO_MAX_RENDER * DEMO_RENDER_STRIDE];
scalar demo_player_x_value;
scalar demo_player_y_value;
scalar demo_player_z_value;
scalar demo_time_accumulator;

scalar demo_abs(scalar value) { return value < 0.0f ? -value : value; }

scalar demo_sqrt(scalar value) {
  return value > 0.0f ? sqrtf(value) : 0.0f;
}

scalar demo_clamp(scalar value, scalar low, scalar high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

b3Vec3 demo_vec3(scalar x, scalar y, scalar z) {
  return b3Vec3_Make(x, y, z);
}

b3Vec3 demo_vec3_add(b3Vec3 a, b3Vec3 b) {
  return b3Vec3_Make(a.x + b.x, a.y + b.y, a.z + b.z);
}

b3Vec3 demo_vec3_sub(b3Vec3 a, b3Vec3 b) {
  return b3Vec3_Make(a.x - b.x, a.y - b.y, a.z - b.z);
}

b3Vec3 demo_vec3_mul(b3Vec3 a, scalar value) {
  return b3Vec3_Make(a.x * value, a.y * value, a.z * value);
}

scalar demo_vec3_dot(b3Vec3 a, b3Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

scalar demo_vec3_len_sq(b3Vec3 value) {
  return demo_vec3_dot(value, value);
}

b3Vec3 demo_vec3_normalize(b3Vec3 value) {
  scalar len_sq = demo_vec3_len_sq(value);
  if (len_sq <= 0.000001f) {
    return b3Vec3_Make(0.0f, 0.0f, -1.0f);
  }
  return demo_vec3_mul(value, 1.0f / sqrtf(len_sq));
}

b3Vec3 demo_vec3_rotate_z(b3Vec3 value, scalar angle) {
  scalar s = sinf(angle);
  scalar c = cosf(angle);
  return demo_vec3(value.x * c - value.y * s, value.x * s + value.y * c,
                   value.z);
}

void demo_zero_state(void) {
  demo_world = NULL;
  memset(demo_entries, 0, sizeof(demo_entries));
  memset(demo_render_data_values, 0, sizeof(demo_render_data_values));
  demo_entry_count = 0u;
  demo_player_x_value = 0.0f;
  demo_player_y_value = 0.0f;
  demo_player_z_value = 7.5f;
  demo_time_accumulator = 0.0f;
}

void demo_destroy_world(void) {
  if (demo_world != NULL) {
    b3World_Destroy(demo_world);
  }
  demo_zero_state();
}

DemoRenderBody *demo_alloc_entry(void) {
  if (demo_entry_count >= DEMO_MAX_RENDER) {
    return NULL;
  }
  DemoRenderBody *entry = &demo_entries[demo_entry_count++];
  memset(entry, 0, sizeof(*entry));
  return entry;
}

b3Body *demo_create_box_body(int kind, b3BodyType type, bool target,
                             b3Vec3 position, b3Quat orientation, scalar sx,
                             scalar sy, scalar sz, scalar density,
                             scalar friction, scalar restitution) {
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
  body_def.type = type;
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
  fixture_def.density = type == e_dynamicBody ? density : 0.0f;
  fixture_def.friction = friction;
  fixture_def.restitution = restitution;
  b3Fixture *fixture = b3Body_CreateFixture(body, &fixture_def);
  if (fixture != NULL) {
    b3Fixture_SetUserData(fixture, entry);
  }

  entry->body = body;
  return body;
}

b3Body *demo_create_box(int kind, bool dynamic, bool target, b3Vec3 position,
                        b3Quat orientation, scalar sx, scalar sy, scalar sz,
                        scalar density, scalar friction, scalar restitution) {
  return demo_create_box_body(kind, dynamic ? e_dynamicBody : e_staticBody,
                              target, position, orientation, sx, sy, sz,
                              density, friction, restitution);
}

b3Body *demo_create_sphere_body(int kind, b3BodyType type, bool target,
                                b3Vec3 position, scalar radius,
                                scalar density, scalar friction,
                                scalar restitution) {
  DemoRenderBody *entry = demo_alloc_entry();
  if (entry == NULL || demo_world == NULL) {
    return NULL;
  }

  entry->sx = radius;
  entry->sy = radius;
  entry->sz = radius;
  entry->kind = kind;
  entry->active = true;
  entry->target = target;

  b3BodyDef body_def;
  b3BodyDef_Default(&body_def);
  body_def.type = type;
  body_def.position = position;
  body_def.userData = entry;
  body_def.linearDamping = demo_vec3(0.02f, 0.02f, 0.02f);
  body_def.angularDamping = demo_vec3(0.02f, 0.02f, 0.02f);

  b3Body *body = b3World_CreateBody(demo_world, &body_def);
  if (body == NULL) {
    entry->active = false;
    return NULL;
  }

  b3SphereShape sphere;
  b3SphereShape_Init(&sphere, radius);

  b3FixtureDef fixture_def;
  b3FixtureDef_Default(&fixture_def);
  fixture_def.shape = &sphere.base;
  fixture_def.density = type == e_dynamicBody ? density : 0.0f;
  fixture_def.friction = friction;
  fixture_def.restitution = restitution;
  b3Fixture *fixture = b3Body_CreateFixture(body, &fixture_def);
  if (fixture != NULL) {
    b3Fixture_SetUserData(fixture, entry);
  }

  entry->body = body;
  return body;
}

b3Body *demo_create_sphere(int kind, bool dynamic, bool target,
                           b3Vec3 position, scalar radius, scalar density,
                           scalar friction, scalar restitution) {
  return demo_create_sphere_body(kind, dynamic ? e_dynamicBody : e_staticBody,
                                 target, position, radius, density, friction,
                                 restitution);
}

void demo_create_arena(void) {
  demo_create_box(DEMO_KIND_FLOOR, false, false, demo_vec3(0.0f, -0.25f, -3.0f),
                  b3Quat_identity, 34.0f, 0.5f, 34.0f, 0.0f, 0.85f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(-16.75f, 2.0f, -3.0f),
                  b3Quat_identity, 0.5f, 4.0f, 34.0f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(16.75f, 2.0f, -3.0f),
                  b3Quat_identity, 0.5f, 4.0f, 34.0f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(0.0f, 2.0f, -19.75f),
                  b3Quat_identity, 34.0f, 4.0f, 0.5f, 0.0f, 0.7f, 0.02f);
  demo_create_box(DEMO_KIND_WALL, false, false, demo_vec3(0.0f, 2.0f, 13.75f),
                  b3Quat_identity, 34.0f, 4.0f, 0.5f, 0.0f, 0.7f, 0.02f);
}

void demo_update_render_data(void) {
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
