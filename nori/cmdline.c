#include <assert.h>

#include "nori/cmdline.h"

static struct nori_flag *flag_match(struct nori_view key)
{
  for (size_t i = 0; NORI_FLAG_OPTIONS[i]; ++i) {
    struct nori_flag *f = NORI_FLAG_OPTIONS[i];
    for (size_t j = 0; f->nf_keys[j].len > 0; ++j) {
      if (nori_view_eq(key, f->nf_keys[j])) {
        return f;
      }
    }
  }
  return nullptr;
}

struct nori_status
nori_cmdline_parse(const int argc, const char *argv[argc], struct nori_view *out_action)
{
  memset(out_action, 0, sizeof(struct nori_view));

  // The binary assumes a single positional argument defines the action to take. This
  // could be "run" for running the server, "migrate" for running database migrations,
  // etc. Look for this action first since it may dictate what flags are permitted.
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      const struct nori_view key = nori_view_create(argv[i]);
      const struct nori_flag *flag = flag_match(key);
      if (flag) {
        i += flag->nf_arity;
      } else {
        return NORI_FAILURE_ERROR("Unknown flag %s", key.ss);
      }
    } else {
      struct nori_view action = nori_view_create(argv[i]);
      if (out_action->len == 0) {
        *out_action = action;
      } else {
        return NORI_FAILURE_ERROR("Cannot specify > 1 action");
      }
    }
  }

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {
      continue;
    }

    const struct nori_view key = nori_view_create(argv[i]);
    struct nori_flag *flag = flag_match(key);
    assert(flag); // checked in previous loop

    if (i + flag->nf_arity >= argc) {
      return NORI_FAILURE_ERROR("Missing values for '%s'\n", key.ss);
    }

    flag->nf_set = true;
    for (size_t i = 0; i < flag->nf_arity; ++i) {
      flag->nf_vals[i] = nori_view_create(argv[i + 1]);
    }
  }

  return NORI_SUCCESS;
}
