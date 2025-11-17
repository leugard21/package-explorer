#pragma once

#include <string>
#include <vector>

namespace pkg {

struct Package {
  std::string name;
  std::string version;
  std::string description;
  std::string repo;
  std::string architecture;
  std::string install_date;

  std::vector<std::string> depends_on;
  std::vector<std::string> required_by;

  bool is_foreign = false;
  bool is_explicit = false;
};

class PackageManager {
public:
  virtual ~PackageManager() = default;

  virtual std::vector<Package> listInstalled() = 0;
  virtual bool fillDetails(Package &pkg) = 0;
};

} // namespace pkg