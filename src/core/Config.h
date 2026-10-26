#pragma once

// Raw arrays need their size at compile time, so we fix the limits here.
constexpr int MAX_DISKS = 10;
constexpr int MAX_MOVES = (1 << MAX_DISKS) - 1;   // 2^MAX_DISKS - 1 (1023 for 10 disks)
