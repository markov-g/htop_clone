#include "processor.h"

#include <string>
#include <vector>

#include "linux_parser.h"

// Return the aggregate CPU utilization
float Processor::Utilization() {
  std::vector<std::string> utilization = LinuxParser::CpuUtilization();

  double idle = std::stod(utilization[3]) + std::stod(utilization[4]);
  double non_idle = std::stod(utilization[0]) + std::stod(utilization[1]) +
                    std::stod(utilization[2]) + std::stod(utilization[5]) +
                    std::stod(utilization[6]) + std::stod(utilization[7]);

  double total = idle + non_idle;

  return static_cast<float>(non_idle / total);
}
