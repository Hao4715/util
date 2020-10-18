CC = gcc

CFLAGS = -g 
LFLAGS = -shared -fPIC  

TARGET = libFixedPool.so
INCFILE = fixed_pool.h

LIBPATH = /usr/lib64/proxyd/
INCPATH = /usr/include/proxyd/


SOUCE_FILES = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SOUCE_FILES))


$(TARGET): $(SOUCE_FILES)
	$(CC) $(LFLAGS) $(CFLAGS) $^ -o $@

.PHONY:clean
clean:
	$(RM) $(TARGET) $(OBJS)
	$(RM) $(LIBPATH)/$(TARGET)
	$(RM) $(INCPATH)/$(INCFILE)

install:
	cp -rf *.h $(INCPATH)
	cp -rf $(TARGET) $(LIBPATH)
