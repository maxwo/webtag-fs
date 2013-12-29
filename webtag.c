#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <fuse.h>
#include <stdlib.h>
#include <syslog.h>

#include "http.c"
#include "json.c"

struct Connection connection;

char*
last_slash_of_path(char *path) {
	
	char *current, *last_slash;
	
	last_slash = path;
	current = path;
	while ( *current!=0 ) {
		if ( *current=='/' ) {
			last_slash = current;
		}
		current++;
	}
	
	return last_slash;
	
}

int is_tag(char *path)
{
	struct Request request;
	JsonNode *json, *json_tags, *json_tag, *tag_value;
	
	char *work, *last_slash;
	int len = strlen(path);
	
	if ( len==0 || *(path+len-1)=='/' ) {
		return 1;
	}
	
	work = malloc(sizeof(char) * (len+1));
	strcpy(work, path);
	last_slash = (char *)last_slash_of_path(work);
	*last_slash = 0;
	
	init_request(&connection, &request, work, DATA_TYPE_MEMORY);
	process_request(&connection, &request);
	json = json_decode(request.response->data);
	json_tags = json_find_member(json, "tags");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "tag");
		if ( strcmp(tag_value->string_, last_slash+1)==0 ) {
	        return 1;
		}
	}
	json_delete(json);
	destroy_request(&request);
	
	free(work);
	
	return 0;
}

static int
webtag_getattr(const char *path, struct stat *stbuf)
{
	struct Request request;
	JsonNode *json, *json_tags, *json_tag, *filename_value, *size_value;
	char *last_slash, *path_only;
	int path_len;

    memset(stbuf, 0, sizeof(struct stat));
	
	if ( is_tag((char *)path) ) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
		return 0;
	}
	
	path_len = strlen(path);
	
	path_only = malloc(sizeof(char) * (path_len+1));
	strcpy(path_only, path);
	last_slash = last_slash_of_path(path_only);
	*last_slash = 0;
	
	init_request(&connection, &request, path_only, DATA_TYPE_MEMORY);
	process_request(&connection, &request);
	
	json = json_decode(request.response->data);
	json_tags = json_find_member(json, "inodes");
	json_foreach(json_tag, json_tags) {
		filename_value = json_find_member(json_tag, "filename");
		if ( strcmp(filename_value->string_, last_slash+1)==0 ) {
	        stbuf->st_mode = S_IFREG | 0444;
	        stbuf->st_nlink = 1;
			
			size_value = json_find_member(json_tag, "size");
			stbuf->st_size = (size_t) size_value->number_;
		}
	}
	json_delete(json);
	
	destroy_request(&request);
	
	free(path_only);
	
	return 0;
}

static int
webtag_open(const char *path, struct fuse_file_info *fi)
{
    if ((fi->flags & O_ACCMODE) != O_RDONLY) 
        return -EACCES;

    return 0;
}

static int
webtag_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi)
{
	struct Request request;
	JsonNode *json, *json_tags, *json_tag, *tag_value;

    init_request(&connection, &request, path, DATA_TYPE_MEMORY);
    process_request(&connection, &request);

    filler(buf, ".", NULL, 0); 
    filler(buf, "..", NULL, 0);
	
	json = json_decode(request.response->data);
	json_tags = json_find_member(json, "tags");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "tag");
	    filler(buf, tag_value->string_, NULL, 0);
	}
	
	json_tags = json_find_member(json, "inodes");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "filename");
	    filler(buf, tag_value->string_, NULL, 0);
	}
	json_delete(json);
	
	destroy_request(&request);

    return 0;
}

static int
webtag_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
	struct Request request;
	struct Response *response;
	
    init_request(&connection, &request, path, DATA_TYPE_FILE);
    process_request(&connection, &request);

	response = request.response;

    if (offset >= response->size) {
		destroy_request(&request);
        return 0;
	}

    if (offset + size > response->size) 
        size = response->size - offset;
	
	read_response(response, buf, size, offset);

	destroy_request(&request);
	
    return size;
}

static int
webtag_destroy(struct fuse *f) {
	
	destroy_connection(&connection);
	
}

static struct fuse_operations webtag_filesystem_operations = {
    .getattr = webtag_getattr, 
    .open    = webtag_open,    
    .read    = webtag_read,    
    .readdir = webtag_readdir,
	.destroy = webtag_destroy,
};

int
main(int argc, char **argv)
{
	init_connection(&connection, argv[1]);
	
    return fuse_main(argc-1, argv+1, &webtag_filesystem_operations, NULL);
}
