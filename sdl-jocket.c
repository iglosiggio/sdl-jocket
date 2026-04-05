#include <stdio.h>

#include <SDL.h>

// Buttons
//
// SDL_CONTROLLER_BUTTON_A
// SDL_CONTROLLER_BUTTON_B
// SDL_CONTROLLER_BUTTON_X
// SDL_CONTROLLER_BUTTON_Y
// SDL_CONTROLLER_BUTTON_BACK
// SDL_CONTROLLER_BUTTON_GUIDE
// SDL_CONTROLLER_BUTTON_START
// SDL_CONTROLLER_BUTTON_LEFTSTICK
// SDL_CONTROLLER_BUTTON_RIGHTSTICK
// SDL_CONTROLLER_BUTTON_LEFTSHOULDER
// SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
// SDL_CONTROLLER_BUTTON_DPAD_UP
// SDL_CONTROLLER_BUTTON_DPAD_DOWN
// SDL_CONTROLLER_BUTTON_DPAD_LEFT
// SDL_CONTROLLER_BUTTON_DPAD_RIGHT

// Axes
//
// SDL_CONTROLLER_AXIS_LEFTX
// SDL_CONTROLLER_AXIS_LEFTY
// SDL_CONTROLLER_AXIS_RIGHTX
// SDL_CONTROLLER_AXIS_RIGHTY
// SDL_CONTROLLER_AXIS_TRIGGERLEFT
// SDL_CONTROLLER_AXIS_TRIGGERRIGHT

static int joy_index = -1;
static SDL_Joystick* joy;

int sdl_jocket_loop(void* _unused) {
	joy_index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
	if (joy_index == -1) {
		printf("ERROR: 1\n");
		return 1;
	}

	joy = SDL_JoystickOpen(joy_index);
	if (joy == NULL) {
		printf("ERROR: 2\n");
		return 2;
	}

	unsigned i = 0;
	while (1) {
		SDL_JoystickSetVirtualAxis(joy, SDL_CONTROLLER_AXIS_LEFTX, i++);
		SDL_JoystickSetVirtualButton(joy, SDL_CONTROLLER_BUTTON_X, i++);
	}

	SDL_JoystickClose(joy);
	SDL_JoystickDetachVirtual(joy_index);
	return 0;
}

[[gnu::constructor]]
void sdl_jocket_init() {
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_CreateThread(sdl_jocket_loop, "sdl-jocket: main loop", NULL);
}

[[gnu::destructor]]
void sdl_jocket_deinit() {
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}
