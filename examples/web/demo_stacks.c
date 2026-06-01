#include "demo_shared.h"

void demo_stacks_create(void) {
  const scalar cube = 0.88f;
  const scalar gap = 0.91f;
  for (uint32 y = 0u; y < 7u; ++y) {
    for (uint32 x = 0u; x < 9u; ++x) {
      scalar px = -8.4f + (scalar)x * gap;
      scalar py = 0.5f * cube + (scalar)y * gap;
      scalar pz = -8.8f;
      demo_create_box(DEMO_KIND_BOX, true, true, demo_vec3(px, py, pz),
                      b3Quat_identity, cube, cube, cube, 0.85f, 0.62f, 0.03f);
    }
  }

  const scalar tower_cube = 0.76f;
  for (uint32 stack = 0u; stack < 4u; ++stack) {
    for (uint32 level = 0u; level < 6u; ++level) {
      scalar px = -11.5f + (scalar)stack * 1.05f;
      scalar py = 0.5f * tower_cube + (scalar)level * (tower_cube + 0.025f);
      scalar pz = -2.8f + (scalar)(stack & 1u) * 0.55f;
      demo_create_box(DEMO_KIND_BOX, true, true, demo_vec3(px, py, pz),
                      b3Quat_identity, tower_cube, tower_cube, tower_cube,
                      0.72f, 0.68f, 0.03f);
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
}
