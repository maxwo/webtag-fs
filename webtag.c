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

void
log(char* string) {
	
	
	FILE *f = fopen("/Users/max/filesystems/filesystems-c/hello/blabla/webtagfs/logs.txt", "a");
	fputs(string, f);
	fputs("\n", f);
	fclose(f);
	
}

static int
webtag_getattr(const char *path, struct stat *stbuf)
{
	struct Request request;
	JsonNode *json, *json_tags, *json_tag, *tag_value;
	char *file_name;
	
	int path_len = strlen(path);
	int i;
	
	for ( i=0 ; i<path_len ; i++ ) {
		if ( *(path+i)=='/' ) {
			file_name = path + i + 1;
		}
	}
	
	log("**********************");
	log(path);
	log(file_name);
	
    memset(stbuf, 0, sizeof(struct stat));
	
	if ( strcmp(path, "/")==0 ) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
		return 0;
	}

		char* path_only = malloc((int)(file_name - path) + 1);
		strcpy(path_only, path);
		*(path_only + ((int)(file_name - path))) = 0;
		log(path_only);
	
	init_request(&connection, &request, path_only, DATA_TYPE_MEMORY);
	process_request(&connection, &request);

	json = json_decode(request.response->data);
	json_tags = json_find_member(json, "tags");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "tag");
		if ( strcmp(tag_value->string_, file_name)==0 ) {
	        stbuf->st_mode = S_IFDIR | 0755;
	        stbuf->st_nlink = 3;
			stbuf->st_size = 0;
		}
	}
	json_delete(json);
	
	json = json_decode(request.response->data);
	json_tags = json_find_member(json, "inodes");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "filename");
		if ( strcmp(tag_value->string_, file_name)==0 ) {
	        stbuf->st_mode = S_IFREG | 0444;
	        stbuf->st_nlink = 1;
			stbuf->st_size = 0;
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
	json_delete(json);

	json = json_decode(request.response->data);
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
	
	struct Request request;
	JsonNode *json, *json_tags, *json_tag, *tag_value;
	
    init_request(&connection, &request, "/", DATA_TYPE_MEMORY);
    process_request(&connection, &request);
	
	json = json_decode(request.response->data);
	
	json_tags = json_find_member(json, "tags");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "tag");
	    puts(tag_value->string_);
	}
	
	json_tags = json_find_member(json, "inodes");
	json_foreach(json_tag, json_tags) {
		tag_value = json_find_member(json_tag, "filename");
	    puts(tag_value->string_);
	}
	
	json_delete(json);

	destroy_request(&request);
	
	
	char *path = "/path/vers/un/fichier.txt";
	char *file_name;
	
	int path_len = strlen(path);
	int i;
	
	for ( i=0 ; i<path_len ; i++ ) {
		if ( *(path+i)=='/' ) {
			file_name = path + i + 1;
		}
	}
	
	puts(file_name);
	
    return fuse_main(argc-1, argv+1, &webtag_filesystem_operations, NULL);
}
