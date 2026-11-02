// Print the events of an input device: evdump /dev/input/event0
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	struct input_event ev;
	int fd = open(argc > 1 ? argv[1] : "/dev/input/event0", O_RDONLY);

	if (fd < 0) {
		perror("open");
		return 1;
	}
	while (read(fd, &ev, sizeof(ev)) == sizeof(ev))
		if (ev.type)
			printf("type %u code %u value %d\n", ev.type, ev.code,
			       ev.value);
	return 0;
}
