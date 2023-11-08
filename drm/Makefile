CC ?= gcc
CFLAGS ?= -I/usr/include/libdrm
LDFLAGS ?= -L/usr/lib/x86_64-linux-gnu -ldrm

drm-connector-test: drm-connector-test.o
	$(CC) $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: drm-connector-test