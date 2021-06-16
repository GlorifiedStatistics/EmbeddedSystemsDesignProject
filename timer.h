#include <sam.h>

#include "core_cm0plus.h"

#define CLOCK_SPEED 48000000

void timer3_init();
void timer3_reset();
void timer3_set(uint16_t period_ms);
