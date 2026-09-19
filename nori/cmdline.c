#include <assert.h>

#include "nori/cmdline.h"
#include "nori/util.h"

static struct nori_flag *nori_flag_search(const struct nori_str_view key)
{
  for (size_t i = 0; NORI_FLAG_OPTIONS[i]; ++i) {
    struct nori_flag *f = NORI_FLAG_OPTIONS[i];
    if (nori_str_view_eq(key, f->nf_lflag) || nori_str_view_eq(key, f->nf_sflag)) {
      return f;
    }
  }
  return nullptr;
}

struct nori_status nori_cmdline_parse(
    const int argc,
    const char *argv[const argc],
    struct nori_str_view out[static 1])
{
  memset(out, 0, sizeof(*out));

  for (int i = 1; i < argc; ++i) {
    struct nori_str_view arg = nori_str_view_of(argv[i]);

    if (argv[i][0] == '-') {
      arg = nori_str_view_suffix(arg, argv[i][1] == '-' ? 2 : 1);

      struct nori_flag *const flag = nori_flag_search(arg);
      if (flag == nullptr) {
        nori_log_error("Unknown flag %s", argv[i]);
        return NORI_FAILURE;
      } else if (i + flag->nf_arity >= argc) {
        nori_log_error("Missing values for %s", argv[i]);
        return NORI_FAILURE;
      }

      flag->nf_set = true;
      for (size_t j = 1; j <= flag->nf_arity; ++i, ++j) {
        flag->nf_vals[j] = nori_str_view_of(argv[i + j]);
      }
    } else if (out->len == 0) {
      *out = arg;
    } else {
      nori_log_error("Cannot specify > 1 action");
      return NORI_FAILURE;
    }
  }

  return NORI_SUCCESS;
}
