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
} Device;			/* per-init() data */

typedef struct {
  char *s;
} File;				/* per-open() data */

static Device device;


// open()
static int open(struct inode *inode, struct file *filp) {
  File *file=(File *)kmalloc(sizeof(*file),GFP_KERNEL);
  if (!file) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  file->s=(char *)kmalloc(strlen(device.s)+1,GFP_KERNEL);
  if (!file->s) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  strcpy(file->s,device.s);
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
  // if there is one find its index and return -1

  if (copy_to_user(buf,file->s,n)) {  // copy_to_user() is a kernel function
    printk(KERN_ERR "%s: copy_to_user() failed\n",DEVNAME);
    return 0;
  }
  return n;
}

// // write()
// static ssize_t write() {
//   return 0;  // return success or failure
//
// }
// copy_from_user()

// // ioctl()
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
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
  const char *s="Hello world!\n";
  int err;
  device.s=(char *)kmalloc(strlen(s)+1,GFP_KERNEL);
  if (!device.s) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  strcpy(device.s,s);
  err=alloc_chrdev_region(&device.devno,0,1,DEVNAME);
  if (err<0) {
    printk(KERN_ERR "%s: alloc_chrdev_region() failed\n",DEVNAME);
    return err;
  }
  cdev_init(&device.cdev,&ops);
  device.cdev.owner=THIS_MODULE;
  err=cdev_add(&device.cdev,device.devno,1);
  if (err) {
    printk(KERN_ERR "%s: cdev_add() failed\n",DEVNAME);
    return err;
  }
  printk(KERN_INFO "%s: init\n",DEVNAME);
  return 0;
}

// exit()
static void __exit my_exit(void) {
  cdev_del(&device.cdev);
  unregister_chrdev_region(device.devno,1);
  kfree(device.s);
  printk(KERN_INFO "%s: exit\n",DEVNAME);
}

module_init(my_init);
module_exit(my_exit);
