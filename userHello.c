#include <stdio.h>


typedef struct {
  dev_t devno;
  struct cdev cdev;
  char *s;
} Device;			/* per-init() data */

typedef struct {
  char *s;
  bool flag;  // knowing if seperators should be overwritten
} File;				/* per-open() data */

static Device device;
//The init() function must establish a reasonable set of default separators.
//The default set can be overridden in particular scanner instances
static int init(void) {
  device.s=" \t\n";
  return 0;
}


// The next token is scanned by calling read(). 
// return the number of characters scanned
// If the length of the token exceeds the number of characters requested, another read() scans more of the same token
// •A return value of 0 indicates “end of token.”
// •A return value of -1 indicates “end of data.
// int read(File* file, size_t count) {
//     int n = strlen(file->s);
//     n = (n < count ? n : count);
//     if (copy_to_user(, file->s, n)) {
//         return 0;
//     }
//     return n;
// }

// The open() function must create an instance of a scanner. A process
// can have multiple instances open concurrently. They might be scanning
// different data with different separators
FILE* open(const char* filename) {
    File* file = (File*)malloc(sizeof(*file));  // allocate memory for file
    file.
    FILE* file = fopen(filename, "r");  // open file
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return NULL;
    }
    return file;  // return file pointer
}

// The set of separators is specified by calling ioctl() with a request of 0,
// followed by calling write() with the separator characters.
// The separators are not null-terminated.
int ioctl(int request, FILE* file) {
    if (request != 0) {
        printf("do nothing\n");
        return -1;
    }
    // file points to stuct that contains diver data
    File.flag = true;
    return 0;
}
