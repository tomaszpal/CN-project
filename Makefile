IDIR = include

SRC = src

CC = gcc

CFLAGS = -Wall -I$(IDIR)

BUILD = build

ODIR = $(BUILD)/obj

LIBS = -lpthread

_DEPS = protocol.h tools.h queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJA = access_node.o tools.o queue.o
OBJA = $(patsubst %,$(ODIR)/%,$(_OBJA))

_OBJS = slave_node.o tools.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

default: access_node slave_node

$(ODIR)/%.o: $(SRC)/%.c $(DEPS)
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

access_node: $(OBJA)
	$(CC) -o $(BUILD)/$@ $^ $(CFLAGS) $(LIBS)

slave_node: $(OBJS)
	$(CC) -o $(BUILD)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -rf $(BUILD)
