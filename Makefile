
WORKDIR := $(shell pwd)

all: ./src/external/parson.c ./src/pengine/files.h ./bin/ ./lib/
	make -C ./src/pengine WORKDIR=$(WORKDIR)
	make -C ./src/shaders
	@echo "[OK] You have pengine"

#INFO the headers are written as <pengine/...> against src, so installing the
#src/pengine tree under /usr/local/include gives a consumer the same names with
#no -I beyond the default ones
install: all
	rm -rf /usr/local/include/pengine
	cd src && find pengine -name '*.h' -exec install -Dm644 {} /usr/local/include/{} \;
	install -Dm644 lib/libpengine.a /usr/local/lib/libpengine.a

compile_commands:
	make --always-make --dry-run -C ./src/pengine WORKDIR=$(WORKDIR)


./src/external/parson.c:
	./configure

./src/pengine/files.h: ./scripts/create_engine_file_h.sh
	./scripts/create_engine_file_h.sh

./lib/:
	mkdir -p $(WORKDIR)/lib


clean:
	rm -f ./src/pengine/files.h
	make -C ./src/pengine WORKDIR=$(WORKDIR) clean
	make -C ./src/shaders clean
	rm -f ./bin/peditor

#$(LOG).SILENT:
