OBJS=linux_dmabuf_client.o xdg_shell_client.o
INC=-I/usr/include/drm -I/usr/include/libdrm
LIBS=-lwayland-client -ldrm -lm -lstb
test_dmabuf: ${OBJS} test_dmabuf.c
	gcc -o $@ ${INC} ${OBJS}  $@.c ${LIBS}

dmabuf_logo: ${OBJS} dmabuf_logo.c
	gcc -o $@ ${INC} ${OBJS}  $@.c ${LIBS}

dmabuf_logo_immed: ${OBJS} dmabuf_logo_immed.c
	gcc -o $@ ${INC} ${OBJS}  $@.c ${LIBS}

all: test_dmabuf dmabuf_logo dmabuf_logo_immed