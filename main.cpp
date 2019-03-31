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
#include <sys/stat.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "main.h"
#include "json.h"
#include "spawn.h"
#include "timer.h"
#include "ws.h"
#include "systemd.h"
#include "user.h"

using namespace std;
namespace fs = filesystem;
namespace b = boost;
namespace bp = boost::process;
namespace asio = boost::asio;
namespace pt = b::property_tree;
using tcp = boost::asio::ip::tcp;

string systemd_units;
string lsblk;
vector<user> users;
vector<string> shells;
struct sys_info sys = {};
struct sys_info prev_sys = {};
map<int, proc_info> procs;
map<int, proc_net_info> procs_net;
map<int, proc_disk_info> procs_disk;
map<int, proc_info> prev_proc;

map<string, diskstat> get_diskstats() {
    map<string, diskstat> diskstats;
    ifstream file("/proc/diskstats");
    string line;
    string dummy;
    while (getline(file, line)) {
        istringstream iss(line);
        diskstat d;
        iss >> dummy >> dummy >> d.name >>
            d.reads_completed >> d.reads_merged >> d.sectors_read >> d.time_read >>
            d.writes_completed >> d.writes_merged >> d.sectors_written >> d.time_write >>
            d.io_in_progress >> d.time_io >> d.weighted_time_io;
        diskstats[d.name] = d;
    }
    return diskstats;
}

map<string, net_dev> get_net_devs() {
    map<string, net_dev> net_devs;
    ifstream file("/proc/net/dev");
    string line;
    getline(file, line);
    getline(file, line);
    string dummy;
    while (getline(file, line)) {
        istringstream iss(line);
        net_dev d = {};
        iss >> d.interface
            >> d.receive_bytes >> d.receive_packets >> d.receive_errs >> d.receive_drop >> d.receive_fifo
            >> d.receive_frame >> d.receive_compressed >> d.receive_multicast
            >> d.transmit_bytes >> d.transmit_packets >> d.transmit_errs >> d.transmit_drop >> d.transmit_fifo
            >> d.transmit_colls >> d.transmit_carrier >> d.transmit_compressed;
        net_devs[d.interface] = d;
    }
    return net_devs;
}

void calc_disk(diskstat &disk) {
    if (prev_sys.diskstats.find(disk.name) != prev_sys.diskstats.end()) {
        diskstat &prev = prev_sys.diskstats[disk.name];
        disk.usage = (disk.time_io - prev.time_io) / 1000.0;
    } else {
        disk.usage = disk.time_io / sys.uptime;
    }
}

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
    info.ncpus = max(static_cast<int>(info.cpus.size()) - 1, 1);

    info.diskstats = get_diskstats();

    return info;
}

proc_info get_proc_info(int pid) {
    proc_info info = {};
    string dummy;

    struct stat filestat;
    if (stat(("/proc/" + to_string(pid)).c_str(), &filestat)) {
        info.uid = filestat.st_uid;
    }

    ifstream file("/proc/" + to_string(pid) + "/stat");
    file >> info.pid;
    getline(file, info.comm, ')');
    info.comm += ')';
    info.comm = info.comm.substr(1);
    file >> info.state >> info.ppid >> info.pgrp >> info.session >> info.tty_nr >> info.tpgid
         >> info.flags >> info.minflt >> info.cminflt >> info.majflt >> info.cmajflt >> info.utime >> info.stime
         >> info.cutime >> info.cstime >> info.priority >> info.nice >> info.num_threads >> info.itrealvalue
         >> info.starttime >> info.vsize >> info.rss >> info.rsslim >> info.startcode >> info.endcode >> info.startstack
         >> info.kstkesp >> info.kstkeip >> info.signal >> info.blocked >> info.sigignore >> info.sigcatch >> info.wchan
         >> info.nswap >> info.cnswap >> info.exit_signal >> info.processor >> info.rt_priority >> info.policy
         >> info.delayacct_blkio_ticks >> info.guest_time >> info.cguest_time >> info.start_data >> info.end_data
         >> info.start_brk >> info.arg_start >> info.arg_end >> info.env_start >> info.env_end >> info.exit_code;

    file = ifstream("/proc/" + to_string(pid) + "/statm");
    file >> info.statm_size >> info.statm_resident >> info.statm_shared >> info.statm_text >> info.statm_lib
         >> info.statm_data >> info.statm_dt;

    file = ifstream("/proc/" + to_string(pid) + "/cmdline");
    getline(file, info.cmdline);
    for (char &i : info.cmdline)
        if (i == '\0')
            i = ' ';

    file = ifstream("/proc/" + to_string(pid) + "/cgroup");
    string line;
    while (getline(file, line)) {
        if (line[0] == '0') {
            //vector<string> parts;
            //b::split(parts, line, b::is_any_of("/"));
            //info.cgroup = parts[parts.size() - 1];
            info.cgroup = line.substr(3);
            break;
        }
    }

    try {
        fs::path p = "/proc/" + to_string(pid) + "/cwd";
        if (fs::exists(p) && fs::is_symlink(p)) {
            error_code ec;
            fs::path target = fs::read_symlink(p);
            info.cwd = target;
        }
    } catch (fs::filesystem_error &e) {
        info.cwd = "";
    }
    try {
        fs::path p = "/proc/" + to_string(pid) + "/exe";
        if (fs::exists(p) && fs::is_symlink(p)) {
            error_code ec;
            fs::path target = fs::read_symlink(p);
            info.exe = target;
        }
    } catch (fs::filesystem_error &e) {
        info.exe = "";
    }

    return info;
}

void get_gpu_info() {
    {
        // pt::ptree pt;
        // bp::ipstream is;
        // bp::child c("nvidia-smi -x -q", bp::std_out > is);
        // pt::read_xml(is, pt);

        istringstream is(exec("nvidia-smi -x -q"));
        pt::ptree pt;
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
        // c.wait();

    }
    {
        // pt::ptree pt;
        // bp::ipstream is;
        string line;
        string dummy;
        // bp::child c("nvidia-smi pmon -c 1", bp::std_out > is);
        istringstream is(exec("nvidia-smi pmon -c 1"));
        getline(is, line);  // two dummy lines
        getline(is, line);
        while (/* c.running() && */ getline(is, line)) {
            istringstream iss(line);
            int pid;
            iss >> dummy >> pid;
            iss >> procs[pid].nv_type >> procs[pid].nv_sm_usage >> procs[pid].nv_mem_usage >> procs[pid].nv_enc_usage
                >> procs[pid].nv_dec_usage;
        }
        // c.wait();
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

float calc_mem(proc_info &proc) { // FIXME: minus shared mem
    proc.mem = (proc.rss - proc.statm_shared) * sys.page_size;
    proc.virtual_mem = proc.vsize;
    proc.shared_mem = proc.statm_shared * sys.page_size;
    proc.resident_mem = proc.rss * sys.page_size;
    return -1;
}

float calc_net() {
    sys.net_receive_speed = 0;
    sys.net_transmit_speed = 0;
    for (const auto &kv : sys.net_devs) {
        net_dev dev = kv.second;
        if (dev.interface == "lo:") continue;  // loopback device doesn't count
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
        sys.net_receive_speed += (receive - prev_receive) / (uptime - prev_uptime);
        sys.net_transmit_speed += (transmit - prev_transmit) / (uptime - prev_uptime);
    }
    return -1;
}

vector<int> get_pids() {
    vector<int> pids;
    for (auto &p : fs::directory_iterator("/proc")) {
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

        json.kv("net_receive_speed", sys.net_receive_speed);
        json.kv("net_transmit_speed", sys.net_transmit_speed);

        json.kv("ncpus", sys.ncpus);
        json.key("cpus");
        json.start();
        {
            for (auto &kv: sys.cpus) {
                json.kv(kv.first, kv.second.usage);
            }
        }
        json.end();

        json.key("diskstats");
        json.start();
        {
            for (auto &kv: sys.diskstats) {
                json.key(kv.first);
                json.start();
                json.kv("sectors_read", kv.second.sectors_read);
                json.kv("sectors_written", kv.second.sectors_written);
                json.kv("usage", kv.second.usage);
                json.end();
            }
        }
        json.end();
    }
    json.end();

    json.key("systemd_units");
    json.raw_value(systemd_units);

    json.key("lsblk");
    json.raw_value(lsblk);

    json.key("users");
    json.list_start();
    for (auto &u: users) {
        json.list_entry();
        json.start();
        json.kv("uid", u.uid);
        json.kv("gid", u.gid);
        json.kv("name", u.name);
        json.end();
    }
    json.list_end();

    json.key("shells");
    json.list_start();
    for (auto &s: shells) {
        json.list_entry(s);
    }
    json.list_end();

    json.key("processes");
    json.start();
    {
        for (auto &kv: procs) {
            int pid = kv.first;
            json.key(to_string(pid));
            json.start();
            {
                proc_info proc = kv.second;
                json.kv("pid", pid);
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

                json.kv("cgroup", proc.cgroup);
                json.kv("cwd", proc.cwd);
                json.kv("exe", proc.exe);
                json.kv("uid", proc.uid);

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
    for (auto &kv: sys.diskstats) {
        calc_disk(kv.second);
    }
    sys.net_devs = get_net_devs();
    calc_net();

    vector<int> pids = get_pids();
    procs.clear();
    for (int &pid : pids) {
        proc_info proc = get_proc_info(pid);
        calc_cpu(proc);
        calc_mem(proc);
        procs[proc.pid] = proc;
    }

    get_gpu_info();

    string json = generate_json();
//    cout << json << endl;
    for (auto &s: get_sessions()) {
        s->write(json);
    }

    prev_proc = procs;
    prev_sys = sys;
}


int main() {
    try {
        systemd_units = systemd_list_units();
        lsblk = exec("lsblk -J");
        users = get_users();
        shells = get_shells();
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
                iss >> dummy >> dummy >> proc_disk.pid >> proc_disk.disk_read >> proc_disk.disk_write
                    >> proc_disk.disk_canceled_write >> proc_disk.disk_iodelay;
                procs_disk[proc_disk.pid] = proc_disk;
            }
        }, io);

        timer t(1, io, [&]() {
            query();
        });

        auto const address = asio::ip::make_address("127.0.0.1");
        auto const port = static_cast<unsigned short>(8089);
        make_shared<listener>(io, tcp::endpoint(address, port))->run();

        io.run();
    } catch (const exception &e) {
        cout << e.what() << endl;
    }
}
