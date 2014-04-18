#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <libaio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#if TEST_OS == LINUX
#	include <errno.h>
#	include <sys/mount.h>
#endif 

#include "trace_replay.h"
#include "flist.h"

#ifndef ENOIOCTLCMD
#	define ENOIOCTLCMD ENOTTY
#endif



#define BUFFER_SIZE 4096
#define PAGE_SIZE 4096
#define SECTOR_SIZE 512
#define SPP (PAGE_SIZE/SECTOR_SIZE)

#define MB (1024*1024)
#define STR_SIZE 128

#define OPEN_FLAGS (O_RDWR | O_NONBLOCK)
#define OPEN_FLAGS_STR "O_NONBLOCK"

static int open_flags = OPEN_FLAGS;
extern int fd;

#define RND(x) ((x>0)?(genrand() % (x)):0)


#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* Initializing the array with a seed */
void
sgenrand(seed)
unsigned long seed;	
{
	int i;

	for (i=0;i<N;i++) {
		mt[i] = seed & 0xffff0000;
		seed = 69069 * seed + 1;
		mt[i] |= (seed & 0xffff0000) >> 16;
		seed = 69069 * seed + 1;
	}
	mti = N;
}

/* Initialization by "sgenrand()" is an example. Theoretically,      */
/* there are 2^19937-1 possible states as an intial state.           */
/* This function allows to choose any of 2^19937-1 ones.             */
/* Essential bits in "seed_array[]" is following 19937 bits:         */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]. */
/* (seed_array[0]&LOWER_MASK) is discarded.                          */ 
/* Theoretically,                                                    */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]  */
/* can take any values except all zeros.                             */
void
lsgenrand(seed_array)
unsigned long seed_array[]; 
/* the length of seed_array[] must be at least N */
{
	int i;

	for (i=0;i<N;i++) 
		mt[i] = seed_array[i];
	mti=N;
}

unsigned long 
genrand()
{
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if sgenrand() has not been called, */
			sgenrand(4357); /* a default initial seed is used   */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}

	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y; 
}
int timeval_subtract (result, x, y)
 struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by
  * updating y. */
  if (x->tv_usec < y->tv_usec) {
   int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
   y->tv_usec -= 1000000 * nsec;
   y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
  	    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
	    y->tv_usec += 1000000 * nsec;
	    y->tv_sec -= nsec;
 }
							  	
  /* Compute the time remaining to wait.
  * 	     tv_usec  is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
						  	
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void sum_time(struct timeval *t1,struct timeval *t2)
{
	if((t1->tv_usec + t2->tv_usec ) < 1000000){
		t1->tv_usec += t2->tv_usec;
		t1->tv_sec += t2->tv_sec;
	}else{
		int nsec = (t1->tv_usec + t2->tv_usec) / 1000000;
		t1->tv_usec += t2->tv_usec;
		t1->tv_usec -= 1000000*nsec;
		t1->tv_sec += t2->tv_sec;
		t1->tv_sec += nsec;
	}
}

static double time_since(struct timeval *start_tv, struct timeval *stop_tv)
{
    double sec, usec;
    double ret;
    sec = stop_tv->tv_sec - start_tv->tv_sec;
    usec = stop_tv->tv_usec - start_tv->tv_usec;
    if (sec > 0 && usec < 0) {
        sec--;
	usec += 1000000;
    } 
    ret = sec + usec / (double)1000000;
    if (ret < 0)
        ret = 0;
    return ret;
}

/*
 * return seconds between start_tv and now in double precision
 */
static double time_since_now(struct timeval *start_tv)
{
    struct timeval stop_time;
    gettimeofday(&stop_time, NULL);
    return time_since(start_tv, &stop_time);
}

/* allocate a alignment-bytes aligned buffer */
void *allocate_aligned_buffer(size_t size)
{
	void *p;

#if 0  
	posix_memalign(&p, getpagesize(), size);
#else
	p=(void *)memalign(getpagesize(), size);
#endif

	if(!p) {
		perror("memalign");
		exit (0);
		return NULL;
	}

	return p;
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

float tv_to_sec(struct timeval *tv){
	return (float)tv->tv_sec + (float)tv->tv_usec/1000000;
}

/* Fatal error handler */
static void io_error(const char *func, int rc)
{
	if (rc == -ENOSYS)
		fprintf(stderr, "AIO not in this kernel");
	else if (rc < 0)
		fprintf(stderr, "%s: %s", func, strerror(rc));
	else
		fprintf(stderr, "%s: error %d", func, rc);

//	if (dstfd > 0)
//		close(dstfd);
//	if (dstname)
//		unlink(dstname);
//	exit(1);
}


void usage_help(){
	printf(" Invalid command!!\n");
	printf(" Usage:\n");
	printf(" #./trace_replay tracefile devicefile qdepth\n\n");
}


#define MAX_QDEPTH 1
#define NUM_THREADS 128

int nr_thread;
pthread_spinlock_t spinlock;

struct thread_info_t{
	struct flist_head queue;
	pthread_mutex_t mutex;
	pthread_cond_t cond_main, cond_sub;
	io_context_t io_ctx;
	struct io_event events[MAX_QDEPTH];

	double latency_sum;
	int latency_count;

	int queue_count;
	int active_count;

	int fd;
	long long total_capacity;
	unsigned long total_pages;
	unsigned long total_sectors;

	int done;
}th_info[NUM_THREADS];

FILE *trace_fp;
unsigned long long total_operations = 0;
unsigned long long total_bytes = 0;
unsigned long long total_error_bytes = 0;

struct io_job{
	struct iocb iocb;
	struct flist_head list;
    struct timeval start_time, stop_time;
	long long offset;
	size_t bytes;
	int rw;
	char *buf;
};


#if 0
void *sub_worker(void *threadid)
{
	long tid =  (long)threadid;
	struct io_job *job, *test_job;
	struct io_job *jobq[MAX_QDEPTH];
	struct iocb *ioq[MAX_QDEPTH];
	int rc;

	//printf (" pthread start id = %d \n", (int)tid);

	while(1){
		struct flist_head *ptr, *tmp;
		int cnt = 0;
		int complete_count = 0;
		int i, k;

		//printf(" tid %d qcount %d \n", (int)tid, th_info[tid].queue_count);
		for(k = 0;k < 1;k++){
			unsigned long page;
			job = (struct io_job *)malloc(sizeof(struct io_job));

			page = RND(th_info[i].total_pages);
			job->offset = (long long)page * PAGE_SIZE;
			job->bytes = PAGE_SIZE;
			job->rw = 0;
			job->buf = allocate_aligned_buffer(job->bytes);

			gettimeofday(&job->start_time, NULL);
			flist_add_tail(&job->list, &th_info[tid].queue);
			th_info[tid].queue_count++;

			pthread_spin_lock(&spinlock);
			total_bytes += job->bytes;
			pthread_spin_unlock(&spinlock);
		}

		flist_for_each_safe(ptr, tmp, &th_info[tid].queue){
			job = flist_entry(ptr, struct io_job, list);
			flist_del(&job->list);
			th_info[tid].queue_count--;
			jobq[cnt++] = job;
		}
		//pthread_mutex_unlock(&th_info[tid].mutex);
		//pthread_cond_signal(&th_info[tid].cond_main);

		if(!cnt)
			continue;

		
		for(i = 0;i < cnt;i++){
			job = jobq[i]; 
			ioq[i] = &job->iocb;
			if(job->rw)
				io_prep_pread(&job->iocb, th_info[tid].fd, job->buf, job->bytes, job->offset);
			else
				io_prep_pwrite(&job->iocb, th_info[tid].fd, job->buf, job->bytes, job->offset);
		}

		rc = io_submit(th_info[tid].io_ctx, cnt, ioq);
		if (rc < 0)
			io_error("io_submit", rc);

		while(cnt){
			complete_count = io_getevents(th_info[tid].io_ctx, cnt, cnt, th_info[tid].events, NULL);
			for(i = 0;i < complete_count;i++){
				test_job = (struct io_job *)((unsigned long)th_info[tid].events[i].obj);
				gettimeofday(&test_job->stop_time, NULL);
				//printf(" tid = %d,no = %d blkno = %d, bytes = %d, lat = %f\n",
				//		(int) tid, i, (int)test_job->offset, (int)test_job->bytes,
				//		time_since(&test_job->start_time, &test_job->stop_time));

			//	pthread_mutex_lock(&th_info[tid].mutex);
				th_info[tid].latency_sum += time_since(&test_job->start_time, &test_job->stop_time);
				th_info[tid].latency_count ++;
			//	pthread_mutex_unlock(&th_info[tid].mutex);
				cnt--;

				free(test_job->buf);
				free(test_job);
			}
		}
	}

	printf (" pthread end id = %d \n", (int)tid);


	return NULL;
}

#else
void *sub_worker(void *threadid)
{
	long tid =  (long)threadid;
	struct io_job *job, *test_job;
	struct io_job *jobq[MAX_QDEPTH];
	struct iocb *ioq[MAX_QDEPTH];
	int rc;

	//printf (" pthread start id = %d \n", (int)tid);

	while(1){
		struct flist_head *ptr, *tmp;
		int cnt = 0;
		int complete_count = 0;
		int i;

		pthread_mutex_lock(&th_info[tid].mutex);

		while(!th_info[tid].queue_count)
			pthread_cond_wait(&th_info[tid].cond_sub, &th_info[tid].mutex);

		//printf(" tid %d qcount %d \n", (int)tid, th_info[tid].queue_count);

		flist_for_each_safe(ptr, tmp, &th_info[tid].queue){
			job = flist_entry(ptr, struct io_job, list);
			gettimeofday(&job->start_time, NULL);
			flist_del(&job->list);
			th_info[tid].queue_count--;
			jobq[cnt++] = job;
		}
		pthread_mutex_unlock(&th_info[tid].mutex);
		pthread_cond_signal(&th_info[tid].cond_main);

		if(!cnt)
			continue;

		
#if 1
		for(i = 0;i < cnt;i++){
			job = jobq[i]; 
			ioq[i] = &job->iocb;
			if(job->rw)
				io_prep_pread(&job->iocb, th_info[tid].fd, job->buf, job->bytes, job->offset);
			else
				io_prep_pwrite(&job->iocb, th_info[tid].fd, job->buf, job->bytes, job->offset);
		}

		rc = io_submit(th_info[tid].io_ctx, cnt, ioq);
		if (rc < 0)
			io_error("io_submit", rc);

		while(cnt){

			//printf(" io getevents \n");
			complete_count = io_getevents(th_info[tid].io_ctx, cnt, cnt, th_info[tid].events, NULL);
			//printf(" complete count = %d cnt = %d \n", complete_count, cnt);

			for(i = 0;i < complete_count;i++){
				test_job = (struct io_job *)((unsigned long)th_info[tid].events[i].obj);
				gettimeofday(&test_job->stop_time, NULL);
				//printf(" tid = %d,no = %d blkno = %d, bytes = %d, lat = %f\n",
				//		(int) tid, i, (int)test_job->offset, (int)test_job->bytes,
				//		time_since(&test_job->start_time, &test_job->stop_time));

				pthread_mutex_lock(&th_info[tid].mutex);
			//	th_info[tid].active_count--;
				th_info[tid].latency_sum += time_since(&test_job->start_time, &test_job->stop_time);
				th_info[tid].latency_count ++;
				//if(th_info[i].queue_count)
				//	printf(" wait ... qcount = %d\n", th_info[i].queue_count);
				pthread_mutex_unlock(&th_info[tid].mutex);
				cnt--;

				free(test_job->buf);
				free(test_job);
			}
		}
#else
		for(i = 0;i < cnt;i++){
			job = jobq[i]; 
			pthread_mutex_lock(&th_info[tid].mutex);
			//	th_info[tid].active_count--;
			th_info[tid].latency_sum += time_since(&job->start_time, &job->stop_time);
			th_info[tid].latency_count ++;
			//if(th_info[i].queue_count)
			//	printf(" wait ... qcount = %d\n", th_info[i].queue_count);
			pthread_mutex_unlock(&th_info[tid].mutex);
			cnt--;

			free(job->buf);
			free(job);
		}
#endif 
	}

	printf (" pthread end id = %d \n", (int)tid);


	return NULL;
}

#endif 

void main_worker(){
	struct io_job *job;
	int i, j, k;

	for(j = 0;j < 4*256*64;j++){
		for (i=0; i<nr_thread; i++) {
			unsigned long page;

			pthread_mutex_lock(&th_info[i].mutex);

			while(th_info[i].queue_count>=MAX_QDEPTH){
			//	printf(" wait ... qcount = %d\n", th_info[i].queue_count);
				pthread_cond_wait(&th_info[i].cond_main, &th_info[i].mutex);
			}

			for(k = 0;k < 1;k++){
				job = (struct io_job *)malloc(sizeof(struct io_job));

				page = RND(th_info[i].total_pages);
				job->offset = (long long)page * PAGE_SIZE;
				job->bytes = PAGE_SIZE;
				job->rw = 0;
				job->buf = allocate_aligned_buffer(job->bytes);

				//gettimeofday(&job->start_time, NULL);
				flist_add_tail(&job->list, &th_info[i].queue);
				th_info[i].queue_count++;

				pthread_spin_lock(&spinlock);
				total_bytes += job->bytes;
				pthread_spin_unlock(&spinlock);
			}


		//	if(th_info[i].queue_count>=MAX_QDEPTH)
			//printf(" qcount = %d\n", th_info[i].queue_count);


			pthread_mutex_unlock(&th_info[i].mutex);
			pthread_cond_signal(&th_info[i].cond_sub);

		}
	}
	printf(" finish main worker .. \n");
}

int main(int argc, char **argv){

	char filename[STR_SIZE];

	char str[4096];
	int opt;
	int opt_r = 0, opt_s = 0;		
	int opt_R = 0, opt_W = 0;

	int part_size = 1024;
	int max_req_size = 1;

	int is_rw = 0;
	int is_rand = 0; 

	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;

	int rc;
	long t;
	int fd;

	struct timeval tv_start, tv_end, tv_result;

	if(argc != 4){
		usage_help();
		return 0; 
	}

	pthread_spin_init(&spinlock, 0);

	trace_fp = fopen(argv[1], "r");
	if(trace_fp == NULL){
		printf("file open error %s\n", argv[1]);
		return -1;
	}

	if(fgets(str, 4096, trace_fp)==NULL){
		printf(" Null trace .. \n");
		return -1;
	}

	gettimeofday(&tv_start, NULL);
//	printf(" %s %s \n", argv[1], argv[2]);

	strcpy(filename, argv[2]);
	open_flags = O_RDWR|O_DIRECT;
	fd =disk_open(filename, open_flags); 
	if(fd < 0)
		return -1;

	nr_thread = atoi(argv[3]);
	if(nr_thread<1 || nr_thread>NUM_THREADS){
		printf(" invalid qdepth num = %d \n", nr_thread);
		return -1;
	}

	for(t=0;t<nr_thread;t++){

		INIT_FLIST_HEAD(&th_info[t].queue);
		pthread_mutex_init(&th_info[t].mutex, NULL);
		pthread_cond_init(&th_info[t].cond_sub, NULL);
		pthread_cond_init(&th_info[t].cond_main, NULL);

		memset(&th_info[t].io_ctx, 0, sizeof(io_context_t));
		io_queue_init(32, &th_info[t].io_ctx);

		th_info[t].fd = fd;
		th_info[t].queue_count = 0;
		th_info[t].active_count = 0;
		th_info[t].latency_sum = 0.0;
		th_info[t].latency_count = 0;

		ioctl(fd, BLKGETSIZE64, &th_info[t].total_capacity);
		th_info[t].total_pages = th_info[t].total_capacity/PAGE_SIZE; 
		th_info[t].total_sectors = th_info[t].total_pages * SPP;
		th_info[t].total_capacity = th_info[t].total_pages * PAGE_SIZE;

		//printf(" %d %f GB, %d sectors\n", fd, (double)th_info[t].total_capacity/1024/1024/1024,
		//		(int)th_info[t].total_sectors);
	}

	for(t=0;t<nr_thread;t++){
	//	printf("In main: creating thread %ld\n", t);
		rc = pthread_create(&threads[t], NULL, sub_worker, (void *)t);
		if (rc){
			printf("ERROR; return code from pthread_create( is %d\n", rc);
			exit(-1);
		}
	}

	main_worker();

	for(t=0;t<nr_thread;t++){
	//	printf("In main: creating thread %ld\n", t);
		pthread_cancel(threads[t]);
		pthread_cond_signal(&th_info[t].cond_sub);
		rc = pthread_join(threads[t], NULL);
		if (rc){
			//printf("ERROR; return code from pthread_create( is %d\n", rc);
			//exit(-1);
		}
		pthread_mutex_destroy(&th_info[t].mutex);
		pthread_cond_destroy(&th_info[t].cond_sub);
		pthread_cond_destroy(&th_info[t].cond_main);
		io_queue_release(th_info[t].io_ctx);
		printf(" %d thread latency = %f \n", (int)t, (double)th_info[t].latency_sum/th_info[t].latency_count);
	}

	/* Last thing that main( should do */
///	pthread_exit(NULL);

	fclose(trace_fp);
	disk_close();

	gettimeofday(&tv_end, NULL);
	timeval_subtract(&tv_result, &tv_end, &tv_start);

	printf("Total: %.6f seconds\n", tv_to_sec(&tv_result));
	printf("Total: %llu operations \n", total_operations);
	printf("Total: %f iops\n", (double)total_operations/tv_to_sec(&tv_result));
	printf("Total: %f MB/s\n", (double)total_bytes/(1024*1024)/tv_to_sec(&tv_result));
	printf("Total: %f MB\n", (double)total_bytes/(1024*1024));
	printf("Total: error %f MB\n", (double)total_error_bytes/(1024*1024));
	fflush(stdout);

	return 0;
}

int issue_req(int is_rw, int is_rand, int req_size, int part_size, 
						struct timeval *tv_result, char *filename){
	struct timeval tv_start, tv_end;
	FILE *micro_fp = NULL;
	char *rand_map = NULL;
	char *alignedbuff = NULL;
	int blk_count = 0;
	int i = 0;

	micro_fp = fopen(filename, "w");

	if(micro_fp == NULL){
		printf("cannot open !!!\n");
		return -1;
	}

	blk_count = (int)((__s64)part_size * MB / req_size);

	alignedbuff = allocate_aligned_buffer(req_size);
	if(alignedbuff == NULL){
		printf("alloc aligned buffer error\n");
		return -1;
	}
	memset(alignedbuff, 0xff, req_size);

	rand_map = (char *)malloc(blk_count);
	if(rand_map == NULL){
		printf(" malloc error \n");
		return -1;
	}
	memset(rand_map, 0x0, blk_count);

	gettimeofday(&tv_start, NULL);

	for(i = 0;i < blk_count;i++){
		struct timeval micro_result;
		__u32 offset = RND(blk_count);

		if(!is_rand)
			offset = i;

		if(rand_map[offset] == 1){
			i--;
			continue;
		}else{
			rand_map[offset] = 1;
		}


		if(is_rw){
			if(io_read(alignedbuff, req_size, offset, &micro_result) < 0 )
				return -1;
		}else{
			if(io_write(alignedbuff, req_size, offset, &micro_result) < 0 )
				return -1;
		}
		
		fprintf(micro_fp, "%d\t%.6f\n", i, tv_to_sec(&micro_result));
	}

	gettimeofday(&tv_end, NULL);
	timeval_subtract(tv_result, &tv_end, &tv_start);

	free(alignedbuff);
	free(rand_map);

	fclose(micro_fp);

	return 0;
}

int start_io_test(int is_rw, int is_rand, int part_size, int max_req_size){
	FILE *result_fp;
	unsigned int req_size;	
	char result_str[STR_SIZE];
	char rw_str[STR_SIZE];
	char rand_str[STR_SIZE];
	int i = 1;

	max_req_size *= MB;

	if(is_rw){
		sprintf(rw_str,"read");
	}else{
		sprintf(rw_str,"write");
	}

	if(is_rand){
		sprintf(rand_str,"rand");
	}else{
		sprintf(rand_str,"seq");
	}

	sprintf(result_str, "result/bandwidth_%s_%s_result.txt", rw_str, rand_str);
	result_fp = fopen(result_str,"w");
	if(result_fp == NULL){
		printf("file open error %s\n",result_str);
		return -1;
	}

	fprintf(result_fp, "#Index\tReq Size\tBandwidth(MB/s)\n");

	for(req_size = PAGE_SIZE; req_size <= max_req_size; req_size *= 2, i++){
		struct timeval tv_result;
		char filename[STR_SIZE];
		int res;

		sprintf(filename,"result/response_%s_%s_%d.txt", rw_str, rand_str, req_size);

		printf("  >>%s %s : req size %d\n",rw_str, rand_str, req_size);	
		fflush ( stdout );

		if((res = issue_req(is_rw, is_rand, req_size, part_size, &tv_result, filename)) < 0){
			return res;
		}

		if(tv_result.tv_sec || tv_result.tv_usec){
			fprintf(stdout, " %d\t%d\t%f\n\n", i, 
					req_size, (float)part_size/(float)tv_to_sec(&tv_result));
			fprintf(result_fp, "%d\t%d\t%f\n", i, 
					req_size, (float)part_size/(float)tv_to_sec(&tv_result));
			fflush(result_fp);
		}

		sync();
		flush_buffer_cache(fd);
	}

	fclose(result_fp);
}
#if 0
	while (!feof(trace_fp)) {
		int r;
		struct timeval micro_result;

		pthread_spin_lock(&spinlock);
		if(done){
			pthread_spin_unlock(&spinlock);
			break;
		}
		p = fgets(str, 4096, trace_fp);
		if(p==NULL){
		//	printf(" trace null \n");
			done = 1;
			pthread_spin_unlock(&spinlock);
			break;
		}
		pthread_spin_unlock(&spinlock);


		r = sscanf(p, "%256s %256s %llu %u", fname, act, &offset, &bytes);

		if(r!=4)
			continue;

		printf(" %s %s %llu %u\n", fname, act, offset, bytes);

	/*	if(offset % getpagesize()){
			printf(" rem = %llu \n", offset % getpagesize());
			printf(" rem = %llu \n", (unsigned long long)bytes % getpagesize());
		} */

		if(offset % getpagesize()){
			offset = offset - (offset % getpagesize());
		}
		if(bytes%getpagesize()){
			bytes = bytes + getpagesize() - (bytes % getpagesize());
		}

		if(offset > (unsigned long long)400*1024*1024*1024){
			offset = (offset+bytes)%((unsigned long long)400*1024*1024*1024);
		}
#if 0 
		if(offset % getpagesize() == 0){
			//printf (" corrected .. \n");
		}
#endif 

		if (!strcmp(act, "read")){
			if(io_read(alignedbuff, bytes, offset, &micro_result) < 0 ){
				error_bytes += bytes;
				continue;
			}
		}else if (!strcmp(act, "write")){
			if(io_write(alignedbuff, bytes, offset, &micro_result) < 0 ){
				error_bytes += bytes;
				continue;
			}
		}

		if(error_bytes>1*1024*1024*1024){
			printf(" Error exceeds %llu\n", (unsigned long long)error_bytes/1024/1024);
		}
		operations++;
		sum_bytes+=bytes;
	}
#endif 
