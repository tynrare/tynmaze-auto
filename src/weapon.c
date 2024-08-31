#include "weapon.h"
#include "main.h"
#include "tynmath.h"
#include <math.h>
#include <raylib.h>

static Model model = {0};
static Model model_gun = {0};
static Model model_sword = {0};
static Camera camera = {0};

typedef enum WEAPON_TYPE {
  WEAPON_TYPE_SWORD = 0,
  WEAPON_TYPE_GUN,
  __WEAPON_TYPES_COUNT
} WEAPON_TYPE;

static WEAPON_TYPE weapon_type = WEAPON_TYPE_SWORD;

void set_weapon(WEAPON_TYPE type) {
  switch (type) {
  default:
  case WEAPON_TYPE_SWORD:
    model = model_sword;
    break;
  case WEAPON_TYPE_GUN:
    model = model_gun;
    break;
  }

  weapon_type = type;
}

void init_weapon() {
  model_gun = LoadModel(RES_PATH "models/gun.glb");
  model_sword = LoadModel(RES_PATH "models/sword.glb");

  float roty = 0.4;
  camera.position = (Vector3){1.0f, 0.8f, 2.0f}; // Camera position
  camera.target =
      (Vector3){0.0f - roty, 0.4f, -2.0f}; // Camera looking at point
  camera.up = (Vector3){0.0f, 1.0f,
                        -0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 1.0f;           // Camera field-of-view Y
  camera.projection = CAMERA_ORTHOGRAPHIC;

  set_weapon(WEAPON_TYPE_SWORD);
}

void swap_weapon() { set_weapon((weapon_type + 1) % __WEAPON_TYPES_COUNT); }

static float weapon_rot_f_target = 0.0f;
static float weapon_rot_f_current = 0.0f;

bool is_weapon_ready() { return weapon_rot_f_current < 1e-2; }

void use_weapon() {
  if (!is_weapon_ready()) {
    return;
  }
  weapon_rot_f_target = 1.0f;
}

void update_weapon() {
  // weapon pos on screen
  camera.position.x = fminf(1.5f, (float)(viewport_w) / 800.0f * 1.6f);

  weapon_rot_f_current =
      dlerp(weapon_rot_f_current, weapon_rot_f_target, 12.0f, GetFrameTime());
  if (weapon_rot_f_target > 0 &&
      weapon_rot_f_target - weapon_rot_f_current < 1e-1) {
    weapon_rot_f_target = 0.0f;
  }
}

void draw_weapon() {
  BeginMode3D(camera);
  float rot = 0.0;
  switch (weapon_type) {
  default:
  case WEAPON_TYPE_SWORD:
    rot = sinf(GetTime() * 0.7) * 10 * (1 - weapon_rot_f_current) + 20 -
          120 * weapon_rot_f_current;
    break;
  case WEAPON_TYPE_GUN:
    rot = sinf(GetTime()) * 2 + 10 + (weapon_rot_f_current * 35);
    break;
  }
  DrawModelEx(model, Vector3Zero(), (Vector3){1.0, 0.0, 0.0}, rot, Vector3One(),
              WHITE);
  EndMode3D();
}

void dispose_weapon() { UnloadModel(model); }
