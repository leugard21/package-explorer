#pragma once

#include <string>
#include <vector>

namespace pkg {

struct Package {
  std::string name;
  std::string version;
  std::string description;
};

class PackageManager {
public:
  virtual ~PackageManager() = default;

  virtual std::vector<Package> listInstalled() = 0;
};

} // namespace pkg