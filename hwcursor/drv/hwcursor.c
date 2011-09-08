#include <linux/types.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/ipu.h>
#include <asm/cacheflush.h>
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");

// IPU_REG_BASE = 0x4000000 + 0x1e000000 = 0x5e000000
#define IPU_REG_BASE        0x5e000000
#define IPU_CM_REG_BASE		0x00000000
#define IPU_SRM_REG_BASE	0x01040000

#define IPU_SRM_PRI2        (ipu_cm_reg + 0x00A4/4)

#define DP_COM_CONF(flow)   (ipu_dp_reg + flow/4)
#define DP_CUR_POS(flow)    (ipu_dp_reg + 0x0c/4 + flow/4)
#define DP_CUR_MAP(flow)    (ipu_dp_reg + 0x10/4 + flow/4)

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC

#define DP_COC_DEF_MASK  0x00000070

static int hw_cur_major;
static struct class *hw_cur_class;
static struct device *hw_cur_dev;

static u32 *ipu_dp_reg;
static u32 *ipu_cm_reg;

static int hw_cur_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    return ret;
}

static int hw_cur_mmap(struct file *file, struct vm_area_struct *vma)
{
	u32 *addr = NULL;
    vma->vm_page_prot = pgprot_writethru(vma->vm_page_prot);

	if ( vma->vm_pgoff == 0 )
		addr = IPU_REG_BASE + IPU_CM_REG_BASE;
	else if ( vma->vm_pgoff == 1 )
		addr = IPU_REG_BASE + IPU_SRM_REG_BASE;
	else
		return -EINVAL;

    if (remap_pfn_range(vma, vma->vm_start, (u32)addr >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot)) {
        printk(KERN_ERR "hw cursor mmap failed!\n");
        return -ENOBUFS;
    }
    return 0;
}

static int hw_cur_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations hw_cur_fops = {
    .owner = THIS_MODULE,
    .open  = hw_cur_open,
    .mmap  = hw_cur_mmap,
    .release = hw_cur_release,
};

static int hw_cur_init(void)
{
	int ret = 0;
	u32 reg;

	hw_cur_major = register_chrdev(0, "hw_cursor", &hw_cur_fops);
	if ( hw_cur_major < 0 ) {
		printk(KERN_ERR "Unable to register hw_cursor as char device\n");
		return hw_cur_major;
	}

	hw_cur_class = class_create(THIS_MODULE, "hw_cursor");
	if ( IS_ERR(hw_cur_class) ) {
		printk(KERN_ERR "Unable to create class for hw_cursor\n");
		ret = PTR_ERR(hw_cur_class);
		goto err1;
	}

	hw_cur_dev = device_create(hw_cur_class, NULL, MKDEV(hw_cur_major, 0),
		NULL, "hw_cursor");
	if ( IS_ERR(hw_cur_dev) ) {
		printk(KERN_ERR "Unable to create device for hw_cursor\n");
		ret = PTR_ERR(hw_cur_dev);
		goto err2;
	}

	//ipu_dp_reg = ioremap(IPU_REG_BASE + IPU_SRM_REG_BASE, PAGE_SIZE);
	//ipu_cm_reg = ioremap(IPU_REG_BASE + IPU_CM_REG_BASE, PAGE_SIZE);

	//reg = __raw_readl(IPU_SRM_PRI2);
	//printk(KERN_ERR "common = %x\n", reg);
	//reg &= ~DP_COC_DEF_MASK;
	//reg |= 0x10;
	//__raw_writel(reg, DP_COM_CONF(DP_SYNC));

	//__raw_writel(0x00ff0000, DP_CUR_MAP(DP_SYNC));
	//__raw_writel(0x30643094, DP_CUR_POS(DP_SYNC));

	//reg = __raw_readl(IPU_SRM_PRI2) | 0x8;
	//__raw_writel(reg, IPU_SRM_PRI2);

    printk(KERN_ALERT "hw cursor driver init\n");
    return ret;

err2:
	class_destroy(hw_cur_class);

err1:
	unregister_chrdev(hw_cur_major, "hw_cursor");
	return ret;
}

static void hw_cur_exit(void)
{
	//u32 reg = __raw_readl(DP_COM_CONF(DP_SYNC));
	//reg &= ~DP_COC_DEF_MASK;
	//__raw_writel(reg, DP_COM_CONF(DP_SYNC));

	//reg = __raw_readl(IPU_SRM_PRI2) | 0x8;
	//__raw_writel(reg, IPU_SRM_PRI2);

	//reg = __raw_readl(IPU_SRM_PRI2);
	//printk(KERN_ERR "common = %x\n", reg);

	//iounmap(ipu_dp_reg);
	//iounmap(ipu_cm_reg);

	if ( !IS_ERR(hw_cur_dev) )
		device_destroy(hw_cur_class, MKDEV(hw_cur_major, 0));
	if ( !IS_ERR(hw_cur_class) )
		class_destroy(hw_cur_class);
	if ( hw_cur_major >= 0 )
		unregister_chrdev(hw_cur_major, "hw_cursor");
    printk(KERN_ALERT "hw cursor say: Goodbye, cruel world\n");
}

module_init(hw_cur_init);
module_exit(hw_cur_exit);
