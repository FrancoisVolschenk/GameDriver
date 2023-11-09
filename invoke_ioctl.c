#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define R_FLIP _IOW('a', 'b', int32_t *)

int main() {
	int value;
	int dev = open("/dev/devDriver", O_WRONLY);
	if(dev == -1) {
		printf("An error occured when trying to open the device file!\n");
		return -1;
	}

	ioctl(dev, R_FLIP, &value);

	printf("IOCTL invoked\n");
	close(dev);
	return 0;
}
