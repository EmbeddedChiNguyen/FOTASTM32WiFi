/*
 * ESP8266_HAL.h
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */

#ifndef INC_ESP8266_HAL_H_
#define INC_ESP8266_HAL_H_


void ESP_Init (char *SSID, char *PASSWD);
void bufclr(char *buf);
void ESP_Get_Latest_Version(uint8_t* bufToPasteInto);
void ESP_Get_Firmware(uint8_t* buff );


#endif /* INC_ESP8266_HAL_H_ */
