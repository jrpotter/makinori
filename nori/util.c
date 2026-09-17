#include "nori/util.h"

// The log level is usually specified at runtime. This specifies what the log
// level should be prior to discovering the runtime value.
static constexpr unsigned long INIT_LOG_LEVEL =
    LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_ERR;

struct nori_status nori_init(void)
{
#ifdef NDEBUG
  nori_set_log_level(INIT_LOG_LEVEL);
#else
  nori_set_log_level(INIT_LOG_LEVEL | LLL_DEBUG);
#endif

  return NORI_SUCCESS;
}
