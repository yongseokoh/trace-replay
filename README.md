# trace-replay #

trace-replay is a tool to generate realistic block-level I/O traces with multiple threads to mimic virtualization like workloads. It mainly utilizes traces forming DiskSim format. It also offers some options such as the number of threads, I/O depth, block size, and synthetic workloads (e.g., sequential and random workloads) to accurately evaluate storage devices.

### Developers ###

* Yongseok Oh (SK telecom Storage Tech. Lab, yongseok.oh@sk.com)
* Eunjae Lee (UNIST, kckjn97@gmail.com)


### Installation Guide ###

trace-reply requires aio and pthread libraries. 

```sh
$ git clone git@bitbucket.org:yongseokoh/trace-replay.git
```

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions