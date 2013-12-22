#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <fuse.h>
#include <stdlib.h>
#include <syslog.h>

#include "http.c"

struct Connection connection;

static int
webtag_getattr(const char *path, struct stat *stbuf)
{
	struct Request request;
	
    init_request(&connection, &request, path, DATA_TYPE_MEMORY);
    process_request(&connection, &request);
	
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) { 
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
    } else { 	
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
		if ( request.status==REQUEST_STATUS_FINISHED ) {
        	stbuf->st_size = request.response->size;
		}
    }
	
	destroy_request(&request);

    return 0;
}

static int
webtag_open(const char *path, struct fuse_file_info *fi)
{
    //if (strcmp(path, file_path) != 0) 
    //    return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY) 
        return -EACCES;

    return 0;
}

static int
webtag_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi)
{
    if (strcmp(path, "/") != 0) 
        return -ENOENT;

    filler(buf, ".", NULL, 0); 
    filler(buf, "..", NULL, 0); 
    filler(buf, "/index.html" + 1, NULL, 0); 

    return 0;
}

static int
webtag_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
	struct Request request;
	struct Response *response;
	
    init_request(&connection, &request, path, DATA_TYPE_MEMORY);
    process_request(&connection, &request);

	//response->size = 10;

    if (offset >= response->size) {
		destroy_request(&request);
        return 0;
	}

    if (offset + size > response->size) 
        size = response->size - offset;

    memcpy(buf, request.response->data + offset, size); 

	destroy_request(&request);
	
    return size;
}

static struct fuse_operations webtag_filesystem_operations = {
    .getattr = webtag_getattr, 
    .open    = webtag_open,    
    .read    = webtag_read,    
    .readdir = webtag_readdir, 
};

int
main(int argc, char **argv)
{
	init_connection(&connection, argv[1]);
	
    return fuse_main(argc-1, argv+1, &webtag_filesystem_operations, NULL);
}
