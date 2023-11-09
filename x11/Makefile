CFLAGS += `pkg-config --cflags x11`
LDFLAGS += `pkg-config --libs x11`

basic: basic.c
	gcc -o $@ $< $(LDFLAGS)

moving: moving.c
	gcc -o $@ $< $(LDFLAGS)

menu: menu.c
	gcc -o $@ $< $(LDFLAGS)
