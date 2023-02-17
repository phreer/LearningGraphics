OBJS=linux_dmabuf_client.o xdg_shell_client.o
INC=-I/usr/include/drm -I/usr/include/libdrm
test_dmabuf: ${OBJS} test_dmabuf.c
	gcc -o $@ ${INC} ${OBJS}  test_dmabuf.c -lwayland-client -ldrm
