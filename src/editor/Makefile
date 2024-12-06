include ../../include.make

.PHONY: all clean install

all: peditor

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $<

windows_src := $(wildcard ./windows/*.c)
windows_objs := $(windows_src:%.c=%.o)
textures_src := $(wildcard ./Textures/*.c)
textures_objs := $(textures_src:%.c=%.o)



./windows/%.o : ./windows/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./Textures/%.o : ./Textures/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@


ld:
	ld $(LIBC) *.o -L../engine -lengine $(LIBRARIES) -o $(WORKDIR)/bin/peditor

peditor: $(OBJS) $(textures_objs) $(windows_objs)
	gcc *.o $(windows_objs) $(textures_objs) -o $(WORKDIR)/bin/peditor -L../engine $(LIBRARIES) -lengine
	


clean:
	rm -f *.o
