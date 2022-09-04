// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright XXX 2022
 *
 * Author: Tuo Cao <91tuocao@gmail.com>
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/timekeeping.h>
#include <linux/ktime.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <asm/barrier.h>
//#include <spin_lock_test.h>

//test mode
#define SPIN_LOCK_0_M			0 //just test spin_lock()
#define SPIN_LOCK_IRQSAVE_1_M		1 //just test spin_lock_irqsave()
#define SPIN_MIX_01_M			2 //first test spin_lock() and then spin_lock_irqsave()
#define SPIN_MIX_10_M			3 //first test spin_lock_irqsave() and then spin_lock()

spinlock_t my_lock;
spinlock_t my_lock_exp;

enum {
	MODE = 0,
	LOOP_COUNT,
}

static int api_exec_time_test(unsigned int count, int mode)
{
	int i;
	unsigned long flags;
	ktime_t tval1 = 0;
	ktime_t tval2 = 0;
	
	spin_lock(&my_lock_exp);
	local_irq_disable();
	isb();
	
	if (mode == SPIN_LOCK_0_M) {
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock(&my_lock);
			spin_unlock(&my_lock);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
	}
	else if (mode == SPIN_LOCK_IRQSAVE_1_M) {
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock_irqsave(&my_lock, flags);
			spin_unlock_irqrestore(&my_lock, flags);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock_irqsave, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
	}
	if (mode == SPIN_MIX_01_M) {
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock(&my_lock);
			spin_unlock(&my_lock);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
		
		isb();
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock_irqsave(&my_lock, flags);
			spin_unlock_irqrestore(&my_lock, flags);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock_irqsave, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
	}
	if (mode == SPIN_MIX_10_M) {
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock_irqsave(&my_lock, flags);
			spin_unlock_irqrestore(&my_lock, flags);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock_irqsave, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
		
		isb();
		tval1 = ktime_get();
		isb();
		for (i=0; i<count; i++) {
			spin_lock(&my_lock);
			spin_unlock(&my_lock);
		}
		isb();
		tval2 = ktime_get();
		prink(KERN_CRIT"[my_driver] leave spin_lock, loop:%d, tval2-tval1=%ld(ns)\n", count, ktime_to_ns(ktime_sub(tval2, tval1)));
	}
	
	local_irq_enable();
	spin_unlock(&my_lock_exp);

	return 0;
}

static ssize_t my_driver_write(struct file *file, const char __user *ubuffer, size_t len, loff_t *data)
{
	char kbuffer[10] = {0};
	unsigned int kint;
	
	if (len <= sizeof(kbuffer)) {
		if (copy_from_user(kbuffer, ubuffer, len-1))
			return -EFAULT;
		
		if (kstrtoint(&kbuffer[LOOP_COUNT], 10, &kint))
			return -ERANGE;
			
		printk(KERN_CRIT"[my_driver] Get buffer from user:%s => (mode:%c, loop_count:%d)\n", kbuffer, kbuffer[MODE], kint);
		
		if (kbuffer[MODE] == '0') {
			api_exec_time_test(kint, SPIN_LOCK_0_M);
		}
		else if (kbuffer[MODE] == '1') {
			api_exec_time_test(kint, SPIN_LOCK_IRQSAVE_1_M);
		}
		else if (kbuffer[MODE] == '2') {
			api_exec_time_test(kint, SPIN_MIX_01_M);
		}
		else if (kbuffer[MODE] == '3') {
			api_exec_time_test(kint, SPIN_MIX_10_M);
		}
		printk(KERN_CRIT"[my_driver] .................................................\n");
	}
	else {
		printk(KERN_CRIT"[my_driver] Too much buffer from user...\n");
		return -ERANGE;
	}
	
	return len;
}

static const struct proc_ops my_driver_proc_fops = {
	.proc_write = my_driver_write,
};

static int __init my_driver_init(void) 
{
	printk(KERN_CRIT"[my_driver] Init my_driver_init...\n");
	spin_lock_init(&my_lock);
	spin_lock_init(&my_lock_exp);
	proc_create_data("my_driver", 0777, NULL, &my_driver_proc_fops, NULL);
	
	return 0;
}

static void __exit my_driver_exit(void) 
{
	printk(KERN_CRIT"[my_driver] Exit my_driver_exit...\n");
}

module_init(my_driver_init);
module_exit(my_driver_exit);
MODULE_AUTHOR("Tuo Cao");
MODULE_DESCRIPTION("MY DRIVER");
MODULE_LICENSE("GPL-2");
