#pragma once

#include "nori/util.h"

/// The arity associated with a command line flag.
enum nori_flag_arity {
  NORI_FLAG_ARITY_ZERO = 0,
  NORI_FLAG_ARITY_ONE = 1,
  NORI_FLAG_ARITY_TWO = 2,
  NORI_FLAG_ARITY_THREE = 3,
  NORI_FLAG_ARITY_FOUR = 4,
  NORI_FLAG_ARITY_FIVE = 5,
  NORI_FLAG_ARITY_SIX = 6,
  NORI_FLAG_ARITY_SEVEN = 7,
  NORI_FLAG_ARITY_MAX,
};

/// The parsed representation of a command line flag.
///
/// Refer to `NORI_FLAG_OPTIONS` for usage.
struct nori_flag {
  struct nori_str_view nf_vals[NORI_FLAG_ARITY_MAX];
  struct nori_str_view nf_sflag;
  struct nori_str_view nf_lflag;
  enum nori_flag_arity nf_arity;
  bool nf_set;
};

/// The user-defined command line flags.
///
/// `struct nori_flag` instances should be supplied to the `NORI_FLAG_OPTIONS`
/// array for command line parsing. For example, support for a singular port flag
/// might look like the following:
///
/// ```c
/// static struct nori_flag FLAG_PORT = {
///   .nf_sflag = NORI_VIEW("p"),
///   .nf_lflag = NORI_VIEW("port"),
///   .nf_arity = NORI_FLAG_ARITY_ONE,
/// };
///
/// struct nori_flag *NORI_FLAG_OPTIONS[] = {&FLAG_PORT, nullptr};
/// ```
///
/// Afterwards a call to `nori_cmdline_parse()` is made. If -p or --port was
/// found in the command line (with the correct number of arguments as defined by
/// the arity), `FLAG_PORT.nf_set` will be `true` and `FLAG_PORT.nf_vals[0]` will
/// contain the singular argument.
extern struct nori_flag *NORI_FLAG_OPTIONS[];

/// A command line parsing utility.
///
/// The command line allows at most one argument (called the "action") and any
/// number of flags as specified in `NORI_FLAG_OPTIONS`. The following shows
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
/// @out_param out_action - A reference to the single positional argument.
/// @return - `struct nori_status` indicating successful parsing.
struct nori_status nori_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct nori_str_view out[static 1]);
