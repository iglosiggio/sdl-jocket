#include <dlfcn.h>
#include <stdio.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <SDL.h>

static int joy_index = -1;
static SDL_Joystick* joy;
static int sock = -1;

int
lacks_terminator(const char* buf, int size) {
	for (int i = 0; i < size; i++) {
		if (buf[i] == 0) return 0;
		if (buf[i] == '\n') return 0;
	}
	return 1;
}

void
sdl_jocket_command(const char* command, int len) {
	printf("[DEBUG] Processing command: %.*s\n", len, command);
	if (len == 5) {
		int button;
		Uint8 value;
		if      (command[0] == '+') value = SDL_PRESSED;
		else if (command[0] == '-') value = SDL_RELEASED;
		else {
			printf("[ERROR] Button command lacks pressed-or-released indicator!\n");
			return;
		}

		if      (memcmp("BUTA", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_A;
		else if (memcmp("BUTB", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_B;
		else if (memcmp("BUTX", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_X;
		else if (memcmp("BUTY", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_Y;
		else if (memcmp("BACK", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_BACK;
		else if (memcmp("GUID", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_GUIDE;
		else if (memcmp("STRT", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_START;
		else if (memcmp("LSTK", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_LEFTSTICK;
		else if (memcmp("RSTK", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
		else if (memcmp("LSHD", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
		else if (memcmp("RSHD", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
		else if (memcmp("PADU", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_DPAD_UP;
		else if (memcmp("PADD", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
		else if (memcmp("PADL", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
		else if (memcmp("PADR", command + 1, 4) == 0) button = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
		else {
			printf("[ERROR] Unknown button: %.4s\n", command + 1);
			return;
		}
		printf("[DEBUG] SDL_JoystickSetVirtualButton(%p, %d, %d)\n", (void*) joy, button, value);
		SDL_JoystickSetVirtualButton(joy, button, value);
	} else {
		int axis;
		if (len < 4 + 1 + 1 || 4 + 1 + 5 < len) {
			printf("[ERROR] Axis command has unexpected size! (%d bytes)\n", len);
			return;
		}
		if (command[4] != ':') {
			printf("[ERROR] Axis command lacks value separator!\n");
			return;
		}
		long value = ((long) strtoul(command + 5, NULL, 10)) - (1l << 15);
		if (value < -32768 || 32767 < value) {
			printf("[ERROR] Axis command value (%ld) is outside the expected range!\n", value);
			return;
		}

		if      (memcmp("AXLX", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_LEFTX;
		else if (memcmp("AXLY", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_LEFTY;
		else if (memcmp("AXRX", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_RIGHTX;
		else if (memcmp("AXRY", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_RIGHTY;
		else if (memcmp("TRGL", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
		else if (memcmp("TRGR", command, 4) == 0) axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
		else {
			printf("[ERROR] Unknown axis: %.4s\n", command);
			return;
		}
		printf("[DEBUG] SDL_JoystickSetVirtualAxis(%p, %d, %ld)\n", (void*) joy, axis, value);
		SDL_JoystickSetVirtualAxis(joy, axis, value);
	}
}

static int
sdl_jocket_loop(void* _unused) {
	const char* failed = NULL;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		failed = "Failed to create a new socket";
		goto cleanup_joystick;
	}
	struct sockaddr_in address, client_address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(1810);
	if (bind(sock, (void*) &address, sizeof(address))) {
		failed = "Failed to bind the socket to 0.0.0.0:1810";
		goto cleanup_socket;
	}
	if (listen(sock, 0)) {
		failed = "Failed to listen on the socket";
		goto cleanup_socket;
	}

accept_connection:
	printf("[INFO] Waiting for connection\n");
	socklen_t client_address_len = sizeof(client_address);
	int conn = accept(sock, (void*) &client_address, &client_address_len);
	if (conn == -1) {
		printf("[ERROR] Failed to accept() a new connection\n");
		goto accept_connection;
	}
	printf("[INFO] Connected\n");

	while (1) {
		int bufsiz = 0;
		char buf[4 + 1 + 5 + 1];
		while (lacks_terminator(buf, bufsiz)) {
			int read = recv(conn, buf + bufsiz, 1, 0);
			if (read == -1 || bufsiz == sizeof(buf)) {
				close(conn);
				printf("[INFO] Closing connection...\n");
				goto accept_connection;
			}
			bufsiz += read;
		}
		sdl_jocket_command(buf, bufsiz - 1);
	}

cleanup_conn:
	close(conn);
cleanup_socket:
	close(sock);
cleanup_joystick:
	SDL_JoystickClose(joy);
	SDL_JoystickDetachVirtual(joy_index);
	if (failed) {
		printf("[ERROR] %s\n", failed);
	}
	return failed != NULL;
}

bool
linked_against_sdl() {
	void* self = dlopen(NULL, RTLD_LAZY);
	if (!self) return false;

	void* well_known_sdl_symbol = dlsym(self, "SDL_InitSubSystem");
	dlclose(self);

	return well_known_sdl_symbol != NULL;
}

[[gnu::constructor]]
void
sdl_jocket_init() {
	if (!linked_against_sdl()) return;

	printf("[INFO] Starting up sdl-jocket...\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	joy_index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
	if (joy_index == -1) {
		printf("[ERROR] Failed to attach a virtual joystick\n");
		return;
	}

	joy = SDL_JoystickOpen(joy_index);
	if (joy == NULL) {
		SDL_JoystickDetachVirtual(joy_index);
		return;
	}

	SDL_CreateThread(sdl_jocket_loop, "sdl-jocket: main loop", NULL);
}

[[gnu::destructor]]
void
sdl_jocket_deinit() {
	if (!linked_against_sdl()) return;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}
