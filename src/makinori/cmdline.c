#include <string.h>

#ifndef MN_CMDLINE_SOURCE
#define MN_CMDLINE_SOURCE
#endif

#include "makinori/cmdline.h"
#include "makinori/util.h"

static struct mn_flag *mn_flag_search(struct mn_view const key)
{
  for (size_t i = 0; MN_FLAG_OPTIONS[i]; ++i) {
    struct mn_flag *f = MN_FLAG_OPTIONS[i];
    if (mn_view_eq(key, mn_str_to_view(f->lflag)) ||
        mn_view_eq(key, mn_str_to_view(f->sflag))) {
      return f;
    }
  }
  return nullptr;
}

struct mn_status mn_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct mn_str action[static 1])
{
  memset(action, 0, sizeof(*action));

  for (int i = 1; i < argc; ++i) {
    struct mn_str arg = mn_str_ref(argv[i], 0);

    if (argv[i][0] == '-') {
      struct mn_view subarg = mn_str_substr(arg, argv[i][1] == '-' ? 2 : 1, SIZE_MAX);

      struct mn_flag *const flag = mn_flag_search(subarg);
      if (flag == nullptr) {
        return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Unknown flag %s", argv[i]);
      } else if (i + flag->arity >= argc) {
        return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Missing values for %s", argv[i]);
      }

      flag->set = true;
      for (size_t j = 0; j < flag->arity; ++i, ++j) {
        flag->vals[j] = mn_str_ref(argv[i + j + 1], 0);
      }
    } else if (action->len == 0) {
      *action = arg;
    } else {
      return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Cannot specify > 1 action");
    }
  }

  return MN_SUCCESS;
}
