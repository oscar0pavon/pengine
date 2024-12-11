
WORKDIR := $(shell pwd)

all: ./src/engine/files.h ./bin/ ./lib/
	make -C ./src/engine WORKDIR=$(WORKDIR)
	make -C ./src/editor WORKDIR=$(WORKDIR)
	make -C ./src/shaders


compile_commands:
	make --always-make --dry-run -C ./src/engine WORKDIR=$(WORKDIR)
	make --always-make --dry-run -C ./src/editor WORKDIR=$(WORKDIR)


./src/engine/files.h: ./scripts/create_engine_file_h.sh
	./scripts/create_engine_file_h.sh

./bin/:
	mkdir -p $(WORKDIR)/bin

./lib/:
	mkdir -p $(WORKDIR)/lib

clean:
	rm -f ./src/engine/files.h
	make -C ./src/editor WORKDIR=$(WORKDIR) clean
	make -C ./src/engine WORKDIR=$(WORKDIR) clean
	make -C ./src/shaders clean
	rm -f ./bin/peditor
