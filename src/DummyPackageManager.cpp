#include "DummyPackageManager.h"

namespace pkg {

std::vector<Package> DummyPackageManager::listInstalled() {
  std::vector<Package> result;
  result.reserve(50);

  for (int i = 1; i <= 50; ++i) {
    Package p;
    p.name = "package-" + std::to_string(i);
    p.version = "1.0." + std::to_string(i % 10);
    p.description = "Dummy package number " + std::to_string(i);
    p.repo = "dummy";
    p.architecture = "x86_64";
    p.install_date = "N/A";

    if (i > 1) {
      p.depends_on.push_back("package-" + std::to_string(i - 1));
    }
    if (i < 50) {
      p.required_by.push_back("package-" + std::to_string(i + 1));
    }

    result.push_back(std::move(p));
  }

  return result;
}

bool DummyPackageManager::fillDetails(Package & /*pkg*/) { return true; }

} // namespace pkg
