#include "demo_shared.h"

static void demo_connect_sphere_joint(b3Body *body_a, b3Body *body_b,
                                      b3Vec3 anchor) {
  if (demo_world == NULL || body_a == NULL || body_b == NULL) {
    return;
  }

  b3SphereJointDef joint_def;
  b3SphereJointDef_Default(&joint_def);
  b3SphereJointDef_Initialize(&joint_def, body_a, body_b, anchor);
  joint_def.collideConnected = false;
  b3World_CreateSphereJoint(demo_world, &joint_def);
}

void demo_chain_create(void) {
  const scalar x = -5.4f;
  const scalar z = 3.0f;
  const scalar anchor_y = 4.8f;
  const scalar link_gap = 0.52f;
  const uint32 link_count = 5u;

  b3Body *frame = demo_create_box(DEMO_KIND_CHAIN_FRAME, false, false,
                                  demo_vec3(x, anchor_y, z), b3Quat_identity,
                                  1.9f, 0.18f, 0.18f, 0.0f, 0.72f, 0.02f);
  demo_create_box(DEMO_KIND_CHAIN_FRAME, false, false,
                  demo_vec3(x - 0.95f, 2.45f, z), b3Quat_identity, 0.16f,
                  4.7f, 0.16f, 0.0f, 0.72f, 0.02f);
  demo_create_box(DEMO_KIND_CHAIN_FRAME, false, false,
                  demo_vec3(x + 0.95f, 2.45f, z), b3Quat_identity, 0.16f,
                  4.7f, 0.16f, 0.0f, 0.72f, 0.02f);

  b3Body *previous = frame;
  b3Vec3 previous_center = demo_vec3(x, anchor_y, z);
  for (uint32 i = 0u; i < link_count; ++i) {
    b3Vec3 center =
        demo_vec3(x, anchor_y - (scalar)(i + 1u) * link_gap, z);
    b3Body *link = demo_create_sphere(DEMO_KIND_CHAIN_LINK, true, false, center,
                                      0.13f, 1.2f, 0.55f, 0.02f);
    if (link == NULL) {
      continue;
    }
    b3Body_SetSleepingAllowed(link, false);
    b3Vec3 anchor = demo_vec3_mul(demo_vec3_add(previous_center, center), 0.5f);
    demo_connect_sphere_joint(previous, link, anchor);
    previous = link;
    previous_center = center;
  }

  b3Vec3 ball_center = demo_vec3(x, 1.22f, z);
  b3Body *ball = demo_create_sphere(DEMO_KIND_CHAIN_BALL, true, false,
                                    ball_center, 0.62f, 7.0f, 0.68f, 0.08f);
  if (ball == NULL) {
    return;
  }
  b3Body_SetSleepingAllowed(ball, false);
  b3Body_SetMaxLinearVelocity(ball, 22.0f);
  demo_connect_sphere_joint(
      previous, ball,
      demo_vec3_mul(demo_vec3_add(previous_center, ball_center), 0.5f));
  demo_connect_sphere_joint(frame, ball, demo_vec3(x, anchor_y, z));

  const scalar box = 0.58f;
  for (uint32 y = 0u; y < 5u; ++y) {
    for (uint32 x_index = 0u; x_index < 5u; ++x_index) {
      scalar px = -6.55f + (scalar)x_index * (box + 0.035f);
      scalar py = 0.5f * box + (scalar)y * (box + 0.028f);
      scalar pz = 0.58f;
      demo_create_box(DEMO_KIND_BOX, true, true, demo_vec3(px, py, pz),
                      b3Quat_identity, box, box, box, 0.64f, 0.66f, 0.03f);
    }
  }
}
