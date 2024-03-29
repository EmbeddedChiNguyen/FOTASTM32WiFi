#include <boot.h>
#include <stdint.h>
//create by Nguyen

/*void jump_to_app(uint32_t AppAddress) {
	uint32_t JumpAddress = *((volatile uint32_t*) (AppAddress + 4));
	void (*reset_handler)(void) = (void*)JumpAddress;
	HAL_RCC_DeInit();
	HAL_DeInit();
	SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk |\
	SCB_SHCSR_BUSFAULTENA_Msk |
	SCB_SHCSR_MEMFAULTENA_Msk);
	__set_MSP(*((volatile uint32_t*) AppAddress)); //
	SCB->VTOR = (uint32_t) AppAddress;
	// de SCB->VTOR chỗ này mới chạy được hàm này thành công, nếu để đầu chương trình sẽ lỗi

	reset_handler();

}
*/

void jump_to_app(uint32_t ADDRESS)
{
	//1.pointing the function pointer to the reset handler of the user application.
	uint32_t reset_handler_add = *((volatile uint32_t *) (ADDRESS+4));
	void (*app_reset_handler)(void) = (void*) reset_handler_add;
	//2.Disable RCC, set it to default (after reset) settings Internal clock, no PLL, etc
	HAL_RCC_DeInit();
	HAL_DeInit();
	//3.Disabling Systick timer and loading it with the default values.
	SysTick->CTRL = 0x0;
	SysTick->LOAD=0;
	SysTick->VAL=0;
	//4.Remapping the vector table offset
	SCB->VTOR = ADDRESS;
	//5.Setting the MSP value and calling the reset handler of the the user-application.
	uint32_t msp_value = *((volatile uint32_t *)ADDRESS);
	__set_MSP(msp_value);
	app_reset_handler();

}


