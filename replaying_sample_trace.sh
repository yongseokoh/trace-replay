#!/bin/bash

device=/dev/nvme0n1

echo "Replaying sample1.dat trace configured with 8 threads and 32 iodepth during 60 seconds"
./trace_replay 32 8 sample_trace_result.txt 60 1 $device sample_traces/sample1.dat  0 0 0

echo "Replaying sample2.dat trace configured with 8 threads and 32 iodepth during 60 seconds"
./trace_replay 32 8 sample_trace_result.txt 60 1 $device sample_traces/sample2.dat  0 0 0

echo "Replaying simultaneously configured with 8 threads and 32 iodepth during 60 seconds"
./trace_replay 32 8 sample_trace_result.txt 60 1 $device sample_traces/sample1.dat  0 0 0 sample_traces/sample2.dat  0 0 0
