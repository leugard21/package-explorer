#include "PacmanPackageManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace pkg {

namespace {

std::string trim(const std::string &s) {
  std::size_t start = 0;
  while (start < s.size() &&
         std::isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }

  std::size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }

  return s.substr(start, end - start);
}

bool starts_with(const std::string &s, const std::string &prefix) {
  return s.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), s.begin());
}

std::vector<std::string> split_dep_list(const std::string &raw) {
  std::vector<std::string> out;

  if (raw == "None") {
    return out;
  }

  std::stringstream ss(raw);
  std::string token;

  while (std::getline(ss, token, ' ')) {
    token = trim(token);
    if (token.empty())
      continue;
    if (token == ",")
      continue;

    if (!token.empty() && token.back() == ',') {
      token.pop_back();
    }

    std::size_t idx = 0;
    while (idx < token.size() &&
           (std::isalnum(static_cast<unsigned char>(token[idx])) ||
            token[idx] == '@' || token[idx] == '_' || token[idx] == '-' ||
            token[idx] == '.')) {
      ++idx;
    }
    std::string name = token.substr(0, idx);
    if (!name.empty()) {
      out.push_back(name);
    }
  }

  return out;
}

} // namespace

std::vector<Package> PacmanPackageManager::listInstalled() {
  std::vector<Package> packages;

  const char *cmd = "pacman -Q";

  FILE *fp = popen(cmd, "r");
  if (!fp) {
    return packages;
  }

  std::array<char, 4096> buffer{};

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), fp) != nullptr) {
    std::string line(buffer.data());

    if (!line.empty() && line.back() == '\n') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }

    std::size_t sep = line.find(' ');
    if (sep == std::string::npos) {
      continue;
    }

    std::string name = line.substr(0, sep);
    std::string version = line.substr(sep + 1);

    Package pkg;
    pkg.name = std::move(name);
    pkg.version = std::move(version);
    pkg.description.clear();
    pkg.repo.clear();
    pkg.architecture.clear();
    pkg.install_date.clear();
    pkg.depends_on.clear();
    pkg.required_by.clear();

    packages.push_back(std::move(pkg));
  }

  pclose(fp);
  return packages;
}

bool PacmanPackageManager::fillDetails(Package &pkg) {
  if (!pkg.description.empty() || !pkg.repo.empty()) {
    return true;
  }

  std::string cmd = "pacman -Qi -- " + pkg.name + " 2>/dev/null";

  FILE *fp = popen(cmd.c_str(), "r");
  if (!fp) {
    return false;
  }

  std::array<char, 4096> buffer{};
  bool ok = false;
  std::string depends_raw;
  std::string required_by_raw;

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), fp) != nullptr) {
    std::string line(buffer.data());

    if (!line.empty() && line.back() == '\n') {
      line.pop_back();
    }

    if (starts_with(line, "Description")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        pkg.description = trim(line.substr(colon + 1));
        ok = true;
      }
    } else if (starts_with(line, "Repository")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        pkg.repo = trim(line.substr(colon + 1));
        ok = true;
      }
    } else if (starts_with(line, "Architecture")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        pkg.architecture = trim(line.substr(colon + 1));
      }
    } else if (starts_with(line, "Install Date")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        pkg.install_date = trim(line.substr(colon + 1));
      }
    } else if (starts_with(line, "Depends On")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        depends_raw = trim(line.substr(colon + 1));
      }
    } else if (starts_with(line, "Required By")) {
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        required_by_raw = trim(line.substr(colon + 1));
      }
    }
  }

  pclose(fp);

  pkg.depends_on = split_dep_list(depends_raw);
  pkg.required_by = split_dep_list(required_by_raw);

  return ok;
}

} // namespace pkg
