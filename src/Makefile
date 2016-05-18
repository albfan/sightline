all: sightline

OBJS = \
       sl-line-reader.o \
       sl-log-reader.o \
       sl-make-target.o \
       $(NULL)

PKGS = gio-2.0

LIBS = $(shell pkg-config --libs $(PKGS)) -lclang
CFLAGS = $(shell pkg-config --cflags $(PKGS))

WARNINGS = -Wall -Werror
DEBUG = -ggdb -O0
OPTIMIZE =

%.o: %.c %.h
	$(CC) -c -o $@ $(WARNINGS) $(DEBUG) $(OPTIMIZE) $*.c $(CFLAGS)

sightline: $(OBJS) main.c
	$(CC) $(WARNINGS) $(DEBUG) $(OPTIMIZE) -o $@ $(OBJS) main.c $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) sightline
