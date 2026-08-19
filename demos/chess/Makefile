WORKDIR := $(realpath ../..)

include ../../include.make

.PHONY: all clean

all: pchess

#INFO the same CFLAGS the engine is built with, not just GLOBAL_DEFINE. the
#engine passes &main_camera (a CameraComponent) to camera_init(Camera*), so a
#consumer without -Wno-incompatible-pointer-types cannot compile its headers
pchess: chess.c chess.h $(WORKDIR)/lib/libpengine.a $(WORKDIR)/src/engine/files.h
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) chess.c \
		-L$(WORKDIR)/lib -lpengine $(LIBRARIES) -o pchess

$(WORKDIR)/lib/libpengine.a $(WORKDIR)/src/engine/files.h:
	make -C $(WORKDIR)

clean:
	rm -f pchess
