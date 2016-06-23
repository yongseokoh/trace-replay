/****************************************************************************
 * Block I/O Trace Replayer 
 * Yongseok Oh (ysoh@uos.ac.kr) 2013 - 2014

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "trace_replay.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "sgio.h"

#ifndef ENOIOCTLCMD
#	define ENOIOCTLCMD ENOTTY
#endif


int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval * y);
int verbose;

int disk_open(const char *dev,int flag){
	int fd;
	
	fd = open(dev, flag);
	if(fd < 0)
		printf("device open error\n");

//	printf("open fd = %d\n",fd);

	return fd;
}

void flush_buffer_cache (int fd)
{
	fsync (fd);				/* flush buffers */
	if (ioctl(fd, BLKFLSBUF, NULL))		/* do it again, big time */
		perror("BLKFLSBUF failed");
	/* await completion */
	if (do_drive_cmd(fd, NULL) && errno != EINVAL && errno != ENOTTY && errno != ENOIOCTLCMD)
		perror("HDIO_DRIVE_CMD(null) (wait for flush complete) failed");
}

void disk_close(int fd){
	flush_buffer_cache(fd);
	close(fd);
}

int io_read(char *buf, unsigned int req_size, unsigned long long offset, struct timeval *tv_result){
	struct timeval tv_start, tv_end;
    int i = 0;
	int fd;

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
	int fd;

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
