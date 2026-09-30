#include "../test.h"

#define MN_TEST_CONFIG_VALID "tests/configs/valid.lua"
#define MN_TEST_CONFIG_INVALID "tests/configs/invalid.lua"

static char const valid_lua[] = {
#embed MN_TEST_CONFIG_VALID
    , '\0'};

static char const invalid_lua[] = {
#embed MN_TEST_CONFIG_INVALID
    , '\0'};

MN_TEST_CASE(mn_config_load_defaults, {
  struct mn_config config = {};
  mn_assert_success(mn_config_load(&config));

  // Default values reported in documentation.
  mn_assert_eq(config.coro_pages, 4);
  mn_assert_eq(config.log_level, MN_LOG_LEVEL_DEBUG);
  mn_assert_eq(config.port, 1314);
  mn_assert_ne(config.lua_, nullptr);
})

MN_TEST_CASE(mn_config_load_file_valid, {
  struct mn_config config = {};
  mn_assert_success(mn_config_load_file(mn_str_lit(MN_TEST_CONFIG_VALID), &config));

  // Overridden values
  mn_assert_eq(config.coro_pages, 8);
  mn_assert_eq(config.log_level, MN_LOG_LEVEL_INFO);

  // Default values reported in documentation.
  mn_assert_eq(config.port, 1314);
  mn_assert_ne(config.lua_, nullptr);
})

MN_TEST_CASE(mn_config_load_file_invalid, {
  struct mn_config config = {};
  mn_assert_status(
      mn_config_load_file(mn_str_lit(MN_TEST_CONFIG_INVALID), &config),
      MN_ERROR_CONFIG);
  mn_assert_eq(config.lua_, nullptr); // Not set.
})

MN_TEST_CASE(mn_config_load_chunk_valid, {
  struct mn_config config = {};
  mn_assert_success(mn_config_load_chunk(valid_lua, &config));

  // Overridden values
  mn_assert_eq(config.coro_pages, 8);
  mn_assert_eq(config.log_level, MN_LOG_LEVEL_INFO);

  // Default values reported in documentation.
  mn_assert_eq(config.port, 1314);
  mn_assert_ne(config.lua_, nullptr);
})

MN_TEST_CASE(mn_config_load_chunk_invalid, {
  struct mn_config config = {};
  mn_assert_status(mn_config_load_chunk(invalid_lua, &config), MN_ERROR_CONFIG);
  mn_assert_eq(config.lua_, nullptr); // Not set.
})
