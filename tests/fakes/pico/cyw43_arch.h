#include <stddef.h>
int cyw43_arch_init(void);
void *cyw43_arch_async_context(void);
void async_context_acquire_lock_blocking(void *);
void async_context_release_lock(void *);
