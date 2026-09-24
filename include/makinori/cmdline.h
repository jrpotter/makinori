#pragma once

#include "makinori/util.h"

/// The arity associated with a command line flag.
enum mn_flag_arity {
  MN_FLAG_ARITY_ZERO = 0,
  MN_FLAG_ARITY_ONE = 1,
  MN_FLAG_ARITY_TWO = 2,
  MN_FLAG_ARITY_THREE = 3,
  MN_FLAG_ARITY_FOUR = 4,
  MN_FLAG_ARITY_FIVE = 5,
  MN_FLAG_ARITY_SIX = 6,
  MN_FLAG_ARITY_SEVEN = 7,
  MN_FLAG_ARITY_MAX,
};

/// The parsed representation of a command line flag.
///
/// Refer to `MN_FLAG_OPTIONS` for usage.
struct mn_flag {
  struct mn_str vals[MN_FLAG_ARITY_MAX];
  struct mn_str sflag;
  struct mn_str lflag;
  enum mn_flag_arity arity;
  bool set;
};

/// The user-defined command line flags.
///
/// `struct mn_flag` instances should be supplied to the `MN_FLAG_OPTIONS`
/// array for command line parsing. For example, support for a singular port flag
/// might look like the following:
///
/// ```c
/// static struct mn_flag FLAG_PORT = {
///   .nf_sflag = mn_view_lit("p"),
///   .nf_lflag = mn_view_lit("port"),
///   .nf_arity = MN_FLAG_ARITY_ONE,
/// };
///
/// struct mn_flag *MN_FLAG_OPTIONS[] = {&FLAG_PORT, nullptr};
/// ```
///
/// Afterwards a call to `mn_cmdline_parse()` is made. If -p or --port was
/// found in the command line (with the correct number of arguments as defined by
/// the arity), `FLAG_PORT.nf_set` will be `true` and `FLAG_PORT.nf_vals[0]` will
/// contain the singular argument.
extern struct mn_flag *MN_FLAG_OPTIONS[];

/// A command line parsing utility.
///
/// The command line allows at most one argument (called the "action") and any
/// number of flags as specified in `MN_FLAG_OPTIONS`. The following shows
/// examples of valid and invalid command line invocations:
///
/// ```shell
/// $ ./server                         # Valid. Zero args, zero flags.
/// $ ./server x                       # Valid. One arg, zero flags.
/// $ ./server x --port 8000           # Valid. One arg, one flag.
/// $ ./server -c conf.toml x -p 8000  # Valid. One arg, two flags.
/// $ ./server x y                     # Invalid. Two arguments, zero flags.
/// ```
///
/// @param argc - A reference to `argc` passed to `main()`.
/// @param argv - A reference to `argv` passed to `main()`.
/// @out_param out - A reference to the single positional argument.
/// @return - `struct mn_status` indicating successful parsing.
struct mn_status mn_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct mn_str out[static 1]);
