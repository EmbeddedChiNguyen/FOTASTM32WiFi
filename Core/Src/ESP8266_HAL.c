/*
 * ESP8266_HAL.c
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */

#include "main.h"
#include "ESP8266_HAL.h"
#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart1;
//extern UART_HandleTypeDef huart2;
#define wifi_uart &huart1
//#define pc_uart &huart2
char buffer[20];

/*****************************************************************************************************************************************/

void ESP_Init(char *SSID, char *PASSWD) {
	char data[80];
//hàm wait for trả về giá trị 0 nếu có timeout xảy ra nhưng mà while(!0) thì luôn chạy có nghĩa là
//while (!wait_for()); thì nếu gặp timeout thì sẽ làm lại cho đến khi nhận được chuỗi trong waitfor);
//	Ringbuf_init();
//vì lí do nào đó thêm pc uart vô thì inialized lâu
	Uart_sendstring("AT+RST\r\n", wifi_uart);

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 1160 * 1000; j++)
			;
	}

	/********* AT **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT\r\n", wifi_uart);

	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
	//time wait is 500ms;
//	Uart_sendstring("AT---->OK\n\n", pc_uart);

	/********* AT+CWMODE=1 **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CWMODE=1\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
//	Uart_sendstring("CW MODE---->1\n\n", pc_uart);

	/********* AT+CWJAP="SSID","PASSWD" **********/
	Uart_flush(wifi_uart);
//	Uart_sendstring("connecting... to the provided AP\n", pc_uart);
	sprintf(data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PASSWD);
	Uart_sendstring(data, wifi_uart);

	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
}

void bufclr(char *buf) {
	int len = strlen(buf);
	for (int i = 0; i < len; i++)
		buf[i] = '\0';
}

// Get the latest uploaded FW file name written on "latest_version.txt" file, on the web
void ESP_Get_Latest_Version(uint8_t *bufToPasteInto) {

	/*QUY TRÌNH

	 1.Client thiết lập kết nối TCP

	 2.Client gửi HTTP Request

	 3.Server gửi HTTP Response

	 4.Đóng kết nối TCP */

	// Some temporary local buffer
	char local_buf[500] = { 0 };
	char local_buf2[30] = { 0 };

	// Create TCPIP connection to the web server
	Uart_flush(wifi_uart);
	Uart_sendstring(
			"AT+CIPSTART=\"TCP\",\"nguyenwebstm32.000webhostapp.com\",80\r\n",
			wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
	// Send HTTP GET request to get the content of latest_version.txt
	// Prepair the HTTP GET request data
	bufclr(local_buf); // Make sure it cleared
	sprintf(local_buf, "GET /uploads/latest_version_test.txt HTTP/1.1\r\n"
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
	Uart_sendstring(local_buf, wifi_uart);
	while (!(Wait_for("SEND OK\r\n", wifi_uart)))
		;
	while (!(Wait_for("\r\n\r\n", wifi_uart)))
		;
	while (!(Copy_upto_rn("\r\n", bufToPasteInto, wifi_uart)))
		;
}

// Send HTTP GET request to read the firmware file on web

