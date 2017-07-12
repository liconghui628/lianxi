
CC = arm-none-linux-gnueabi-gcc
TARGET = sys.cgi
FILE = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(FILE))

$(TARGET):$(OBJECTS)
	$(CC) $(OBJECTS) -o $@
#	@echo $(FILE)

$(OBJECTS):%.o:%.c
	@echo "	CC	"	$@
	@$(CC) -c -o $@ $<

.PHONY:clean
clean:
	rm -rf $(OBJECTS) $(TARGET)
