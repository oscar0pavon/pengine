
frag := $(wildcard *.frag)
frag_spv := $(frag:.frag=.spv)

vert := $(wildcard *.vert)
vert_spv := $(vert:.vert=.spv)

%.spv : %.frag
	glslc $< -o $@

%.spv : %.vert
	glslc $< -o $@

shaders: $(frag_spv) $(vert_spv)

clean:
	rm -f *.spv
