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

  if (bold) {
    attron(A_BOLD);
  }

  mvprintw(row, x, "%s", text.c_str());

  if (bold) {
    attroff(A_BOLD);
  }
}

} // namespace

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  clear();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  const std::string title = "Package Explorer";
  const std::string subtitle = "C++20 + ncurses";
  const std::string hint = "Press 'q' to quit";

  int center_row = max_y / 2;

  draw_centered_text(center_row - 1, title, true);
  draw_centered_text(center_row, subtitle, false);
  draw_centered_text(center_row + 2, hint, false);

  refresh();

  int ch;
  while ((ch = getch()) != 'q') {
  }

  endwin();
  return 0;
}
