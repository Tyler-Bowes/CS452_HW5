#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct {
    dev_t devno;
    struct cdev cdev;
    char *s;
    char *separators;
} Device;			/* per-init() data */

typedef struct {
    char *s;
    bool flag;  // knowing if seperators should be overwritten
    char *separators;
  // seperators and can initialized in init
} File;				/* per-open() data */

static Device device;


//The init() function must establish a reasonable set of default separators.
//The default set can be overridden in particular scanner instances
static void init(void) {
    const char *s = "Hello world!\n";
    const char *default_separators = " ,.;\n";
    device.s = (char *)malloc(strlen(s) + 1);
    device.separators = (char *)malloc(strlen(default_separators) + 1);
    if (!device.s  || !device.separators) {
        printf("%s: malloc() failed\n", "DEVNAME");
        return -1;
    }
    strcpy(device.s, s);
    strcpy(device.separators, default_separators);
    printf("%s: init\n", "DEVNAME");
    return 0;
}


// The open() function must create an instance of a scanner. A process
// can have multiple instances open concurrently. They might be scanning
// different data with different separators

/*allocates memory for a File structure and a string, copies 
device_s to the string, and sets the File pointer to point to 
the new File structure
*/
int open(File **file, const char *device_s, const char *separators) {
    *file = (File *)malloc(sizeof(**file));
    if (!*file) { // check if malloc failed
        printf("%s: malloc() failed\n", "DEVNAME");
        return -1;
    }
    (*file)->s = (char *)malloc(strlen(device_s) + 1); 
    (*file)->separators = (char *)malloc(strlen(separators) + 1);
    if (!(*file)->s || !(*file)->separators) { // check if malloc failed
        printf("%s: malloc() failed\n", "DEVNAME");
        return -1;
    }
    strcpy((*file)->s, device_s); // copy device_s to the string
    strcpy((*file)->separators, separators); // copy separators to the string
    return 0;
}

// The next token is scanned by calling read(). 
// return the number of characters scanned
// If the length of the token exceeds the number of characters requested, another read() scans more of the same token
// •A return value of 0 indicates “end of token.”
// •A return value of -1 indicates “end of data.
ssize_t read_file(const char* filename, char* buf, size_t count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return 0;
    }
    int n = strlen(file->s);
    n = (n < count ? n : count);
    strncpy(buf, file->s, n);
    buf[n] = '\0';

    // size_t n = fread(buf, sizeof(char), count, file); 
    // buf[n] = '\0'; // Ensure the buffer is null-terminated

    fclose(file);
    return n;
}

ssize_t read_file(File* file, char* buf, size_t count) {
    // Check if there's more data to read
    if (file->s == NULL) {
        return -1;
    }

  // Calculate the number of characters to read (consider count and remaining data)
  int n = (strlen(device.s) -  < count) ? strlen(device.s) -  : count;

  // Copy data to the user buffer
  strncpy(buf, device.s + , n);
  buf[n] = '\0'; // Ensure null termination

  return n;
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
    file.flag = true;
    // // Update separators based on data written using write() (assuming device.separators holds the separators)
    // free(device.separators);  // Free existing memory
    // device.separators = (char *)malloc(count + 1); // Allocate memory for new separators
    // if (device.separators == NULL) {
    //     printf("malloc failed\n");
    //     return -1;
    // }
    // strncpy(device.separators, buf, count);  // Copy new separators from write()
    // device.separators[count] = '\0';  // Ensure null termination
    return 0;
}


// The sequence of characters to scan is specified by calling write(). 
// Such calls are not cumulative: each call specifies a new sequence to scan.

ssize_t write(int file_desc, const char* buf, size_t count) {
    // 1. Set the separators by calling ioctl() with a request of 0
    // ioctl(fd, 0, NULL);

    // 2. Specify the sequence of characters to scan by calling write()
    // ssize_t n = write(fd, buf, count); 

    // Update scanner data (assuming device.s holds the data)
    device.s = realloc(device.s, strlen(device.s) + count + 1);
    if (device.s == NULL) {
        printf("realloc failed\n");
        return -1;
    }
    strncat(device.s, buf, count);
    device.s[strlen(device.s)] = '\0'; // Ensure null termination

    // Check for errors
    // if (n == -1) {
    //     printf("write\n");
    //     return -1;
    // }

    // return n;
}