#include <stdlib.h>

#include "test.h"
#include "test_makinori/test_cmdline.c"
#include "test_makinori/test_string.c"

#define MN_ELLIPSIS 88
#define MN_ELLIPSIS_S MN_STR_TO(MN_ELLIPSIS)
#define MN_GREEN "\e[0;32m"
#define MN_RED "\e[0;31m"
#define MN_RESET "\e[0m"

static unsigned long MN_TEST_PASS = 0;
static unsigned long MN_TEST_FAIL = 0;

struct mn_test_case MN_TEST_SUITE[8192] = {};

static inline ssize_t mn_last_sep(const char *filename)
{
  ssize_t pos = -1;
  for (size_t i = 0; filename[i]; ++i) {
    if (filename[i] == '/') {
      pos = i;
    }
  }
  return pos;
}

int main(void)
{
  mn_log_set_level(MN_LOG_LEVEL_OFF);

  for (size_t i = 0; MN_TEST_SUITE[i].func; ++i) {
    auto tc = &MN_TEST_SUITE[i];
    tc->func(tc);

    char ellipsis[MN_ELLIPSIS] = {};
    memset(ellipsis, '.', MN_ELLIPSIS);

    char buffer[MN_MESSAGE_SIZE];
    snprintf(
        buffer, MN_ARR_SIZE(buffer), "[%s:%s:%s]%s",
        tc->file + mn_last_sep(tc->file) + 1, tc->line, tc->name, ellipsis);

    if (tc->error[0]) {
      fprintf(
          stderr, MN_RED "%" MN_ELLIPSIS_S "." MN_ELLIPSIS_S "sFAIL\n" MN_RESET,
          buffer);
      fprintf(stderr, "\t%s\n", tc->error);
      MN_TEST_FAIL += 1;
    } else {
      fprintf(
          stdout, MN_GREEN "%" MN_ELLIPSIS_S "." MN_ELLIPSIS_S "sPASS\n" MN_RESET,
          buffer);
      MN_TEST_PASS += 1;
    }
  }

  fprintf(stdout, "=========\nPASSED: %lu\nFAILED: %lu\n", MN_TEST_PASS, MN_TEST_FAIL);

  return MN_TEST_FAIL > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
