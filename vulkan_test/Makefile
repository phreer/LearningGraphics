CFLAGS += -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
CXX := g++

vulkan_test: main.cpp
	$(CXX) $(CFLAGS) -o $@ $< $(LDFLAGS)

hello_triangle: hello_triangle.cpp
	$(CXX) $(CFLAGS) -o $@ $< $(LDFLAGS)

vert.spv: shader.vert
	glslc $< -o $@

frag.spv: shader.frag
	glslc $< -o $@

.DEFAULT: all
.PHONY: test clean all

test: vulkan_test hello_triangle
	./vulkan_test
	./hello_triangle

clean:
	rm -f ./vulkan_test ./hello_triangle *.spv
	
all: vulkan_test hello_triangle vert.spv frag.spv
