#include "DummyPackageManager.h"

#include <utility>

namespace pkg {

std::vector<Package> DummyPackageManager::listInstalled() {
  std::vector<Package> result;
  result.reserve(50);

  for (int i = 1; i <= 50; ++i) {
    Package p;
    p.name = "package-" + std::to_string(i);
    p.version = "1.0." + std::to_string(i % 10);
    p.description = "Dummy package number " + std::to_string(i);

    result.push_back(std::move(p));
  }

  return result;
}

} // namespace pkg
