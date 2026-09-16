#include <stdlib.h>

#include "nori/string.h"

// ================================================================
// Command Line
// ================================================================

enum action {
  ACTION_NONE,
  ACTION_HELP,
  ACTION_RUN,
};

struct user_args {
  enum action user_action;
  struct nori_str user_config; // -c, --config
};

struct flag {
  struct nori_str flag_short;
  struct nori_str flag_long;
  unsigned short flag_arity;
};

static constexpr struct flag FLAG_HELP = {
    .flag_short = NORI_STR("-h"),
    .flag_long = NORI_STR("--help"),
    .flag_arity = 0,
};

static constexpr struct flag FLAG_CONFIG = {
    .flag_short = NORI_STR("-c"),
    .flag_long = NORI_STR("--config"),
    .flag_arity = 1,
};

static const struct flag *FLAG_OPTIONS[] = {
    &FLAG_HELP,
    &FLAG_CONFIG,
};

static const struct flag *flag_match(struct nori_str key)
{
  for (size_t i = 0; i < sizeof(FLAG_OPTIONS) / sizeof(FLAG_OPTIONS[0]); ++i) {
    if (nori_str_eq(key, FLAG_OPTIONS[i]->flag_short) ||
        nori_str_eq(key, FLAG_OPTIONS[i]->flag_long)) {
      return FLAG_OPTIONS[i];
    }
  }
  return nullptr;
}

// ================================================================
// Main
// ================================================================

int main(int argc, const char *argv[argc])
{
  flag_match(NORI_STR(""));
  return EXIT_SUCCESS;
}
