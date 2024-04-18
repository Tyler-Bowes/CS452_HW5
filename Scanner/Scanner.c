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


//init()
// register_chrdev?

// open()
static int open(struct inode *inode, struct file *filp) {
  // allocate memory for file struct (can make many instances)
  File *file=(File *)kmalloc(sizeof(*file),GFP_KERNEL);
  if (!file) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM; // return error code
  }

  file->s=(char *)kmalloc(strlen(device.s)+1,GFP_KERNEL);
  if (!file->s) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    kfree(file); // free allocated memory
    return -ENOMEM;
  }
  strcpy(file->s,device.s);
  file->flag = false; // initialize flag to false (will be overwritten in ioctl())
  file->separators = device.separators; // initialize separators
  file->buf_size = strlen(device.s) + 1; // initialize buffer size
  filp->private_data=file;
  return 0;
}

// release()
static int release(struct inode *inode, struct file *filp) {
  File *file=filp->private_data;
  kfree(file->s); 
  kfree(file);
  return 0;
}

// read() copies until seperator
static ssize_t read(struct file *filp, // used in kernel space
		    char *buf, // buffer (what write to)
		    size_t count,  // size of buffer
		    loff_t *f_pos) { // pos offset, dont worry about
  File *file=filp->private_data;  // pointer to file struct
  
  int n=strlen(file->s);  // calculate the length of the string
  n=(n<count ? n : count); 

  // check for finding seperator
  // if there is one find its index and return the index -1
  // if there is no seperator return the length of the string
  for (int i=0; i<n; i++) {
    if (strchr(file->separators,file->s[i])) {
      n=i+1;
      break;
    }
  }

  if (copy_to_user(buf,file->s,n)) {  // copy_to_user() is a kernel function
    printk(KERN_ERR "%s: copy_to_user() failed\n",DEVNAME);
    return 0;
  }
  return n;
}

// // write()
// static ssize_t write(struct file *file, 
                      // const char __user *buf, 
                      // size_t count, 
                      // loff_t *ppos) {
//   return 0;  // return success or failure
//
// }
// copy_from_user()

// // ioctl()
static long ioctl(struct file *file, 
                  unsigned int cmd,  // used for rewriting seperators
                  unsigned long arg) { // used for 
  File *file=filp->private_data;
  if (cmd == 0) { // set seperators
  // rewriting seperators happens in write()
    // if (copy_from_user(file->separators,(char *)arg,strlen(file->separators)+1)) {
    //   printk(KERN_ERR "%s: copy_from_user() failed\n",DEVNAME);
    //   return -EFAULT;
    // }
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

// init()
static int __init my_init(void) {
  const char *s="Hello:world!\n";  // Testing string
  int err;
  // allocate memory for device.s
  device.s=(char *)kmalloc(strlen(s)+1,GFP_KERNEL);
  if (!device.s) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  strcpy(device.s,s); // copy s to device.s
  // register device 
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
