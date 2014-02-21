#include "trace_replay.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int fd;
int verbose;

int disk_open(const char *dev,int flag){
	
	fd = open(dev, flag);
	if(fd < 0)
		printf("device open error\n");

//	printf("open fd = %d\n",fd);

	return fd;
}

void disk_close(){
	
	flush_buffer_cache(fd);
	close(fd);
}

int io_read(char *buf, unsigned int req_size, unsigned long long offset, struct timeval *tv_result){
	struct timeval tv_start, tv_end;
    int i = 0;
  	FILE *fp = NULL;

	__u64 temp = (__u64)offset;

	gettimeofday(&tv_start, NULL);
  	if(lseek64(fd, temp, SEEK_SET)==-1){
		//printf("lseek is error %llu\n",temp);
		return -1;		
	}

	if(read(fd, buf, req_size) < 0){
		//printf("read error %llu\n", temp);
		return -1;
	}

	gettimeofday(&tv_end, NULL);
	timeval_subtract(tv_result, &tv_end, &tv_start);

    return 0;
}

int io_write(char *buf, unsigned int req_size, unsigned long offset, struct timeval *tv_result){
	struct timeval tv_start, tv_end;
    int i = 0;
  	FILE *fp = NULL;

	__u64 temp = (__u64)offset;

	gettimeofday(&tv_start, NULL);
  	if(lseek64(fd, temp, SEEK_SET)==-1){
		//printf("lseek is error %llu\n",temp);
		return -1;		
	}

	if(write(fd, buf, req_size) < 0){
		//printf("write error\n");
		return -1;
	}

	gettimeofday(&tv_end, NULL);
	timeval_subtract(tv_result, &tv_end, &tv_start);

    return 0;
}