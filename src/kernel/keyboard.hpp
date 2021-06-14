#pragma once

#include <deque>

#include "message.hpp"

const int kKbdLControlBitMask = 1u << 0;
const int kKbdLShiftBitMask = 1u << 1;
const int kKbdLAltBitMask = 1u << 2;
const int kKbdLGUIBitMask = 1u << 3;
const int kKbdRControlBitMask = 1u << 4;
const int kKbdRShiftBitMask = 1u << 5;
const int kKbdRAltBitMask = 1u << 6;
const int kKbdRGUIBitMask = 1u << 7;

void InitializeKeyboard();
