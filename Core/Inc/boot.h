#ifndef BOOT_H
#define BOOT_H
#include "stm32f4xx_hal.h"

// Khai báo hàm jump_to_app()
void jump_to_app(uint32_t JumpAddress);

// Khai báo hàm jump_to_boot()
void jump_to_boot(uint32_t BootAddress);

#endif /* BOOT_H */
