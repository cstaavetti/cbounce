#define CBOUNCE_IMPLEMENTATION
#include "cbounce.h"

#include <stdio.h>

static int check_version(void) {
  if (b3_version.major != 0u || b3_version.minor != 1u ||
      b3_version.revision != 0u) {
    fprintf(stderr, "unexpected cbounce version\n");
    return 1;
  }
  return 0;
}

static int check_quickhull(void) {
  const b3Vec3 vertices[4] = {
      {0.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 1.0f},
  };

  b3Hull *hull = b3Hull_Create(vertices, 4u, B3_LINEAR_SLOP);
  if (hull == NULL) {
    fprintf(stderr, "failed to create hull\n");
    return 1;
  }

  b3Hull_Destroy(hull);
  return 0;
}

static int run_falling_box(void) {
  b3World *world = b3World_Create();
  if (world == NULL) {
    fprintf(stderr, "failed to create world\n");
    return 1;
  }

  b3World_SetGravity(world, b3Vec3_Make(0.0f, -9.8f, 0.0f));

  b3BodyDef groundDef;
  b3BodyDef_Default(&groundDef);
  b3Body *ground = b3World_CreateBody(world, &groundDef);

  b3BoxHull groundBox;
  b3BoxHull_SetExtents(&groundBox, 10.0f, 1.0f, 10.0f);

  b3HullShape groundShape;
  b3HullShape_Init(&groundShape, &groundBox);

  b3FixtureDef groundFixture;
  b3FixtureDef_Default(&groundFixture);
  groundFixture.shape = &groundShape.base;
  b3Body_CreateFixture(ground, &groundFixture);

  b3BodyDef bodyDef;
  b3BodyDef_Default(&bodyDef);
  bodyDef.type = e_dynamicBody;
  bodyDef.position = b3Vec3_Make(0.0f, 10.0f, 0.0f);
  bodyDef.angularVelocity = b3Vec3_Make(0.0f, B3_PI, 0.0f);

  b3Body *body = b3World_CreateBody(world, &bodyDef);

  b3BoxHull bodyBox;
  b3BoxHull_SetIdentity(&bodyBox);

  b3HullShape bodyShape;
  b3HullShape_Init(&bodyShape, &bodyBox);

  b3FixtureDef bodyFixture;
  b3FixtureDef_Default(&bodyFixture);
  bodyFixture.shape = &bodyShape.base;
  bodyFixture.density = 1.0f;
  b3Body_CreateFixture(body, &bodyFixture);

  const scalar timeStep = 1.0f / 60.0f;
  const uint32 velocityIterations = 8u;
  const uint32 positionIterations = 2u;

  for (uint32 i = 0; i < 60u; ++i) {
    b3World_Step(world, timeStep, velocityIterations, positionIterations);
  }

  b3Vec3 position = b3Body_GetPosition(body);
  b3Quat orientation = b3Body_GetOrientation(body);
  b3Vec3 axis;
  scalar angle;
  b3Quat_GetAxisAngle(&orientation, &axis, &angle);

  printf("position = %.2f %.2f %.2f\n", position.x, position.y, position.z);
  printf("axis = %.2f %.2f %.2f, angle = %.2f\n", axis.x, axis.y, axis.z,
         angle);

  int failed = 0;
  if (position.y >= 10.0f) {
    fprintf(stderr, "falling box did not move downward\n");
    failed = 1;
  }

  b3World_Destroy(world);
  return failed;
}

int main(void) {
  if (check_version() != 0) {
    return 1;
  }
  if (check_quickhull() != 0) {
    return 2;
  }
  if (run_falling_box() != 0) {
    return 3;
  }

  puts("cbounce example passed");
  return 0;
}
