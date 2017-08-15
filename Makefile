
LISPY_OBJS = \
	mpc.o \
	main.o

CFLAGS = -Wall -std=c99 -g

LISPY_BIN = lispy

all: $(LISPY_BIN)

$(LISPY_BIN): $(LISPY_OBJS)
	$(CC) $^ -o $@ -ledit

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o *~ $(LISPY_BIN)
