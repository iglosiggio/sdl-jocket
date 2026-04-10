sdl-jocket.dylib: sdl-jocket.c
	$(CC) $(CFLAGS) -std=c23 $$(sdl2-config --cflags) -undefined dynamic_lookup $< -dynamiclib -o $@

sdl-jocket.so: sdl-jocket.c
	$(CC) $(CFLAGS) -std=c23 $$(sdl2-config --cflags) $< -shared -rdynamic -fPIC -o $@
