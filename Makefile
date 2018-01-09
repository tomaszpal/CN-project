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

default: access_node slave_node

$(ODIR)/%.o: $(SRC)/%.c $(DEPS)
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

access_node: $(OBJ) $(ODIR)/access_node.o
	$(CC) -o $(BUILD)/$@ $^ $(CFLAGS) $(LIBS)

slave_node: $(OBJ) $(ODIR)/slave_node.o
	$(CC) -o $(BUILD)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -rf $(BUILD)
