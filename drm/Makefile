CC ?= gcc
CFLAGS ?= -I/usr/include/libdrm
LDFLAGS ?= -L/usr/lib/x86_64-linux-gnu -ldrm

all: drm-connector-test modeset-atomic-gbm lmem deviceptr modeset-lmem-deviceptr

drm-connector-test: drm-connector-test.o
	$(CC) $< -o $@ $(LDFLAGS)

modeset-atomic-gbm: modeset-atomic-gbm.o
	$(CC) $< -o $@ $(LDFLAGS) -lgbm

lmem: lmem.o
	$(CC) $< -o $@ $(LDFLAGS) -lgbm

deviceptr: deviceptr.o
	$(CC) $< -o $@ $(LDFLAGS) -lgbm

intel-gpu-blit: intel-gpu-blit.o
	$(CC) $< -o $@ $(LDFLAGS) -lgbm -Wall

modeset-lmem-deviceptr: modeset-lmem-deviceptr.o
	$(CC) $< -o $@ $(LDFLAGS) -lgbm

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o drm-connector-test modeset-atomic-gbm lmem deviceptr modeset-lmem-deviceptr 2>/dev/null
