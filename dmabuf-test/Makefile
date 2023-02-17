CC      ?= gcc
CFLAGS  ?= -pedantic -Wall -I /usr/include/libdrm
LDFLAGS ?= -ldrm

OBJ = test_prime.o
PROGNAME = test_prime

exec_prefix ?= /usr
bindir ?= $(exec_prefix)/bin

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OBJ) $(LDFLAGS)

install: all
	install -d $(DESTDIR)$(bindir)
	install -m 0755 $(PROGNAME) $(DESTDIR)$(bindir)

clean:
	@echo "Clean object files"
	@rm -f $(OBJ)
	@rm -f $(PROGNAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
