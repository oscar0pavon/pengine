
WORKDIR := $(shell pwd)

all: ./src/engine/files.h ./bin/
	make -C ./src/engine WORKDIR=$(WORKDIR)
	make -C ./src/editor WORKDIR=$(WORKDIR)
	make -C ./src/shaders

./src/engine/files.h: ./scripts/create_engine_file_h.sh
	./scripts/create_engine_file_h.sh

./bin/:
	mkdir -p $(WORKDIR)/bin

clean:
	rm -f ./src/engine/files.h
	make -C ./src/editor WORKDIR=$(WORKDIR) clean
	make -C ./src/engine WORKDIR=$(WORKDIR) clean
	make -C ./src/shaders clean
	rm -f ./bin/peditor
