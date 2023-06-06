MAKEFLAGS += -j4
WARNINGS=-Wall -Wextra -pedantic -Wno-unknown-pragmas
CFLAGS=-Og -g $(WARNINGS) -I. -I..
LDFLAGS=-lzip -lm
IMPLEMENTATIONS=-DSTB_DS_IMPLEMENTATION -DMINIRENT_IMPLEMENTATION
HOHEADERS=stb_ds.h

ifdef USE_ASAN
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

ifeq ($(OS),Windows_NT)
HOHEADERS += ../minirent.h
endif

SRC=$(wildcard ../*.c) main.c test.c
HEADERS=$(wildcard ../*.h) stb_ds.h munit.h
TEMPWILDCARD=$(patsubst %.c,%.o,$(wildcard ../*.c))
LIBS=$(TEMPWILDCARD:../%=%) holibs.o
OBJS=$(LIBS) munit.o main.o test.o

all: main test
main: main.o $(LIBS)
test: test.o $(LIBS) munit.o
$(OBJS): $(HEADERS)

%.o: ../%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

holibs.o: $(HOHEADERS)
	cat $(HOHEADERS) | $(CC) -c $(CFLAGS) $(CPPFLAGS) $(IMPLEMENTATIONS) -o $@ -x c -

clean:
	rm -rf *.o depend.mk main test munit.c munit.h stb_ds.h

run_tests: test
	./test

stb_ds.h:
	curl --silent -O https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h

munit.h:
	curl --silent -O https://raw.githubusercontent.com/nemequ/munit/master/munit.h

munit.c:
	curl --silent -O https://raw.githubusercontent.com/nemequ/munit/master/munit.c
