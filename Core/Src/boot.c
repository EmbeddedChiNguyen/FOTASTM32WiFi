#include <boot.h>
#include <stdint.h>

//create by Nguyen

void jump_to_app(uint32_t AppAddress) {
	//SCB->VTOR = (uint32_t) AppAddress;
	HAL_RCC_DeInit();
	HAL_DeInit();
	SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk |\
 SCB_SHCSR_BUSFAULTENA_Msk |
	SCB_SHCSR_MEMFAULTENA_Msk);
	__set_MSP(*((volatile uint32_t*) AppAddress)); //
	SCB->VTOR = (uint32_t) AppAddress;
	// de SCB->VTOR chỗ này mới chạy được hàm này thành công, nếu để đầu chương trình sẽ lỗi
	uint32_t JumpAddress = *((volatile uint32_t*) (AppAddress + 4));
	void (*reset_handler)(void) = (void*)JumpAddress;
	reset_handler();

}

void jump_to_boot(uint32_t BootAddress) {
	jump_to_app(BootAddress);

}
