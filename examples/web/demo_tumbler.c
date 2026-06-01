#include "demo_shared.h"

#define DEMO_TUMBLER_WALL_COUNT 6u

typedef struct DemoTumblerWall {
  DemoRenderBody *entry;
  b3Vec3 local_offset;
} DemoTumblerWall;

static DemoTumblerWall demo_tumbler_walls[DEMO_TUMBLER_WALL_COUNT];
static uint32 demo_tumbler_wall_count;
static scalar demo_tumbler_angle;

static b3Vec3 demo_tumbler_center(void) {
  return demo_vec3(8.8f, 1.65f, 5.0f);
}

static void demo_tumbler_add_wall(b3Vec3 local_offset, scalar sx, scalar sy,
                                  scalar sz) {
  if (demo_tumbler_wall_count >= DEMO_TUMBLER_WALL_COUNT) {
    return;
  }

  b3Vec3 position = demo_vec3_add(demo_tumbler_center(), local_offset);
  b3Body *body = demo_create_box_body(DEMO_KIND_TUMBLER_WALL, e_kinematicBody,
                                      false, position, b3Quat_identity, sx, sy,
                                      sz, 0.0f, 0.82f, 0.02f);
  if (body == NULL) {
    return;
  }

  DemoTumblerWall *wall = &demo_tumbler_walls[demo_tumbler_wall_count++];
  wall->entry = (DemoRenderBody *)b3Body_GetUserData(body);
  wall->local_offset = local_offset;
}

void demo_tumbler_create(void) {
  demo_tumbler_wall_count = 0u;
  demo_tumbler_angle = 0.0f;

  demo_tumbler_add_wall(demo_vec3(0.0f, -0.92f, 0.0f), 2.9f, 0.18f, 1.8f);
  demo_tumbler_add_wall(demo_vec3(0.0f, 0.92f, 0.0f), 2.9f, 0.18f, 1.8f);
  demo_tumbler_add_wall(demo_vec3(-1.45f, 0.0f, 0.0f), 0.18f, 1.9f, 1.8f);
  demo_tumbler_add_wall(demo_vec3(1.45f, 0.0f, 0.0f), 0.18f, 1.9f, 1.8f);
  demo_tumbler_add_wall(demo_vec3(0.0f, 0.0f, -0.9f), 2.9f, 1.9f, 0.18f);
  demo_tumbler_add_wall(demo_vec3(0.0f, 0.0f, 0.9f), 2.9f, 1.9f, 0.18f);

  b3Vec3 center = demo_tumbler_center();
  for (uint32 i = 0u; i < 9u; ++i) {
    scalar px = center.x - 0.84f + (scalar)(i % 3u) * 0.42f;
    scalar py = center.y - 0.42f + (scalar)(i / 3u) * 0.36f;
    scalar pz = center.z - 0.42f + (scalar)(i % 2u) * 0.34f;
    b3Body *ball = demo_create_sphere(DEMO_KIND_TUMBLER_BALL, true, false,
                                      demo_vec3(px, py, pz), 0.17f, 0.7f,
                                      0.45f, 0.10f);
    if (ball != NULL) {
      b3Body_SetMaxLinearVelocity(ball, 24.0f);
    }
  }

  for (uint32 i = 0u; i < 9u; ++i) {
    scalar px = center.x + 0.12f + (scalar)(i % 3u) * 0.36f;
    scalar py = center.y - 0.34f + (scalar)(i / 3u) * 0.32f;
    scalar pz = center.z - 0.36f + (scalar)((i + 1u) % 2u) * 0.32f;
    b3Body *box = demo_create_box(DEMO_KIND_TUMBLER_BOX, true, false,
                                  demo_vec3(px, py, pz), b3Quat_identity,
                                  0.28f, 0.28f, 0.28f, 0.55f, 0.52f, 0.05f);
    if (box != NULL) {
      b3Body_SetMaxLinearVelocity(box, 24.0f);
    }
  }
}

void demo_tumbler_step(scalar dt) {
  if (demo_world == NULL || demo_tumbler_wall_count == 0u) {
    return;
  }

  const scalar speed = 0.82f;
  demo_tumbler_angle += speed * dt;
  if (demo_tumbler_angle > 2.0f * B3_PI) {
    demo_tumbler_angle -= 2.0f * B3_PI;
  }

  b3Vec3 center = demo_tumbler_center();
  b3Quat rotation = b3Quat_RotationZ(demo_tumbler_angle);
  for (uint32 i = 0u; i < demo_tumbler_wall_count; ++i) {
    DemoTumblerWall *wall = &demo_tumbler_walls[i];
    if (wall->entry == NULL || wall->entry->body == NULL) {
      continue;
    }
    b3Vec3 offset = demo_vec3_rotate_z(wall->local_offset, demo_tumbler_angle);
    b3Vec3 position = demo_vec3_add(center, offset);
    b3Vec3 linear_velocity = demo_vec3(-speed * offset.y, speed * offset.x,
                                       0.0f);
    b3Body_SetTransform(wall->entry->body, position, rotation);
    b3Body_SetLinearVelocity(wall->entry->body, linear_velocity);
    b3Body_SetAngularVelocity(wall->entry->body, demo_vec3(0.0f, 0.0f, speed));
    b3Body_SetAwake(wall->entry->body, true);
  }
}
