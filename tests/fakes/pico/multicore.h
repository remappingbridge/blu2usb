#include <stddef.h>
#include <stdint.h>
void multicore_launch_core1_with_stack(void (*)(void), uint32_t *, size_t);
