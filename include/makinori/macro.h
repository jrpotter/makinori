#pragma once

#define MN_ARR_SIZE(X) (sizeof(X) / sizeof(X[0]))

#define MN_MAX(X, Y)                                                                   \
  ({                                                                                   \
    auto e1 = (X);                                                                     \
    auto e2 = (Y);                                                                     \
    e1 > e2 ? e1 : e2;                                                                 \
  })

#define MN_MIN(X, Y)                                                                   \
  ({                                                                                   \
    auto e1 = (X);                                                                     \
    auto e2 = (Y);                                                                     \
    e1 < e2 ? e1 : e2;                                                                 \
  })

#define MN_PAIR(T1, T2)                                                                \
  struct {                                                                             \
    T1 fst;                                                                            \
    T2 snd;                                                                            \
  }

#define MN_STR_LEN(X) (MN_ARR_SIZE(X) - 1)

#define MN_STR_PROXY_(X) #X
#define MN_STR_TO(X) MN_STR_PROXY_(X)
