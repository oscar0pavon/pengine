CC := cc

SRCS := $(wildcard *.c)
OBJS := $(SRCS:c=o)

#INFO no -Wno- here on purpose. -Wno-implicit-function-declaration was the
#dangerous one: since the product is a static library an undeclared call does
#not fail at ar time either, so a call to a function nobody defined only
#surfaced in a consumer's link. that is how main.c came to call time_start()
#and base.c to call pway_create_window() with one argument instead of three
CFLAGS := -g -fPIC -fcommon

CINCLUDES := -I$(WORKDIR)/src -I/usr/local/include

GLOBAL_DEFINE := -D LINUX -D DESKTOP -D VULKAN -D DEBUG -D EDITOR
#INFO the vulkan renderer expects a 0..1 depth range and a left handed world.
#swordfish built engine/ and renderer/ with these two and nothing else with
#them, so they have to be here or every projection matrix comes out different
GLOBAL_DEFINE += -DCGLM_FORCE_DEPTH_ZERO_TO_ONE -DCGLM_FORCE_LEFT_HANDED

COMPILE := $(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES)

LIBC := /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/libc.so /usr/lib/crtn.o -dynamic-linker /lib/ld-linux.so.2

WAYLAND_LIBS := -lEGL -lwayland-client -lwayland-egl
WAYLAND_LIBS += -lxkbcommon

#INFO libpengine.a calls into pway for the window and lodepng for image
#decoding, so a program linking the engine needs both. they are here rather
#than in each consumer's own link line
LIBRARIES := $(WAYLAND_LIBS) -lvulkan -lm -lpthread -ldl
LIBRARIES += -lpway -llodepng
