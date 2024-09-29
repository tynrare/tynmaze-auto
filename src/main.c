// @tynroar

// --- system includes
#include "main.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
// #include <GL/gl.h>
#include "rlgl.h"

#define RLIGHTS_IMPLEMENTATION
#include "external/rlights.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// --- gameplay includes
#include "weapon.h"

// --- gameplay defines

#define WALKER_RUSHES 13
#define DRAW_MODEL false
#define DRAW_UI false

// ---

void toggle_fullscreen();
bool is_toggle_fullscreen_requested();

// ---

bool active = false;

TouchPoint touch_points[MAX_TOUCH_POINTS] = {0};

int viewport_w = VIEWPORT_W;
int viewport_h = VIEWPORT_H;

static const int SAVES_HASH = 0xaa;

// ---

int min(int a, int b) { return a > b ? b : a; }

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float rlerp(float a, float b, float t) {
  float CS = (1.0f - t) * cosf(a) + t * cosf(b);
  float SN = (1.0f - t) * sinf(a) + t * sinf(b);

  return atan2f(SN, CS);
}

Vector2 Vec2Up = {0.0, 1.0};
Vector2 Vec2Right = {1.0, 0.0};

// ---

void SaveProgress(const char *key, const int value) {
#if defined(PLATFORM_WEB)
  char script[256];
  snprintf(script, sizeof(script), "saveProgress('%s', '%d');", key, value);
  emscripten_run_script(script);
#endif
}

const int LoadProgress(const char *key) {
#if defined(PLATFORM_WEB)
  char script[256];
  snprintf(script, sizeof(script), "loadProgress('%s');", key);
  return emscripten_run_script_int(script);
#else
  return 0;
#endif
}

// ---

bool _draw_help_enabled = false;

Vector3 mapPosition = {-0.0f, 0.0f, -0.0f}; // Set model position
Vector2 inputDirection = {0.0f, 0.0f};
Vector2 pawnPosition = {1.0f, 1.0f};
float playerTurn = 0.0f;
float cameraRot = 0.0f;
int steps = 0;

// --- app
Camera3D camera = {0};
Shader shader_maze = {0};

Color *mapPixels = NULL;
Color *map_walker_pixels = NULL;
Texture2D scene_texture = {0};
RenderTexture scene_render_texture = { 0 };
Texture2D texture = {0};
Texture2D tex_noise0 = {0};
Model model = {0};

// --- res
Texture pic_forwards = {0};
Texture pic_punch = {0};
Texture pic_rotate = {0};
Texture pic_rotate_left = {0};
Texture pic_rotate_right = {0};

// ---

typedef enum ACTIONS {
  ACTION_NONE = 0,
  ACTION_FORWARD,
  ACTION_BACKWARD,
  ACTION_LEFT,
  ACTION_RIGHT,
  ACTION_RUN,
  ACTION_SWAP,
  ACTION_USE,
} ACTION;

typedef enum {
  STORAGE_POSITION_SCORE = 0,
  STORAGE_POSITION_POS_X,
  STORAGE_POSITION_POS_Y
} StorageData;

Vector2 input_delta = {0};
Vector2 input_pos = {0};
Vector2 gesture_delta = {0};

static bool sui_btn_a_down = false;
static bool sui_btn_b_down = false;
static bool sui_btn_c_down = false;
static bool sui_screen_down = false;
static bool sui_btn_a_pressed = false;
static bool sui_btn_b_pressed = false;
static bool sui_btn_c_pressed = false;
static bool sui_screen_pressed = false;
static bool sui_screen_released = false;

static ACTION action_a = ACTION_NONE;
static ACTION action_b = ACTION_NONE;

static float walker_dt = 0;
static Vector2 walker_last_input = { 0 };
static void walker() {
	walker_dt += GetFrameTime();
	if (walker_dt < 0.3) {
		return;
	}

	walker_dt = 0;

	float forwardx = roundf(pawnPosition.x + sinf(playerTurn));
	float forwardy = roundf(pawnPosition.y + cosf(playerTurn));
	int forward = map_walker_pixels[(int)(forwardy)*scene_texture.width + (int)(forwardx)].r;
	float backwardx = roundf(pawnPosition.x - sinf(playerTurn));
	float backwardy = roundf(pawnPosition.y - cosf(playerTurn));
	int backward = map_walker_pixels[(int)(backwardy)*scene_texture.width + (int)(backwardx)].r;
	float leftx = roundf(pawnPosition.x + sinf(playerTurn + PI * 0.5f));
	float lefty = roundf(pawnPosition.y + cosf(playerTurn + PI * 0.5f));
	int left = map_walker_pixels[(int)(lefty)*scene_texture.width + (int)(leftx)].r;
	float rightx = roundf(pawnPosition.x + sinf(playerTurn - PI * 0.5f));
	float righty = roundf(pawnPosition.y + cosf(playerTurn - PI * 0.5f));
	int right = map_walker_pixels[(int)(righty)*scene_texture.width + (int)(rightx)].r;

	int paths = 
		(forward == 0 ? 1 : 0) + 
		(backward == 0 ? 1 : 0) + 
		(left == 0 ? 1 : 0) + 
		(right == 0 ? 1 : 0);

	int turn = 1;
	if (left < 1 && right < 1) {
		turn = left > right ? 1 : -1;
		if (left == right) {
			turn = GetRandomValue(0, 1) * 2 - 1;
		}
	} else if (left < 1) {
		turn = 1;
	} else if (right < 1) {
		turn = -1;
	}

	if (forward > 0) {
		inputDirection.y = turn; // turn left
	} else if (forward < 1 && paths <= 2) {
		inputDirection.x = 1; // go forward
	} else if (forward < 1 && paths > 2 && walker_last_input.y != 0) {
		inputDirection.x = 1; // go forward
	} else if (forward < left && forward < right) {
		inputDirection.x = 1; // go forward
	} else {
		inputDirection.y = turn; // 
	}

	walker_last_input.x = inputDirection.x;
	walker_last_input.y = inputDirection.y;

	map_walker_pixels[(int)(pawnPosition.y)*scene_texture.width + (int)(pawnPosition.x)].r += 0.01;

	if (inputDirection.x != 0 && GetRandomValue(0, WALKER_RUSHES) == 0) {
		action_b = ACTION_RUN;
	}
}

static void update() {
	walker();

  if (inputDirection.x) {
    steps += 1;
  }

  // rotate
  playerTurn += PI / 2.0f * inputDirection.y;

  // move
  int collider = 0;
  while (!collider && inputDirection.x) {
    float newx = roundf(pawnPosition.x + sinf(playerTurn) * inputDirection.x);
    float newy = roundf(pawnPosition.y + cosf(playerTurn) * inputDirection.x);
    collider = mapPixels[(int)(newy)*scene_texture.width + (int)(newx)].r;
    /*
    if(inputDirection.x != 0.0f) {
            printf("newx: %f, newy: %f, turn: %f \n", newx, newy, playerTurn);
    }
    */
    if (collider < 1) {
      pawnPosition.x = newx;
      pawnPosition.y = newy;
    }
    if (action_b != ACTION_RUN) {
      break;
    }
  }

  cameraRot = rlerp(cameraRot, playerTurn, 0.5);

  camera.position.x = lerp(camera.position.x, pawnPosition.x, 0.5);
  camera.position.z = lerp(camera.position.z, pawnPosition.y, 0.5);
  camera.target.x = camera.position.x + sinf(cameraRot);
  camera.target.y = camera.position.y;
  camera.target.z = camera.position.z + cosf(cameraRot);

  SaveProgress("steps", steps);
  SaveProgress("hash", SAVES_HASH);
  SaveProgress("px", pawnPosition.x);
  SaveProgress("py", pawnPosition.y);
  SaveProgress("turn", playerTurn * 1e4);

  if (action_b == ACTION_SWAP) {
    swap_weapon();
  } else if (action_b == ACTION_USE) {
    use_weapon();
  }

  update_weapon();
}

static void draw_map() {
	const int dim = viewport_w <= viewport_h ? viewport_w : viewport_h;
	const int dim_tex = scene_texture.width <= scene_texture.height ? scene_texture.width : scene_texture.height;
	const int detx = (viewport_w - dim) * 0.5;
	const int dety = (viewport_h - dim) * 0.5;

	const float scale = (float)dim / dim_tex;
	const float pawn_x = detx + pawnPosition.x * scale;
	const float pawn_y = dety + pawnPosition.y * scale;

// void DrawTexturePro( Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint )
//
	Rectangle tex_rec = { 0, 0, scene_texture.width, scene_texture.height };

	BeginTextureMode(scene_render_texture);
		DrawTexturePro(scene_texture, 
				tex_rec,
				tex_rec,
				(Vector2){ 0, 0 },
				0,
				WHITE);

		DrawRectangle(pawnPosition.x, pawnPosition.y, 1, 1, RED);
	EndTextureMode();

	DrawTexturePro(scene_render_texture.texture, 
			tex_rec,
			(Rectangle){ detx, dety, viewport_w - detx * 2, viewport_h - dety  * 2},
			(Vector2){ 0, 0 },
			0,
			WHITE);
	return;
}

#define SUI_BTN_A                                                              \
  (Rectangle) { 16, viewport_h - 16 - 64, 64, 64 }
#define SUI_BTN_B                                                              \
  (Rectangle) { 16 + 64 * 2 - 8 * 1, viewport_h - 16 - 64 * 1 - 8 * 0, 64, 64 }
#define SUI_BTN_C                                                              \
  (Rectangle) { viewport_w - 16 - 64, viewport_h - 16 - 64, 64, 64 }

static void draw_inputs() {
  Rectangle sba = SUI_BTN_A;
  Rectangle sbb = SUI_BTN_B;
  Rectangle sbc = SUI_BTN_C;

  Color sba_color = sui_btn_a_down ? RED : WHITE;
  Color sbb_color = sui_btn_b_down ? RED : WHITE;
  Color sbc_color = sui_btn_c_down ? RED : WHITE;

  DrawRectangleLinesEx(sba, 4, sba_color);
  DrawRectangleLinesEx(sbb, 4, sbb_color);
  DrawRectangleLinesEx(sbc, 4, sbc_color);

  DrawTexturePro(pic_rotate,
                 (Rectangle){0, 0, pic_rotate.width, pic_rotate.height}, sba,
                 Vector2Zero(), 0.0, sba_color);

  DrawText("R", sba.x + sba.width - 8 + 2, sba.y + sba.height - 16 + 2, 20,
           BLACK);
  DrawText("R", sba.x + sba.width - 8, sba.y + sba.height - 16, 20, sba_color);

  DrawTexturePro(pic_punch,
                 (Rectangle){0, 0, pic_punch.width, pic_punch.height}, sbb,
                 Vector2Zero(), 0.0, sbb_color);

  DrawText("F", sbb.x + sbb.width - 8 + 2, sbb.y + sbb.height - 16 + 2, 20,
           BLACK);
  DrawText("F", sbb.x + sbb.width - 8, sbb.y + sbb.height - 16, 20, sbb_color);

  DrawTexturePro(pic_forwards,
                 (Rectangle){0, 0, pic_forwards.width, pic_forwards.height},
                 sbc, Vector2Zero(), 0.0, sbc_color);

  DrawText("SHIFT", sbc.x - 32 + 2, sbc.y + sbc.height - 16 + 2, 20, BLACK);
  DrawText("SHIFT", sbc.x - 32, sbc.y + sbc.height - 16, 20, sbc_color);
}

static void draw() {
  ClearBackground(BLACK);

#if DRAW_MODEL
  BeginMode3D(camera);
  DrawModel(model, mapPosition, 1.0f, WHITE); // Draw maze map
  EndMode3D();
#endif

	draw_map();

  DrawText(TextFormat("Steps: %i", steps), 16 + 2, 16 + 2, 20, BLACK);
  DrawText(TextFormat("Steps: %i", steps), 16, 16, 20, WHITE);
#if DRAW_UI
  draw_inputs();
#endif

  draw_weapon();
}

static void inputs_sui_buttons() {
  bool a_past_down = sui_btn_a_down;
  bool b_past_down = sui_btn_b_down;
  bool c_past_down = sui_btn_c_down;
  bool screen_past_down = sui_screen_down;
  sui_btn_a_down = false;
  sui_btn_b_down = false;
  sui_btn_c_down = false;
  sui_screen_down = false;
  sui_btn_a_pressed = false;
  sui_btn_b_pressed = false;
  sui_btn_c_pressed = false;
  sui_screen_pressed = false;
  sui_screen_released = false;

  for (int i = 0; i < MAX_TOUCH_POINTS; ++i) {
    TouchPoint tp = touch_points[i];
    if (!tp.active) {
      break;
    }

    bool collide_a = CheckCollisionPointRec(tp.pos, SUI_BTN_A);
    bool collide_b = CheckCollisionPointRec(tp.pos, SUI_BTN_B);
    bool collide_c = CheckCollisionPointRec(tp.pos, SUI_BTN_C);

    sui_btn_a_down = sui_btn_a_down || collide_a;
    sui_btn_b_down = sui_btn_b_down || collide_b;
    sui_btn_c_down = sui_btn_c_down || collide_c;
    if (!sui_screen_down && !(collide_a || collide_b || collide_c)) {
      sui_screen_down = true;
      if (screen_past_down) {
        input_delta = Vector2Subtract(input_pos, tp.pos);
      } else {
        input_delta.x = 0;
        input_delta.y = 0;
      }
      input_pos = tp.pos;
    }
  }

  sui_btn_a_pressed = sui_btn_a_down && !a_past_down;
  sui_btn_b_pressed = sui_btn_b_down && !b_past_down;
  sui_btn_c_pressed = sui_btn_c_down && !c_past_down;
  sui_screen_pressed = sui_screen_down && !screen_past_down;
  sui_screen_released = !sui_screen_down && screen_past_down;
}

static void inputs() {
  if (is_toggle_fullscreen_requested()) {
    toggle_fullscreen();
  }

  int tCount = GetTouchPointCount();
  for (int i = 0; i < MAX_TOUCH_POINTS; ++i) {
    bool active = i < tCount;
    touch_points[i].active = active;

    if (!active) {
      continue;
    }

    touch_points[i].pos = GetTouchPosition(i);
    touch_points[i].id = GetTouchPointId(i);
  }

  inputs_sui_buttons();

  if (IsKeyPressed(KEY_H)) {
    _draw_help_enabled = !_draw_help_enabled;
  }

  bool mode_mouse = !tCount;

  sui_screen_released = sui_screen_released || IsKeyPressed(KEY_SPACE) ||
                        IsKeyPressed(KEY_W) ||
                        (mode_mouse && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
  sui_btn_a_pressed = sui_btn_a_pressed || IsKeyPressed(KEY_R);
  sui_btn_b_pressed = sui_btn_b_pressed || IsKeyPressed(KEY_F);
  sui_btn_c_down = sui_btn_c_down || IsKeyDown(KEY_LEFT_SHIFT);

  inputDirection.x = 0.0f;
  inputDirection.y = 0.0f;

  action_a = ACTION_NONE;
  action_b = ACTION_NONE;

  bool gesture_registered = Vector2Length(gesture_delta) > 64;
  if (gesture_registered && sui_screen_released) {
    Vector2 dir = Vector2Normalize(gesture_delta);
    float dot_up = Vector2DotProduct(dir, Vec2Up);
    float dot_right = Vector2DotProduct(dir, Vec2Right);
    if (fabs(dot_up) < 0.5) {
      if (dot_right < 0) {
        action_a = ACTION_RIGHT;
      } else {
        action_a = ACTION_LEFT;
      }
    } else if (dot_up > 0) {
      action_a = ACTION_FORWARD;
      action_b = ACTION_RUN;
    } else {
      action_a = ACTION_BACKWARD;
    }
  } else {
    if (sui_screen_released) {
      action_a = ACTION_FORWARD;
    } else if (IsKeyPressed(KEY_A)) {
      action_a = ACTION_LEFT;
    } else if (IsKeyPressed(KEY_D)) {
      action_a = ACTION_RIGHT;
    } else if (IsKeyPressed(KEY_S)) {
      action_a = ACTION_BACKWARD;
    }
  }

  if (action_a == ACTION_FORWARD) {
    inputDirection.x = 1.0f;
  } else if (action_a == ACTION_BACKWARD) {
    inputDirection.x = -1.0f;
  } else if (action_a == ACTION_LEFT) {
    inputDirection.y = 1.0f;
  } else if (action_a == ACTION_RIGHT) {
    inputDirection.y = -1.0f;
  }

  if (sui_btn_a_pressed) {
    action_b = ACTION_SWAP;
  } else if (sui_btn_b_pressed) {
    action_b = ACTION_USE;
  } else if (sui_btn_c_down) {
    action_b = ACTION_RUN;
  }

  if (sui_screen_released) {
    gesture_delta.x = 0;
    gesture_delta.y = 0;
  } else if (sui_screen_down) {
    gesture_delta.x += input_delta.x;
    gesture_delta.y += input_delta.y;
  }
}

// --- flow

static void dispose() {
  UnloadShader(shader_maze);
  UnloadTexture(tex_noise0);
  UnloadTexture(pic_forwards);
  UnloadTexture(pic_punch);
  UnloadTexture(pic_rotate);
  UnloadTexture(pic_rotate_left);
  UnloadTexture(pic_rotate_right);
  UnloadImageColors(mapPixels); // Unload color array
  UnloadImageColors(map_walker_pixels); // Unload color array

  UnloadTexture(scene_texture); // Unload scene_texture texture
	UnloadRenderTexture(scene_render_texture);
  UnloadTexture(texture);  // Unload map texture
  UnloadModel(model);      // Unload map model
  CloseAudioDevice();
}

static void init() {
  dispose();

  int saves_hash = LoadProgress("hash");
  if (saves_hash == SAVES_HASH) {
    steps = LoadProgress("steps");
    pawnPosition.x = LoadProgress("px");
    pawnPosition.y = LoadProgress("py");
    pawnPosition.x = pawnPosition.x ? pawnPosition.x : 1;
    pawnPosition.y = pawnPosition.y ? pawnPosition.y : 1;
    playerTurn = LoadProgress("turn") / 1e4;

		// Align angle
		double t = 0;
		playerTurn = playerTurn - modf((playerTurn / (PI * 0.5)), &t) * PI * 0.5f;
  }

  init_weapon();

  InitAudioDevice();

  shader_maze = LoadShader(RES_PATH "maze.vs", RES_PATH "maze.fs");

  pic_forwards = LoadTexture(RES_PATH "pic/forwards.png");
  pic_punch = LoadTexture(RES_PATH "pic/punch-blast.png");
  pic_rotate = LoadTexture(RES_PATH "pic/rotate.png");
  pic_rotate_left = LoadTexture(RES_PATH "pic/rotate_left.png");
  pic_rotate_right = LoadTexture(RES_PATH "pic/rotate_right.png");

  camera.position = (Vector3){0.0f, 0.6f, 0.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.5f, 1.0f};   // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 90.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE;

  Image imMap = LoadImage(RES_PATH "maze-0.png"); // Load cubicmap image (RAM)
  scene_texture =
      LoadTextureFromImage(imMap); // Convert image to texture to display (VRAM)
  Mesh mesh = GenMeshCubicmap(imMap, (Vector3){1.0f, 1.0f, 1.0f});
  model = LoadModelFromMesh(mesh);
	scene_render_texture = LoadRenderTexture( scene_texture.width, scene_texture.height );

  // NOTE: By default each cube is mapped to one part of texture atlas
  texture = LoadTexture(RES_PATH "atlas_maze.png"); // Load map texture
  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture =
      texture; // Set map diffuse texture

  model.materials[0].shader = shader_maze;

  tex_noise0 = LoadTexture(RES_PATH "tex_noise0.png");
  // somewhy does not work with direct SetShaderValueTexture. Assignin location
  // to some of reserver channels
  model.materials[0].shader.locs[SHADER_LOC_MAP_HEIGHT] =
      GetShaderLocation(shader_maze, "tex_noise0");
  model.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = tex_noise0;
  // int noise_location = GetShaderLocation(model.materials[0].shader,
  // "tex_noise0"); SetShaderValueTexture(model.materials[0].shader,
  // noise_location, tex_noise0);

  // Get map image data to be used for collision detection
  mapPixels = LoadImageColors(imMap);
  map_walker_pixels = LoadImageColors(imMap);
  UnloadImage(imMap); // Unload image from RAM

  // --- load
}

// --- system

static long resize_timestamp = -1;
static const float resize_threshold = 0.3;
static Vector2 requested_viewport = {VIEWPORT_W, VIEWPORT_H};
static void equilizer() {
  const int vw = GetScreenWidth();
  const int vh = GetScreenHeight();

  const long now = GetTime();

  // thresholds resizing
  if (requested_viewport.x != vw || requested_viewport.y != vh) {
    requested_viewport.x = vw;
    requested_viewport.y = vh;

    // first resize triggers intantly (important in web build)
    if (resize_timestamp > 0) {
      resize_timestamp = now;
      return;
    }
  }

  // reinits after riseze stops
  const bool resized =
      requested_viewport.x != viewport_w || requested_viewport.y != viewport_h;
  if (resized && now - resize_timestamp > resize_threshold) {
    resize_timestamp = now;
    viewport_w = vw;
    viewport_h = vh;
    // init();
  }
}

void step(void) {
  equilizer();

  inputs();
  update();

  BeginDrawing();
  draw();
  EndDrawing();
}

void loop() {
#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(step, 0, 1);
#else

  while (!WindowShouldClose()) {
    step();
  }
#endif
}

int main(void) {
  const int seed = 2;

  InitWindow(viewport_w, viewport_h, "tynmaze");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  SetRandomSeed(seed);

  init();

  active = true;
  loop();

  dispose();
  CloseWindow();

  return 0;
}

bool is_toggle_fullscreen_requested() {
  return IsKeyPressed(KEY_ENTER) &&
         (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
}

void toggle_fullscreen() {
  // see what display we are on right now
  int display = GetCurrentMonitor();

  if (IsWindowFullscreen()) {

    // if we are full screen, then go back to the windowed size
    SetWindowSize(viewport_w, viewport_h);
  } else {
    // if we are not full screen, set the window size to match the monitor we
    // are on
    SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
  }

  // toggle the state
  ToggleFullscreen();
}
