/**
 * I want to monitor: CPU Memory Disk Network GPU
 */
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <thread>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <iomanip>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
namespace fs = filesystem;
namespace bp = boost::process;
namespace asio = boost::asio;
namespace b = boost;
namespace pt = b::property_tree;
typedef unsigned long long ulonglong;

struct cpu_info {
    string name;
    ulong idle;
    float usage;
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
};
struct proc_net_info {
    int pid;
    float net_send, net_receive; // in kilobytes
};
struct proc_disk_info {
    int pid;
    float disk_read, disk_write, disk_canceled_write, disk_iodelay;
};

struct sys_info sys = {};
struct sys_info prev_sys = {};
map<int, proc_info> procs;
map<int, proc_net_info> procs_net;
map<int, proc_disk_info> procs_disk;
map<int, proc_info> prev_proc;


sys_info get_sys_info() {
    sys_info info = {};
    string dummy;

    info.clock_ticks = sysconf(_SC_CLK_TCK);
    info.page_size = sysconf(_SC_PAGE_SIZE);

    ifstream file("/proc/uptime");
    file >> info.uptime;

    file = ifstream("/proc/meminfo");
    file >>
         dummy >> info.MemTotal >> dummy >>
         dummy >> info.MemFree >> dummy >>
         dummy >> info.MemAvailable >> dummy >>
         dummy >> info.Buffers >> dummy >>
         dummy >> info.Cached >> dummy >>
         dummy >> info.SwapCached >> dummy >>
         dummy >> info.Active >> dummy >>
         dummy >> info.Inactive >> dummy >>
         dummy >> info.Active_anon >> dummy >>
         dummy >> info.Inactive_anon >> dummy >>
         dummy >> info.Active_file >> dummy >>
         dummy >> info.Inactive_file >> dummy >>
         dummy >> info.Unevictable >> dummy >>
         dummy >> info.Mlocked >> dummy >>
         dummy >> info.SwapTotal >> dummy >>
         dummy >> info.SwapFree >> dummy >>
         dummy >> info.Dirty >> dummy >>
         dummy >> info.Writeback >> dummy >>
         dummy >> info.AnonPages >> dummy >>
         dummy >> info.Mapped >> dummy >>
         dummy >> info.Shmem >> dummy >>
         dummy >> info.Slab >> dummy >>
         dummy >> info.SReclaimable >> dummy >>
         dummy >> info.SUnreclaim >> dummy >>
         dummy >> info.KernelStack >> dummy >>
         dummy >> info.PageTables >> dummy >>
         dummy >> info.NFS_Unstable >> dummy >>
         dummy >> info.Bounce >> dummy >>
         dummy >> info.WritebackTmp >> dummy >>
         dummy >> info.CommitLimit >> dummy >>
         dummy >> info.Committed_AS >> dummy >>
         dummy >> info.VmallocTotal >> dummy >>
         dummy >> info.VmallocUsed >> dummy >>
         dummy >> info.VmallocChunk >> dummy >>
         dummy >> info.HardwareCorrupted >> dummy >>
         dummy >> info.AnonHugePages >> dummy >>
         dummy >> info.ShmemHugePages >> dummy >>
         dummy >> info.ShmemPmdMapped >> dummy >>
         dummy >> info.CmaTotal >> dummy >>
         dummy >> info.CmaFree >> dummy >>
         dummy >> info.HugePages_Total >>
         dummy >> info.HugePages_Free >>
         dummy >> info.HugePages_Rsvd >>
         dummy >> info.HugePages_Surp >>
         dummy >> info.Hugepagesize >> dummy >>
         dummy >> info.DirectMap4k >> dummy >>
         dummy >> info.DirectMap2M >> dummy >>
         dummy >> info.DirectMap1G;

    file = ifstream("/proc/stat");
    string line;
    while (getline(file, line)) {
        if (line.find("cpu") != 0) break;
        istringstream iss(line);
        cpu_info cpu;
        iss >> cpu.name >> dummy >> dummy >> dummy >> cpu.idle;
        info.cpus[cpu.name] = cpu;
    }


    return info;
}

void get_gpu_info() {
    {
        pt::ptree pt;
        bp::ipstream is;
        bp::child c("nvidia-smi -x -q", bp::std_out > is);
        pt::read_xml(is, pt);
        sys.gpu_name = pt.get<string>("nvidia_smi_log.gpu.product_name");
        sys.gpu_fan_speed = stof(pt.get<string>("nvidia_smi_log.gpu.fan_speed"));
        sys.gpu_memory_total = stoi(pt.get<string>("nvidia_smi_log.gpu.fb_memory_usage.total"));
        sys.gpu_memory_free = stoi(pt.get<string>("nvidia_smi_log.gpu.fb_memory_usage.free"));
        sys.gpu_shared_memory_total = stoi(pt.get<string>("nvidia_smi_log.gpu.bar1_memory_usage.total"));
        sys.gpu_shared_memory_free = stoi(pt.get<string>("nvidia_smi_log.gpu.bar1_memory_usage.free"));
        sys.nv_gpu_usage = stof(pt.get<string>("nvidia_smi_log.gpu.utilization.gpu_util"));
        sys.nv_mem_usage = stof(pt.get<string>("nvidia_smi_log.gpu.utilization.memory_util"));
        sys.nv_enc_usage = stof(pt.get<string>("nvidia_smi_log.gpu.utilization.encoder_util"));
        sys.nv_dec_usage = stof(pt.get<string>("nvidia_smi_log.gpu.utilization.decoder_util"));
        sys.gpu_temp = stoi(pt.get<string>("nvidia_smi_log.gpu.temperature.gpu_temp"));
        sys.gpu_power_draw = stof(pt.get<string>("nvidia_smi_log.gpu.power_readings.power_draw"));
        sys.gpu_power_limit = stof(pt.get<string>("nvidia_smi_log.gpu.power_readings.power_limit"));
        sys.gpu_graphics_clock = stoi(pt.get<string>("nvidia_smi_log.gpu.clocks.graphics_clock"));
        sys.gpu_sm_clock = stoi(pt.get<string>("nvidia_smi_log.gpu.clocks.sm_clock"));
        sys.gpu_mem_clock = stoi(pt.get<string>("nvidia_smi_log.gpu.clocks.mem_clock"));
        sys.gpu_video_clock = stoi(pt.get<string>("nvidia_smi_log.gpu.clocks.video_clock"));
        for (auto &v : pt.get_child("nvidia_smi_log.gpu.processes")) {
            procs[stoi(v.second.get<string>("pid"))].gpu_memory_used = stoi(v.second.get<string>("used_memory"));
        }
        c.wait();
    }
    {
        pt::ptree pt;
        bp::ipstream is;
        string line;
        string dummy;
        bp::child c("nvidia-smi pmon -c 1", bp::std_out > is);
        getline(is, line);  // two dummy lines
        getline(is, line);
        while (c.running() && getline(is, line)) {
            istringstream iss(line);
            int pid;
            iss >> dummy >> pid;
            iss >> procs[pid].nv_type >> procs[pid].nv_sm_usage >> procs[pid].nv_mem_usage >> procs[pid].nv_enc_usage
                >> procs[pid].nv_dec_usage;
        }
    }
}

float calc_cpu(cpu_info &cpu) {
    float prev_uptime = 0;
    ulong prev_totaltime = 0;
    if (prev_sys.cpus.find(cpu.name) != prev_sys.cpus.end()) {
        prev_uptime = prev_sys.uptime;
        prev_totaltime = prev_sys.cpus[cpu.name].idle;
    }
    ulong totaltime = cpu.idle;
    float idle_usage = (float) (totaltime - prev_totaltime) / sys.clock_ticks / (sys.uptime - prev_uptime);
    int max_usage = 1;
    if (cpu.name == "cpu") {
        max_usage = max(1, (int) sys.cpus.size() - 1);
    }
    cpu.usage = max_usage - idle_usage;
    return max_usage - idle_usage;
}

proc_info get_proc_info(int pid) {
    proc_info info = {};
    string dummy;

    ifstream file("/proc/" + to_string(pid) + "/stat");
    file >> info.pid >> info.comm >> info.state >> info.ppid >> info.pgrp >> info.session >> info.tty_nr >> info.tpgid
         >> info.flags >> info.minflt >> info.cminflt >> info.majflt >> info.cmajflt >> info.utime >> info.stime
         >> info.cutime >> info.cstime >> info.priority >> info.nice >> info.num_threads >> info.itrealvalue
         >> info.starttime >> info.vsize >> info.rss >> info.rsslim >> info.startcode >> info.endcode >> info.startstack
         >> info.kstkesp >> info.kstkeip >> info.signal >> info.blocked >> info.sigignore >> info.sigcatch >> info.wchan
         >> info.nswap >> info.cnswap >> info.exit_signal >> info.processor >> info.rt_priority >> info.policy
         >> info.delayacct_blkio_ticks >> info.guest_time >> info.cguest_time >> info.start_data >> info.end_data
         >> info.start_brk >> info.arg_start >> info.arg_end >> info.env_start >> info.env_end >> info.exit_code;

/*    file = ifstream("/proc/" + to_string(pid) + "/io");
    file >>
         dummy >> info.rchar >>
         dummy >> info.wchan >>
         dummy >> info.syscr >>
         dummy >> info.syscw >>
         dummy >> info.read_bytes >>
         dummy >> info.write_bytes >>
         dummy >> info.cancelled_write_bytes;*/

    file = ifstream("/proc/" + to_string(pid) + "/statm");
    file >> info.statm_size >> info.statm_resident >> info.statm_shared >> info.statm_text >> info.statm_lib
         >> info.statm_data >> info.statm_dt;

    file = ifstream("/proc/" + to_string(pid) + "/cmdline");
    getline(file, info.cmdline);
    for (int i = 0; i < info.cmdline.length(); ++i)
        if (info.cmdline[i] == '\0')
            info.cmdline[i] = ' ';

    return info;
}

float calc_cpu(proc_info &proc) {
    float prev_uptime = proc.starttime / sys.clock_ticks;
    ulong prev_totaltime = 0;
    if (prev_proc.find(proc.pid) != prev_proc.end()) {
        prev_uptime = prev_sys.uptime;
        prev_totaltime = prev_proc[proc.pid].utime + prev_proc[proc.pid].stime;
    }
    ulong totaltime = proc.utime + proc.stime;
    float cpu_usage = (float) (totaltime - prev_totaltime) / sys.clock_ticks / (sys.uptime - prev_uptime);
    proc.cpu_usage = cpu_usage;
    return cpu_usage;
}

/*float calc_io(proc_info &proc) {
    ulong prev_read = 0;
    ulong prev_write = 0;
    float prev_uptime = proc.starttime / sys.clock_ticks;
    if (prev_proc.find(proc.pid) != prev_proc.end()) {
        prev_uptime = prev_sys.uptime;
        prev_read = prev_proc[proc.pid].read_bytes;
        prev_write = prev_proc[proc.pid].write_bytes;
    }
    float uptime = sys.uptime;
    ulong read = proc.read_bytes;
    ulong write = proc.write_bytes;
    proc.io_read_speed = (read - prev_read) / (uptime - prev_uptime);
    proc.io_write_speed = (write - prev_write) / (uptime - prev_uptime);
    return -1;
}*/

/*float calc_net() {
    sys.io_receive_speed = 0;
    sys.io_transmit_speed = 0;
    for (const auto &kv : sys.net_devs) {
        net_dev dev = kv.second;
        if (dev.interface == "lo:") continue;  // loopback device doesn't count
        cout << dev.interface;
        ulong prev_receive = 0;
        ulong prev_transmit = 0;
        float prev_uptime = 0;
        if (prev_sys.net_devs.find(dev.interface) != prev_sys.net_devs.end()) {
            prev_receive = prev_sys.net_devs[dev.interface].receive_bytes;
            prev_transmit = prev_sys.net_devs[dev.interface].transmit_bytes;
            prev_uptime = prev_sys.uptime;
        }
        float uptime = sys.uptime;
        ulong receive = dev.receive_bytes;
        ulong transmit = dev.transmit_bytes;
        sys.io_receive_speed += (receive - prev_receive) / (uptime - prev_uptime);
        sys.io_transmit_speed += (transmit - prev_transmit) / (uptime - prev_uptime);
    }
    return -1;
}*/

float calc_mem(proc_info &proc) { // FIXME: minus shared mem
    proc.mem = (proc.rss - proc.statm_shared) * sys.page_size;
    proc.virtual_mem = proc.vsize;
    proc.shared_mem = proc.statm_shared * sys.page_size;
    proc.resident_mem = proc.rss * sys.page_size;
    return -1;
}

vector<int> get_pids() {
    vector<int> pids;
    string path = "/proc";
    for (auto &p : fs::directory_iterator(path)) {
        string path = p.path();
        string filename = path.substr(path.find_last_of('/') + 1);
        int pid = -1;
        try {
            pid = stoi(filename);
        } catch (const invalid_argument &) {
            continue;
        }
        pids.push_back(pid);
    }
    return pids;
}

class json_generator {
private:
    ostringstream oss;

    std::string escape_json(const std::string &s) {
        std::ostringstream o;
        for (auto c = s.cbegin(); c != s.cend(); c++) {
            switch (*c) {
                case '"':
                    o << "\\\"";
                    break;
                case '\\':
                    o << "\\\\";
                    break;
                case '\b':
                    o << "\\b";
                    break;
                case '\f':
                    o << "\\f";
                    break;
                case '\n':
                    o << "\\n";
                    break;
                case '\r':
                    o << "\\r";
                    break;
                case '\t':
                    o << "\\t";
                    break;
                default:
                    if ('\x00' <= *c && *c <= '\x1f') {
                        o << "\\u"
                          << std::hex << std::setw(4) << std::setfill('0') << (int) *c;
                    } else {
                        o << *c;
                    }
            }
        }
        return o.str();
    }

    string delim;
public:
    json_generator() : delim("") {}

    void start() {
        oss << '{';
        delim = "";
    }

    void end() {
        oss << '}';
        delim = ",";
    }

    void key(string key) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":";
    }

    void kv(string key, long v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(string key, ulong v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(string key, int v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(string key, uint v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(string key, float v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << fixed << setprecision(8) << v;
    }

    void kv(string key, string v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << '"' << escape_json(v) << '"';
    }

    string str() {
        return "{" + oss.str() + "}";
    }
};

string generate_json() {
    json_generator json;
    json.key("sys");
    json.start();
    {
        json.kv("MemTotal", sys.MemTotal);
        json.kv("MemAvailable", sys.MemAvailable);
        json.kv("SwapTotal", sys.SwapTotal);
        json.kv("SwapFree", sys.SwapFree);

        json.kv("gpu_name", sys.gpu_name);
        json.kv("gpu_fan_speed", sys.gpu_fan_speed);
        json.kv("gpu_memory_total", sys.gpu_memory_total);
        json.kv("gpu_memory_free", sys.gpu_memory_free);
        json.kv("gpu_shared_memory_total", sys.gpu_shared_memory_total);
        json.kv("gpu_shared_memory_free", sys.gpu_shared_memory_free);
        json.kv("nv_gpu_usage", sys.nv_gpu_usage);
        json.kv("nv_mem_usage", sys.nv_mem_usage);
        json.kv("nv_enc_usage", sys.nv_enc_usage);
        json.kv("nv_dec_usage", sys.nv_dec_usage);
        json.kv("gpu_temp", sys.gpu_temp);
        json.kv("gpu_power_draw", sys.gpu_power_draw);
        json.kv("gpu_power_limit", sys.gpu_power_limit);
        json.kv("gpu_graphics_clock", sys.gpu_graphics_clock);
        json.kv("gpu_sm_clock", sys.gpu_sm_clock);
        json.kv("gpu_mem_clock", sys.gpu_mem_clock);
        json.kv("gpu_video_clock", sys.gpu_video_clock);

        json.key("cpus");
        json.start();
        {
            for (auto &kv: sys.cpus) {
                json.kv(kv.first, kv.second.usage);
            }
        }
        json.end();
    }
    json.end();
    json.key("proc");
    json.start();
    {
        for (auto &kv: procs) {
            int pid = kv.first;
            json.key(to_string(pid));
            json.start();
            {
                proc_info proc = kv.second;
                json.kv("ppid", proc.ppid);
                json.kv("comm", proc.comm);
                json.kv("cmdline", proc.cmdline);
                json.kv("mem", proc.mem);
                json.kv("shared_mem", proc.shared_mem);
                json.kv("resident_mem", proc.resident_mem);
                json.kv("virtual_mem", proc.virtual_mem);
                json.kv("cpu_usage", proc.cpu_usage);

                json.kv("nv_type", proc.nv_type);
                json.kv("nv_sm_usage", proc.nv_sm_usage);
                json.kv("nv_mem_usage", proc.nv_mem_usage);
                json.kv("nv_enc_usage", proc.nv_enc_usage);
                json.kv("nv_dec_usage", proc.nv_dec_usage);
                json.kv("gpu_memory_used", proc.gpu_memory_used);

                if (procs_net.find(pid) != procs_net.end()) {
                    json.kv("net_receive", procs_net[pid].net_receive);
                    json.kv("net_send", procs_net[pid].net_send);
                } else {
                    json.kv("net_receive", 0);
                    json.kv("net_send", 0);
                }

                if (procs_disk.find(pid) != procs_disk.end()) {
                    json.kv("disk_read", procs_disk[pid].disk_read);
                    json.kv("disk_write", procs_disk[pid].disk_write);
                } else {
                    json.kv("disk_read", 0);
                    json.kv("disk_write", 0);
                }
            }
            json.end();
        }
    }
    json.end();
    return json.str();
}

void query() {
    sys = get_sys_info();
    for (auto &kv: sys.cpus) {
        calc_cpu(kv.second);
    }
    vector<int> pids = get_pids();
    procs.clear();
    for (int &pid : pids) {
        proc_info proc = get_proc_info(pid);
        calc_cpu(proc);
        calc_mem(proc);
        procs[proc.pid] = proc;
    }

    get_gpu_info();

    cout << generate_json() << endl;

    prev_proc = procs;
    prev_sys = sys;
}

class spawn_process {
private:
    bp::async_pipe output;
    bp::child child;
    asio::streambuf buffer;
    function<void(istream &)> handler;
public:
    void readloop() {
        asio::async_read_until(output, buffer, '\n', [&](boost::system::error_code ec, size_t transferred) {
            if (transferred) {
                std::istream is(&buffer);
                handler(is);
            }
            if (ec) {
                std::cerr << "Output pipe: " << ec.message() << std::endl;
            } else {
                readloop();
            }
        });
    }

    spawn_process(const string &cmd, function<void(istream &)> handler, asio::io_service &ios) :
            output(ios), child(cmd, bp::std_out > output, bp::std_err > bp::null, ios), handler(handler) {
        readloop();
    }
};

int main() {
    boost::asio::io_service io;
    spawn_process nethogs("nethogs -t", [&, first_refresh = false](istream &is) mutable {
        string line;
        getline(is, line);
//        cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch()).count() << ':' << line << endl;
        if (line == "Refreshing:") {
            procs_net.clear();
            first_refresh = true;
        }
        if (first_refresh) {
            if (line == "Refreshing:") {
                return;
            }
            if (line.empty()) {
                return;
            }
            proc_net_info proc_net = {};
            vector<string> parts;
            b::split(parts, line, b::is_any_of("/"));
            int pid = stoi(parts[parts.size() - 2]);
            if (pid == 0) {
                return;
            }

            istringstream iss(parts[parts.size() - 1]);
            string dummy;
            iss >> dummy >> proc_net.net_send >> proc_net.net_receive;
            procs_net[pid] = proc_net;
        }
//        cout << "!!!!!!" << line << endl;
    }, io);
    spawn_process iotop("pidstat -dlhH 1", [&, first_refresh = false](istream &is) mutable {
        string line;
        getline(is, line);
        if (line[0] == '#') {
            procs_disk.clear();
            first_refresh = true;
        }
        if (first_refresh) {
            if (line[0] == '#') {
                return;
            }
            if (line.empty()) {
                return;
            }
            proc_disk_info proc_disk = {};
            string dummy;
            istringstream iss(line);
            iss >> dummy >> dummy >> proc_disk.pid >> proc_disk.disk_read >> proc_disk.disk_write >> proc_disk.disk_canceled_write >> proc_disk.disk_iodelay;
            procs_disk[proc_disk.pid] = proc_disk;
        }
//        cout << "####" << line << endl;
    }, io);

    asio::steady_timer t(io, chrono::seconds(1));
    std::function<void()> queryloop = [&]() {
        t.async_wait([&](auto &ec) {
            query();
            t.expires_at(t.expires_at() + chrono::seconds(1));
            queryloop();
        });
    };
    queryloop();
    io.run();
}
