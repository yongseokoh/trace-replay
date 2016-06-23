# trace-replay #

trace-replay is a tool to generate realistic block-level I/O traces with multiple threads to mimic virtualization like workloads. It mainly utilizes traces forming DiskSim format. It also offers some options such as the number of threads, I/O depth, block size, and synthetic workloads (e.g., sequential and random workloads) to accurately evaluate storage devices.

## Developers ##

* Yongseok Oh (SK telecom Storage Tech. Lab, yongseok.oh@sk.com)
* Eunjae Lee (UNIST, kckjn97@gmail.com)


## Installation Guide ##

trace-reply requires aio and pthread libraries. 

**Build** 

```sh
$ git clone https://bitbucket.org/yongseokoh/trace-replay.git
$ make
$ ./trace_replay 
```


** Example of Replaying a Single Trace **

```sh
$ ./trace_replay [qdepth] [per_thread] [output] [runtime in seconds] [trace_repeat] [devicefile] [tracefile1] [timescale1] [0] [0]
$ ./trace_replay 32 8 result.txt 60 1 /dev/sdb1 trace.dat 0 0 0
```


** Example of Replaying Multiple Traces **

```sh
$ ./trace_replay [qdepth] [per_thread] [output] [runtime in seconds] [trace_repeat] [devicefile] [tracefile1] [timescale1] [0] [0]
$ ./trace_replay 32 8 result.txt 60 1 /dev/sdb1 trace1.dat 0 0 0 trace2.dat 0 0 0 trace3.dat 0 0 0 trace4.dat 0 0 0
```


** Example of Generating Synthetic Workloads ** 
```sh
$ ./trace_replay [qdepth] [per_thread] [output] [runtime in seconds] [trace_repeat] [synth_type] [wss] [utilization] [iosize]
 synth_type: rand_read, rand_write, rand_mixed, seq_read, seq_write, seq_mixed
 wss (in MB unit)
 utilization (in pecent unit)
 iosize (in KB unit)

$ ./trace_replay 32 8 result.txt 60 1 /dev/sdb1 rand_write 128 100 4
```
## Transformation to DiskSim traces##

** To Do **

## Refences ##

* Sungyong Ahn, "Improving I/O Resource Sharing of Linux Cgroup for NVMe SSDs on Multi-core Systems," USENIX HotStorage 2016
* Yongsseok Oh, "Enabling Cost-Effective Flash based Caching with an Array of of Commodity SSDs," ACM/USENIX MIDDLEWARE 2015
* Eunjae Lee, "SSD caching to overcome small write problem of disk-based RAID in enterprise environments" ACM SAC 2015
* Jaeho Kim, "Towards SLO Complying SSDs through OPS Isolation" USENIX FAST 2015