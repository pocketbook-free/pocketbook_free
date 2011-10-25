/*
 *
 * This file splits out the tables describing the defaults and access
 * status of the WM8350 registers since they are rather large.
 *
 * Copyright 2007, 2008 Wolfson Microelectronics PLC.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/mfd/max8698/core.h>
/*
 * Access masks.
 */

const struct max17043_temp_access max17043_temp_map[] = {
/*V_bat_ntc R_center T_temp_ntc */
	{ 17124,  19565,  -40 }, /* R0  */
	{ 17076,  18491,  -39 }, /* R1   - ID */
	{ 17026,  17485,  -38 }, /* R2 */
	{ 16973,  16539,  -37 }, /* R3   - System Control 1 */
	{ 16919,  15651,  -36 }, /* R4   - System Control 2 */
	{ 16862,  14817,  -35 }, /* R5   - System Hibernate */
	{ 16803,  14033,  -34 }, /* R6   - Interface Control */
	{ 16741,  13296,  -33 }, /* R7 */
	{ 16677,  12602,  -32 }, /* R8   - Power mgmt (1) */
	{ 16610,  11949,  -31 }, /* R9   - Power mgmt (2) */
	{ 16541,  11335,  -30 }, /* R10  - Power mgmt (3) */
	{ 16469,  10756,  -29 }, /* R11  - Power mgmt (4) */
	{ 16395,  10212,  -28 }, /* R12  - Power mgmt (5) */
	{ 16318,   9698,  -27 }, /* R13  - Power mgmt (6) */
	{ 16238,   9213,  -26 }, /* R14  - Power mgmt (7) */
	{ 16156,   8756,  -25 }, /* R15 */
	{ 16070,   8324,  -24 }, /* R16  - RTC Seconds/Minutes */
	{ 15981,   7917,  -23 }, /* R17  - RTC Hours/Day */
	{ 15891,   7532,  -22 }, /* R18  - RTC Date/Month */
	{ 15796,   7168,  -21 }, /* R19  - RTC Year */
	{ 15699,   6824,  -20 }, /* R20  - Alarm Seconds/Minutes */
	{ 15599,   6499,  -19 }, /* R21  - Alarm Hours/Day */
	{ 15497,   6192,  -18 }, /* R22  - Alarm Date/Month */
	{ 15392,   5901,  -17 }, /* R23  - RTC Time Control */
	{ 15283,   5626,  -16 }, /* R24  - System Interrupts */
	{ 15172,   5365,  -15 }, /* R25  - Interrupt Status 1 */
	{ 15058,   5118,  -14 }, /* R26  - Interrupt Status 2 */
	{ 14941,   4883,  -13 }, /* R27  - Power Up Interrupt Status */
	{ 14821,   4661,  -12 }, /* R28  - Under Voltage Interrupt status */
	{ 14698,   4451,  -11 }, /* R29  - Over Current Interrupt status */
	{ 14572,   4251,  -10 }, /* R30  - GPIO Interrupt Status */
	{ 14443,   4060,  -9 }, /* R31  - Comparator Interrupt Status */
	{ 14310,   3879,  -8 }, /* R32  - System Interrupts Mask */
	{ 14176,   3707,  -7 }, /* R33  - Interrupt Status 1 Mask */
	{ 14039,   3544,  -6 }, /* R34  - Interrupt Status 2 Mask */
	{ 13899,   3389,  -5 }, /* R35  - Power Up Interrupt Status Mask */
	{ 13757,   3242,  -4 }, /* R36  - Under Voltage Int status Mask */
	{ 13612,   3102,  -3 }, /* R37  - Over Current Int status Mask */
	{ 13465,   2969,  -2 }, /* R38  - GPIO Interrupt Status Mask */
	{ 13315,   2842,  -1 }, /* R39  - Comparator IntStatus Mask */
	{ 13164,   2722,   0 }, /* R40  - Clock Control 1 */
	{ 13011,   2608,   1 }, /* R41  - Clock Control 2 */
	{ 12855,   2499,   2 }, /* R42  - FLL Control 1 */
	{ 12698,   2395,   3 }, /* R43  - FLL Control 2 */
	{ 12539,   2296,   4 }, /* R44  - FLL Control 3 */
	{ 12378,   2202,   5 }, /* R45  - FLL Control 4 */
	{ 12207,   2112,   6 }, /* R46 */
	{ 12053,   2027,   7 }, /* R47 */
	{ 11888,   1945,   8 }, /* R48  - DAC Control */
	{ 11722,   1867,   9 }, /* R49 */
	{ 11554,   1793,  10 }, /* R50  - DAC Digital Volume L */
	{ 11386,   1721,  11 }, /* R51  - DAC Digital Volume R */
	{ 11216,   1653,  12 }, /* R52 */
	{ 11046,   1589,  13 }, /* R53  - DAC LR Rate */
	{ 10876,   1527,  14 }, /* R54  - DAC Clock Control */
	{ 10705,   1467,  15 }, /* R55 */
	{ 10533,   1411,  16 }, /* R56 */
	{ 10362,   1357,  17 }, /* R57 */
	{ 10191,   1305,  18 }, /* R58  - DAC Mute */
	{ 10019,   1255,  19 }, /* R59  - DAC Mute Volume */
	{  9848,   1208,  20 }, /* R60  - DAC Side */
	{  9677,   1163,  21 }, /* R61 */
	{  9507,   1119,  22 }, /* R62 */
	{  9338,   1078,  23 }, /* R63 */
	{  9168,   1038,  24 }, /* R64  - ADC Control */
	{  9000,   1000,  25 }, /* R65 */
	{  8832,    963,  26 }, /* R66  - ADC Digital Volume L */
	{  8666,    928,  27 }, /* R67  - ADC Digital Volume R */
	{  8500,    894,  28 }, /* R68  - ADC Divider */
	{  8335,    862,  29 }, /* R69 */
	{  8171,    831,  30 }, /* R70  - ADC LR Rate */
	{  8010,    802,  31 }, /* R71 */
	{  7850,    773,  32 }, /* R72  - Input Control */
	{  7691,    746,  33 }, /* R73  - IN3 Input Control */
	{  7534,    720,  34 }, /* R74  - Mic Bias Control */
	{  7380,    695,  35 }, /* R75 */
	{  7226,    671,  36 }, /* R76  - Output Control */
	{  7074,    648,  37 }, /* R77  - Jack Detect */
	{  6925,    625,  38 }, /* R78  - Anti Pop Control */
	{  6777,    604,  39 }, /* R79 */
	{  6632,    583,  40 }, /* R80  - Left Input Volume */
	{  6488,    564,  41 }, /* R81  - Right Input Volume */
	{  6346,    545,  42 }, /* R82 */
	{  6206,    526,  43 }, /* R83 */
	{  6069,    509,  44 }, /* R84 */
	{  5933,    492,  45 }, /* R85 */
	{  5800,    475,  46 }, /* R86 */
	{  5669,    460,  47 }, /* R87 */
	{  5540,    445,  48 }, /* R88  - Left Mixer Control */
	{  5413,    430,  49 }, /* R89  - Right Mixer Control */
	{  5289,    416,  50 }, /* R90 */
	{  5167,    403,  51 }, /* R91 */
	{  5047,    390,  52 }, /* R92  - OUT3 Mixer Control */
	{  4929,    377,  53 }, /* R93  - OUT4 Mixer Control */
	{  4814,    365,  54 }, /* R94 */
	{  4701,    353,  55 }, /* R95 */
	{  4590,    342,  56 }, /* R96  - Output Left Mixer Volume */
	{  4481,    331,  57 }, /* R97  - Output Right Mixer Volume */
	{  4375,    321,  58 }, /* R98  - Input Mixer Volume L */
	{  4271,    311,  59 }, /* R99  - Input Mixer Volume R */
	{  4169,    301,  60 }, /* R100 - Input Mixer Volume */
	{  4071,    292,  61 }, /* R101 */
	{  3974,    283,  62 }, /* R102 */
	{  3880,    275,  63 }, /* R103 */
	{  3788,    267,  64 }, /* R104 - LOUT1 Volume */
	{  3699,    259,  65 }, /* R105 - ROUT1 Volume */
	{  3611,    251,  66 }, /* R106 - LOUT2 Volume */
	{  3525,    244,  67 }, /* R107 - ROUT2 Volume */
	{  3441,    236,  68 }, /* R108 */
	{  3359,    229,  69 }, /* R109 */
	{  3279,    223,  70 }, /* R110 */
	{  3201,    216,  71 }, /* R111 - BEEP Volume */
	{  3124,    210,  72 }, /* R112 - AI Formating */
	{  3049,    204,  73 }, /* R113 - ADC DAC COMP */
	{  2976,    198,  74 }, /* R114 - AI ADC Control */
	{  2905,    192,  75 }, /* R115 - AI DAC Control */
	{  2835,    187,  76 }, /* R116 - AIF Test */
	{  2768,    182,  77 }, /* R117 */
	{  2701,    177,  78 }, /* R118 */
	{  2637,    172,  79 }, /* R119 */
	{  2574,    167,  80 }, /* R120 */
	{  2513,    162,  81 }, /* R121 */
	{  2453,    158,  82 }, /* R122 */
	{  2395,    153,  83 }, /* R123 */
	{  2338,    149,  84 }, /* R124 */
	{  2282,    145,  85 }, /* R125 */
	{  2228,    141,  86 }, /* R126 */
	{  2176,    137,  87 }, /* R127 */
	{  2124,    134,  88 }, /* R128 - GPIO Debounce */
	{  2074,    130,  89 }, /* R129 - GPIO Pin pull up Control */
	{  2026,    127,  90 }, /* R130 - GPIO Pull down Control */
	{  1978,    123,  91 }, /* R131 - GPIO Interrupt Mode */
	{  1931,    120,  92 }, /* R132 */
	{  1885,    117,  93 }, /* R133 - GPIO Control */
	{  1841,    114,  94 }, /* R134 - GPIO Configuration (i/o) */
	{  1798,    111,  95 }, /* R135 - GPIO Pin Polarity / Type */
	{  1756,    108,  96 }, /* R136 */
	{  1715,    105,  97 }, /* R137 */
	{  1674,    103,  98 }, /* R138 */
	{  1635,    100,  99 }, /* R139 */
	{  1597,     97,  100 }, /* R140 - GPIO Function Select 1 */
	{  1560,     95,  101 }, /* R141 - GPIO Function Select 2 */
	{  1525,     93,  102 }, /* R142 - GPIO Function Select 3 */
	{  1490,     90,  103 }, /* R143 - GPIO Function Select 4 */
	{  1456,     88,  104 }, /* R144 - Digitiser Control (1) */
	{  1422,     86,  105 }, /* R145 - Digitiser Control (2) */
	{  1390,     84,  106 }, /* R146 */
	{  1358,     82,  107 }, /* R147 */
	{  1328,     80,  108 }, /* R148 */
	{  1298,     78,  109 }, /* R149 */
	{  1268,     76,  110 }, /* R150 */
	{  1240,     74,  111 }, /* R151 */
	{  1212,     72,  112 }, /* R152 - AUX1 Readback */
	{  1185,     70,  113 }, /* R153 - AUX2 Readback */
	{  1158,     69,  114 }, /* R154 - AUX3 Readback */
	{  1133,     67,  115 }, /* R155 - AUX4 Readback */
	{  1107,     66,  116 }, /* R156 - USB Voltage Readback */
	{  1083,     64,  117 }, /* R157 - LINE Voltage Readback */
	{  1059,     63,  118 }, /* R158 - BATT Voltage Readback */
	{  1036,     61,  119 }, /* R159 - Chip Temp Readback */
	{  1013,     60,  120 }, /* R160 */
	{   991,     58,  121 }, /* R161 */
	{   970,     57,  122 }, /* R162 */
	{   948,     56,  123 }, /* R163 - Generic Comparator Control */
	{   928,     54,  124 }, /* R164 - Generic comparator 1 */
	{   908,     53,  125 }, /* R165 - Generic comparator 2 */
};
