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

    if (i % 5 == 0) {
      p.is_foreign = true;
    }

    if (i % 2 == 1) {
      p.is_explicit = true;
    } else {
      p.is_explicit = false;
    }

    result.push_back(std::move(p));
  }

  return result;
}

bool DummyPackageManager::fillDetails(Package &) { return true; }

} // namespace pkg
