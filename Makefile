SRC_DIR = ./src
INC_DIR = ./inc
LIB_DIR = ./lib
OBJ_DIR = ./obj

CC = gcc
#CFLAGS = -I$(INC_DIR) -O3 -std=c99 -fopenmp
CFLAGS = -I$(INC_DIR) -O3 -std=c99 -g
LDFLAGS =

HEADERS = $(wildcard $(INC_DIR)/*.h)

all: $(OBJ_DIR) libstinger.a benchmarks convert_raw

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

benchmarks: bfs bfs_direction_optimizing components pagerank edgestream

$(OBJ_DIR)/x86_full_empty.o: $(SRC_DIR)/lib/x86_full_empty.c $(HEADERS)
	$(CC) -I$(INC_DIR) -O0 -std=c99 -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/lib/%.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

STINGER_SRC = $(wildcard src/lib/*.c)
STINGER_OBJ = $(patsubst %.c,%.o,$(STINGER_SRC))

libstinger.a: $(STINGER_OBJ)
	echo $(STINGER_SRC)
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

convert_raw: $(SRC_DIR)/convert_raw/main.c $(HEADERS) libstinger.a
	$(CC) $(CFLAGS) -o convert_raw $< libstinger.a

.PHONY: clean

clean:
	rm -f obj/*.o libstinger.a bfs components pagerank edge_stream
