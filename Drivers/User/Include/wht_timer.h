#include <stdio.h>

#include "gd32f4xx.h"
#include "systick.h"

extern void (*timer1_callback)(void);

void timer1_init(void (*callback)(void));