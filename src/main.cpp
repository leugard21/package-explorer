#include <cctype>
#include <cstdlib>
#include <memory>
#include <ncurses.h>
#include <string>
#include <vector>

#include "DummyPackageManager.h"
#include "PackageManager.h"
#include "PacmanPackageManager.h"

namespace {

WINDOW *create_window(int h, int w, int y, int x, const std::string &title) {
  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " %s ", title.c_str());
  wattroff(win, A_BOLD);

  wrefresh(win);
  return win;
}

std::string to_lower(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char ch : s) {
    out.push_back(static_cast<char>(std::tolower(ch)));
  }
  return out;
}

bool fuzzy_match(const std::string &pattern, const std::string &text) {
  if (pattern.empty())
    return true;

  std::string p = to_lower(pattern);
  std::string t = to_lower(text);

  std::size_t ti = 0;
  for (char pc : p) {
    bool found = false;
    while (ti < t.size()) {
      if (t[ti] == pc) {
        found = true;
        ++ti;
        break;
      }
      ++ti;
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

void recompute_visible_indices(const std::vector<pkg::Package> &packages,
                               const std::string &query,
                               std::vector<int> &visible_indices) {
  visible_indices.clear();

  if (query.empty()) {
    visible_indices.reserve(packages.size());
    for (int i = 0; i < static_cast<int>(packages.size()); ++i) {
      visible_indices.push_back(i);
    }
    return;
  }

  for (int i = 0; i < static_cast<int>(packages.size()); ++i) {
    const auto &pkg = packages[i];
    if (fuzzy_match(query, pkg.name)) {
      visible_indices.push_back(i);
    }
  }
}

void render_packages(WINDOW *win, const std::vector<pkg::Package> &packages,
                     const std::vector<int> &visible_indices,
                     int selected_visible_index, int scroll_offset,
                     const std::string &search_query, bool search_mode) {
  int height, width;
  getmaxyx(win, height, width);

  int search_row = 1;
  int list_start_row = 2;
  int list_height = height - 3;

  werase(win);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Packages ");
  wattroff(win, A_BOLD);

  std::string prompt = search_mode ? "/ " : "/ ";
  std::string query_display = search_query;
  int max_query_w = width - 4 - static_cast<int>(prompt.size());
  if (static_cast<int>(query_display.size()) > max_query_w) {
    query_display.erase(max_query_w);
  }

  mvwprintw(win, search_row, 1, "%-*s", width - 2, "");
  mvwprintw(win, search_row, 2, "%s%s", prompt.c_str(), query_display.c_str());
  if (search_mode) {
    wmove(win, search_row,
          2 + static_cast<int>(prompt.size()) +
              static_cast<int>(query_display.size()));
    curs_set(1);
  } else {
    curs_set(0);
  }

  int max_visible = static_cast<int>(visible_indices.size());

  for (int i = 0; i < list_height; ++i) {
    int vis_index = scroll_offset + i;
    if (vis_index >= max_visible) {
      break;
    }

    int global_index = visible_indices[vis_index];
    const auto &pkg = packages[global_index];

    int row = list_start_row + i;

    std::string line = pkg.name;
    if (!pkg.version.empty()) {
      line += " ";
      line += pkg.version;
    }

    if (static_cast<int>(line.size()) > width - 2) {
      line.resize(width - 5);
      line += "...";
    }

    if (vis_index == selected_visible_index) {
      wattron(win, A_REVERSE);
      mvwprintw(win, row, 1, "%-*s", width - 2, line.c_str());
      wattroff(win, A_REVERSE);
    } else {
      mvwprintw(win, row, 1, "%-*s", width - 2, line.c_str());
    }
  }

  wrefresh(win);
}

void render_details(WINDOW *win, const std::vector<pkg::Package> &packages,
                    int global_index) {
  int height, width;
  getmaxyx(win, height, width);

  werase(win);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Details ");
  wattroff(win, A_BOLD);

  mvwprintw(win, 2, 2, "Selected package:");

  if (!packages.empty() && global_index >= 0 &&
      global_index < static_cast<int>(packages.size())) {

    const auto &pkg = packages[global_index];

    mvwprintw(win, 4, 4, "Name: %s", pkg.name.c_str());
    mvwprintw(win, 5, 4, "Version: %s", pkg.version.c_str());

    if (!pkg.repo.empty()) {
      mvwprintw(win, 6, 4, "Repository: %s", pkg.repo.c_str());
    }

    if (!pkg.architecture.empty()) {
      mvwprintw(win, 7, 4, "Arch: %s", pkg.architecture.c_str());
    }

    if (!pkg.install_date.empty()) {
      mvwprintw(win, 8, 4, "Installed: %s", pkg.install_date.c_str());
    }

    int row = 10;
    mvwprintw(win, row, 4, "Depends On:");
    row++;

    if (!pkg.depends_on.empty()) {
      std::string deps_line;
      for (std::size_t i = 0; i < pkg.depends_on.size(); ++i) {
        if (i > 0)
          deps_line += ", ";
        deps_line += pkg.depends_on[i];
      }
      if (static_cast<int>(deps_line.size()) > width - 8) {
        deps_line.resize(width - 11);
        deps_line += "...";
      }
      mvwprintw(win, row, 6, "%.*s", width - 8, deps_line.c_str());
    } else {
      mvwprintw(win, row, 6, "(none)");
    }

    row += 2;
    mvwprintw(win, row, 4, "Required By:");
    row++;

    if (!pkg.required_by.empty()) {
      std::string req_line;
      for (std::size_t i = 0; i < pkg.required_by.size(); ++i) {
        if (i > 0)
          req_line += ", ";
        req_line += pkg.required_by[i];
      }
      if (static_cast<int>(req_line.size()) > width - 8) {
        req_line.resize(width - 11);
        req_line += "...";
      }
      mvwprintw(win, row, 6, "%.*s", width - 8, req_line.c_str());
    } else {
      mvwprintw(win, row, 6, "(none)");
    }

    row += 2;
    mvwprintw(win, row, 4, "Description:");
    row++;

    if (!pkg.description.empty()) {
      mvwprintw(win, row, 6, "%.*s", width - 8, pkg.description.c_str());
    } else {
      mvwprintw(win, row, 6, "(none)");
    }

  } else {
    mvwprintw(win, 4, 4, "(none)");
  }

  mvwprintw(win, height - 2, 2, "Up/Down, / search, ESC clear, q quit");

  wrefresh(win);
}

bool has_pacman() {
  int rc = std::system("pacman -V > /dev/null 2>&1");
  return rc == 0;
}

} // namespace

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  clear();

  int left_w = static_cast<int>(max_x * 0.30);
  int right_w = max_x - left_w;
  int height = max_y;

  WINDOW *packages_win = create_window(height, left_w, 0, 0, "Packages");
  WINDOW *details_win = create_window(height, right_w, 0, left_w, "Details");

  std::unique_ptr<pkg::PackageManager> manager;
  if (has_pacman()) {
    manager = std::make_unique<pkg::PacmanPackageManager>();
  } else {
    manager = std::make_unique<pkg::DummyPackageManager>();
  }

  std::vector<pkg::Package> packages = manager->listInstalled();

  std::string search_query;
  bool search_mode = false;

  std::vector<int> visible_indices;
  recompute_visible_indices(packages, search_query, visible_indices);

  int selected_visible_index = 0;
  int scroll_offset = 0;

  int list_height;
  {
    int h, w;
    getmaxyx(packages_win, h, w);
    list_height = h - 3;
  }

  int current_global_index = -1;
  if (!visible_indices.empty()) {
    current_global_index = visible_indices[selected_visible_index];
    manager->fillDetails(packages[current_global_index]);
  }

  render_packages(packages_win, packages, visible_indices,
                  selected_visible_index, scroll_offset, search_query,
                  search_mode);
  render_details(details_win, packages, current_global_index);

  int ch;
  while ((ch = getch()) != 'q') {
    bool need_rerender = false;

    if (search_mode) {
      if (ch == 27) {
        search_mode = false;
        search_query.clear();
        recompute_visible_indices(packages, search_query, visible_indices);
        selected_visible_index = 0;
        scroll_offset = 0;
        if (!visible_indices.empty()) {
          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        } else {
          current_global_index = -1;
        }
        need_rerender = true;
      } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!search_query.empty()) {
          search_query.pop_back();
          recompute_visible_indices(packages, search_query, visible_indices);
          selected_visible_index = 0;
          scroll_offset = 0;
          if (!visible_indices.empty()) {
            current_global_index = visible_indices[selected_visible_index];
            manager->fillDetails(packages[current_global_index]);
          } else {
            current_global_index = -1;
          }
          need_rerender = true;
        } else {
          need_rerender = true;
        }
      } else if (ch == '\n' || ch == KEY_ENTER) {
        search_mode = false;
        need_rerender = true;
      } else if (ch == KEY_UP || ch == KEY_DOWN) {
        if (!visible_indices.empty()) {
          if (ch == KEY_UP && selected_visible_index > 0) {
            selected_visible_index--;
          } else if (ch == KEY_DOWN &&
                     selected_visible_index + 1 <
                         static_cast<int>(visible_indices.size())) {
            selected_visible_index++;
          }

          if (selected_visible_index < scroll_offset) {
            scroll_offset = selected_visible_index;
          } else if (selected_visible_index >= scroll_offset + list_height) {
            scroll_offset = selected_visible_index - list_height + 1;
          }

          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        }
        need_rerender = true;
      } else if (ch >= 32 && ch <= 126) {
        search_query.push_back(static_cast<char>(ch));
        recompute_visible_indices(packages, search_query, visible_indices);
        selected_visible_index = 0;
        scroll_offset = 0;
        if (!visible_indices.empty()) {
          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        } else {
          current_global_index = -1;
        }
        need_rerender = true;
      } else {
      }
    } else {
      if (ch == '/') {
        search_mode = true;
        need_rerender = true;
      } else if (ch == KEY_UP || ch == KEY_DOWN) {
        if (!visible_indices.empty()) {
          if (ch == KEY_UP && selected_visible_index > 0) {
            selected_visible_index--;
          } else if (ch == KEY_DOWN &&
                     selected_visible_index + 1 <
                         static_cast<int>(visible_indices.size())) {
            selected_visible_index++;
          }

          if (selected_visible_index < scroll_offset) {
            scroll_offset = selected_visible_index;
          } else if (selected_visible_index >= scroll_offset + list_height) {
            scroll_offset = selected_visible_index - list_height + 1;
          }

          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        }
        need_rerender = true;
      }
    }

    if (need_rerender) {
      render_packages(packages_win, packages, visible_indices,
                      selected_visible_index, scroll_offset, search_query,
                      search_mode);
      render_details(details_win, packages, current_global_index);
    }
  }

  delwin(packages_win);
  delwin(details_win);
  endwin();
  return 0;
}
