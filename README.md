# Arduino-MKR-1400-OTA-Update
OTA Update for large sketches on the Arduino MKR 1400

This sketch downloads a large UPDATE.bin to the memory of the modem. 
After checking the size, it will copy the downloaded UPDATE.bin to the SD card.

In the loop you can reset the board with software NVIC_SystemReset(); or simply repower to update the sketch on the board from the SD card with the 
#include <SDU.h> lib.


Advantages:
- You don't risk the bootloader (With the SSU lib, i experienced a lot of bootloader damages)
- It is still possible to update the board anytime with a manually inserted SD card

Disadvantages:

- To save SRAM, the operations are done in 512 bytes blocks which makes the process not very fast..
- It does NOT work with initialized watchdogs (WDTZero.h)
- It has no error handling for broken socket, a good mobile signal is needed.


It's just a assamblage of code written by others, mainly by Giampaolo Mancini, Alexander Entinger, Sandeep Mistry et all so the credit goes there  
