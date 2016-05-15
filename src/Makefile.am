bin_PROGRAMS += wasmVR

wasmVR_LDFLAGS = \
	-s USE_WEBGL2=1 \
	-s TOTAL_MEMORY=@total_memory@ \
	-s DEMANGLE_SUPPORT=1 \
	--bind

wasmVR_CFLAGS = \
	-I./system/include \
	--bind

wasmVR_CPPFLAGS = \
	$(wasmVR_CFLAGS) \
	-stdlib=libstdc++ \
	-std=c++11

wasmVR_SOURCES = \
	src/wasmVR.cpp \
	src/Node.hpp \
	src/Node.cpp \
	src/Audio.hpp \
	src/Audio.cpp \
	src/Renderer/Renderer.hpp \
	src/Renderer/Renderer.cpp \
	src/VR.hpp \
	src/VR.cpp \
	src/Mesh.hpp \
	src/Primitive.hpp \
	src/Vertex.hpp \
	src/Camera.hpp
