#ifndef G07_FAKE_CRITICAL_SECTION_H
#define G07_FAKE_CRITICAL_SECTION_H
#include <assert.h>
#include <stdbool.h>
typedef struct { bool initialized, locked; } critical_section_t;
static inline void critical_section_init(critical_section_t *p) {p->initialized=true;p->locked=false;}
static inline void critical_section_enter_blocking(critical_section_t *p) {assert(p->initialized && !p->locked);p->locked=true;}
static inline void critical_section_exit(critical_section_t *p) {assert(p->locked);p->locked=false;}
#endif
