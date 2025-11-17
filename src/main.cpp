#include <ncurses.h>
#include <string>
#include <vector>

#include "DummyPackageManager.h"
#include "PackageManager.h"

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

void render_packages(WINDOW *win, const std::vector<pkg::Package> &packages,
                     int selected_index, int scroll_offset) {
  int height, width;
  getmaxyx(win, height, width);

  int list_height = height - 2;
  int start_row = 1;

  werase(win);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Packages ");
  wattroff(win, A_BOLD);

  int max_index = static_cast<int>(packages.size());

  for (int i = 0; i < list_height; ++i) {
    int pkg_index = scroll_offset + i;
    if (pkg_index >= max_index) {
      break;
    }

    int row = start_row + i;
    const auto &pkg = packages[pkg_index];

    std::string line = pkg.name + " " + pkg.version;

    if (static_cast<int>(line.size()) > width - 2) {
      line.resize(width - 5);
      line += "...";
    }

    if (pkg_index == selected_index) {
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
                    int selected_index) {
  int height, width;
  getmaxyx(win, height, width);

  werase(win);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Details ");
  wattroff(win, A_BOLD);

  mvwprintw(win, 2, 2, "Selected package:");

  if (!packages.empty() && selected_index >= 0 &&
      selected_index < static_cast<int>(packages.size())) {

    const auto &pkg = packages[selected_index];

    mvwprintw(win, 4, 4, "Name: %s", pkg.name.c_str());
    mvwprintw(win, 5, 4, "Version: %s", pkg.version.c_str());

    int desc_row = 7;
    mvwprintw(win, desc_row, 4, "Description:");

    if (!pkg.description.empty()) {
      mvwprintw(win, desc_row + 1, 6, "%.*s", width - 8,
                pkg.description.c_str());
    } else {
      mvwprintw(win, desc_row + 1, 6, "(none)");
    }

  } else {
    mvwprintw(win, 4, 4, "(none)");
  }

  mvwprintw(win, height - 2, 2, "Use Up/Down, q to quit");

  wrefresh(win);
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

  pkg::DummyPackageManager manager;
  std::vector<pkg::Package> packages = manager.listInstalled();

  int selected_index = 0;
  int scroll_offset = 0;

  int list_height;
  {
    int h, w;
    getmaxyx(packages_win, h, w);
    list_height = h - 2;
  }

  render_packages(packages_win, packages, selected_index, scroll_offset);
  render_details(details_win, packages, selected_index);

  int ch;
  while ((ch = getch()) != 'q') {
    bool changed = false;

    switch (ch) {
    case KEY_UP:
      if (selected_index > 0) {
        selected_index--;
        changed = true;
      }
      break;
    case KEY_DOWN:
      if (selected_index + 1 < static_cast<int>(packages.size())) {
        selected_index++;
        changed = true;
      }
      break;
    default:
      break;
    }

    if (changed) {
      if (selected_index < scroll_offset) {
        scroll_offset = selected_index;
      } else if (selected_index >= scroll_offset + list_height) {
        scroll_offset = selected_index - list_height + 1;
      }

      render_packages(packages_win, packages, selected_index, scroll_offset);
      render_details(details_win, packages, selected_index);
    }
  }

  delwin(packages_win);
  delwin(details_win);
  endwin();
  return 0;
}
