#pragma once

#include <optional>
#include <string>
#include <vector>

struct LuaConfig {
  std::optional<std::vector<std::string>> libs;
};