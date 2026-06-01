#ifndef CBOUNCE_WEB_DEMO_SHARED_H
#define CBOUNCE_WEB_DEMO_SHARED_H

#include "cbounce.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEMO_MAX_RENDER 320u
#define DEMO_MAX_PROJECTILES 32u
#define DEMO_RENDER_STRIDE 13u
#define DEMO_FIXED_DT (1.0f / 120.0f)

enum DemoKind {
  DEMO_KIND_BOX = 1,
  DEMO_KIND_WOOD = 2,
  DEMO_KIND_PROJECTILE = 3,
  DEMO_KIND_FLOOR = 4,
  DEMO_KIND_WALL = 5,
  DEMO_KIND_CHAIN_BALL = 6,
  DEMO_KIND_CHAIN_LINK = 7,
  DEMO_KIND_CHAIN_FRAME = 8,
  DEMO_KIND_TUMBLER_WALL = 9,
  DEMO_KIND_TUMBLER_BALL = 10,
  DEMO_KIND_TUMBLER_BOX = 11,
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

extern b3World *demo_world;
extern DemoRenderBody demo_entries[DEMO_MAX_RENDER];
extern uint32 demo_entry_count;
extern scalar demo_render_data_values[DEMO_MAX_RENDER * DEMO_RENDER_STRIDE];
extern scalar demo_player_x_value;
extern scalar demo_player_y_value;
extern scalar demo_player_z_value;
extern scalar demo_time_accumulator;

scalar demo_abs(scalar value);
scalar demo_sqrt(scalar value);
scalar demo_clamp(scalar value, scalar low, scalar high);
b3Vec3 demo_vec3(scalar x, scalar y, scalar z);
b3Vec3 demo_vec3_add(b3Vec3 a, b3Vec3 b);
b3Vec3 demo_vec3_sub(b3Vec3 a, b3Vec3 b);
b3Vec3 demo_vec3_mul(b3Vec3 a, scalar value);
scalar demo_vec3_dot(b3Vec3 a, b3Vec3 b);
scalar demo_vec3_len_sq(b3Vec3 value);
b3Vec3 demo_vec3_normalize(b3Vec3 value);
b3Vec3 demo_vec3_rotate_z(b3Vec3 value, scalar angle);

void demo_zero_state(void);
void demo_destroy_world(void);
DemoRenderBody *demo_alloc_entry(void);
void demo_update_render_data(void);
void demo_create_arena(void);

b3Body *demo_create_box_body(int kind, b3BodyType type, bool target,
                             b3Vec3 position, b3Quat orientation, scalar sx,
                             scalar sy, scalar sz, scalar density,
                             scalar friction, scalar restitution);
b3Body *demo_create_box(int kind, bool dynamic, bool target, b3Vec3 position,
                        b3Quat orientation, scalar sx, scalar sy, scalar sz,
                        scalar density, scalar friction, scalar restitution);
b3Body *demo_create_sphere_body(int kind, b3BodyType type, bool target,
                                b3Vec3 position, scalar radius,
                                scalar density, scalar friction,
                                scalar restitution);
b3Body *demo_create_sphere(int kind, bool dynamic, bool target,
                           b3Vec3 position, scalar radius, scalar density,
                           scalar friction, scalar restitution);

void demo_fps_init_projectiles(void);
void demo_fps_update_projectiles(scalar dt);
void demo_fps_move_player(scalar right, scalar forward, scalar yaw, scalar dt);
int demo_fps_shoot_projectile(scalar x, scalar y, scalar z, scalar dx,
                              scalar dy, scalar dz);
int demo_fps_active_projectiles(void);

void demo_stacks_create(void);
void demo_chain_create(void);
void demo_tumbler_create(void);
void demo_tumbler_step(scalar dt);

#endif
