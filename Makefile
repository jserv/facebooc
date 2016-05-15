CFLAGS = -O2 -g -std=c99 -Wall -I include
LDFLAGS = -lsqlite3

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lws2_32
endif

UNAME_S = $(shell uname -s)

# strtok_r is provided by POSIX.1c-1995 and POSIX.1i-1995, however, with
# the POSIX_C_SOURCE=1 on Mac OS X is corresponding to the version of
# 1988 which is too old (defined in sys/cdefs.h)
CFLAGS += -D_POSIX_C_SOURCE=199506L


OUT = bin
EXEC = $(OUT)/facebooc
OBJS = \
	src/kv.o \
	src/response.o \
	src/template.o \
	src/main.o \
	src/bs.o \
	src/request.o \
	src/list.o \
	src/models/like.o \
	src/models/account.o \
	src/models/connection.o \
	src/models/session.o \
	src/models/post.o \
	src/server.o

deps := $(OBJS:%.o=%.o.d)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -MMD -MF $@.d -c $<

$(EXEC): $(OBJS)
	mkdir -p $(OUT)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

all: $(EXEC)
run: $(EXEC)
	@echo "Starting Facebooc service..."
	@./$(EXEC) $(port)

clean:
	$(RM) $(OBJS) $(EXEC) $(deps)
distclean: clean
	$(RM) db.sqlite3

-include $(deps)
