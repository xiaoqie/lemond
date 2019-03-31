//
// Created by xiaoq on 3/26/19.
//

#ifndef LEMOND_MAIN_H
#define LEMOND_MAIN_H

#include <string>
#include <unistd.h>

using namespace::std;

typedef unsigned long long ulonglong;

struct cpu_info {
    string name;
    ulong idle;
    float usage;
};

struct diskstat {
    string name;
    uint reads_completed, reads_merged, sectors_read, time_read;
    uint writes_completed, writes_merged, sectors_written, time_write;
    uint io_in_progress, time_io, weighted_time_io;
    // new since kernel 4.18+
    uint discards_completed, discards_merged, sectors_discarded, time_discard;
    // ---------------------------------
    float usage;
};

struct net_dev {
    string interface;
    ulong receive_bytes, receive_packets, receive_errs, receive_drop, receive_fifo, receive_frame, receive_compressed, receive_multicast;
    ulong transmit_bytes, transmit_packets, transmit_errs, transmit_drop, transmit_fifo, transmit_colls, transmit_carrier, transmit_compressed;
};

struct sys_info {
    float uptime;
    int clock_ticks;
    int page_size;
    ulong MemTotal, MemFree, MemAvailable, Buffers, Cached, SwapCached, Active, Inactive, Active_anon, Inactive_anon,
            Active_file, Inactive_file, Unevictable, Mlocked, SwapTotal, SwapFree, Dirty, Writeback, AnonPages,
            Mapped, Shmem, Slab, SReclaimable, SUnreclaim, KernelStack, PageTables, NFS_Unstable, Bounce, WritebackTmp,
            CommitLimit, Committed_AS, VmallocTotal, VmallocUsed, VmallocChunk, HardwareCorrupted, AnonHugePages,
            ShmemHugePages, ShmemPmdMapped, CmaTotal, CmaFree, HugePages_Total, HugePages_Free, HugePages_Rsvd,
            HugePages_Surp, Hugepagesize, DirectMap4k, DirectMap2M, DirectMap1G;
    // ----------------------------
    string gpu_name;
    float gpu_fan_speed;
    uint gpu_memory_total, gpu_memory_free, gpu_shared_memory_total, gpu_shared_memory_free;
    float nv_gpu_usage, nv_mem_usage, nv_enc_usage, nv_dec_usage;
    int gpu_temp;
    float gpu_power_draw, gpu_power_limit;
    int gpu_graphics_clock, gpu_sm_clock, gpu_mem_clock, gpu_video_clock;
    // ----------------------------
    map<string, cpu_info> cpus;
    uint ncpus;
    // ----------------------------
    map<string, diskstat> diskstats;
    // ----------------------------
    map<string, net_dev> net_devs;
    float net_receive_speed, net_transmit_speed;
};

struct proc_info {
    int pid;
    string comm;
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    uint flags;
    ulong minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, priority, nice, num_threads, itrealvalue;
    ulonglong starttime;
    ulong vsize;
    long rss;
    ulong rsslim, startcode, endcode, startstack, kstkesp, kstkeip, signal, blocked,
            sigignore, sigcatch, wchan, nswap, cnswap;
    int exit_signal, processor;
    uint rt_priority, policy;
    ulonglong delayacct_blkio_ticks;
    ulong guest_time;
    uint cguest_time;
    ulong start_data, end_data, start_brk, arg_start, arg_end, env_start, env_end;
    int exit_code;
    // ----------- above from /proc/[pid]/stat ------------
    ulong statm_size, statm_resident, statm_shared, statm_text, statm_lib, statm_data, statm_dt;
    // ----------- above from /proc/[pid]/statm
    string nv_type;
    float nv_sm_usage, nv_mem_usage, nv_enc_usage, nv_dec_usage;
    int gpu_memory_used;
    string cmdline;
    float cpu_usage; // cpu usage of process [0, #cores]
    ulong virtual_mem;  // in bytes
    long mem, shared_mem, resident_mem;  // in bytes
    string cgroup;
    string cwd, exe;
    uint uid;
};
struct proc_net_info {
    int pid;
    float net_send, net_receive; // in kilobytes
};
struct proc_disk_info {
    int pid;
    float disk_read, disk_write, disk_canceled_write, disk_iodelay;
};

#endif //LEMOND_MAIN_H
