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


int main(void) 
	{
   Nrf24 nrf;
	  static const uint8_t nRF24_ADDR[] = { 'E', 'S', 'B' };

	  UART_Init(115200);
    UART_SendStr("\r\nSTM32F103RET6 is online.\r\n");


    // Initialize delay
    Delay_Init();
     
		nrf.Init();

    // Configure the nRF24L01+
    UART_SendStr("nRF24L01+ check: ");
    if (!nrf.Check()) 
			{
				UART_SendStr("FAIL\r\n");
				while (1);
      }
	  UART_SendStr("OK\r\n");
    // Initialize the nRF24L01 to its default state    nRF24_Init();
    // The transmitter sends a 10-byte packets to the address 'ESB' with Auto-ACK (ShockBurst enabled)
    // Set RF channel
		nrf.SetRFChannel(40);
    // Set data rate
    nrf.SetDataRate(nRF24_DR_250kbps);
    // Set CRC scheme
    nrf.SetCRCScheme(nRF24_CRC_2byte);
    // Set address width, its common for all pipes (RX and TX)
    nrf.SetAddrWidth(3);
    // Configure RX PIPE
    nrf.SetAddr(nRF24_PIPE1, nRF24_ADDR); // program address for pipe
    nrf.SetRXPipe(nRF24_PIPE1, nRF24_AA_ON, 10); // Auto-ACK: enabled, payload length: 10 bytes
    // Set TX power for Auto-ACK (maximum, to ensure that transmitter will hear ACK reply)
    nrf.SetTXPower(nRF24_TXPWR_18dBm);
    // Put the transceiver to the RX mode
    nrf.SetRxMode();

    // The main loop
    while (1) 
			{
    	//
    	// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
    	//
    	// This is far from best solution, but it's ok for testing purposes
    	// More smart way is to use the IRQ pin :)
    	//
    	if (nrf.DataAvailable()) 
				{
    		// Get a payload from the transceiver
    		pipe = nrf.ReadPayload(nRF24_payload, &payload_length);

    		// Clear all pending IRQ flags
				nrf.ClearIRQFlags();

			// Print a payload contents to UART
				UART_SendStr("RCV PIPE#");
				UART_SendInt(pipe);
				UART_SendStr(" PAYLOAD:>");
				UART_SendBufHex((char *)nRF24_payload, payload_length);
				UART_SendStr("<\r\n");
    	 }
    }

}
