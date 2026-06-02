#include "demo_shared.h"

#include <math.h>
#include <string.h>

#define DEMO_CAR_WHEEL_COUNT 4u
#define DEMO_CAR_ENTER_RADIUS 4.3f
#define DEMO_CAR_MAX_STEER 0.62f
#define DEMO_CAR_WHEEL_RADIUS 0.46f

typedef struct DemoVehicle {
  b3Body *chassis;
  b3Body *wheels[DEMO_CAR_WHEEL_COUNT];
  b3Joint *wheel_joints[DEMO_CAR_WHEEL_COUNT];
  scalar steer_angle;
  scalar wheel_roll;
} DemoVehicle;

static DemoVehicle demo_vehicle;

static b3Vec3 demo_horizontal_normalize(b3Vec3 value, b3Vec3 fallback) {
  value.y = 0.0f;
  if (demo_vec3_len_sq(value) <= 0.000001f) {
    fallback.y = 0.0f;
    return demo_vec3_normalize(fallback);
  }
  return demo_vec3_normalize(value);
}

static b3Vec3 demo_vehicle_forward(void) {
  if (demo_vehicle.chassis == NULL) {
    return demo_vec3(0.0f, 0.0f, -1.0f);
  }
  return demo_horizontal_normalize(
      b3Body_GetWorldVector(demo_vehicle.chassis, demo_vec3(0.0f, 0.0f, -1.0f)),
      demo_vec3(0.0f, 0.0f, -1.0f));
}

static b3Vec3 demo_vehicle_right(void) {
  if (demo_vehicle.chassis == NULL) {
    return demo_vec3(1.0f, 0.0f, 0.0f);
  }
  return demo_horizontal_normalize(
      b3Body_GetWorldVector(demo_vehicle.chassis, demo_vec3(1.0f, 0.0f, 0.0f)),
      demo_vec3(1.0f, 0.0f, 0.0f));
}

static b3Joint *demo_vehicle_connect_wheel(b3Body *wheel, b3Vec3 anchor) {
  if (demo_world == NULL || demo_vehicle.chassis == NULL || wheel == NULL) {
    return NULL;
  }

  b3WheelJointDef joint_def;
  b3WheelJointDef_Default(&joint_def);
  b3WheelJointDef_Initialize(&joint_def, demo_vehicle.chassis, wheel, anchor,
                             demo_vec3(0.0f, 1.0f, 0.0f),
                             demo_vec3(1.0f, 0.0f, 0.0f));
  joint_def.enableMotor = true;
  joint_def.maxMotorTorque = 850.0f;
  joint_def.motorSpeed = 0.0f;
  joint_def.frequencyHz = 5.5f;
  joint_def.dampingRatio = 0.82f;
  joint_def.collideConnected = false;
  return b3World_CreateWheelJoint(demo_world, &joint_def);
}

void demo_vehicle_create(void) {
  memset(&demo_vehicle, 0, sizeof(demo_vehicle));

  b3Vec3 chassis_position = demo_vec3(0.0f, 0.96f, 15.0f);
  demo_vehicle.chassis =
      demo_create_box(DEMO_KIND_CAR_CHASSIS, true, false, chassis_position,
                      b3Quat_identity, 2.2f, 0.58f, 3.65f, 0.58f, 0.72f,
                      0.03f);
  if (demo_vehicle.chassis == NULL) {
    return;
  }

  b3Body_SetLinearDamping(demo_vehicle.chassis, demo_vec3(0.24f, 0.06f, 0.24f));
  b3Body_SetAngularDamping(demo_vehicle.chassis, demo_vec3(0.72f, 0.92f, 0.72f));
  b3Body_SetMaxLinearVelocity(demo_vehicle.chassis, 22.0f);
  b3Body_SetMaxAngularVelocity(demo_vehicle.chassis, 9.0f);
  b3Body_SetSleepingAllowed(demo_vehicle.chassis, false);

  const scalar wheel_x = 1.14f;
  const scalar wheel_y = 0.53f;
  const scalar wheel_z = 1.34f;
  const b3Vec3 wheel_positions[DEMO_CAR_WHEEL_COUNT] = {
      {-wheel_x, wheel_y, chassis_position.z - wheel_z},
      {wheel_x, wheel_y, chassis_position.z - wheel_z},
      {-wheel_x, wheel_y, chassis_position.z + wheel_z},
      {wheel_x, wheel_y, chassis_position.z + wheel_z},
  };
  const b3Quat wheel_orientation = b3QuatRotationZ(0.5f * B3_PI);

  for (uint32 i = 0u; i < DEMO_CAR_WHEEL_COUNT; ++i) {
    demo_vehicle.wheels[i] = demo_create_cylinder(
        DEMO_KIND_CAR_WHEEL, true, false, wheel_positions[i],
        wheel_orientation, DEMO_CAR_WHEEL_RADIUS, 0.23f, 0.72f, 1.35f,
        0.02f);
    if (demo_vehicle.wheels[i] == NULL) {
      continue;
    }
    b3Body_SetLinearDamping(demo_vehicle.wheels[i],
                            demo_vec3(0.04f, 0.04f, 0.04f));
    b3Body_SetAngularDamping(demo_vehicle.wheels[i],
                             demo_vec3(0.04f, 0.04f, 0.04f));
    b3Body_SetMaxLinearVelocity(demo_vehicle.wheels[i], 34.0f);
    b3Body_SetMaxAngularVelocity(demo_vehicle.wheels[i], 60.0f);
    b3Body_SetSleepingAllowed(demo_vehicle.wheels[i], false);
    demo_vehicle.wheel_joints[i] =
        demo_vehicle_connect_wheel(demo_vehicle.wheels[i], wheel_positions[i]);
  }
}

void demo_vehicle_drive(scalar throttle, scalar steer, scalar dt) {
  if (demo_vehicle.chassis == NULL) {
    return;
  }

  throttle = demo_clamp(throttle, -1.0f, 1.0f);
  steer = demo_clamp(steer, -1.0f, 1.0f);
  dt = demo_clamp(dt, 0.0f, 0.05f);

  scalar target_steer = steer * DEMO_CAR_MAX_STEER;
  scalar steer_rate = dt > 0.0f ? 1.0f - powf(0.001f, dt * 5.5f) : 1.0f;
  demo_vehicle.steer_angle +=
      (target_steer - demo_vehicle.steer_angle) * steer_rate;

  b3Vec3 forward = demo_vehicle_forward();
  b3Vec3 right = demo_vehicle_right();
  b3Vec3 velocity = b3Body_GetLinearVelocity(demo_vehicle.chassis);
  scalar forward_speed = demo_vec3_dot(velocity, forward);
  scalar lateral_speed = demo_vec3_dot(velocity, right);
  scalar mass = b3Body_GetMass(demo_vehicle.chassis);
  scalar steer_sin = sinf(demo_vehicle.steer_angle);
  scalar steer_cos = cosf(demo_vehicle.steer_angle);
  b3Vec3 front_direction = demo_vec3_add(demo_vec3_mul(forward, steer_cos),
                                         demo_vec3_mul(right, steer_sin));
  front_direction = demo_vec3_normalize(front_direction);

  scalar drive_force = throttle >= 0.0f ? 60.0f : 42.0f;
  b3Body_ApplyForceToCenter(demo_vehicle.chassis,
                            demo_vec3_mul(front_direction,
                                          throttle * drive_force),
                            true);
  b3Body_ApplyForceToCenter(
      demo_vehicle.chassis,
      demo_vec3_mul(right, -lateral_speed * mass * 18.0f), true);

  scalar turn_speed = demo_clamp(forward_speed, -10.0f, 10.0f);
  if (demo_abs(turn_speed) < 1.2f && demo_abs(throttle) > 0.05f) {
    turn_speed = throttle * 1.2f;
  }
  b3Body_ApplyTorque(
      demo_vehicle.chassis,
      demo_vec3(0.0f, demo_vehicle.steer_angle * turn_speed * 24.0f, 0.0f),
      true);

  scalar wheel_speed = -throttle * 15.0f;
  scalar motor_torque = demo_abs(throttle) > 0.02f ? 620.0f : 150.0f;
  demo_vehicle.wheel_roll -= forward_speed * dt / DEMO_CAR_WHEEL_RADIUS;
  for (uint32 i = 0u; i < DEMO_CAR_WHEEL_COUNT; ++i) {
    if (demo_vehicle.wheels[i] != NULL) {
      b3Body_SetAwake(demo_vehicle.wheels[i], true);
    }
    if (demo_vehicle.wheel_joints[i] == NULL) {
      continue;
    }
    b3Joint_SetEnableMotor(demo_vehicle.wheel_joints[i], true);
    b3Joint_SetMotorSpeed(demo_vehicle.wheel_joints[i], wheel_speed);
    b3Joint_SetMaxMotorTorque(demo_vehicle.wheel_joints[i], motor_torque);
  }

  (void)dt;
}

int demo_vehicle_can_enter(b3Vec3 player_position) {
  if (demo_vehicle.chassis == NULL) {
    return 0;
  }
  b3Vec3 car_position = b3Body_GetPosition(demo_vehicle.chassis);
  b3Vec3 delta = demo_vec3_sub(player_position, car_position);
  delta.y = 0.0f;
  return demo_vec3_len_sq(delta) <=
                 DEMO_CAR_ENTER_RADIUS * DEMO_CAR_ENTER_RADIUS
             ? 1
             : 0;
}

int demo_vehicle_exit_to_player(void) {
  if (demo_vehicle.chassis == NULL) {
    return 0;
  }

  demo_vehicle_drive(0.0f, 0.0f, 0.0f);
  b3Vec3 position = b3Body_GetPosition(demo_vehicle.chassis);
  b3Vec3 right = demo_vehicle_right();
  b3Vec3 exit_position = demo_vec3_add(position, demo_vec3_mul(right, 2.25f));
  exit_position.x =
      demo_clamp(exit_position.x, -DEMO_ARENA_HALF_X + 1.0f,
                 DEMO_ARENA_HALF_X - 1.0f);
  exit_position.z =
      demo_clamp(exit_position.z, -DEMO_ARENA_HALF_Z + 1.0f,
                 DEMO_ARENA_HALF_Z - 1.0f);
  demo_player_x_value = exit_position.x;
  demo_player_y_value = 0.03f;
  demo_player_z_value = exit_position.z;
  return 1;
}

b3Vec3 demo_vehicle_position(void) {
  if (demo_vehicle.chassis == NULL) {
    return demo_vec3(0.0f, 0.0f, 0.0f);
  }
  return b3Body_GetPosition(demo_vehicle.chassis);
}

b3Quat demo_vehicle_orientation(void) {
  if (demo_vehicle.chassis == NULL) {
    return b3Quat_identity;
  }
  return b3Body_GetOrientation(demo_vehicle.chassis);
}

scalar demo_vehicle_steer_angle(void) { return demo_vehicle.steer_angle; }

scalar demo_vehicle_wheel_roll(void) { return demo_vehicle.wheel_roll; }
