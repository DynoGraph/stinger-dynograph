SRC_DIR = ./src
INC_DIR = ./inc
LIB_DIR = ./lib
OBJ_DIR = ./obj

CC = gcc
#CFLAGS = -I$(INC_DIR) -O3 -std=c99 -fopenmp
CFLAGS = -I$(INC_DIR) -O3 -std=c99 -g
LDFLAGS = 

_HEADERS = stinger_atomics.h stinger_bench.h stinger_defs.h stinger.h stinger_internal.h stinger_traversal.h stinger_vertex.h x86_full_empty.h xmalloc.h hooks.h
HEADERS = $(patsubst %, $(INC_DIR)/%, $(_HEADERS))


all: $(OBJ_DIR) libstinger.a benchmarks

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

benchmarks: bfs bfs_direction_optimizing components pagerank edgestream

$(OBJ_DIR)/xmalloc.o: $(SRC_DIR)/lib/xmalloc.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/x86_full_empty.o: $(SRC_DIR)/lib/x86_full_empty.c $(HEADERS)
	$(CC) -I$(INC_DIR) -O0 -std=c99 -o $@ -c $<

$(OBJ_DIR)/stinger_vertex.o: $(SRC_DIR)/lib/stinger_vertex.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/stinger_load.o: $(SRC_DIR)/lib/stinger_load.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/stinger.o: $(SRC_DIR)/lib/stinger.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/hooks.o: $(SRC_DIR)/lib/hooks.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

_STINGER_OBJ = xmalloc.o x86_full_empty.o stinger_vertex.o stinger_load.o stinger.o hooks.o
STINGER_OBJ = $(patsubst %, $(OBJ_DIR)/%, $(_STINGER_OBJ))

libstinger.a: $(STINGER_OBJ)
	ar rcs libstinger.a $(STINGER_OBJ)

bfs: $(SRC_DIR)/bfs/main.c $(HEADERS) libstinger.a  
	$(CC) $(CFLAGS) -o bfs $< libstinger.a

bfs_direction_optimizing: $(SRC_DIR)/bfs_direction_optimizing/main.c $(HEADERS) libstinger.a  
	$(CC) $(CFLAGS) -o bfs_do $< libstinger.a

components: $(SRC_DIR)/components/main.c $(HEADERS) libstinger.a 
	$(CC) $(CFLAGS) -o components $< libstinger.a

pagerank: $(SRC_DIR)/pagerank/main.c $(HEADERS) libstinger.a
	$(CC) $(CFLAGS) -o pagerank $< libstinger.a

edgestream: $(SRC_DIR)/edge_stream/main.c $(HEADERS) libstinger.a
	$(CC) $(CFLAGS) -o edge_stream $< libstinger.a

.PHONY: clean

clean:
	rm -f obj/*.o libstinger.a bfs components pagerank edge_stream
