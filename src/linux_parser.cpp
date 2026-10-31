#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// Read and return the system memory utilization
/*
Total used memory = MemTotal - MemFree
Non cache/buffer memory (green) = Total used memory - (Buffers + Cached memory)
Buffers (blue) = Buffers
Cached memory (yellow) = Cached + SReclaimable - Shmem
Swap = SwapTotal - SwapFree
*/
float LinuxParser::MemoryUtilization() { 
  string line, key, value;
  long mem_total = 0, mem_avail = 0; 
  std::ifstream fs(kProcDirectory + kMeminfoFilename);

  if (fs.is_open()) {
    while (std::getline(fs, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;

      if(key == "MemTotal:") {
        try {
          mem_total = std::stol(value);
        } catch (const std::invalid_argument& e) {
          std::cerr << "Invalid uptime value: " << e.what() << "\n";
        } catch (const std::out_of_range& e) {
          std::cerr << "MemTotal value out of range: " << e.what() << "\n";
        }        
      } 
      if(key == "MemAvailable:") {
        try {
          mem_avail = std::stol(value);
        } catch (const std::invalid_argument& e) {
          std::cerr << "Invalid uptime value: " << e.what() << "\n";
        } catch (const std::out_of_range& e) {
          std::cerr << "MamAvailable value out of range: " << e.what() << "\n";
        }
      }
    }
  }

  if (mem_total == 0) {
    return 0.0f;
  }
  
  return static_cast<float>(mem_total - mem_avail) / mem_total; 
}

// Read and return the system uptime
long LinuxParser::UpTime() { 
  long total_sec = 0;
  string line, total, idle;

  std::ifstream fs(kProcDirectory + kUptimeFilename);
  if (fs.is_open()) {
    std::getline(fs, line);
    std::istringstream linestream(line);
    linestream >> total >> idle;

    try {
      total_sec = std::stol(total);
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid uptime value: " << e.what() << "\n";
    } catch (const std::out_of_range& e) {
      std::cerr << "Uptime value out of range: " << e.what() << "\n";
    }
  }

  return total_sec; 
}

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { 
  string line, cpu;
  long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;

  std::ifstream fs(kProcDirectory + kStatFilename);
  
  if(fs.is_open()) {
    std::getline(fs, line); // only need 1st line
    std::istringstream linestream(line);
    linestream >> cpu >> user >> nice >> system >> idle
       >> iowait >> irq >> softirq >> steal;

    if (linestream && cpu == "cpu") {
      return user + nice + system + idle + iowait + irq + softirq + steal;
    }
  }

  return 0; 
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) { 
  string line;
  string comm, state;
  long pid_val{}, ppid{}, pgrp{}, session{}, tty_nr{}, tpgid{}, flags{},
       minflt{}, cminflt{}, majflt{}, cmajflt{},
       utime{}, stime{}, cutime{}, cstime{};

  std::ifstream fs(kProcDirectory + std::to_string(pid) + kStatFilename);
  if(fs.is_open()) {
    std::getline(fs, line); // /proc/[pid]/stat is a single line
    std::istringstream linestream(line); 

    linestream >> pid_val >> comm >> state >> ppid >> pgrp >> session
               >> tty_nr >> tpgid >> flags >> minflt >> cminflt >> majflt
               >> cmajflt >> utime >> stime >> cutime >> cstime;

    return utime + stime + cutime + cstime;
  }

  return 0; 
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() { 
  string line, cpu;
  long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;

  std::ifstream fs(kProcDirectory + kStatFilename);
  if (fs.is_open()) {
    // we only need 1st line
    std::getline(fs, line);
    std::istringstream linestream(line);

    linestream >> cpu >> user >> nice >> system >> idle
               >> iowait >> irq >> softirq >> steal;

    if(linestream && cpu == "cpu") {
      return user + nice + system + irq + softirq + steal;
    }
  }
  
  return 0; 
}

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() { 
  string line, cpu;
  long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;

  std::ifstream fs(kProcDirectory + kStatFilename);
  if (fs.is_open()) {
    std::getline(fs, line);
    std::istringstream linestream(line);
    linestream >> cpu >> user >> nice >> system >> idle
               >> iowait >> irq >> softirq >> steal; 

    return idle + iowait;
  }
  return 0; 
}

// Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() { 
  string line, cpu;
  vector<string> result{};
  string user, nice, system, idle, iowait, irq, softirq, steal;

  std::ifstream fs(kProcDirectory + kStatFilename);
  if (fs.is_open()) {
    std::getline(fs, line);
    std::istringstream linestream(line);

    linestream >> cpu >> user >> nice >> system >> idle
               >> iowait >> irq >> softirq >> steal;

    if(linestream && cpu == "cpu") {
      // result.push_back(cpu);
      result.push_back(user);
      result.push_back(nice);
      result.push_back(system);
      result.push_back(idle);
      result.push_back(iowait);
      result.push_back(irq);
      result.push_back(softirq);
      result.push_back(steal);
    }

    return result;
  }
  
  return result; 
}

// Read and return the total number of processes: ls /proc | grep -E '^[0-9]+$'
int LinuxParser::TotalProcesses() { 
  string line, key, value;
  long procs = 0;
  std::ifstream fs(kProcDirectory + kStatFilename);
  if(fs.is_open()) {
    while(std::getline(fs, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;

      if(key == "processes") {
        procs = std::stol(value);
        return procs;
      }
    }
  }
  
  return 0; 
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() { 
  string line, key, value;
  long running_procs = 0;

  std::ifstream fs(kProcDirectory + kStatFilename);
  if(fs.is_open()) {
    while(std::getline(fs, line)) {
      std::istringstream linestream(line);
      if (linestream >> key >> value && key == "procs_running") {
        running_procs = std::stol(value);
        return running_procs;
      }
      
    }
  }
  
  return 0; 
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) { 
  string line, command;
  std::ifstream fs(kProcDirectory + std::to_string(pid) + kCmdlineFilename);
  
  if(fs.is_open()){
    std::getline(fs, line);
    std::istringstream linestream(line);

    linestream >> command;
  }

  return command; 
}

// Read and return the memory used by a process
string LinuxParser::Ram(int pid) { 
  string line, key, value;
  
  std::ifstream fs(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if(fs.is_open()) {
    while(std::getline(fs, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value; 
      if (key == "VmRSS:") {
        return value;
      }
    }
  }
  
  return string(); 
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) { 
  string line, key, value;
  
  std::ifstream fs(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if(fs.is_open()) {
    while(std::getline(fs, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value; 
      if (key == "Uid:") {
        return value;
      }
    }
  }
  
  return string();
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) {
  string username, password, uid, line;
  string input_uid = LinuxParser::Uid(pid);

  std::ifstream fs(kPasswordPath);
  if (fs.is_open()) {
    while(std::getline(fs, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);      
      linestream >> username >> password >> uid;
      if (uid == input_uid) {
        return username;
      }
    }
  }

  return string(); 
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) { 
  string line, field; 
  double uptime = 0;

  std::ifstream fs(kProcDirectory + std::to_string(pid) + kStatFilename);
  if (fs.is_open()) {
    std::getline(fs, line);
    std::istringstream linestream(line);

    for (int fieldNumber = 1; fieldNumber <= 22; ++fieldNumber) {
        if (!(linestream >> field)) {
            return 0;
        }
    }

    long starttime = std::stol(field);
    long ticks = sysconf(_SC_CLK_TCK);

    uptime = LinuxParser::UpTime() - starttime / ticks;

    return uptime;
  }
  
  return 0; 
}

// int main() { // For simple testing during development
//   std::cout << "Memory Utilization: " << LinuxParser::MemoryUtilization() << std::endl;
//   long total_sec = LinuxParser::UpTime();  
//   long days = total_sec / (24 * 60 * 60);
//   long hrs  = (total_sec % (24 * 60 * 60)) / (60 * 60);
//   long mins = (total_sec % (60 * 60)) / 60;
//   long secs = total_sec % 60;
//   std::cout << "Uptime: " << days << "D and " << hrs << ":" << mins << ":" << secs << "(HH:MM:SS)" << std::endl;

//   long jiffies = LinuxParser::Jiffies();
//   std::cout << "Jiffies: " << jiffies << std::endl;

//   long pid_jiffies = LinuxParser::ActiveJiffies(1);
//   std::cout << "PID:1 Jiffies: " << pid_jiffies << std::endl;

//   long active_jiffies = LinuxParser::ActiveJiffies();
//   std::cout << "Active Jiffies: " << active_jiffies << std::endl;

//   long idle_jiffies = LinuxParser::IdleJiffies();
//   std::cout << "Idle Jiffies: " << idle_jiffies << std::endl;

//   vector<string> cpu_utilization = LinuxParser::CpuUtilization();
//   std::cout << "=-=-=- CPU UTILIZATION =-=-=-" << std::endl;
//   for (string c : cpu_utilization) { std::cout << c << std::endl; }
//   std::cout << "=-=-=- END: CPU UTILIZATION =-=-=-" << std::endl;

//   long totalProcs = LinuxParser::TotalProcesses();
//   std::cout << "Total # of processes: " << totalProcs << std::endl;

//   long runningProcs = LinuxParser::RunningProcesses();
//   std::cout << "# of running processes: " << runningProcs << std::endl;

//   string proc_cmd = LinuxParser::Command(1);
//   std::cout << "Process PID: 1 command: " << proc_cmd << std::endl;

//   string used_mem_for_proc = LinuxParser::Ram(1);
//   std::cout << "Memory usage for PID #1: " << used_mem_for_proc << std::endl;
  
//   string proc_uid = LinuxParser::Uid(1);
//   std::cout << "UID for #1: " << proc_uid << std::endl;

//   string proc_user = LinuxParser::User(1);
//   std::cout << "User for #1: " << proc_user << std::endl;

//   long proc_uptime = LinuxParser::UpTime(1);
//   std::cout << "Uptime for process with PID #1: " << std::to_string(proc_uptime) << std::endl;
// }  
