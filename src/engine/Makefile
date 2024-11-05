

include $(WORKDIR)/include.make

.PHONY: all clean install

all: engine.a

components_src := $(shell find ./components -name '*.c')
components_objs := $(components_src:%.c=%.o)

opengl_src := $(wildcard ./renderer/opengl/*.c)
opengl_obj := $(opengl_src:%.c=%.o)
renderer_src := $(wildcard ./renderer/*.c)
renderer_objs := $(renderer_src:%.c=%.o)
animation_src := $(wildcard ./animation/*.c)
animation_objs := $(animation_src:%.c=%.o)



$(OBJS): %.o : %.c 
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $<


./components/%.o : ./components/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./renderer/opengl/%.o: ./renderer/opengl/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./animation/%.o: ./animation/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./renderer/%.o: ./renderer/%.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

../ThirdParty/parson.o: ../ThirdParty/parson.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./platforms/windows_manager.o: ./platforms/windows_manager.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@

./Collision/collision.o: ./Collision/collision.c
	$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) -c $< -o $@


ALL_OBJS := $(OBJS) $(components_objs) $(opengl_obj) $(renderer_objs) $(animation_objs) ./Collision/collision.o ./platforms/windows_manager.o ../ThirdParty/parson.o


engine.a: $(ALL_OBJS) 	
	make -C ./renderer/vulkan WORKDIR=$(WORKDIR)
	ar rcs libengine.a *.o ./renderer/vulkan/*.o $^


clean:
	rm -f *.o
	rm -f libengine.a
	rm -f $(ALL_OBJS)
	make -C ./renderer/vulkan clean
	
	
