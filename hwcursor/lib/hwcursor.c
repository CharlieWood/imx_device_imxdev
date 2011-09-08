#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define LOG_TAG "hwcursor"
#include <cutils/log.h>

#include <hardware/hwcursor.h>

#define IPU_SRM_PRI2(p)        ((p) + 0x00A4/4)

#define DP_COM_CONF(p, flow)   ((p) + flow/4)
#define DP_CUR_POS(p, flow)    ((p) + 0x0c/4 + flow/4)
#define DP_CUR_MAP(p, flow)    ((p) + 0x10/4 + flow/4)

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC

#define DP_COC_DEF_MASK  0x00000070

#define HW_CURSOR_DEVICE "/dev/hw_cursor"

static int dev_fd = -1;
static uint32_t *pdp = MAP_FAILED;
static uint32_t *pcm = MAP_FAILED;
static int cursor_w_half;
static int cursor_h_half;
static int need_restore_cursor = 0;

static uint32_t readl(const uint32_t *p)
{
	return *(volatile uint32_t *)p;
}

static void writel(uint32_t val, uint32_t *p)
{
	*(volatile uint32_t *)p = val;
}

static inline void set_cursor_size_internal(int w, int h)
{
	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	val &= ~0xf800f800;
	val |= ((w&0x1f) << 27) | ((h&0x1f) << 11);
	writel(val, DP_CUR_POS(pdp, DP_SYNC));
}

static inline void srm_update()
{
	writel(readl(IPU_SRM_PRI2(pcm))|0x8, IPU_SRM_PRI2(pcm));
}
//================================================================
int have_hw_cursor()
{
	return access(HW_CURSOR_DEVICE, F_OK) == 0 ? 1 : 0;
}

int open_cursor()
{
	if ( dev_fd >= 0 ) {
		LOGE("open_cursor: dev_fd = %d, already opened", dev_fd);
		return -1;
	}

	dev_fd = open(HW_CURSOR_DEVICE, O_RDWR);
	if ( dev_fd < 0 ) {
		LOGE("open_cursor: open failed, errno = %d", errno);
		return -1;
	}

	pdp = (uint32_t *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 4096);
	if ( pdp == MAP_FAILED ) {
		LOGE("open_cursor: mmap pdp failed, errno = %d", errno);
		goto err1;
	}

	pcm = (uint32_t *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
	if ( pcm == MAP_FAILED ) {
		LOGE("open_cursor: mmap pcm failed, errno = %d", errno);
		goto err2;
	}

	need_restore_cursor = 0;

	LOGI("open_cursor: open cursor success, dev_fd = %d, pdp = %p, pcm = %p", dev_fd, pdp, pcm);
	return 0;

err2:
	munmap(pdp, 4096);

err1:
	close(dev_fd);
	dev_fd = -1;
	return -1;
}

void close_cursor()
{
	if ( dev_fd < 0 ) {
		LOGE("close_cursor: cursor not open");
		return;
	}

	munmap(pdp, 4096);
	munmap(pcm, 4096);
	close(dev_fd);
	dev_fd = -1;
}

void show_cursor()
{
	uint32_t val = readl(DP_COM_CONF(pdp, DP_SYNC));
	val &= ~DP_COC_DEF_MASK;
	val |= 0x10;
	writel(val, DP_COM_CONF(pdp, DP_SYNC));

	srm_update();
}

void hide_cursor()
{
	uint32_t reg = readl(DP_COM_CONF(pdp, DP_SYNC));
	reg &= ~DP_COC_DEF_MASK;
	writel(reg, DP_COM_CONF(pdp, DP_SYNC));

	srm_update();
}

void set_cursor_pos(int x, int y)
{
	x -= cursor_w_half;
	y -= cursor_h_half;

	if ( x < 0 || y < 0 ) {
		int w = cursor_w_half * 2 + 1;
		int h = cursor_h_half * 2 + 1;
		if ( x < 0 ) {
			w += x;
			x = 0;
		}

		if ( y < 0 ) {
			h += y;
			y = 0;
		}

		set_cursor_size_internal(w, h);
		need_restore_cursor = 1;
	} else if ( need_restore_cursor ) {
		set_cursor_size_internal(cursor_w_half * 2 + 1, cursor_h_half * 2 + 1);
		need_restore_cursor = 0;
	}

	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	val &= ~0x07ff07ff;
	val |= ((x&0x7ff) << 16) | (y&0x7ff);
	writel(val, DP_CUR_POS(pdp, DP_SYNC));

	srm_update();
}

void set_cursor_color(int r, int g, int b)
{
	uint32_t clr = (r&0xff) << 16 | (g&0xff) << 8 | (b&0xff);
	writel(clr, DP_CUR_MAP(pdp, DP_SYNC));

	srm_update();
}

unsigned int get_cursor_color()
{
	return readl(DP_CUR_MAP(pdp, DP_SYNC));
}

void set_cursor_size(int w, int h)
{
	set_cursor_size_internal(w|1, h|1);
	cursor_w_half = w / 2;
	cursor_h_half = h / 2;

	srm_update();
}

void get_cursor_size(int *w, int *h)
{
	uint32_t val = readl(DP_CUR_POS(pdp, DP_SYNC));
	*w = (val >> 27) & 0x1f;
	*h = (val >> 11) & 0x1f;
}

