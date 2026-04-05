sdl-jocket.dylib: sdl-jocket.c
	$(CC) $$(sdl2-config --cflags) -undefined dynamic_lookup $< -dynamiclib -o $@

sdl-jocket.so: sdl-jocket.c
	$(CC) $$(sdl2-config --cflags) -undefined dynamic_lookup $< -shared -fPIC -o $@
