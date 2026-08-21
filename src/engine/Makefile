

include ../../include.make

.PHONY: all clean install

all: libpengine.a

renderer_src := $(wildcard ./renderer/*.c)
renderer_objs := $(renderer_src:%.c=%.o)

animation_src := $(wildcard ./animation/*.c)
animation_objs := $(animation_src:%.c=%.o)

#INFO -MMD writes a .d beside each object naming every header that object read,
#and the -include at the bottom feeds those back as rules. without them make
#only ever compared a .o against its .c, so editing a header rebuilt nothing
#and the archive kept objects built against the old declarations. that is how a
#stale animation/node.o went on including a header that had been deleted
DEPFLAGS := -MMD -MP

COMPILE_O = $(CC) $(CFLAGS) $(DEPFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

%.o : %.c
	@echo "Compiling $@"
	$(COMPILE_O)

./animation/%.o: ./animation/%.c
	$(COMPILE_O)

./renderer/%.o: ./renderer/%.c
	$(COMPILE_O)

../ThirdParty/parson.o: ../ThirdParty/parson.c
	$(COMPILE_O)

./Collision/collision.o: ./Collision/collision.c
	$(COMPILE_O)

ALL_OBJS := $(OBJS) $(renderer_objs) $(animation_objs) ./Collision/collision.o ../ThirdParty/parson.o

ALL_DEPS := $(ALL_OBJS:.o=.d)


#INFO built from scratch, from $^ alone, with full path member names. *.o
#passed every top level object a second time, and ar names members by basename,
#so two sources sharing a basename both wanted the same member name - one of
#the two silently lost, and edits to it stopped reaching the library. P keeps
#the paths apart
libpengine.a: $(ALL_OBJS)
	rm -f $(WORKDIR)/lib/libpengine.a
	ar rcsP $(WORKDIR)/lib/libpengine.a $^


clean:
	rm -f *.o *.d
	rm -f $(WORKDIR)/lib/libpengine.a
	rm -f $(ALL_OBJS)
	rm -f $(ALL_DEPS)


#INFO last, and with a leading dash: the .d files do not exist on a first build
#and make has to carry on without them
-include $(ALL_DEPS)

$(LOG).SILENT:
