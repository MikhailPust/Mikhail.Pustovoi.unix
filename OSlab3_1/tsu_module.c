#include<linux/kernel.h>
#include <linux/module.h> /* Needed by all modules */
#include <linux/printk.h> /* Needed for pr_info() */
#include<linux/proc_fs.h>
#include<linux/uaccess.h>
#include <linux/version.h>
 
MODULE_LICENSE("GPL");
 
static int __init tsu_module_init(void) 
{
    pr_info("Welcome to the Tomsk State University\n");
    return 0;
}
 
static void __exit tsu_module_exit(void)
{
    pr_info("Tomsk State University forever!\n");
}
 
module_init(tsu_module_init);
module_exit(tsu_module_exit);
MODULE_LICENSE("GPL");
