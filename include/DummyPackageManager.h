#pragma once

#include "PackageManager.h"
#include <vector>

namespace pkg {

class DummyPackageManager : public PackageManager {
public:
  std::vector<Package> listInstalled() override;
};

} // namespace pkg