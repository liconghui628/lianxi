
CC = gcc
TARGET=ping
OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))

$(TARGET):$(OBJECTS)
	$(CC) $(OBJECTS) -o $@

$(OBJECTS):%.o:%.c
	@echo "	CC	"	$@
	$(CC) -c -o $@ $<

.PHONY:clean
clean:
	rm -rf $(OBJECTS) $(TARGET)
