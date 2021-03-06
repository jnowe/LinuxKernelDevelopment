/* **************** LFD420:4.6 s_18/lab2_devmemkmem.c **************** */
/*
 * The code herein is: Copyright the Linux Foundation, 2016
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://training.linuxfoundation.org
 *     email:  trainingquestions@linuxfoundation.org
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/*
 * Reading /dev/mem and /dev/kmem
 *
 * Write a program that reads the values of jiffies from /dev/mem
 * (physical memory address) and /dev/kmem (virtual memory
 * address). Because of permissions, you'll need to be superuser to
 * execute.
 *
 * In both cases you'll have to be careful to pass the correct address
 * of jiffies to your program (which you can obtain from
 * /lib/modules/$(uname -r)$/build/System.map for instance.)
 *
 * For each device node two methods are presented in the solution;
 * using normal a read() and using mmap(). One of these four doesn't
 * shouldn't work (read()> on /dev/kmem) because loff_t is signed and
 * the address will appear to be negative.
 *
 * Note: Red Hat Enteprise Linux kernels will differ from vanilla
 * kernels for this exercise, in that there is no /dev/kmem and
 * /dev/mem is restricted to the first megabyte of RAM and memory
 * address regions that do not correspond to actual RAM.
 *
@*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>

#define DEATH(mess) { perror(mess); exit(errno); }

#if defined  __i386__

#define PAGE_OFFSET           (0xC0000000UL)
#define __pa(x)               ((unsigned long)(x)-PAGE_OFFSET)
#define __va(x)               ((void *)((unsigned long)(x)+PAGE_OFFSET))

#elif defined __x86_64__

#define START_KERNEL_map      0xffffffff80000000UL
#define PAGE_OFFSET           0xffff880000000000UL
#define __pa(x)   (((unsigned long)(x)>=START_KERNEL_map)?  \
                   (unsigned long)(x) - (unsigned long)START_KERNEL_map: \
                   (unsigned long)(x) - PAGE_OFFSET)
#define __va(x)   ((void *)((unsigned long)(x)+PAGE_OFFSET))
#endif

int PAGE_SIZE, PAGE_SHIFT;
unsigned long PAGE_MASK;
unsigned long jiffies_address = 0UL;	/* make sure you give it as arg */

void mmapit(char *fname, size_t size, unsigned long add)
{
	unsigned long base, jiffies;
	off_t off;
	char *area;
	int fd;

	printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	printf("\n Now doing mmap of %s\n\n", fname);

	base = add & PAGE_MASK;
	off = add & ~PAGE_MASK;
	printf("base = 0x%lu, off = %d\n", base, (int)off);

	if ((fd = open(fname, O_RDONLY)) < 0)
		DEATH(fname);
	area = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, base);
	close(fd);
	printf("area = %p, area+off = %p\n", area, area + off);

	jiffies = 0;
	jiffies = *((unsigned long *)(area + off));
	printf("jiffies = %ld\n", jiffies);
	printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	munmap(area, 4096);
}

void do_read(char *fname, size_t size, unsigned long add)
{
	int fd, rc;
	off_t off;
	unsigned long jiffies;

	printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	printf("\n Now doing read of %s\n\n", fname);
	if ((fd = open(fname, O_RDONLY)) < 0)
		DEATH(fname);

	off = lseek(fd, add, SEEK_SET);
	printf("add = 0x%lu, off_lseek = %lu\n", add, off);
	jiffies = 0;
	rc = read(fd, &jiffies, sizeof(unsigned long));
	if (rc < 0)
		perror("Had a problem reading!!!!!!!!!\n");
	printf("rc read = %d,  jiffies = %lu\n", rc, jiffies);
	printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	close(fd);
}

int main(int argc, char *argv[])
{
	unsigned long va, pa;

	if (argc < 2) {
		printf("You must give jiffies address as an argument.\n");
		printf("get it from System.map file\n");
		exit(EXIT_FAILURE);
	} else
		jiffies_address = strtoul(argv[1], NULL, 16);
	printf("\njiffies_address = 0x%lx\n", jiffies_address);

	PAGE_SIZE = getpagesize();
	PAGE_SHIFT = ffs(PAGE_SIZE) - 1;
	PAGE_MASK = (unsigned long)(~(PAGE_SIZE - 1));

	fprintf(stdout, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	printf("PAGE_SIZE= %d, PAGE_SHIFT=%d, PAGE_MASK= %lx\n", PAGE_SIZE,
	       PAGE_SHIFT, PAGE_MASK);
	fprintf(stdout, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

	fprintf(stdout, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

	pa = __pa(jiffies_address);
	do_read("/dev/mem", sizeof(unsigned long), pa);
	mmapit("/dev/mem", PAGE_SIZE, pa);

	sleep(1);

	va = (unsigned long)__va(pa);
	do_read("/dev/kmem", sizeof(unsigned long), va);
	mmapit("/dev/kmem", PAGE_SIZE, va);

	exit(EXIT_SUCCESS);
}
