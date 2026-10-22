#include "Display/P4MacText.h"

#include <cstdio>

namespace wattcycle::display {

void copyMacText(char (&slot)[kP4MacText], const char* text) {
  if (text == nullptr || text[0] == '\0') {
    slot[0] = '\0';
    return;
  }
  std::snprintf(slot, kP4MacText, "%s", text);
}

const char* macOrDash(const char (&slot)[kP4MacText]) {
  return slot[0] != '\0' ? slot : "--";
}

}  // namespace wattcycle::display
