include ../../include.make

all: pchess

pchess: chess.c
	cc chess.c $(GLOBAL_DEFINE) -I../../src $(LIBRARIES) -L../../lib -lpengine -o pchess
