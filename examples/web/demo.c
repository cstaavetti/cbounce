#define CBOUNCE_IMPLEMENTATION
#include "demo_shared.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define DEMO_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define DEMO_EXPORT
#endif

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

  demo_player_x_value = 2.6f;
  demo_player_y_value = 0.03f;
  demo_player_z_value = 17.8f;

  demo_create_arena();
  demo_vehicle_create();
  demo_stacks_create();
  demo_chain_create();
  demo_tumbler_create();
  demo_fps_init_projectiles();
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
    demo_tumbler_step(DEMO_FIXED_DT);
    b3World_Step(demo_world, DEMO_FIXED_DT, 12u, 8u);
    demo_fps_update_projectiles(DEMO_FIXED_DT);
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
  demo_fps_move_player(right, forward, yaw, dt);
}

DEMO_EXPORT int demo_shoot_projectile(scalar x, scalar y, scalar z, scalar dx,
                                      scalar dy, scalar dz) {
  return demo_fps_shoot_projectile(x, y, z, dx, dy, dz);
}

DEMO_EXPORT void demo_drive_car(scalar throttle, scalar steer, scalar dt) {
  demo_vehicle_drive(throttle, steer, dt);
}

DEMO_EXPORT int demo_car_can_enter(scalar x, scalar y, scalar z) {
  return demo_vehicle_can_enter(demo_vec3(x, y, z));
}

DEMO_EXPORT int demo_car_exit(void) { return demo_vehicle_exit_to_player(); }

DEMO_EXPORT scalar demo_car_x(void) { return demo_vehicle_position().x; }
DEMO_EXPORT scalar demo_car_y(void) { return demo_vehicle_position().y; }
DEMO_EXPORT scalar demo_car_z(void) { return demo_vehicle_position().z; }
DEMO_EXPORT scalar demo_car_qx(void) { return demo_vehicle_orientation().v.x; }
DEMO_EXPORT scalar demo_car_qy(void) { return demo_vehicle_orientation().v.y; }
DEMO_EXPORT scalar demo_car_qz(void) { return demo_vehicle_orientation().v.z; }
DEMO_EXPORT scalar demo_car_qw(void) { return demo_vehicle_orientation().s; }
DEMO_EXPORT scalar demo_car_steer(void) { return demo_vehicle_steer_angle(); }
DEMO_EXPORT scalar demo_car_wheel_roll(void) {
  return demo_vehicle_wheel_roll();
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
  return demo_fps_active_projectiles();
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
