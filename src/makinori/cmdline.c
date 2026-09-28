#include <string.h>

#include "makinori/cmdline.h"
#include "makinori/util.h"

static struct mn_flag *
mn_flag_search(struct mn_view const key, struct mn_cmdline cl[static 1])
{
  for (size_t i = 0; i < MN_CMDLINE_MAX_FLAGS && cl->flags[i]; ++i) {
    if (mn_view_eq(key, cl->flags[i]->lflag.view) ||
        mn_view_eq(key, cl->flags[i]->sflag.view)) {
      return cl->flags[i];
    }
  }
  return nullptr;
}

struct mn_status mn_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct mn_cmdline cl[static 1])
{
  for (int i = 1; i < argc; ++i) {
    struct mn_str arg = mn_str_ref(argv[i], 0);

    if (argv[i][0] == '-') {
      struct mn_view subarg =
          mn_view_substr(arg.view, argv[i][1] == '-' ? 2 : 1, SIZE_MAX);

      struct mn_flag *const flag = mn_flag_search(subarg, cl);
      if (flag == nullptr) {
        return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Unknown flag %s", argv[i]);
      }

      mn_assert(flag->arity <= MN_CMDLINE_MAX_ARITY);
      if (i + flag->arity >= argc) {
        return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Missing values for %s", argv[i]);
      }

      flag->set = true;
      for (size_t j = 0; j < flag->arity; ++i, ++j) {
        flag->vals[j] = mn_str_ref(argv[i + j + 1], 0);
      }
    } else if (cl->action.len == 0) {
      cl->action = arg;
    } else {
      return MN_ERROR_EMIT(MN_ERROR_CONFIG, "Cannot specify > 1 action");
    }
  }

  return MN_SUCCESS;
}
