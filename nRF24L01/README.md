# nRF24L01+ C++ Transceiver Library for STM32F1 (SPL)

This "library" is aimed to deal with Nordic nRF24L01+ transceiver.

## Structure
The library basically consists two files:
* nrf24.cpp
* nrf24.h

## Description of some functions
- `Init()` - configures nRF24 to its initial state (as after reset), flushes RX/TX FIFOs and clears all pending interrupts. With one exception: the addresses of pipes remains untouched.
- `Check()` - check presence of the nRF24. Function configures address length to 5 bytes and writes a predefined TX address (`"nRF24"`), then tries to read it back and compare. If the received value is equal to predefined address, the function returns `1` (nRF24 is present), `0` otherwise. So if software uses address length different from 5 bytes, it must re-program its value after calling this function.
- `SetAddr()` - program address for the specified pipe. The first argument - pipe number, it can be from `0` to `5` for RX pipe (one of `nRF24_PIPEx` values) and `6` (or `nRF24_PIPETX`). Second argument must be a pointer to the buffer with address. For TX and RX pipes #0 and #1, the length of the buffer must be equal to a current address length programmed in the nRF24. For RX pipes 2..5 the buffer length can be one byte since these pipe addresses differs from the address of pipe #1 by one last byte (read the datasheet, though).
- `DisableAA()` - disable auto-retransmit (a.k.a. ShockBurst) for the specified RX pipe or all pipes at once. Function accepts one parameter, if it is from 0 to 5, then AA will be disabled for the pipe with the specified number. Any other value disable AA for **ALL** RX pipes.

### Receiver, one logic address (pipe#1), with ShockBurst
```tcc
uint8_t ADDR[] = { 'n', 'R', 'F', '2', '4' }; // the address for RX pipe
nrf.SetRFChannel(90); // set RF channel to 2490MHz
nrf.SetDataRate(nRF24_DR_2Mbps); // 2Mbit/s data rate
nrf.SetCRCScheme(nRF24_CRC_1byte); // 1-byte CRC scheme
nrf.SetAddrWidth(5); // address width is 5 bytes
nrf.SetAddr(nRF24_PIPE1, ADDR); // program pipe address
nrf.SetRXPipe(nRF24_PIPE1, nRF24_AA_ON, 10); // enable RX pipe#1 with Auto-ACK: enabled, payload length: 10 bytes
nrf.SetTXPower(nRF24_TXPWR_0dBm); // configure TX power for Auto-ACK, good choice - same power level as on transmitter
nrf.SetRxMode();    // Put the transceiver to the RX mode

```

### Transmiter, with ShockBurst
```tcc
uint8_t ADDR[] = { 'n', 'R', 'F', '2', '4' }; // the TX address
nrf.SetRFChannel(90); // set RF channel to 2490MHz
nrf.SetDataRate(nRF24_DR_2Mbps); // 2Mbit/s data rate
nrf.SetCRCScheme(nRF24_CRC_1byte); // 1-byte CRC scheme
nrf.SetAddrWidth(5); // address width is 5 bytes
nrf.SetAddr(nRF24_PIPETX, ADDR); // program TX address
nrf.SetAddr(nRF24_PIPE0, ADDR); // program pipe#0 RX address, must be same as TX (for ACK packets)
nrf.SetTXPower(nRF24_TXPWR_0dBm); // configure TX power
nrf.SetAutoRetr(nRF24_ARD_2500us, 10); // configure auto retransmit: 10 retransmissions with pause of 2500s in between
nrf.EnableAA(nRF24_PIPE0); // enable Auto-ACK for pipe#0 (for ACK packets)
nrf.SetTxMode();    // Set operational mode (PTX == transmitter)
// the nRF24 is ready for transmission, upload a payload, then pull CE pin to HIGH and it will transmit a packet...
```

## Receive
Configure nRF24 for data receive, pull the CE pin HIGH. After this the transceiver begins to listen ether waiting for data packet. The main software can poll the nRF24 status or for an IRQ. The polling method is far from optimal and if it possible, it is better to use the dedicated IRQ pin.
Simple example code of receiver in polling mode:
```tcc
uint8_t nRF24_payload[32]; // buffer for payload
uint8_t payload_length; // variable to store a length of received payload
uint8_t pipe; // pipe number
nrf.StartListening();
while (1) {
    // constantly poll the status of RX FIFO...
    if (nrf.DataAvailable()) {
        // the RX FIFO have some data, take a note what nRF24 can hold up to three payloads of 32 bytes...
        pipe = nrf.ReadPayload(nRF24_payload, &payload_length); // read a payload to buffer
        nrf.ClearIRQFlags(); // clear any pending IRQ bits
        // now the nRF24_payload buffer holds received data
        // payload_length variable holds a length of received data
        // pipe variable holds a number of the pipe which has received the data
        // ... do something with received data ...
    }
}
```


### Example of handling the transmit with ShockBurst enabled
```tcc
// Here was a transmit procedure... 
nrf.TransmitPacket(nRF24_payload, payload_length);
status = nrf.GetStatus(); // get status of nRF24
nrf.ClearIRQFlags(); // clear any pending IRQ flags
uint8_t otx = nrf.GetRetransmitCounters(); // get some stats, this variable will contain two counters
uint8_t otx_plos_cnt = (otx & nRF24_MASK_PLOS_CNT) >> 4; // packets lost counter
uint8_t otx_arc_cnt  = (otx & nR24_MASK_ARC_CNT); // auto retransmissions counter
if (otx_arc_cnt > 0) {
    // ARC_CNT is non zero, has been few retransmissions
    // ... do something here with this knowlege ...
    // This counter will be reset when a new transmission starts
}
if (status & nRF24_FLAG_TX_DS) { // success }
if (status & nRF24_FLAG_MAX_RT) {
    // Maximum number of retransmits happened, the lost packets counter must be non zero
    // This counter counts up to 15 and then stops (protection from overflow as datasheet says)
    // For example the software can have some variable as counter for lost packets (e.g. uint32_t lost_pckts)...
    lost_pckts += otx_plos_cnt; // update global counter of lost packets
    nrf.ResetPLOS(); // reset PLOS_CNT value
}
```

## Demo code
`Examples` contains two simple Transmitter and Receiver demo code. It written for **STM32F103** MCU using the old **SPL** in C++ and keil uVision 5. 
