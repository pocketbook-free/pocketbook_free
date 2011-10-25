
#ifndef __ELAN_INITIAL_6410VS668_H__
#define __ELAN_INITIAL_6410VS668_H__


extern void Elan_Initial_6410vs668(void) ;


//extern IO --------------------------------------------
extern void Elan_Define_668_KIN_AS_OUTPUT_PIN(void);
extern void Elan_Define_668_KIN_AS_INPUT_PIN(void);
extern void Elan_Define_668_VDD_AS_OUTPUT_PIN(void);
extern void Elan_Define_668_VDD_AS_INPUT_PIN(void);
extern void Elan_Define_668_RST_AS_OUTPUT_PIN(void);
extern void Elan_Define_668_RST_AS_INPUT_PIN(void);
extern void Elan_Define_668_DAT_AS_OUTPUT_PIN(void);
extern void Elan_Define_668_DAT_AS_INPUT_PIN(void);
extern void Elan_Define_668_CLK_AS_OUTPUT_PIN(void);
extern void Elan_Define_668_CLK_AS_INPUT_PIN(void);


//extern Delay Time ------------------------------------
extern void Elan_Define_Delay1us(void);
extern void Elan_Define_Delay5us(void);
extern void Elan_Define_Delay10us(void);
extern void Elan_Define_Delay50us(void);
extern void Elan_Define_Delay100us(void);
extern void Elan_Define_Delay500us(void);
extern void Elan_Define_Delay1ms(void);
extern void Elan_Define_Delay5ms(void);
extern void Elan_Define_Delay100ms(void);
extern void Elan_Define_Delay500ms(void);

#endif