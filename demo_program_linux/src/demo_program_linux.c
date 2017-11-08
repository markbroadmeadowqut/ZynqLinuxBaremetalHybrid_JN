#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

#define COMM_BASE_ADDR 0xFFFF8000
#define COMM_FLAG_LINUX_OFFSET 0x00 //set 1 if data from Linux sent
#define COMM_DATA_LINUX_OFFSET 0x04 //Linux data location
#define COMM_FLAG_BAREMETAL_OFFSET 0x08 //set 1 if data from baremetal sent
#define COMM_DATA_BAREMETAL_OFFSET 0x0C //baremetal data location
#define CPU1_INITIALIZE_ADDR 0xFFFFFFF0
#define BAREMETAL_START_ADDR 0x18000000
#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))
#define MAX_CHAR 5

//function to start the baremetal application
int start_baremetal(int fd) {
	volatile uint8_t *bare_mm;
	int unmapbare_mm;
	bare_mm = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CPU1_INITIALIZE_ADDR);
	if (bare_mm == MAP_FAILED) {
		fprintf(stderr, "mmap failed : (%d)\n",
				errno);
		return 1;
	}

	*(volatile uint32_t *)(bare_mm) = BAREMETAL_START_ADDR;

    unmapbare_mm = munmap((void *)bare_mm, PAGE_SIZE);
    if (unmapbare_mm == -1) {
    	printf("munmap_bare failed.\n");
    	return 1;
    }

	return 0;
}

int main(int argc, char **argv) 
{
	FILE *fp;
	int fd,unmapmm,closefd;
	char* filename;
	volatile uint8_t *mm;
	uint32_t value;
	char number[MAX_CHAR];

	filename = argv[1];
	
	fp = fopen(filename,"r");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s : (%d)\n",filename, errno);
		return 1;
	}
	
	fscanf(fp,"%s",number);
	value = strtoull(number,NULL,10);

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "failed to open /dev/mem : (%d)\n", errno);
        return 1;
    }

    start_baremetal(fd);

    mm = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, COMM_BASE_ADDR);
    if (mm == MAP_FAILED) {
        fprintf(stderr, "mmap failed : (%d)\n",
                errno);
        return 1;
    }

    *(volatile uint32_t *)(mm + COMM_DATA_LINUX_OFFSET) = value;
    *(volatile uint32_t *)(mm + COMM_FLAG_LINUX_OFFSET) = 1;


    //loop until baremetal core is done
    while(1) {
    	if ( (*(volatile uint32_t *)(mm + COMM_FLAG_BAREMETAL_OFFSET)) ) {
    		value = *(volatile uint32_t *)(mm + COMM_DATA_BAREMETAL_OFFSET);
    		fprintf(fp,"%d",value);
    		break;
    	}
    }

    unmapmm = munmap((void *)mm, PAGE_SIZE);
    if (unmapmm == -1) {
    	printf("munmap failed.\n");
    	return 1;
    }

	closefd = close(fd);
	if (closefd == -1) {
		printf("mem close failed.\n");
		return 1;
	}

	fclose(fp);

	return 0;
}
