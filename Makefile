TARGETS = webtag

CC ?= gcc
CFLAGS_OSXFUSE = -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=26 -I/opt/local/include -I/usr/local/include/osxfuse/fuse -D_DARWIN_USE_64_BIT_INODE -mmacosx-version-min=10.5
CFLAGS_EXTRA = -Wall -g $(CFLAGS)
LIBS = -losxfuse -lcurl

.c:
	$(CC) $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $< $(LIBS)

all: $(TARGETS)

http: http.c

webtag: webtag.c

clean:
	rm -f $(TARGETS) *.o
