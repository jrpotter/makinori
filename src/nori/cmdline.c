#include <string.h>

#include "nori/cmdline.h"
#include "nori/util.h"

static struct nori_flag *nori_flag_search(struct nori_view const key)
{
  for (size_t i = 0; NORI_FLAG_OPTIONS[i]; ++i) {
    struct nori_flag *f = NORI_FLAG_OPTIONS[i];
    if (nori_view_eq(key, nori_str_to_view(f->nf_lflag)) ||
        nori_view_eq(key, nori_str_to_view(f->nf_sflag))) {
      return f;
    }
  }
  return nullptr;
}

struct nori_status nori_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct nori_str out[static 1])
{
  memset(out, 0, sizeof(*out));

  for (int i = 1; i < argc; ++i) {
    struct nori_str arg = nori_str_ref(argv[i], 0);

    if (argv[i][0] == '-') {
      struct nori_view subarg =
          nori_str_substr(arg, argv[i][1] == '-' ? 2 : 1, SIZE_MAX);

      struct nori_flag *const flag = nori_flag_search(subarg);
      if (flag == nullptr) {
        return NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Unknown flag %s", argv[i]);
      } else if (i + flag->nf_arity >= argc) {
        return NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Missing values for %s", argv[i]);
      }

      flag->nf_set = true;
      for (size_t j = 0; j < flag->nf_arity; ++i, ++j) {
        flag->nf_vals[j] = nori_str_ref(argv[i + j + 1], 0);
      }
    } else if (out->len == 0) {
      *out = arg;
    } else {
      return NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Cannot specify > 1 action");
    }
  }

  return NORI_SUCCESS;
}
