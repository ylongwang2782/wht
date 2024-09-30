/**
 * @file main.c
 * @author wang.yunlong (wang.yunlong9@byd.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-25
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>

#include "gd32f4xx.h"
#include "systick.h"

extern void master_mode_entry(void);
extern void slave_mode_entry(void);

int main(void) {
    
#ifdef MASTER_MODE
    master_mode_entry();
#endif

#ifdef SLAVE_MODE
    slave_mode_entry();
#endif
}

