
#ifndef __ELAN_UPDATE668FW_FUNC_H__
#define __ELAN_UPDATE668FW_FUNC_H__

unsigned short Elan_Update668FW_func(unsigned short Elan_FUNCTIONS);
//if input  0x01, means ask 668 status only.
//   return 0x81, means 668 isn't protected.
//   return 0x82, means 668 is protected.

//if input  0x02, means update 668 EEPROM, ROM, CODEOPTION.
//   return 0x80, means update finish.

//if input  0x03, means update 668 EEPROM only.
//   return 0x80, means update finish.

//if input  0x04, means update 668 ROM only.
//   return 0x80, means update finish.

//if input  0x05, means Step1. write 668 CODEOPTION WORD0 bit8,7 = 01 (enable LVR 2.7V/2.9V) only.
//                      Step2. read 668 CODEOPTION WORD0 ~ WORD2.
//   return 0x80, means update finish.

//if input  0x06, means read 668 CODEOPTION WORD0 ~ WORD2 only.
//   return 0x80, means read finish.

//if input  0x07, means write 668 CODEOPTION WORD0 bit8,7 = 01 (enable LVR 2.7V/2.9V) only. (no control RST pin)
//   return 0x80, means update finish.

//if input  0x08, means read 668 CODEOPTION WORD0 ~ WORD2 only. (no control RST pin)
//   return 0x80, means read finish.

//if input  0x09, means Step1. read 668 CODEOPTION WORD0 ~ WORD2. (no control RST pin)
//											Step2. write 668 CODEOPTION WORD0 bit8,7 = 01 (enable LVR 2.7V/2.9V). (no control RST pin)
//											Step3. read 668 CODEOPTION WORD0 ~ WORD2. (no control RST pin)
//   return 0x80, means read finish.

//if input  0x10, means Step1. read 668 CODEOPTION WORD0 ~ WORD2. (no control RST pin)
//											Step2. Step2. if WORD1 = 0x2058, write 668 CODEOPTION WORD0 bit8,7 = 01 (enable LVR 2.7V/2.9V).
//											Step3. read 668 CODEOPTION WORD0 ~ WORD2. (no control RST pin)
//   return 0x80, means read finish.

//if return 0x85, means invalid command.

#endif