#include <ncurses.h>
#include <string>
#include <vector>

namespace {

void draw_centered_text(int row, const std::string &text, bool bold = false) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int x = 0;
  if (static_cast<int>(text.size()) < max_x) {
    x = (max_x - static_cast<int>(text.size())) / 2;
  }

  if (bold)
    attron(A_BOLD);
  mvprintw(row, x, "%s", text.c_str());
  if (bold)
    attroff(A_BOLD);
}

WINDOW *create_window(int h, int w, int y, int x, const std::string &title) {
  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " %s ", title.c_str());
  wattroff(win, A_BOLD);

  wrefresh(win);
  return win;
}

void render_packages(WINDOW *win, const std::vector<std::string> &packages,
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
    const std::string &name = packages[pkg_index];

    if (pkg_index == selected_index) {
      wattron(win, A_REVERSE);
      mvwprintw(win, row, 1, "%-*s", width - 2, name.c_str());
      wattroff(win, A_REVERSE);
    } else {
      mvwprintw(win, row, 1, "%-*s", width - 2, name.c_str());
    }
  }

  wrefresh(win);
}

void render_details(WINDOW *win, const std::vector<std::string> &packages,
                    int selected_index) {
  int height, width;
  getmaxyx(win, height, width);

  werase(win);
  box(win, 0, 0);

  wattron(win, A_BOLD);
  mvwprintw(win, 0, 2, " Details ");
  wattroff(win, A_BOLD);

  int row = 2;
  mvwprintw(win, row, 2, "Selected package:");

  if (!packages.empty() && selected_index >= 0 &&
      selected_index < static_cast<int>(packages.size())) {
    mvwprintw(win, row + 1, 4, "%s", packages[selected_index].c_str());
  } else {
    mvwprintw(win, row + 1, 4, "(none)");
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

  std::vector<std::string> packages;
  for (int i = 1; i <= 50; ++i) {
    packages.push_back("package-" + std::to_string(i));
  }

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
