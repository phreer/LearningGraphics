#include <stdio.h>
#include <stdlib.h>

#include <libdrm/drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int drm_fd;

void show_connector_info(uint32_t id) {
	drmModeConnectorPtr c;
	c = drmModeGetConnector(drm_fd, id);
	if (c == NULL) {
		fprintf(stderr, "failed to get connector %u\n", id);
		exit(EXIT_FAILURE);
	}
	printf("connector %u:\n", id);
	printf("\tconnection = %d\n", c->connection);
	printf("\t#modes = %d, #encoders = %d, #props = %d\n", c->count_modes, c->count_encoders, c->count_props);
}

int main(int argc, char *argv[]) {
	int ret = 0;
	drmModeResPtr res;

	if (argc != 1) {
		fprintf(stderr, "usage: %s <gpu-name>", argv[0]);
		exit(EXIT_FAILURE);
	}
	drm_fd = drmOpen(argv[0], NULL);
	if (drm_fd < 0) {
		perror("failed to open drm device");
		return EXIT_FAILURE;
	}

	ret = drmSetMaster(drm_fd);
	if (ret) {
		perror("failed to set master");
	}
	res = drmModeGetResources(drm_fd);
	if (res == NULL) {
		fprintf(stderr, "failed to get resources\n");
		return EXIT_FAILURE;
	}

	printf("resources:\n");
	printf("\t#fbs = %d, #crtcs = %d, #encoders = %d, #connectors = %d\n",
		res->count_fbs, res->count_crtcs, res->count_encoders, res->count_connectors);

	for (int i = 0; i < res->count_connectors; ++i) {
		show_connector_info(res->connectors[i]);
	}
	drmModeFreeResources(res);
	drmClose(drm_fd);
	return 0;
}