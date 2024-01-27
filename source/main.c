#include <ps4.h>

struct fsid {
	int val[2];
};

struct statfs {
	unsigned f_version;
	unsigned f_type;
	long field2_0x8;
	long field3_0x10;
	long field4_0x18;
	long field5_0x20;
	long field6_0x28;
	long field7_0x30;
	long field8_0x38;
	long field9_0x40;
	long field10_0x48;
	long field11_0x50;
	long field12_0x58;
	long field13_0x60;
	long field14_0x68;
	long field15_0x70;
	long field16_0x78;
	long field17_0x80;
	long field18_0x88;
	long field19_0x90;
	long field20_0x98;
	long field21_0xa0;
	long field22_0xa8;
	long field23_0xb0;
	char field24_0xb8;
	char field25_0xb9;
	char field26_0xba;
	char field27_0xbb;
	int f_owner;
	struct fsid f_fsid;
	long field30_0xc8;
	long field31_0xd0;
	long field32_0xd8;
	long field33_0xe0;
	long field34_0xe8;
	long field35_0xf0;
	long field36_0xf8;
	long field37_0x100;
	long field38_0x108;
	long field39_0x110;
	char f_fstypename[16];
	char f_mntfromname[88];
	char f_mntonname[88];
};

struct get_mounts_req {
	struct statfs *buf;
	size_t len;
};

struct get_mounts_args {
	void *h;
	struct get_mounts_req *r;
};

typedef int (*getfsstat_t) (struct thread *, struct statfs **, size_t , int, int);

static int get_mounts(struct thread *td, struct get_mounts_args *args) {
	void *base;
	getfsstat_t getfsstat;

	base = &((uint8_t *)__readmsr(0xC0000082))[-K900_XFAST_SYSCALL];
	getfsstat = (getfsstat_t)(base + 0x1D90A0);

	return getfsstat(td, &args->r->buf, args->r->len, 0, 1);
}

int _main(struct thread *td) {
	struct get_mounts_req req;
	int res, i, fd;
	FILE *fp;

	initKernel();
	initLibc();
	jailbreak();
	initSysUtil();

	// allocate buffer
	req.len = sizeof(struct statfs) * 8192;
	req.buf = malloc(req.len);

	if (!req.buf) {
		printf_notification("Failed to allocate %u bytes of memory.", req.len);
		return 0;
	}

	// get mount list
	res = kexec(get_mounts, &req);

	if (res < 0) {
		free(req.buf);
		printf_notification("Failed to get mount list.");
		return 0;
	}

	// create an output file
	fd = open("/mnt/usb0/mount-list.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);

	if (fd < 0) {
		printf_notification("Failed to create /mnt/usb0/mount-list.txt.");
		free(req.buf);
		return 0;
	}

	// write to usb drive
	for (i = 0; i < res; i++) {
		struct statfs *s = req.buf + i;

		if (write(fd, s->f_fstypename, strlen(s->f_fstypename)) < 0 || write(fd, "\t", 1) < 0) {
			printf_notification("Failed to write /mnt/usb0/mount-list.txt.");
			close(fd);
			free(req.buf);
			return 0;
		}

		if (write(fd, s->f_mntfromname, strlen(s->f_mntfromname)) < 0 || write(fd, "\t", 1) < 0) {
			printf_notification("Failed to write /mnt/usb0/mount-list.txt.");
			close(fd);
			free(req.buf);
			return 0;
		}

		if (write(fd, s->f_mntonname, strlen(s->f_mntonname)) < 0 || write(fd, "\r\n", 2) < 0) {
			printf_notification("Failed to write /mnt/usb0/mount-list.txt.");
			close(fd);
			free(req.buf);
			return 0;
		}
	}

	printf_notification("Mount list dump complete!");

	return 0;
}
