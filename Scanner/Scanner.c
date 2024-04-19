#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BSU CS 452 HW4");
MODULE_AUTHOR("<buff@cs.boisestate.edu>");

typedef struct {
  dev_t devno;
  struct cdev cdev;
  char *s;
  char *separators; // same as file (default separators)
} Device;			/* per-init() data */

typedef struct {
  char *s; // character string (buffer)
  bool flag; // knowing if seperators should be overwritten
  char *separators; // seperators and can initialized in init
  size_t buf_size; // buffer size
} File;				/* per-open() data (file descriptor)*/

static Device device;


/* open() initializes a file descriptor with device data as default
*/
static int open(struct inode *inode, struct file *filp) {
  // allocate memory for file struct (can make many instances)
  printk("PRINTED at start of OPEN\n");

  File *file=(File *)kmalloc(sizeof(*filp),GFP_KERNEL);
  if (!file) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -1; // return error code
  }

  // allocate memory for device.s
  // file->s=(char *)kmalloc(strlen(device.s)+1,GFP_KERNEL);
  // if (!file->s) {
  //   printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
  //   kfree(file); // free allocated memory
  //   return -ENOMEM;
  // }
  // strcpy(file->s,device.s);
  // memcpy(file->s,device.s, file->)

  // allocate memory for seperators
  file->separators=(char *)kmalloc(strlen(device.separators)+1,GFP_KERNEL);
  if (!file->separators) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    kfree(file->s); // free allocated memory
    kfree(file); // free allocated memory
    return -2;
  }
  strcpy(file->separators,device.separators);

  file->flag = false; // initialize flag to false (will be overwritten in ioctl())
  file->buf_size = 100;
  // file->separators = device.separators; // initialize separators
  // file->buf_size = strlen(device.s) + 1; // initialize buffer size
  filp->private_data=file;
  printk("PRINTED at end of OPEN\n");
  return 0;
}

/*
release()  will free the memory allocated for the file struct and everything in it
*/
static int release(struct inode *inode, struct file *filp) {
  File *file= filp->private_data;
  kfree(file->s); 
  kfree(file);
  return 0;
}

// read() copies until seperator
/* read takes and scans the string until it finds a seperator, 0 or the end of the string
  if a seperator first it will copy_to_user everything from the start of parsing to the seperator
    then next read would start from the next character, which is the seperator and return 0
  the same sequence of events would happen if the end of the string is reached 

  Example: sting = "cat:dog"
  read1 call: "cat:dog" -> 3
  read2 call: "cat:dog" -> 0  (seperator found)
  read3 call: "cat:dog" -> 3
  read4 call: "cat:dog" -> 0  (end of file)
  read5 call: "cat:dog" -> -1 (end of reading)
*/
static ssize_t read(struct file *filp, // used in kernel space
		    char *buf, // buffer (what write to)
		    size_t count,  // size of buffer
		    loff_t *f_pos) { // pos offset, dont worry about
  File *file= filp->private_data;  // pointer to file struct
  
  // if we finished scanning document
  if (file->buf_size <= 0) {
    return -1;
  }

  // initially n = size of the string
  ssize_t n = file->buf_size;

  // Parse the data for the ':' character to find seperator
  char *found = strchr(file->s, ':');
  // if there is one find its index and return the index -1
  if (found) {
    // If the ':' character is found, copy everything before it back to user space
    n = found - file->s; // length of string before ':'
    if (copy_to_user(buf, file->s, n)) {
        printk(KERN_ERR "%s: copy_to_user() failed\n", DEVNAME);
        kfree(file->s);  // Free memory if copy fails
        return -EFAULT;  // Return error if copy fails
    }
    // check if n is 0
    if (n == 0) { // handles empty tokens
      // move the sting over the seperator
    }
    // Move the string pointer to the character after the ':' character
    file->s = found + 1;
    file->buf_size -= n + 1; // Update the buffer size
  } else {
    // If the ':' character is not found, copy the entire string back to user space
    if (copy_to_user(buf, file->s, n)) {
        printk(KERN_ERR "%s: copy_to_user() failed\n", DEVNAME);
        kfree(file->s);  // Free memory if copy fails
        return -EFAULT;  // Return error if copy fails
    }
    // n bytes were read
    file->buf_size -= n; // Update the buffer size

  }
  // if there is no seperator return the length of the string
  

  // for (int i=0; i<n; i++) {
  //   if (strchr(file->separators,file->s[i])) {
  //     n=i+1;
  //     break;
  //   }
  // }

  // if (copy_to_user(buf,file->s,n)) {  // copy_to_user() is a kernel function
  //   printk(KERN_ERR "%s: copy_to_user() failed\n",DEVNAME);
  //   return 0;
  // }
  return n;
}

/*
The write function takes in the data from the user space and writes it to the kernel space.
It also checks if ioctl was called to rewrite the seperators (setting flag to true)
  if flag was set, it will reset the seperators and set the flag to false
*/
static ssize_t write(struct file *filp, 
                      const char __user *buf, 
                      size_t count, 
                      loff_t *ppos) {
  // reset the seperators if flag is true
  File *file=filp->private_data;  // pointer to file struct
  if (file->flag) {  // flag is true, reset seperators and set to false
    if (file->separators) { kfree(file->separators); } // free allocated memory to reset
    file->separators = (char *)kmalloc(count+1,GFP_KERNEL); // allocate memory for seperators
    if (!file->separators) { 
      printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
      return -ENOMEM;
    }
    if (copy_from_user(file->separators,buf,count)) { // copy seperators from user
      printk(KERN_ERR "%s: copy_from_user() failed\n",DEVNAME);
      return -EFAULT;
    }
    file->flag = false;
  } else {
    // do nothing, seperators in file descriptor are not being overwritten
  }

  // read from scanf / user space
  file->s = (char *)kmalloc(count+1, GFP_KERNEL); // allocate memory for data
  // char *data = (char *)kmalloc(count+1, GFP_KERNEL); // 
  if (!file->s) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  // copy data from user space to kernel space
  if (copy_from_user(file->s, buf, count)) { 
    printk(KERN_ERR "%s: copy_from_user() failed\n",DEVNAME);
    kfree(file->s); // Free the allocated memory
    return -EFAULT;
  }
  file->s[count] = '\0'; // null terminate the string for parsing
  file->buf_size = count; // set the buffer size to the count

  return count;  // return the number of bytes written
}

/* ioctl takes in a command and arguments
there is only one command, 0, which is used to rewrite the seperators
  When the command is called flag will be set so the next write can rewrite the seperators
*/
static long ioctl(struct file *filp, 
                  unsigned int cmd,  // used for rewriting seperators
                  unsigned long arg) { // arguments used for if cmd had args
  File *file= filp->private_data;
  if (cmd == 0) { // set seperators
    file->flag = true; // set flag to true
  } else {
    // do nothing, seperators are not being rewritten
  }
  return 0;
}

static struct file_operations ops={
  .open=open,
  .release=release,
  .read=read,
  .unlocked_ioctl=ioctl,
  .owner=THIS_MODULE
};

/* init() initializes the device and registers it
*/
static int __init my_init(void) {
  // const char *s="Hello:world!\n";  // Testing string
  int err;
  // allocate memory for device.s
  // device.s=(char *)kmalloc(strlen(s)+1,GFP_KERNEL);
  // if (!device.s) {
  //   printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
  //   return -ENOMEM;
  // }
  // strcpy(device.s,s); // copy s to device.s

  // register device 
  printk("PRINTING IN INIT1");
  err=alloc_chrdev_region(&device.devno,0,1,DEVNAME); 
  if (err<0) { 
    printk(KERN_ERR "%s: alloc_chrdev_region() failed\n",DEVNAME);
    return err;
  }
  // initialize cdev
  cdev_init(&device.cdev,&ops); 
  device.cdev.owner=THIS_MODULE; 
  err=cdev_add(&device.cdev,device.devno,1); 
  if (err) {
    printk(KERN_ERR "%s: cdev_add() failed\n",DEVNAME);
    return err;
  }
  // initialize seperators
  const char *sep=" ,";  // default seperators
  device.separators=(char *)kmalloc(strlen(sep)+1,GFP_KERNEL);
  if (!device.separators) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  strcpy(device.separators,sep);
  
  printk(KERN_INFO "%s: init\n",DEVNAME);
  return 0;
}

// exit()
static void __exit my_exit(void) {
  cdev_del(&device.cdev);
  unregister_chrdev_region(device.devno,1);
  kfree(device.s);
  kfree(device.separators);  // frees default separators in device 
  printk(KERN_INFO "%s: exit\n",DEVNAME);
}

module_init(my_init);
module_exit(my_exit);
