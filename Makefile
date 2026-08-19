
WORKDIR := $(shell pwd)

all: ./src/engine/files.h ./bin/ ./lib/
	make -C ./src/engine WORKDIR=$(WORKDIR)
	make -C ./src/shaders
	@echo "[OK] You have pengine"

#INFO the headers go in under the same tree they are written against, so the
#"engine/..." and "renderer/..." includes inside them resolve the way they do
#in this repo. a consumer adds -I/usr/local/include/pengine and nothing else
install: all
	rm -rf /usr/local/include/pengine
	mkdir -p /usr/local/include/pengine
	cd src && find . -name '*.h' -exec install -Dm644 {} /usr/local/include/pengine/{} \;
	install -Dm644 lib/libpengine.a /usr/local/lib/libpengine.a

compile_commands:
	make --always-make --dry-run -C ./src/engine WORKDIR=$(WORKDIR)


./src/engine/files.h: ./scripts/create_engine_file_h.sh
	./scripts/create_engine_file_h.sh

./lib/:
	mkdir -p $(WORKDIR)/lib


clean:
	rm -f ./src/engine/files.h
	make -C ./src/engine WORKDIR=$(WORKDIR) clean
	make -C ./src/shaders clean
	rm -f ./bin/peditor

#$(LOG).SILENT:
