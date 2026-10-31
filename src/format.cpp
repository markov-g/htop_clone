#include "format.h"

#include <iomanip>
#include <sstream>
#include <string>

using std::string;

// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
string Format::ElapsedTime(long total_sec) {
  long hrs = total_sec / 3600;
  long mins = (total_sec % 3600) / 60;
  long secs = total_sec % 60;

  std::ostringstream out;
  out << std::setfill('0') << std::setw(2) << hrs << ":" << std::setw(2) << mins
      << ":" << std::setw(2) << secs;

  return out.str();
}
