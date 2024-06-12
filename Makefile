CC = gcc
CFLAGS = -O2
LDFLAGS =
OBJFILES = upld.o cgilib.o md5.o
TARGET = upld.cgi

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

upld.o: upld.c
	$(CC) $(CFLAGS) -c upld.c

cgilib.o: cgilib.c
	$(CC) $(CFLAGS) -c cgilib.c

md5.o: md5.c
	$(CC) $(CFLAGS) -c md5.c

clean:
	rm -f $(OBJFILES) $(TARGET)

.PHONY: all clean
