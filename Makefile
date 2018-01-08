IDIR = include

SRC = src

CC = gcc

CFLAGS = -Wall -I$(IDIR)

BUILD = build

ODIR = $(BUILD)/obj

LIBS = -lpthread

_DEPS = protocol.h tools.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = tools.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

default: directories access_node slave_node

$(ODIR)/%.o: directories $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

access_node: $(OBJ) $(ODIR)/access_node.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

slave_node: $(OBJ) $(ODIR)/slave_node.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean directories

clean:
	rm -rf $(BUILD)

directories: $(ODIR)

${ODIR}:
	mkdir -p ${ODIR}
