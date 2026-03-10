#pragma once

#include <concepts>
#include <cstdlib>
#include <string>

template <class T>
concept Executor = requires(T e, const std::string &cmd) {
  { e(cmd) } -> std::convertible_to<int>;
};

struct SystemExecutor {
  int operator()(const std::string &cmd) const { return system(cmd.c_str()); }
};
