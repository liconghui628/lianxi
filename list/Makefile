
CC = gcc
DIRS = ./ list/ test/ qsort/
#FILE = $(foreach dir,$(DIRS),$(wildcard $(dir)*.c))
CFLAG = $(DIRS:%=-I %)
TARGET = node
#FILE = $(wildcard *.c)
#OBJECTS = $(patsubst %.c,%.o,$(FILE))
OBJECTS = $(patsubst %.c,%.o,$(wildcard $(DIRS:%=%*.c)))

$(TARGET):$(OBJECTS)
	$(CC) $(OBJECTS) -o $@
#	@echo $(FILE)

$(OBJECTS):%.o:%.c
	@echo "	CC	"	$@
	@$(CC) -c $(CFLAG) -o $@ $<

.PHONY:clean
clean:
	rm -rf $(OBJECTS) $(TARGET)
