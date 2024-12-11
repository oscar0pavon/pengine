include ../../include.make

.PHONY: all clean install

all: $(WORKDIR)/bin/peditor

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $<

textures_src := $(wildcard ./Textures/*.c)
textures_objs := $(textures_src:%.c=%.o)

./Textures/%.o : ./Textures/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@


ld:
	ld $(LIBC) *.o -L../engine -lengine $(LIBRARIES) -o $(WORKDIR)/bin/peditor

$(WORKDIR)/bin/peditor: $(OBJS) $(textures_objs) $(WORKDIR)/lib/libpengine.a
	cc *.o $(textures_objs) -o $(WORKDIR)/bin/peditor -L$(WORKDIR)/lib/ $(LIBRARIES) -lpengine
	
clean:
	rm -f *.o
