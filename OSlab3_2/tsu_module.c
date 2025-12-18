#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/string.h>

#define PROC_FILENAME "tsulab"
#define MARS_SYNODIC_DAYS 780
#define SECONDS_IN_DAY 86400
#define KNOWN_LAST_ENTRY 1725148800 

static struct proc_dir_entry *proc_file = NULL;

static void calculate_mars_logic(long long *days_left, int *years, int *days_rem) {
    struct timespec64 now;
    long long current_time;
    long long target_time = KNOWN_LAST_ENTRY;

    ktime_get_real_ts64(&now);
    current_time = now.tv_sec;

    while (target_time < current_time) {
        target_time += (long long)MARS_SYNODIC_DAYS * SECONDS_IN_DAY;
    }

    long long diff_sec = target_time - current_time;
    *days_left = diff_sec / SECONDS_IN_DAY;

    *years = (int)(*days_left / 365);
    *days_rem = (int)(*days_left % 365);
}

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer, 
                             size_t buffer_length, loff_t* offset) {
    char result[300];
    int len;
    long long total_days;
    int y, d;

    calculate_mars_logic(&total_days, &y, &d);

    len = snprintf(result, sizeof(result), 
        "TSU Libra Lab: Dynamic Mars Tracker\n"
        "--------------------------------------\n"
        "Mars will return to Libra in: %lld days\n"
        "Which is approximately: %d years and %d days.\n"
        "Cycle based on %d-day synodic period.\n", 
        total_days, y, d, MARS_SYNODIC_DAYS);

    if (*offset >= len) return 0;
    if (buffer_length < len - *offset) len = len - *offset;
    if (copy_to_user(buffer, result + *offset, len)) return -EFAULT;

    *offset += len;
    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops tsulab_fops = { .proc_read = procfile_read };
#else
static const struct file_operations tsulab_fops = { .read = procfile_read };
#endif

static int __init tsu_module_init(void) {
    pr_info("Welcome to the Tomsk State University\n");
    proc_file = proc_create(PROC_FILENAME, 0444, NULL, &tsulab_fops);
    if (!proc_file) return -ENOMEM;
    pr_info("/proc/%s created\n", PROC_FILENAME);
    return 0;
}

static void __exit tsu_module_exit(void) {
    if (proc_file) {
        proc_remove(proc_file);
        pr_info("/proc/%s removed\n", PROC_FILENAME);
    }
    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_module_init);
module_exit(tsu_module_exit);
MODULE_LICENSE("GPL");
