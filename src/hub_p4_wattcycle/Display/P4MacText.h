#pragma once
#ifndef P4_MAC_TEXT_H
#define P4_MAC_TEXT_H

namespace wattcycle::display {

constexpr int kP4MacText = 18;

/// Copy a printable MAC into an 18-byte slot. Empty or null clears the slot.
void copyMacText(char (&slot)[kP4MacText], const char* text);

/// Non-empty slot, else "--".
const char* macOrDash(const char (&slot)[kP4MacText]);

}  // namespace wattcycle::display

#endif
