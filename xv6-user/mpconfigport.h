// Include common MicroPython embed configuration.
#include <port/mpconfigport_common.h>

// Use the minimal starting configuration (disables all optional features).
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMAL)
//#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)

// MicroPython configuration.
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)