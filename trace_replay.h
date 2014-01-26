#define DISK_SUCCESS	0
#define DISK_FAIL		1
#define GIGA 25 
#define NUM_SECTORS	(GIGA*2*1024*1024)
#define DISK_BLOCK_SIZE 4096
#define SKIP_WRITE 1
#define LINUX 1
#define WINDOWS 2
#define TEST_OS LINUX
// 0 = file , 1 = disk(for winxp), 2 = memory , 3 = /dev/{node}(for linux)
#define DISK_SELECTION 3 


#if TEST_OS == WINDOWS
#define lseek64 lseek
#define gettimeofday 
#else
//#define O_DIRECT	040000	/* direct disk access, not easily obtained from headers */
#endif 



typedef char                    __s8;   
typedef short                   __s16;  
typedef int                     __s32;  

#if TEST_OS == LINUX
typedef long long					__s64;
#endif 

typedef unsigned char           __u8;  
typedef unsigned short          __u16; 
typedef unsigned int            __u32; 

#if TEST_OS == LINUX
typedef unsigned long long			__u64;
#endif
