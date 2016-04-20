CFLAGS = -O2 -g -Wall -I include
LDFLAGS = -lsqlite3

EXEC = bin/facebooc
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
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

all: $(EXEC)
run: $(EXEC)
	@echo "Starting Facebooc service..."
	@./$(EXEC)

clean:
	$(RM) $(OBJS) $(EXEC) $(deps)
distclean: clean
	$(RM) db.sqlite3

-include $(deps)
