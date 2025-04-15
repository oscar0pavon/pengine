
frag := $(wildcard *.frag)
frag_spv := $(frag:.frag=.spv)

vert := $(wildcard *.vert)
vert_spv := $(vert:.vert=.spv)

%.spv : %.frag
	@echo "Shader $@"
	glslc $< -o $@

%.spv : %.vert
	@echo "Shader $@"
	glslc $< -o $@

shaders: $(frag_spv) $(vert_spv)

clean:
	rm -f *.spv

$(LOG).SILENT:
