#include "colors.h"
#include <stdio.h>
void set_color(char* color) {
  printf("%s", color);
}
void reset_color() {
  printf("%s\n", ANSI_COLOR_RESET);
}