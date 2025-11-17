#include <algorithm>
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

enum class FilterMode { All, ExplicitOnly, AurOnly };

enum class SortMode { NameAsc, NameDesc, ExplicitFirst, AurFirst };

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

bool filter_accept(const pkg::Package &pkg, FilterMode mode) {
  if (mode == FilterMode::All) {
    return true;
  } else if (mode == FilterMode::ExplicitOnly) {
    return pkg.is_explicit;
  } else if (mode == FilterMode::AurOnly) {
    return pkg.is_foreign;
  }
  return true;
}

bool sort_less(const pkg::Package &a, const pkg::Package &b, SortMode mode) {
  if (mode == SortMode::NameAsc || mode == SortMode::NameDesc) {
    std::string la = to_lower(a.name);
    std::string lb = to_lower(b.name);
    if (mode == SortMode::NameAsc) {
      return la < lb;
    } else {
      return la > lb;
    }
  } else if (mode == SortMode::ExplicitFirst) {
    bool ea = a.is_explicit;
    bool eb = b.is_explicit;
    if (ea != eb) {
      return ea && !eb;
    }
    std::string la = to_lower(a.name);
    std::string lb = to_lower(b.name);
    return la < lb;
  } else if (mode == SortMode::AurFirst) {
    bool fa = a.is_foreign;
    bool fb = b.is_foreign;
    if (fa != fb) {
      return fa && !fb;
    }
    std::string la = to_lower(a.name);
    std::string lb = to_lower(b.name);
    return la < lb;
  }
  return false;
}

void recompute_visible_indices(const std::vector<pkg::Package> &packages,
                               const std::string &query, FilterMode filter_mode,
                               SortMode sort_mode,
                               std::vector<int> &visible_indices) {
  visible_indices.clear();

  for (int i = 0; i < static_cast<int>(packages.size()); ++i) {
    const auto &pkg = packages[i];
    if (!filter_accept(pkg, filter_mode)) {
      continue;
    }
    if (!query.empty() && !fuzzy_match(query, pkg.name)) {
      continue;
    }
    visible_indices.push_back(i);
  }

  std::sort(visible_indices.begin(), visible_indices.end(),
            [&](int ia, int ib) {
              const auto &a = packages[ia];
              const auto &b = packages[ib];
              return sort_less(a, b, sort_mode);
            });
}

std::string filter_mode_label(FilterMode mode) {
  switch (mode) {
  case FilterMode::All:
    return "All";
  case FilterMode::ExplicitOnly:
    return "Explicit";
  case FilterMode::AurOnly:
    return "AUR";
  }
  return "";
}

std::string sort_mode_label(SortMode mode) {
  switch (mode) {
  case SortMode::NameAsc:
    return "Name↑";
  case SortMode::NameDesc:
    return "Name↓";
  case SortMode::ExplicitFirst:
    return "Explicit↑";
  case SortMode::AurFirst:
    return "AUR↑";
  }
  return "";
}

void render_packages(WINDOW *win, const std::vector<pkg::Package> &packages,
                     const std::vector<int> &visible_indices,
                     int selected_visible_index, int scroll_offset,
                     const std::string &search_query, bool search_mode,
                     FilterMode filter_mode, SortMode sort_mode) {
  int height, width;
  getmaxyx(win, height, width);

  int search_row = 1;
  int list_start_row = 2;
  int list_height = height - 3;

  werase(win);
  box(win, 0, 0);

  std::string mode_label = filter_mode_label(filter_mode);
  std::string sort_label = sort_mode_label(sort_mode);
  std::string title = " Packages [" + mode_label + "] (" + sort_label + ") ";
  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, "%s", title.c_str());
  wattroff(win, A_BOLD);

  std::string prompt = "/ ";
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

  if (max_visible == 0) {
    std::string msg = "No packages found";
    if (static_cast<int>(msg.size()) > width - 2) {
      msg.resize(width - 2);
    }
    int row = list_start_row;
    mvwprintw(win, row, 1, "%-*s", width - 2, msg.c_str());
    wrefresh(win);
    return;
  }

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

    if (pkg.is_foreign) {
      line += " [AUR]";
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

    int row = 6;

    if (!pkg.repo.empty()) {
      mvwprintw(win, row, 4, "Repository: %s", pkg.repo.c_str());
      row++;
    }

    const char *source_str = pkg.is_foreign ? "AUR (foreign)" : "Official";
    mvwprintw(win, row, 4, "Source: %s", source_str);
    row++;

    mvwprintw(win, row, 4, "Explicit: %s", pkg.is_explicit ? "yes" : "no");
    row++;

    if (!pkg.architecture.empty()) {
      mvwprintw(win, row, 4, "Arch: %s", pkg.architecture.c_str());
      row++;
    }

    if (!pkg.install_date.empty()) {
      mvwprintw(win, row, 4, "Installed: %s", pkg.install_date.c_str());
      row++;
    }

    row += 1;
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

  mvwprintw(
      win, height - 2, 2,
      "Up/Down, / search, f filter, o order, h/? help, ESC clear, q quit");

  wrefresh(win);
}

void render_help_overlay(int max_y, int max_x) {
  int h = 15;
  int w = 46;
  if (h > max_y - 2)
    h = max_y - 2;
  if (w > max_x - 2)
    w = max_x - 2;

  int starty = (max_y - h) / 2;
  int startx = (max_x - w) / 2;

  WINDOW *win = newwin(h, w, starty, startx);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Help ");
  wattroff(win, A_BOLD);

  int row = 2;
  mvwprintw(win, row++, 2, "Up/Down : Move selection");
  mvwprintw(win, row++, 2, "/       : Search packages");
  mvwprintw(win, row++, 2, "ESC     : Clear search");
  mvwprintw(win, row++, 2, "Enter   : Exit search mode");
  mvwprintw(win, row++, 2, "f       : Filter (All/Explicit/AUR)");
  mvwprintw(win, row++, 2, "o       : Order (Name/Explicit/AUR)");
  mvwprintw(win, row++, 2, "h or ?  : Toggle this help");
  mvwprintw(win, row++, 2, "q       : Quit");

  mvwprintw(win, h - 2, 2, "Press h or ? to close");

  wrefresh(win);
  delwin(win);
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
  refresh();

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
  bool show_help = false;
  FilterMode filter_mode = FilterMode::All;
  SortMode sort_mode = SortMode::NameAsc;

  std::vector<int> visible_indices;
  recompute_visible_indices(packages, search_query, filter_mode, sort_mode,
                            visible_indices);

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
                  search_mode, filter_mode, sort_mode);
  render_details(details_win, packages, current_global_index);

  int ch;
  while ((ch = getch()) != 'q') {
    bool need_rerender = false;

    if (show_help) {
      if (ch == 'h' || ch == '?') {
        show_help = false;
        need_rerender = true;
      } else if (ch == 'q') {
        break;
      }
    } else if (search_mode) {
      if (ch == 27) {
        search_mode = false;
        search_query.clear();
        recompute_visible_indices(packages, search_query, filter_mode,
                                  sort_mode, visible_indices);
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
          recompute_visible_indices(packages, search_query, filter_mode,
                                    sort_mode, visible_indices);
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
        recompute_visible_indices(packages, search_query, filter_mode,
                                  sort_mode, visible_indices);
        selected_visible_index = 0;
        scroll_offset = 0;
        if (!visible_indices.empty()) {
          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        } else {
          current_global_index = -1;
        }
        need_rerender = true;
      }
    } else {
      if (ch == '/') {
        search_mode = true;
        need_rerender = true;
      } else if (ch == 'h' || ch == '?') {
        show_help = true;
        need_rerender = true;
      } else if (ch == 'f') {
        if (filter_mode == FilterMode::All) {
          filter_mode = FilterMode::ExplicitOnly;
        } else if (filter_mode == FilterMode::ExplicitOnly) {
          filter_mode = FilterMode::AurOnly;
        } else {
          filter_mode = FilterMode::All;
        }
        recompute_visible_indices(packages, search_query, filter_mode,
                                  sort_mode, visible_indices);
        selected_visible_index = 0;
        scroll_offset = 0;
        if (!visible_indices.empty()) {
          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        } else {
          current_global_index = -1;
        }
        need_rerender = true;
      } else if (ch == 'o') {
        if (sort_mode == SortMode::NameAsc) {
          sort_mode = SortMode::NameDesc;
        } else if (sort_mode == SortMode::NameDesc) {
          sort_mode = SortMode::ExplicitFirst;
        } else if (sort_mode == SortMode::ExplicitFirst) {
          sort_mode = SortMode::AurFirst;
        } else {
          sort_mode = SortMode::NameAsc;
        }
        recompute_visible_indices(packages, search_query, filter_mode,
                                  sort_mode, visible_indices);
        selected_visible_index = 0;
        scroll_offset = 0;
        if (!visible_indices.empty()) {
          current_global_index = visible_indices[selected_visible_index];
          manager->fillDetails(packages[current_global_index]);
        } else {
          current_global_index = -1;
        }
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
      getmaxyx(stdscr, max_y, max_x);
      clear();
      refresh();
      if (show_help) {
        render_help_overlay(max_y, max_x);
      } else {
        render_packages(packages_win, packages, visible_indices,
                        selected_visible_index, scroll_offset, search_query,
                        search_mode, filter_mode, sort_mode);
        render_details(details_win, packages, current_global_index);
      }
    }
  }

  delwin(packages_win);
  delwin(details_win);
  endwin();
  return 0;
}
