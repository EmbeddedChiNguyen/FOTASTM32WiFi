/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FLASH_SECTOR_F4.h"
#include "ESP8266_HAL.h"
#include "UartRingbuffer_multi.h"
#include "parse.h"
#include <string.h>
#include <stdio.h>
#include <boot.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	START_CODE = 0,
	BYTE_COUNT = 1,
	ADDRESS = 2,
	RECORD_TYPE = 3,
	DATA = 4,
	CHECKSUM = 5,
	DONE = 6
} FieldHexFile;

typedef enum {
	READ_SUCCESFULLY = 0, READ_FAILED = 1
} StatusReadHexFile;

typedef struct {
	uint8_t u8ByteCount;
	uint8_t u8Addr[4];
	uint8_t u8RecordType[2];
	uint8_t u8Data[32];
	uint8_t u8Checksum[2];
} HexFormData;

HexFormData hexStruct;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SSID "Nguyen"
#define PASSWORD "0935935516"
#define pc_uart &huart2
#define wifi_uart &huart1
//
#define SECTOR_CURRENT 0x08020000
#define SECTOR_A 0x08040000
#define SECTOR_B 0x08060000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint8_t cBufferData[50] = { ":105000000050002039530008DD520008E352000828" };
uint8_t CinternalBuff[50];
//
uint8_t recv_buf[20000] = { 0 };
uint8_t firmware_buf[100];
uint8_t lat_ver[128] = { 0 };
int lat_ver0;
int lat_ver2;
uint8_t fw_buf[2000] = { 0 };
uint8_t version_buf[30] = { 0 };
int CountForFirmwareSlot = 1;
int TriggerBase = 1;
int BL_Version[2] = { 0, 0 }; //MAJOR and MINOR 0.0
uint32_t current_sector;
uint32_t *FWSelect = (uint32_t*) 0x0800E000;
uint32_t *ReadSlotForFirmware = (uint32_t*) 0x0800E400;
uint32_t *TempForCountFirmware = (uint32_t*) 0x0800E800;
int CountForCompare = 0;
uint32_t *BLversion0 = (uint32_t*) 0x0800F000;
uint32_t *BLversion1 = (uint32_t*) 0x0800F400;

uint8_t tempfordata[32];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
uint32_t convert_uint8_to_int(uint8_t *data) {
    char str[9] = {'\0'};

    // Kiểm tra và loại bỏ tiền tố "0x" nếu có
    if (data[0] == '0' && (data[1] == 'x' || data[1] == 'X')) {
        data += 2; // Bỏ qua "0x"
    }

    // Copy dữ liệu vào chuỗi tạm thời
    for (int i = 0; i < 8; i++) {
        str[i] = data[i];
    }

    uint32_t result = strtoul(str, NULL, 16);
    return result;
}
int hexCharToInt(char hexChar) {
	int result;
	// Chuyển đổi ký tự hex thành giá trị int
	if (hexChar >= '0' && hexChar <= '9') {
		result = hexChar - '0';
	} else if (hexChar >= 'A' && hexChar <= 'F') {
		result = hexChar - 'A' + 10;
	} else if (hexChar >= 'a' && hexChar <= 'f') {
		result = hexChar - 'a' + 10;
	} else {
		// Xử lý ký tự không hợp lệ
		result = -1;
	}

	return result;
}
int hexPairToInt(char hex1, char hex2) {
	int result1, result2;
	// Chuyển đổi ký tự hex1 thành giá trị int
	if (hex1 >= '0' && hex1 <= '9') {
		result1 = hex1 - '0';
	} else if (hex1 >= 'A' && hex1 <= 'F') {
		result1 = hex1 - 'A' + 10;
	} else if (hex1 >= 'a' && hex1 <= 'f') {
		result1 = hex1 - 'a' + 10;
	} else {
		// Xử lý ký tự không hợp lệ
		return -1;
	}
	// Chuyển đổi ký tự hex2 thành giá trị int
	if (hex2 >= '0' && hex2 <= '9') {
		result2 = hex2 - '0';
	} else if (hex2 >= 'A' && hex2 <= 'F') {
		result2 = hex2 - 'A' + 10;
	} else if (hex2 >= 'a' && hex2 <= 'f') {
		result2 = hex2 - 'a' + 10;
	} else {
		// Xử lý ký tự không hợp lệ
		return -1;
	}
	// Kết hợp hai giá trị để tạo ra giá trị int cuối cùng
	return result1 * 16 + result2;
}
void print_pc(const char *str) {
	Uart_sendstring(str, &huart2);
}
//
void print_pc_version() {
	sprintf(version_buf, "%d.%d\n", BL_Version[0], BL_Version[1]);
	print_pc(version_buf);
}
//
int count_string(char *string, char search) {
	int count = 0;
	while (*string != search) {
		count++;
		string++;
	}
	return count;
}
int count_string2(char *string, char search) {
	static int countForChar2 = 0;
	while (*string != search) {
		countForChar2++;
		string++;
	}
	countForChar2++;
	return countForChar2;
}
int count_string3(char *string, char search) {
	static int countForChar3 = 0;
	while (*string != search) {
		countForChar3++;
		string++;
	}
	countForChar3++;
	return countForChar3;
}
StatusReadHexFile ReadAllLine(char *pBufferData) {
	uint8_t u16CaculChecksum;
	FieldHexFile State;
	int u32Count = 0u;
	int u32DataCount = 0u;
	int addressInt1 = 0;
	int addressInt2 = 0;
	int test1 = 0;
	int test2 = 0;
	uint16_t test3 = 0;
	for (u32DataCount = 0; u32DataCount < strlen(hexStruct.u8Data);
			u32DataCount++) {
		hexStruct.u8Data[u32DataCount] = '\0';
	}
	State = START_CODE;
	for (u32Count = 0; u32Count < strlen(pBufferData); u32Count++) {
		switch (State) {
		case START_CODE: {
			if (pBufferData[u32Count] != ':') {
				return READ_FAILED;
			} else {
				State = BYTE_COUNT;
			}
			break;
		}
		case BYTE_COUNT: {
			test1 = hexPairToInt(pBufferData[1], pBufferData[2]);
			hexStruct.u8ByteCount = test1;
			u32Count = 2u;
			State = ADDRESS;
			break;
		}
		case ADDRESS: {
			hexStruct.u8Addr[0] = pBufferData[3];
			hexStruct.u8Addr[1] = pBufferData[4];
			hexStruct.u8Addr[2] = pBufferData[5];
			hexStruct.u8Addr[3] = pBufferData[6];
			addressInt1 = hexPairToInt(hexStruct.u8Addr[0],
					hexStruct.u8Addr[1]);
			addressInt2 = hexPairToInt(hexStruct.u8Addr[2],
					hexStruct.u8Addr[3]);
			u32Count = 6u;
			State = RECORD_TYPE;
			break;
		}
		case RECORD_TYPE: {
			hexStruct.u8RecordType[0] = pBufferData[7];
			hexStruct.u8RecordType[1] = pBufferData[8];
			test2 = hexPairToInt(hexStruct.u8RecordType[0],
					hexStruct.u8RecordType[1]);
			if (test2 == 1) {
				return READ_FAILED;
			}
			u32Count = 8u;
			State = DATA;
			break;
		}
		case DATA: {
			for (int i = 9; i < (hexStruct.u8ByteCount) * 2 + 9; i++) {
				hexStruct.u8Data[i - 9] = pBufferData[i];
				if (test2 == 0) {
				}
			}
			u32Count = 8u + (hexStruct.u8ByteCount) * 2;
			State = CHECKSUM;
			break;
		}
		case CHECKSUM: {
			hexStruct.u8Checksum[0] =
					pBufferData[9 + hexStruct.u8ByteCount * 2];
			hexStruct.u8Checksum[1] =
					pBufferData[10 + hexStruct.u8ByteCount * 2];
			test3 = hexPairToInt(hexStruct.u8Checksum[0],
					hexStruct.u8Checksum[1]);
			State = DONE;
			break;
		}
		case DONE: {   //printf ("byte count : %d\n", test1);
					   //printf ("address:%02d-%02d\n ", addressInt1, addressInt2);
					   //printf ("record type : %d", test2);
			u16CaculChecksum = test1 + addressInt1 + addressInt2 + test2;
			for (u32DataCount = 0; u32DataCount < hexStruct.u8ByteCount * 2;
					u32DataCount += 2) {
				u16CaculChecksum = u16CaculChecksum
						+ hexPairToInt(hexStruct.u8Data[u32DataCount],
								hexStruct.u8Data[u32DataCount + 1]);
			}
			u16CaculChecksum = ~u16CaculChecksum + 1;
			if (u16CaculChecksum == test3) {
				return READ_SUCCESFULLY;
			} else {
				return READ_FAILED;
			}
			break;
		}
		default: {
			return READ_FAILED;
			break;
		}
		}
	}
	return READ_SUCCESFULLY;
}
void CopyandFlash(char *arrayCopy, char *arrayTransfered , uint32_t address) {
	int i = 0;
	int indexTest = 0;
	int indexNumber = 0;
	int toggle = 0;
	int indexOld = count_string2(arrayCopy + indexTest, '\n');
	uint32_t addRess=address;
	Flash_Erase(addRess);
	while (1) {
		indexNumber = count_string3(arrayCopy + indexTest, '\n');
		if (toggle == 0) {
			toggle++;
		} else {
			indexOld = indexNumber - indexTest;
		}
		for (int j = 0; j < indexOld; j++) {
			arrayTransfered[j] = arrayCopy[i++];
		}
		indexTest = indexNumber;
		if (ReadAllLine(arrayTransfered) != READ_SUCCESFULLY) {
			break;
		} else {
			tempfordata[0]=hexStruct.u8Data[6];
			tempfordata[1]=hexStruct.u8Data[7];
			tempfordata[2]=hexStruct.u8Data[4];
			tempfordata[3]=hexStruct.u8Data[5];
			tempfordata[4]=hexStruct.u8Data[2];
			tempfordata[5]=hexStruct.u8Data[3];
			tempfordata[6]=hexStruct.u8Data[0];
			tempfordata[7]=hexStruct.u8Data[1];
			tempfordata[8]=hexStruct.u8Data[14];
			tempfordata[9]=hexStruct.u8Data[15];
			tempfordata[10]=hexStruct.u8Data[12];
			tempfordata[11]=hexStruct.u8Data[13];
			tempfordata[12]=hexStruct.u8Data[10];
			tempfordata[13]=hexStruct.u8Data[11];
			tempfordata[14]=hexStruct.u8Data[8];
			tempfordata[15]=hexStruct.u8Data[9];
			tempfordata[16]=hexStruct.u8Data[22];
			tempfordata[17]=hexStruct.u8Data[23];
			tempfordata[18]=hexStruct.u8Data[20];
			tempfordata[19]=hexStruct.u8Data[21];
			tempfordata[20]=hexStruct.u8Data[18];
			tempfordata[21]=hexStruct.u8Data[19];
			tempfordata[22]=hexStruct.u8Data[16];
			tempfordata[23]=hexStruct.u8Data[17];
			tempfordata[24]=hexStruct.u8Data[30];
			tempfordata[25]=hexStruct.u8Data[31];
			tempfordata[26]=hexStruct.u8Data[28];
			tempfordata[27]=hexStruct.u8Data[29];
			tempfordata[28]=hexStruct.u8Data[26];
			tempfordata[29]=hexStruct.u8Data[27];
			tempfordata[30]=hexStruct.u8Data[24];
			tempfordata[31]=hexStruct.u8Data[25];
			uint16_t test2 = hexPairToInt(hexStruct.u8RecordType[0],hexStruct.u8RecordType[1]);
			if(test2!=0){
				}
			else {
					if(hexStruct.u8ByteCount==0x10){
						for (int i =0; i < 32; i =i+8){
							uint32_t result = convert_uint8_to_int(tempfordata+i);
							Flash_Write_Data_Int(addRess,result);
							addRess +=4;
						}
					}
					else if(hexStruct.u8ByteCount==0x8){
						for (int i =0; i < 16; i =i+8){
						uint32_t result = convert_uint8_to_int(tempfordata+i);
						Flash_Write_Data_Int(addRess,result);
						addRess +=4;
						}
					}
					else if(hexStruct.u8ByteCount==0x4){
						for (int i =0; i < 8; i =i+8){
						uint32_t result = convert_uint8_to_int(tempfordata+i);
						Flash_Write_Data_Int(addRess,result);
						addRess +=4;
					}
				}
			}
		}//vi sao firmwarebuff chi truyen toi 3107000, tu nhien gap 1 dãy +IPD,1452:
		memset(arrayTransfered, '\0', strlen(arrayTransfered));
	}
}
void parse_character_strings() {
	int number_of_major = count_string(lat_ver, '.');
	int number_of_minor = count_string(&lat_ver[number_of_major + 1], 'C');
	char temp1[10];
	for (int i = 0; i < number_of_major; i++) {
		temp1[i] = lat_ver[i];
	}
	temp1[number_of_major] = '\0';
	char temp2[10];
	for (int i = 0; i < number_of_minor; i++) {
		temp2[i] = lat_ver[i + number_of_major + 1];
	}
	temp2[number_of_minor] = '\0';
	lat_ver0 = atoi(temp1);
	lat_ver2 = atoi(temp2);
}
void compare_version() {
	//lat_ver0=(int)lat_ver[0]-48;
	//lat_ver2=(int)lat_ver[2]-48;
	//tim hieu ham atoi() trong stdlib de chuyen chuoi thành số
	BL_Version[0] = *BLversion0;
	BL_Version[1] = *BLversion1;
	parse_character_strings();
	if (lat_ver0 > BL_Version[0]) {
		BL_Version[0] = lat_ver0;
		Flash_Erase(0x0800F000);
		Flash_Write_Data_Int(0x0800F000, BL_Version[0]);
		CountForCompare++;
	}
	if (lat_ver2 > BL_Version[1]) {
		BL_Version[1] = lat_ver2;
		Flash_Erase(0x0800F000);
		Flash_Write_Data_Int(0x0800F400, BL_Version[1]);
		CountForCompare++;
	}
	if (CountForCompare > 0) {
		HAL_Delay(1000);
		print_pc("Got higher version\n");
		print_pc("Version:");
		print_pc_version();
		CountForFirmwareSlot++;
	}
	if (CountForCompare == 0) {
		HAL_Delay(1000);
		print_pc("Version on the server is not higher\n");
		print_pc("Returning to application program...");
		TriggerBase++;
	}
}
//
void ESP_Get_Firmware(uint8_t *buffer) {
	// Some temporary local buffer

	if (*FWSelect == 0x00000001) {
		current_sector = SECTOR_A;
	}
	if (*FWSelect == 0x00000002) {
		current_sector = SECTOR_B;
	}
	char local_buf[200] = { 0 };
	char local_buf2[30] = { 0 };
//	char field_buf[200] = { 0 };
	// Create TCPIP connection to the web server
	Uart_flush(wifi_uart);
	Uart_sendstring(
			"AT+CIPSTART=\"TCP\",\"nguyenwebstm32.000webhostapp.com\",80\r\n",
			wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
	// Send HTTP GET request to get the content of latest fw file
	// Prepair the HTTP GET request data
	bufclr(local_buf); // Make sure it cleared
	sprintf(local_buf, "GET /uploads/1.0.hex HTTP/1.1\r\n"
			"Host: nguyenwebstm32.000webhostapp.com\r\n"
			"Connection: close\r\n\r\n");
	int len = strlen(local_buf); // Get the data length
	// Prepair the CIPSEND command
	bufclr(local_buf2); // Make sure it cleared
	sprintf(local_buf2, "AT+CIPSEND=%d\r\n", len);
	// Send CIPSTART
	Uart_sendstring(local_buf2, wifi_uart);
	while (!(Wait_for(">", wifi_uart)))
		;
	// Send HTTP GET
	// xem xét đev lại chỗ này
	Uart_sendstring(local_buf, wifi_uart);
	while (!(Wait_for("SEND OK\r\n", wifi_uart))) {
		// Handle error if sending HTTP GET is not successful

	}
	while (!(Wait_for("\r\n\r\n", wifi_uart))) {
		// Handle error if the end of the response header is not found
	}
	while (!(Copy_upto_closed("CLOSED\r\n", buffer, wifi_uart))) {
		//buffer truyen vao o day la rev_Buf

	}
//
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	Ringbuf_init();
	print_pc("\r\nBootloader initiated\r\n");
	ESP_Init(SSID, PASSWORD);
	print_pc("Connected to wifi\r\n");
	//get version
	memset(recv_buf, 0, sizeof(recv_buf)); // make sure recv_buf is clear
	ESP_Get_Latest_Version((uint8_t*) recv_buf);
	memset(lat_ver, 0, sizeof(lat_ver));
	print_pc("Got latest file name\r\n");
	compare_version();
	if (*TempForCountFirmware != 0x00000001) {
		Flash_Write_Data_Int(0x0800E400, 0x00000001);
		Flash_Write_Data_Int(0x0800E800, 0x00000001);
	} // này là cờ flag tại địa chỉ 0x0800E800 nếu giá trị tại 0x0800E400 = 1 thì bật cờ này lên để ko cần flash lại E400 =1 nữa
	if (CountForFirmwareSlot == 2) {
		CountForFirmwareSlot = 1;
		uint32_t check = *ReadSlotForFirmware + 1;
		Flash_Write_Data_Int(0x0800E400, check);
	}
	//
	if (*ReadSlotForFirmware % 2 == 0) {
		Flash_Write_Data_Int(0x0800E000, 0x00000002);
	} else if (*ReadSlotForFirmware % 2 != 0) {
		Flash_Write_Data_Int(0x0800E000, 0x00000001);
	}
	if (TriggerBase == 2) {
		Flash_Write_Data_Int(0x0800E000, 0x00000000);
	}
	if (*FWSelect == 0x00000000) {
		current_sector = SECTOR_CURRENT;
	}
	//jump_to_app(current_sector);
	/*#define SECTOR_CURRENT 0x08005000
	 #define SECTOR_A 0x08008000
	 #define SECTOR_B 0x0800B000
	 */
	HAL_Delay(1000);
	memset(recv_buf, 0, sizeof(recv_buf));
	ESP_Get_Firmware((uint8_t*) recv_buf);
	print_pc("Downloaded firmware file\r\n");
	HAL_Delay(500);
	CopyandFlash(recv_buf, firmware_buf,current_sector);
	jump_to_app(current_sector);
	/*while (ReadLine(recv_buf,cBufferData)!=0){
	 sprintf(cBufferData, "end 1 line" );
	 print_pc(cBufferData);
	 }*/
	/*
	 if ( *FWSelect ==0x00000000){
	 current_sector=SECTOR_CURRENT;
	 }
	 if ( *FWSelect ==0x00000001){
	 current_sector=SECTOR_A;
	 }
	 if ( *FWSelect ==0x00000002){
	 current_sector=SECTOR_B;
	 }

	 // chua xu li
	 jump_to_app(current_sector);

	 // chua xu li
	 */
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
