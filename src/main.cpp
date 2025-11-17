#include <ncurses.h>
#include <string>

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

  int left_w = max_x * 0.30;
  int right_w = max_x - left_w;

  int height = max_y;

  WINDOW *packages_win = create_window(height, left_w, 0, 0, "Packages");
  WINDOW *details_win = create_window(height, right_w, 0, left_w, "Details");

  refresh();

  int ch;
  while ((ch = getch()) != 'q') {
  }

  delwin(packages_win);
  delwin(details_win);
  endwin();
  return 0;
}
