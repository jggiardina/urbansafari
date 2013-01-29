CFLAGS := -g
targets=server
src = server.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame
all: $(targets)
.PHONY: all
server: $(objs) ../lib/libdagame.a
	gcc $(CFLAGS) -o -g server $(objs) $(libs) -lpthread
$(objs) : $(src)
clean:
	rm $(objs) $(targets)
