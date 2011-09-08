#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TS ("/dev/pixcir_i2c_ts0")
#define CALIBRATION_FLAG 1

int main(void)
{
	int fd, ret;
	char buf[2];

	printf("open device\n");
	fd = open(TS, O_RDWR);

	if (fd < 0) {
		printf("open device failed\n");
		return -1;
	}
	
	buf[0] = 0x37;
	buf[1] = 0x03;

	printf("set calibration flag\n");
	ioctl(fd, CALIBRATION_FLAG, NULL);

	ret = write(fd, buf, sizeof(buf));

	sleep(5);

	if (ret == 2)
		printf("calibration successfully\n");
	else
		printf("calibration failed\n");

	close(fd);

	return 0;
}
