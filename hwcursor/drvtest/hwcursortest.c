#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fb.h>

#define IPU_SRM_PRI2(p)        ((p) + 0x00A4)

#define DP_COM_CONF(p, flow)   ((p) + flow)
#define DP_CUR_POS(p, flow)    ((p) + 0x0c + flow)
#define DP_CUR_MAP(p, flow)    ((p) + 0x10 + flow)

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC

#define DP_COC_DEF_MASK  0x00000070

static void *pdp;
static void *pcm;

static uint32_t readl(const void *p)
{
	return *(volatile uint32_t *)p;
}

static void writel(uint32_t val, void *p)
{
	*(volatile uint32_t *)p = val;
}

void show_cursor()
{
	uint32_t reg = readl(DP_COM_CONF(pdp, DP_SYNC));
	reg &= ~DP_COC_DEF_MASK;
	reg |= 0x10;
	writel(reg, DP_COM_CONF(pdp, DP_SYNC));

	writel(0x00ff0000, DP_CUR_MAP(pdp, DP_SYNC));

	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}

void hide_cursor()
{
	uint32_t reg = readl(DP_COM_CONF(pdp, DP_SYNC));
	reg &= ~DP_COC_DEF_MASK;
	writel(reg, DP_COM_CONF(pdp, DP_SYNC));

	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}

void get_cursor_pos(int *x, int *y)
{
	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	*x = (val >> 16) & 0x7ff;
	*y = val & 0x7ff;
}

void set_cursor_pos(int x, int y)
{
	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	val &= ~0x07ff07ff;
	val |= ((x&0x7ff) << 16) | (y&0x7ff);
	writel(val, DP_CUR_POS(pdp, DP_SYNC));

	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}

void set_cursor_size(int w, int h)
{
	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	val &= ~0xf800f800;
	val |= ((w&0x1f) << 27) | ((h&0x1f) << 11);
	writel(val, DP_CUR_POS(pdp, DP_SYNC));

	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}

void set_cursor_color(int r, int g, int b)
{
	uint32_t clr = (r&0xff) << 16 | (g&0xff) << 8 | b&0xff;
	writel(clr, DP_CUR_MAP(pdp, DP_SYNC));
	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}

int main()
{
	int fd = open("/dev/hw_cursor", O_RDWR);
	if ( fd < 0 ) {
		perror("open");
		return -1;
	}

	pdp = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 4096);
	if ( pdp == MAP_FAILED ) {
		perror("mmap");
		return -1;
	}
	printf("map success, pdp = %p\n", pdp);

	pcm = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ( pcm == MAP_FAILED ) {
		perror("mmap");
		return -1;
	}
	printf("map success, pcm = %p\n", pcm);

	show_cursor(pdp, pcm);

	set_cursor_pos(122, 98);
	set_cursor_size(5, 5);
	set_cursor_color(255, 0, 255);
	int i;
	for ( i = 0; i < 1024; i += 1 ) {
		set_cursor_pos(i, 400);
		usleep(1000);
	}

	hide_cursor(pdp, pcm);


	if ( munmap(pdp, 4096) < 0 || munmap(pcm, 4096) < 0 ) {
		perror("munmap");
		return -1;
	}
	
	close(fd);
	return 0;
}
