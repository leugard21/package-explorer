#pragma once

#include "PackageManager.h"
#include <vector>

namespace pkg {

class PacmanPackageManager : public PackageManager {
public:
  std::vector<Package> listInstalled() override;
  bool fillDetails(Package &pkg) override;
};

} // namespace pkg