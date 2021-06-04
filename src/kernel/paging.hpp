#pragma once

#include <cstdint>
#include <array>

const size_t kPageDirectoryCount = 64;

void SetupIdentityPageTable();
void InitializePaging();
