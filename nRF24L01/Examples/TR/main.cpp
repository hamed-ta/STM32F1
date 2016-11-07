#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>

extern "C" 
	{
		#include "uart.h"
		#include "delay.h"
	}

#include "nrf24.h"


uint32_t i,j,k;
// Buffer to store a payload of maximum width
uint8_t nRF24_payload[32];
// Pipe number
nRF24_RXResult pipe;
// Length of received payload
uint8_t payload_length;
// Result of packet transmission
nRF24_TXResult tx_res;
// Function to transmit data packet
// input:
//   pBuf - pointer to the buffer with data to transmit
//   length - length of the data buffer in bytes
// return: one of nRF24_TX_xx values
int main(void) 
	{

		Nrf24 nrf;
		
	  uint32_t packets_lost = 0; // global counter of lost packets
	  uint8_t otx;
	  uint8_t otx_plos_cnt; // lost packet count
		uint8_t otx_arc_cnt; // retransmit count
	
	    static const uint8_t nRF24_ADDR[] = { 'E', 'S', 'B' };

		UART_Init(115200);
    UART_SendStr("\r\nSTM32F103RET6 is online.\r\n");

		Delay_Init();
			
			
		// Configure the nRF24L01+
    nrf.Init();
    UART_SendStr("nRF24L01+ check: ");
    if (!nrf.Check()) 
			{
				UART_SendStr("FAIL\r\n");
				while (1);
			}
		UART_SendStr("OK\r\n");

/***************************************************************************/
    // The transmitter sends a 10-byte packets to the address 'ESB' with Auto-ACK (ShockBurst enabled)
    // Set RF channel
    nrf.SetRFChannel(40);
    // Set data rate
    nrf.SetDataRate(nRF24_DR_250kbps);
    // Set CRC scheme
    nrf.SetCRCScheme(nRF24_CRC_2byte);
    // Set address width, its common for all pipes (RX and TX)
    nrf.SetAddrWidth(3);
    // Configure TX PIPE
    nrf.SetAddr(nRF24_PIPETX, nRF24_ADDR); // program TX address
    nrf.SetAddr(nRF24_PIPE0, nRF24_ADDR); // program address for pipe#0, must be same as TX (for Auto-ACK)
    // Set TX power (maximum)
    nrf.SetTXPower(nRF24_TXPWR_18dBm);
    // Configure auto retransmit: 10 retransmissions with pause of 2500us in between
    nrf.SetAutoRetr(nRF24_ARD_2500us, 10);
    // Enable Auto-ACK for pipe#0 (for ACK packets)
    nrf.EnableAA(nRF24_PIPE0);
    // Set operational mode (PTX == transmitter)
    nrf.SetTxMode();

    // The main loop
    payload_length = 10;
    j = 0;
    while (1) {
    	// Prepare data packet
    	for (i = 0; i < payload_length; i++) {
    		nRF24_payload[i] = j++;
    		if (j > 0x000000FF) j = 0;
    	}

    	// Print a payload
    	UART_SendStr("PAYLOAD:>");
    	UART_SendBufHex((char *)nRF24_payload, payload_length);
    	UART_SendStr("< ... TX: ");

    	// Transmit a packet
    	tx_res = nrf.TransmitPacket(nRF24_payload, payload_length);
			otx = nrf.GetRetransmitCounters();
			otx_plos_cnt = (otx & nRF24_MASK_PLOS_CNT) >> 4; // packets lost counter
			otx_arc_cnt  = (otx & nRF24_MASK_ARC_CNT); // auto retransmissions counter
			
    	switch (tx_res) {
			case nRF24_TX_SUCCESS:
				UART_SendStr("OK");
				break;
			case nRF24_TX_TIMEOUT:
				UART_SendStr("TIMEOUT");
				break;
			case nRF24_TX_MAXRT:
				UART_SendStr("MAX RETRANSMIT");
				packets_lost += otx_plos_cnt;
				nrf.ResetPLOS();
				break;
			default:
				UART_SendStr("ERROR");
				break;
		}
    	UART_SendStr("   ARC=");
		UART_SendInt(otx_arc_cnt);
		UART_SendStr(" LOST=");
		UART_SendInt(packets_lost);
		UART_SendStr("\r\n");

    	// Wait ~0.5s
    	Delay_ms(1000);
    }

}
