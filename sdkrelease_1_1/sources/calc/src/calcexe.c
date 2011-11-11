#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dlfcn.h>
#include "inkview.h"
#include <float.h>
#include <math.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAX_NUMBER_OF_CHARS		16

#define LINE_WIDTH				6

#define CALC_PI				"3.14159265358"
#define CALC_E					"2.71828182845"

#define DEF_CALC_ROWS			10
#define DEF_CALC_COLS			7

#define MAX_CALC_ROWS			11
#define MAX_CALC_COLS			11

/**
  * Types of the button we use in our calculator
  */
#define BTN_none	0
#define BTN_0		1
#define BTN_1		2
#define BTN_2		3
#define BTN_3		4
#define BTN_4		5
#define BTN_5		6
#define BTN_6		7
#define BTN_7		8
#define BTN_8		9
#define BTN_9		10

#define BTN_multiple		11
#define BTN_clear			12
#define BTN_plus			13
#define BTN_minus			14
#define BTN_div			15
#define BTN_result			16
#define BTN_percent			17
#define BTN_dot			18
#define BTN_sqrt			19
#define BTN_fullscreen        20

#define BTN_off			21
#define BTN_memplus			22
#define BTN_memread			23
#define BTN_memstore		24
#define BTN_memclear          25
#define BTN_back			26
#define BTN_allclear          27

#define BTN_sin			28
#define BTN_pi				29
#define BTN_cos			30
#define BTN_e				31
#define BTN_sqr			32
#define BTN_tan			33
#define BTN_1_div_x			34

#define BTN_plusminus		35
#define BTN_lg				36
#define BTN_abs			37

#define BTN_dec			40
#define BTN_hex			41
#define BTN_oct			42
#define BTN_bin			43

#define BTN_degrees			50
#define BTN_radians			51
#define BTN_grads			52

#define BTN_sinh			60
#define BTN_cosh			61
#define BTN_tanh			62

#define BTN_x_pow_y			63
#define BTN_ln				64
#define BTN_nth_root		65
#define BTN_logyx			66

#define BTN_A				70
#define BTN_B				71
#define BTN_C				72
#define BTN_D				73
#define BTN_E				74
#define BTN_F				75

#define BTN_or				80
#define BTN_and			81
#define BTN_xor			82
#define BTN_not			83

#define BTN_num_mod			84
#define BTN_num_div			85

#define BTN_shl			90
#define BTN_shr			91

#define BTN_screen			100

#define BTN_16_bit			110
#define BTN_32_bit			111
#define BTN_64_bit			112

/**
  * Floating point epsilon. Used to round numbers
  */
#define CALC_EPSILON		0.000000001

/**
  * Limit number of chars that user can type into our calculator
  */
#define MAX_COUNT_OF_DIGITS		70

/**
  * Max number of widgets that can be shown
  */
#define MAX_NUMBER_OF_WIDGETS	20

/**
  * Calculation mode
  */
typedef enum
{
	CM_Degrees = 0,
	CM_Radians = 1,
	CM_Grads = 2,
}
tCalcMode;

/**
  * Calculation mode
  */
typedef enum
{
	NS_Dec = 0,
	NS_Hex = 1,
	NS_Oct = 2,
	NS_Bin = 3,
	NS_Count = 4
}
tNumSystem;

/**
  * Calculation mode
  */
typedef enum
{
	BS_Dec = 1,
	BS_Hex = 2,
	BS_Oct = 4,
	BS_Bin = 8,
}
tBtnNumSystemState;

/**
  * Calculator operand size
  */
typedef enum
{
	OS_16_bits = 0,
	OS_32_bits = 1,
	OS_64_bits = 2,
}
tOperandSize;

/**
  * This structure allows you to add additional functions to buttons
  */
typedef struct
{
	// Text written on the screen
	char text[MAX_COUNT_OF_DIGITS * 2];

	// Button type, See BTN_** for all available types
	int type;

	int ModeButtonActive;
}
tMultiFunc;

/**
  * Structure that contains info about calualtor's button
  */
typedef struct
{
	///< Button's dimensions
	int x;
	int y;
	int w;
	int h;

	// Text written on the screen
	char text[MAX_COUNT_OF_DIGITS * 2];

	// Set to TRUE if button should be written on the screen
	int enabled;

	// Button type, See BTN_** for all available types
	int type;

	int ModeButton;
	int ModeButtonActive;

	unsigned int ButtonNumState;

	int MultiFuncButton;
	tMultiFunc funcs[4];

}
tCalcButton;

/**
  * Structure containing information about rect
  */
typedef struct
{
	int x;
	int y;
	int w;
	int h;
}
tRect;

/**
  * Structure containing information about rect
  */
#define MAX_RECT_COUNT 10
typedef struct
{
	tRect rects[MAX_RECT_COUNT];

	int count;
}
tRects;

/**
  * Structure that contains info about number
  */
typedef struct
{
	int dotAdded;								///< Should be set to true if decimal dot exists inside of the number

	char number[MAX_COUNT_OF_DIGITS * 2];			///< String representation of the number
	int numberLen;								///< Length of the string

	int operId;								///< ID of the operator which is was pressed after this number
	int operAdded;								///< Should be set to TRUE is some operator is added

	tNumSystem numSystem;						///< Numeral system of the number, i.e. Dec, Oct, Hex, Bin

	int valid;								///< Should be set to TRUE if this number is valid and we can use it in calculations
}
tNumber;

int HeaderHeight = 0;							///< Height of the header that contains "Calculator" text. This value calculated at runtime

tNumber mainOp;								///< Number which is shown on the screen
tNumber secondOp;								///< Number which was entered previously and is not shown on the screen
tNumber memOp;									///< Number which were stored to memory when user has pressed M+ button

int g_CalcRows = DEF_CALC_ROWS;
int g_CalcCols = DEF_CALC_COLS;

int write2ndArg;								///< Should be set to TRUE if we want to write new number into the screen

int MaxNumOfChars;								///< Max number of chars that can be entered in decimal calculator
int ResultFont_MaxChars;							///< Max number of chars for the default font
int Result2Font_MaxChars;						///< Max number of chars for the small font

int g_col;									///< Column index of the active button
int g_row;									///< Row index of the active button

int numWritten;								///< Set to TRUE if some number was pressed in our calculator

int initialized;								///< Set to TRUE if calculator was already initialized

int wx;										///< Widget's X coordinate
int wy;										///< Widget's Y coordinate
int ww;										///< Widget's width
int wh;										///< Widget's height

/**
 * This value will contain width of M button;
 */
int M_Button_Width = 0;

/**
  * Type of the value passed into sin/cos/tan functions
  */
tCalcMode		g_calcMode = CM_Degrees;

/**
 * Type of the numeral system which is enabled at the moment of calculations
 */
tNumSystem	g_numSystem = NS_Dec;

/**
 * Size of operand that is used in calculations
 */
tOperandSize	g_operandSize[NS_Count];

/**
 * Use FullUpdate instead of PartialUpdate on small devices like 511
 */
 int UseFullUpdate = FALSE;

/**
  * Height of the calculator's mode buttons
  */
int ModeButtonHeight = 0;

/**
  * Height of the calculator's button
  */
int DefaultButtonHeight = 0;

/**
  * Height of the text written inside of the calculator's button
  */
int DefaultButtonTextHeight = 0;

/**
  * Height of the text written inside of the calculator's result screen
  */
int ResultButtonTextHeight = 0;
int ResultButtonTextHeight2 = 0;

/**
  * Contains either '.' or ',', depending of the system settings
  */
char systemDot = 0;

/**
  * These values will hold dimensions of our calculator
  */
int calc_x = 0;
int calc_y = 0;
int calc_w = 0;
int calc_h = 0;

/**
  * List that contains all buttons
  */
tCalcButton CalcButtons[MAX_CALC_ROWS][MAX_CALC_COLS];

/**
  * Fonts that used to draw text in our calculator
  */
ifont* btnFont = NULL;
ifont* btnResultFont = NULL;
ifont* btnResult2Font = NULL;
ifont* sqrFont = NULL;
ifont* modeButtonFont = NULL;
ifont* statusFont = NULL;
ifont* headerFont = NULL;

/**
  * Change state of the button
  */
void SetActiveButton(int row, int col, int State)
{
	CalcButtons[row][col].enabled = State;
}

/**
 * Convert integer to string value in format of decimal numeral system
 */
void IntToString(char* Dst, int n)
{
	register int r, k;
	int flag = 0;

	int next = 0;

	if (n < 0)
	{
		Dst[next++] = '-';
		n = -n;
	}

	if (n == 0)
	{
		Dst[next++] = '0';
	}
	else
	{
		k = 10000;

		while (k > 0)
		{
			r = n / k;

			if (flag || r > 0)
			{
				Dst[next++] = '0' + r;
				flag = 1;
			}

			n -= r * k;
			k = k / 10;
		}
	}

	Dst[next] = 0;
}

/**
  * Removes useless zeros appended after the floating point comma
  */
void adjustResult(char* val, double origVal)
{
	if (isnan(origVal) || isinf(origVal))
	{
		sprintf(val, "%s", GetLangText("@Error"));
	}
	else
	{
		double val_ceil = ceil(origVal);
		double val_floor = floor(origVal);

		if (origVal - val_floor < CALC_EPSILON)
		{
			origVal = val_floor;
		}
		else if (val_ceil - origVal < CALC_EPSILON)
		{
			origVal = val_ceil;
		}

		if (origVal > 999999999999999999.0)
		{
			sprintf(val, "%E", origVal);
			mainOp.dotAdded = TRUE;
		}
		else
		{
			// Convert float to numeric value
			sprintf(val, "%.12f", origVal);

			// Calculate length of the ouput string
			int resLen = strlen(val);

			// Remove useless zeros
			int tmpIdx = resLen - 1;
			while (tmpIdx > 0)
			{
				if (val[tmpIdx] == '0')
				{
					val[tmpIdx] = '\0';

					tmpIdx--;
				}
				else if (val[tmpIdx] == '.')
				{
					val[tmpIdx] = '\0';

					tmpIdx--;

					break;
				}
				else
				{
					break;
				}
			}
		}

		// Find dot if it is there
		char* dotPtr = strchr(val, systemDot);

		if (dotPtr != NULL)
		{
			mainOp.dotAdded = TRUE;
		}
		else
		{
			mainOp.dotAdded = FALSE;
		}

		// Calculate length of the ouput string
		int resLen = strlen(val);

		// Make sure that string is not too long
		if (resLen > MaxNumOfChars)
		{
			if ((dotPtr != NULL) && (dotPtr - val) < ((MaxNumOfChars - 1) / 2 - 1))
			{
				val[MaxNumOfChars - 1] = '\0';

				// Find dot if it is there
				char* dotPtr = strchr(val, systemDot);

				if (dotPtr != NULL)
				{
					mainOp.dotAdded = TRUE;
				}
				else
				{
					mainOp.dotAdded = FALSE;
				}
			}
			else
			{
				// If we have no space to write this number, let's print is exponent form
				sprintf(val, "%.5E", origVal);
				mainOp.dotAdded = TRUE;
			}
		}
	}
}

/**
  * Converts string represenation of a string to the number
  */
int64_t readBin(char* binString)
{
	int64_t result = 0;

	int idx = 0;

	while(binString[idx] != '\0')
	{
		if (binString[idx] == '1')
		{
			result <<= 1;

			result |= 1;

		}
		else if (binString[idx] == '0')
		{
			result <<= 1;

			result |= 0;
		}
		else
		{
			break;
		}

		idx++;
	}

	return result;
}

/**
  * Converts value to string in form of binary number
  */
void writeBin(u_int64_t inumber, char* number)
{
	int idx = 0;
	int sidx = 0;

	// Convert number to string
	do
	{
		char ch = inumber & 0x1;

		inumber >>= 1;

		number[idx] = ch + '0';

		idx++;
	}
	while(inumber != 0);

	number[idx] = '\0';

	// Reverse order
	for (sidx = 0; sidx < idx / 2; ++sidx)
	{
		char tempch = number[sidx];
		number[sidx] = number[idx - sidx - 1];
		number[idx - sidx - 1] = tempch;
	}
}

/**
  * Read string representation ony any number and convert it to memory value
  */
void readNumber(tNumber* src, double* outf, int64_t* outi)
{
	// Read number
	if (src->numSystem == NS_Dec)
	{
		sscanf(src->number, "%lG", outf);

		*outi = *outf;
	}
	else if (src->numSystem == NS_Hex)
	{
		sscanf(src->number, "%llX", outi);
		*outf = *outi;
	}
	else if (src->numSystem == NS_Oct)
	{
		sscanf(src->number, "%llo", outi);
		*outf = *outi;
	}
	else if (src->numSystem == NS_Bin)
	{
		*outi = readBin(src->number);

		if (g_operandSize[g_numSystem] == OS_32_bits)
		{
			int32_t binVal = *outi & 0xFFFFFFFF;

			*outi = binVal;
		}
		else if (g_operandSize[g_numSystem] == OS_16_bits)
		{
			int32_t binVal = *outi & 0xFFFF;

			*outi = binVal;
		}

		*outf = *outi;
	}
}

/**
  * Store number from memory to the string value
  */
void writeNumber(tNumber* dst, double inf, int64_t ini)
{
	if (dst->numSystem == NS_Dec)
	{
		adjustResult(dst->number, inf);
	}
	else if (dst->numSystem == NS_Hex)
	{
		if (g_operandSize[g_numSystem] == OS_32_bits)
		{
			ini &= 0xFFFFFFFF;
		}
		else if (g_operandSize[g_numSystem] == OS_16_bits)
		{
			ini &= 0xFFFF;
		}

		sprintf(dst->number, "%llX", ini);
	}
	else if (dst->numSystem == NS_Oct)
	{
		if (g_operandSize[g_numSystem] == OS_32_bits)
		{
			ini &= 0xFFFFFFFF;
		}
		else if (g_operandSize[g_numSystem] == OS_16_bits)
		{
			ini &= 0xFFFF;
		}

		sprintf(dst->number, "%llo", ini);
	}
	else if (dst->numSystem == NS_Bin)
	{
		if (g_operandSize[g_numSystem] == OS_32_bits)
		{
			ini &= 0xFFFFFFFF;
		}
		else if (g_operandSize[g_numSystem] == OS_16_bits)
		{
			ini &= 0xFFFF;
		}

		writeBin(ini, dst->number);
	}

	dst->numberLen = strlen(dst->number);
	dst->valid = TRUE;
}

/**
  * This is function which is called when user changes numerical system in calculator
  */
void ChangeNumSystem(tNumber* target, tNumSystem newNumSystem)
{
	double number = 0.0;
	int64_t inumber = 0;

	readNumber(target, &number, &inumber);

	// And store the number
	target->numSystem = newNumSystem;

	writeNumber(target, number, inumber);
}

/**
  * Detect region that we must update on the screen
  */
void detectRegion(tRects* rects, tRect* updateRecr)
{
	int i = 0;

	int lesser_x = wx + ww;
	int lesser_y = wy + wh;

	int max_x = wx;
	int max_y = wx;

	for (i = 0; i < rects->count; ++i)
	{
		if (rects->rects[i].x < lesser_x)
		{
			lesser_x = rects->rects[i].x;
		}

		if (rects->rects[i].y < lesser_y)
		{
			lesser_y = rects->rects[i].y;
		}

		if (rects->rects[i].x + rects->rects[i].w > max_x)
		{
			max_x = rects->rects[i].x + rects->rects[i].w;
		}

		if (rects->rects[i].y + rects->rects[i].h > max_y)
		{
			max_y = rects->rects[i].y + rects->rects[i].h;
		}
	}

	updateRecr->x = lesser_x;
	updateRecr->y = lesser_y;

	updateRecr->w = max_x - lesser_x;
	updateRecr->h = max_y - lesser_y;
}

/**
  * Draw "M" status button
  */
void DrawMButton(int Active)
{
	if (Active == TRUE)
	{
		SetFont(statusFont, WHITE);
	}
	else
	{
		SetFont(statusFont, BLACK);
	}

	if (memOp.valid == TRUE)
	{
		DrawString(CalcButtons[0][0].x + 4, CalcButtons[0][0].y + 2, "M");
	}
	else
	{
		if (Active == TRUE)
		{
			FillArea(CalcButtons[0][0].x + 4, CalcButtons[0][0].y + 2, M_Button_Width, statusFont->height, BLACK);
		}
		else
		{
			FillArea(CalcButtons[0][0].x + 4, CalcButtons[0][0].y + 2, M_Button_Width, statusFont->height, WHITE);
		}
	}
}

/**
  * This function does actual calculation on two passed operands
  */
void oper_result(tNumber* num1, tNumber* num2, tNumber* out, int resultNumSystem, int Write2ndOp)
{
	double oper1_f = 0.0;
	double oper2_f = 0.0;

	int64_t oper1_i = 0;
	int64_t oper2_i = 0;

	// Read numbers
	readNumber(num1, &oper1_f, &oper1_i);
	readNumber(num2, &oper2_f, &oper2_i);

	// Prepare result variable
	tNumber result;
	memset(&result, 0x0, sizeof(result));

	// Process command
	if (num1->operId != BTN_none)
	{
		// Detect if user wants to calculate percents
		if (num2->operId == BTN_percent)
		{
			oper2_f = (oper1_f / 100.0) * oper2_f;
			oper2_i = oper2_f;
		}

		// 1st level commands
		if (num1->operId == BTN_plus)
		{
			oper1_f += oper2_f;
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_minus)
		{
			oper1_f = oper1_f - oper2_f;
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_div)
		{
			oper1_f /= oper2_f;
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_multiple)
		{
			oper1_f *= oper2_f;
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_percent)
		{
			oper1_f = (oper1_f / 100.0) * oper2_f;
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_x_pow_y)
		{
			oper1_f = pow(oper1_f, oper2_f);
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_logyx)
		{
			oper1_f = log(oper2_f) / log(oper1_f);
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_nth_root)
		{
			oper1_f = pow(oper2_f, 1.0 / oper1_f);
			oper1_i = oper1_f;
		}
		else if (num1->operId == BTN_and)
		{
			oper1_i = oper1_i & oper2_i;
			oper1_f = oper1_i;
		}
		else if (num1->operId == BTN_or)
		{
			oper1_i = oper1_i | oper2_i;
			oper1_f = oper1_i;
		}
		else if (num1->operId == BTN_xor)
		{
			oper1_i = oper1_i ^ oper2_i;
			oper1_f = oper1_i;
		}
		else if (num1->operId == BTN_num_mod)
		{
			if (oper2_i != 0)
			{
				oper1_i = oper1_i % oper2_i;
				oper1_f = oper1_i;
			}
			else
			{
				sprintf(result.number, "%s", GetLangText("@Error"));

				result.valid = FALSE;

				memcpy(out, &result, sizeof(result));

				return;
			}
		}
		else if (num1->operId == BTN_num_div)
		{
			if (oper2_i != 0)
			{
				oper1_i = oper1_i / oper2_i;
				oper1_f = oper1_i;
			}
			else
			{
				sprintf(result.number, "%s", GetLangText("@Error"));

				result.valid = FALSE;

				memcpy(out, &result, sizeof(result));

				return;
			}
		}
		else if (num1->operId == BTN_shl)
		{
			oper1_i = oper1_i << oper2_i;
			oper1_f = oper1_i;
		}
		else if (num1->operId == BTN_shr)
		{
			oper1_i = oper1_i >> oper2_i;
			oper1_f = oper1_i;
		}

		result.numSystem = resultNumSystem;

		writeNumber(&result, oper1_f, oper1_i);

		if (Write2ndOp)
		{
			if ((num2->operId != BTN_none) && (num2->operId != BTN_percent))
			{
				result.operId = num2->operId;
				result.operAdded = TRUE;
			}
		}
	}
	else
	{
		memcpy(&result, num2, sizeof(result));
	}

	// Generate output value
	memcpy(out, &result, sizeof(result));
}

/**
  * Clear calculator internal variables
  */
void clearCalc(int forceClear)
{
	if ((initialized == FALSE) || (forceClear == TRUE))
	{
		memset(&mainOp, 0x0, sizeof(mainOp));
		memset(&secondOp, 0x0, sizeof(secondOp));

		strcpy(mainOp.number, "0");
		mainOp.numberLen = 0;

		mainOp.dotAdded = FALSE;
		mainOp.operAdded = FALSE;

		mainOp.numSystem = g_numSystem;

		write2ndArg = FALSE;
		numWritten = FALSE;

		initialized = TRUE;
	}
}

/**
  * This function calculates factorial of the number
  */
double factorial(double param)
{
	int value = param;

	int i = 0;

	double N_factorial = 1.0;

	for (i = 1;  i <= value ;  ++i)
	{
		N_factorial *= i;
	}

	return N_factorial;
}

/**
  * Get max count of chars that we can enter
  */
int getMaxAllowedChars()
{
	int retval = 15;

	if (g_numSystem == NS_Hex)
	{
		if (g_operandSize[g_numSystem] == OS_64_bits)
			return 16;
		if (g_operandSize[g_numSystem] == OS_32_bits)
			return 8;
		if (g_operandSize[g_numSystem] == OS_16_bits)
			return 4;
	}
	else if (g_numSystem == NS_Oct)
	{
		if (g_operandSize[g_numSystem] == OS_64_bits)
			return 22;
		if (g_operandSize[g_numSystem] == OS_32_bits)
			return 11;
		if (g_operandSize[g_numSystem] == OS_16_bits)
			return 6;
	}
	else if (g_numSystem == NS_Bin)
	{
		if (g_operandSize[g_numSystem] == OS_64_bits)
			return 64;
		if (g_operandSize[g_numSystem] == OS_32_bits)
			return 32;
		if (g_operandSize[g_numSystem] == OS_16_bits)
			return 16;
	}

	return retval;
}

/**
  * In this function we calculate sizes and positions for every button in our caclulator
  */
void initCalc(int Portrait)
{
	int i = 0;
	int j = 0;

	// Adjust new dimensions for the calculator
	if (Portrait == TRUE)
	{
		g_CalcRows = DEF_CALC_ROWS - 2;
		g_CalcCols = DEF_CALC_COLS + 2;
	}
	else
	{
		g_CalcRows = DEF_CALC_ROWS;
		g_CalcCols = DEF_CALC_COLS;
	}

	// Make sure that active row and cols variables are in bounds of the table
	if ((g_row >= g_CalcRows) || (g_col >= g_CalcCols))
	{
		g_row = 3;
		g_col = 0;
	}

	// Detect rect where we can put our calculator. It should be box,
	// so let's detect length of the shortest side

	int DrawArea_x = wx + LINE_WIDTH;
	int DrawArea_y = wy + LINE_WIDTH;

	int DrawArea_w = ww - LINE_WIDTH * 2;
	int DrawArea_h = wh - LINE_WIDTH * 2;

	double deviceDimension = ((double)DrawArea_h) / ((double)DrawArea_w);
	double calcDimension = ((double)(g_CalcRows + 1)) / ((double)g_CalcCols);

	int buttonExactSideLength = 0;

	if (deviceDimension < calcDimension)
	{
		// Adjust by calc's height

		buttonExactSideLength = (DrawArea_h / (g_CalcRows + 1));
	}
	else
	{
		// Adjust by calc's width

		buttonExactSideLength = (DrawArea_w / g_CalcCols);
	}

	// Calculate are where we should put our calculator's buttons
	calc_w = buttonExactSideLength * g_CalcCols;
	calc_h = buttonExactSideLength * g_CalcRows;

	calc_x = DrawArea_x + (DrawArea_w - calc_w) / 2;
	calc_y = (DrawArea_y + buttonExactSideLength) + ((DrawArea_h - buttonExactSideLength) - calc_h) / 2;

	// Detect button's sizes
	ModeButtonHeight = buttonExactSideLength / 2 + 1;
	DefaultButtonHeight = buttonExactSideLength;
	DefaultButtonTextHeight = DefaultButtonHeight / 2.5;

	ResultButtonTextHeight = buttonExactSideLength * 0.5;
	ResultButtonTextHeight2 = buttonExactSideLength * 0.3;

	HeaderHeight = buttonExactSideLength;

	// Open fonts
	btnResultFont = OpenFont(DEFAULTFONT, ResultButtonTextHeight, 0);
	btnResult2Font = OpenFont(DEFAULTFONT, ResultButtonTextHeight2, 0);

	headerFont = OpenFont(DEFAULTFONTB, ResultButtonTextHeight, 1);
	btnFont = OpenFont(DEFAULTFONT, DefaultButtonTextHeight, 0);
	sqrFont = OpenFont(DEFAULTFONT, DefaultButtonTextHeight / 2, 0);
	modeButtonFont = OpenFont(DEFAULTFONT, DefaultButtonTextHeight / 2, 1);
	statusFont = OpenFont(DEFAULTFONTB, ResultButtonTextHeight / 2 + 1, 0);

	// Detect length of M button
	SetFont(statusFont, BLACK);

	M_Button_Width = CharWidth('M');

	// Detect max count of number that can be written on the screen
	SetFont(btnResultFont, BLACK);

	// Max count of chars that user can type and see on the screen
	Result2Font_MaxChars = 32;

	// Detect max count of chars that can be displayed using default font
	ResultFont_MaxChars = (calc_w - 7) / StringWidth("0") - 3;

	if (ResultFont_MaxChars > MAX_COUNT_OF_DIGITS)
	{
		ResultFont_MaxChars = MAX_COUNT_OF_DIGITS;
	}

	// Mac count of chars that user can enter
	MaxNumOfChars = ResultFont_MaxChars < MAX_NUMBER_OF_CHARS ? ResultFont_MaxChars : MAX_NUMBER_OF_CHARS;

	char dotMsg[10];
	sprintf(dotMsg, "%0.1f", 1.1);
	systemDot = dotMsg[1];

	// Initialize buttons

	// This is button containing results of calculations
	CalcButtons[0][0].enabled = TRUE;
	CalcButtons[0][0].x = calc_x;
	CalcButtons[0][0].y = calc_y;
	CalcButtons[0][0].w = DefaultButtonHeight * g_CalcCols;
	CalcButtons[0][0].h = DefaultButtonHeight;

	// Setup buttons which will not be shown on the screen
	for (i = 1; i < g_CalcCols; ++i)
	{
		CalcButtons[0][i].enabled = FALSE;
	}

	// Put text into the buttons

	int nextRow = 0;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "0");
	CalcButtons[nextRow][0].type = BTN_screen;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, GetLangText("@Dec"));
	CalcButtons[nextRow][0].type = BTN_dec;
	CalcButtons[nextRow][0].ModeButton = TRUE;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][1].text, GetLangText("@Hex"));
	CalcButtons[nextRow][1].type = BTN_hex;
	CalcButtons[nextRow][1].ModeButton = TRUE;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][2].text, GetLangText("@Oct"));
	CalcButtons[nextRow][2].type = BTN_oct;
	CalcButtons[nextRow][2].ModeButton = TRUE;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][3].text, GetLangText("@Bin"));
	CalcButtons[nextRow][3].type = BTN_bin;
	CalcButtons[nextRow][3].ModeButton = TRUE;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, GetLangText("@Degrees"));
	CalcButtons[nextRow][4].type = BTN_degrees;
	CalcButtons[nextRow][4].ModeButton = TRUE;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	CalcButtons[nextRow][4].MultiFuncButton = TRUE;

	strcpy(CalcButtons[nextRow][4].funcs[0].text, GetLangText("@Degrees"));
	CalcButtons[nextRow][4].funcs[0].type = BTN_degrees;
	strcpy(CalcButtons[nextRow][4].funcs[1].text, GetLangText("@16-bit"));
	CalcButtons[nextRow][4].funcs[1].type = BTN_16_bit;
	strcpy(CalcButtons[nextRow][4].funcs[2].text, GetLangText("@16-bit"));
	CalcButtons[nextRow][4].funcs[2].type = BTN_16_bit;
	strcpy(CalcButtons[nextRow][4].funcs[3].text, GetLangText("@16-bit"));
	CalcButtons[nextRow][4].funcs[3].type = BTN_16_bit;

	strcpy(CalcButtons[nextRow][5].text, GetLangText("@Radians"));
	CalcButtons[nextRow][5].type = BTN_radians;
	CalcButtons[nextRow][5].ModeButton = TRUE;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	CalcButtons[nextRow][5].MultiFuncButton = TRUE;

	strcpy(CalcButtons[nextRow][5].funcs[0].text, GetLangText("@Radians"));
	CalcButtons[nextRow][5].funcs[0].type = BTN_degrees;
	strcpy(CalcButtons[nextRow][5].funcs[1].text, GetLangText("@32-bit"));
	CalcButtons[nextRow][5].funcs[1].type = BTN_32_bit;
	strcpy(CalcButtons[nextRow][5].funcs[2].text, GetLangText("@32-bit"));
	CalcButtons[nextRow][5].funcs[2].type = BTN_32_bit;
	strcpy(CalcButtons[nextRow][5].funcs[3].text, GetLangText("@32-bit"));
	CalcButtons[nextRow][5].funcs[3].type = BTN_32_bit;

	strcpy(CalcButtons[nextRow][6].text, GetLangText("@Grads"));
	CalcButtons[nextRow][6].type = BTN_grads;
	CalcButtons[nextRow][6].ModeButton = TRUE;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	CalcButtons[nextRow][6].MultiFuncButton = TRUE;

	strcpy(CalcButtons[nextRow][6].funcs[0].text, GetLangText("@Grads"));
	CalcButtons[nextRow][6].funcs[0].type = BTN_degrees;
	strcpy(CalcButtons[nextRow][6].funcs[1].text, GetLangText("@64-bit"));
	CalcButtons[nextRow][6].funcs[1].type = BTN_64_bit;
	strcpy(CalcButtons[nextRow][6].funcs[2].text, GetLangText("@64-bit"));
	CalcButtons[nextRow][6].funcs[2].type = BTN_64_bit;
	strcpy(CalcButtons[nextRow][6].funcs[3].text, GetLangText("@64-bit"));
	CalcButtons[nextRow][6].funcs[3].type = BTN_64_bit;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "OFF");
	CalcButtons[nextRow][0].type = BTN_off;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][1].text, "M+");
	CalcButtons[nextRow][1].type = BTN_memplus;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][2].text, "MR");
	CalcButtons[nextRow][2].type = BTN_memread;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][3].text, "MS");
	CalcButtons[nextRow][3].type = BTN_memstore;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, "MC");
	CalcButtons[nextRow][4].type = BTN_memclear;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][5].text, "<-");
	CalcButtons[nextRow][5].type = BTN_back;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][6].text, "C");
	CalcButtons[nextRow][6].type = BTN_clear;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "7");
	CalcButtons[nextRow][0].type = BTN_7;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][1].text, "8");
	CalcButtons[nextRow][1].type = BTN_8;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec | BS_Hex;

	strcpy(CalcButtons[nextRow][2].text, "9");
	CalcButtons[nextRow][2].type = BTN_9;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex;

	strcpy(CalcButtons[nextRow][3].text, "x");
	CalcButtons[nextRow][3].type = BTN_multiple;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, "%");
	CalcButtons[nextRow][4].type = BTN_percent;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][5].text, "sin");
	CalcButtons[nextRow][5].type = BTN_sin;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][6].text, "sinh");
	CalcButtons[nextRow][6].type = BTN_sinh;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "4");
	CalcButtons[nextRow][0].type = BTN_4;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][1].text, "5");
	CalcButtons[nextRow][1].type = BTN_5;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][2].text, "6");
	CalcButtons[nextRow][2].type = BTN_6;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][3].text, "/");
	CalcButtons[nextRow][3].type = BTN_div;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, "+/-");
	CalcButtons[nextRow][4].type = BTN_plusminus;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][5].text, "cos");
	CalcButtons[nextRow][5].type = BTN_cos;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][6].text, "cosh");
	CalcButtons[nextRow][6].type = BTN_cosh;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "1");
	CalcButtons[nextRow][0].type = BTN_1;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][1].text, "2");
	CalcButtons[nextRow][1].type = BTN_2;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][2].text, "3");
	CalcButtons[nextRow][2].type = BTN_3;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex | BS_Oct;

	strcpy(CalcButtons[nextRow][3].text, "+");
	CalcButtons[nextRow][3].type = BTN_plus;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, "sqr");
	CalcButtons[nextRow][4].type = BTN_sqr;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][5].text, "tan");
	CalcButtons[nextRow][5].type = BTN_tan;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][6].text, "tanh");
	CalcButtons[nextRow][6].type = BTN_tanh;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "0");
	CalcButtons[nextRow][0].type = BTN_0;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][1].text, ".");
	CalcButtons[nextRow][1].type = BTN_dot;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][2].text, "=");
	CalcButtons[nextRow][2].type = BTN_result;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][3].text, "-");
	CalcButtons[nextRow][3].type = BTN_minus;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][4].text, "X^Y");
	CalcButtons[nextRow][4].type = BTN_x_pow_y;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	strcpy(CalcButtons[nextRow][5].text, "lg");
	CalcButtons[nextRow][5].type = BTN_lg;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][6].text, "ln");
	CalcButtons[nextRow][6].type = BTN_ln;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec;

	nextRow++;

	// buttons line
	strcpy(CalcButtons[nextRow][0].text, "pi");
	CalcButtons[nextRow][0].type = BTN_pi;
	CalcButtons[nextRow][0].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][1].text, "e");
	CalcButtons[nextRow][1].type = BTN_e;
	CalcButtons[nextRow][1].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][2].text, "1/x");
	CalcButtons[nextRow][2].type = BTN_1_div_x;
	CalcButtons[nextRow][2].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][3].text, "|x|");
	CalcButtons[nextRow][3].type = BTN_abs;
	CalcButtons[nextRow][3].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][4].text, "sqrt");
	CalcButtons[nextRow][4].type = BTN_sqrt;
	CalcButtons[nextRow][4].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][5].text, "root");
	CalcButtons[nextRow][5].type = BTN_nth_root;
	CalcButtons[nextRow][5].ButtonNumState = BS_Dec;

	strcpy(CalcButtons[nextRow][6].text, "logyx");
	CalcButtons[nextRow][6].type = BTN_logyx;
	CalcButtons[nextRow][6].ButtonNumState = BS_Dec;

	nextRow++;

	// buttons line
	int nextCol = 0;

	int* pRow = &nextRow;
	int* pCol = &nextCol;

	if (Portrait == TRUE)
	{
		nextCol = 1;

		pCol = &nextRow;
		pRow = &nextCol;

		(*pCol)--;
	}

	if (Portrait == FALSE)
	{
		strcpy(CalcButtons[*pRow][*pCol].text, "A");
		CalcButtons[*pRow][*pCol].type = BTN_A;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "B");
		CalcButtons[*pRow][*pCol].type = BTN_B;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "C");
		CalcButtons[*pRow][*pCol].type = BTN_C;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;
	}

	strcpy(CalcButtons[*pRow][*pCol].text, "Or");
	CalcButtons[*pRow][*pCol].type = BTN_or;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "And");
	CalcButtons[*pRow][*pCol].type = BTN_and;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "Xor");
	CalcButtons[*pRow][*pCol].type = BTN_xor;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "Not");
	CalcButtons[*pRow][*pCol].type = BTN_not;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	if (Portrait == TRUE)
	{
		strcpy(CalcButtons[*pRow][*pCol].text, "A");
		CalcButtons[*pRow][*pCol].type = BTN_A;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "B");
		CalcButtons[*pRow][*pCol].type = BTN_B;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "C");
		CalcButtons[*pRow][*pCol].type = BTN_C;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;
	}

	nextRow++;

	if (Portrait == TRUE)
	{
		nextCol = 1;
	}
	else
	{
		nextCol = 0;
	}

	// buttons line
	if (Portrait == FALSE)
	{
		strcpy(CalcButtons[*pRow][*pCol].text, "D");
		CalcButtons[*pRow][*pCol].type = BTN_D;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "E");
		CalcButtons[*pRow][*pCol].type = BTN_E;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "F");
		CalcButtons[*pRow][*pCol].type = BTN_F;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;
	}

	strcpy(CalcButtons[*pRow][*pCol].text, "Mod");
	CalcButtons[*pRow][*pCol].type = BTN_num_mod;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "Div");
	CalcButtons[*pRow][*pCol].type = BTN_num_div;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "Shl");
	CalcButtons[*pRow][*pCol].type = BTN_shl;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	strcpy(CalcButtons[*pRow][*pCol].text, "Shr");
	CalcButtons[*pRow][*pCol].type = BTN_shr;
	CalcButtons[*pRow][*pCol].ButtonNumState = BS_Dec | BS_Hex | BS_Oct | BS_Bin;

	nextCol++;

	if (Portrait == TRUE)
	{
		strcpy(CalcButtons[*pRow][*pCol].text, "D");
		CalcButtons[*pRow][*pCol].type = BTN_D;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "E");
		CalcButtons[*pRow][*pCol].type = BTN_E;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;

		strcpy(CalcButtons[*pRow][*pCol].text, "F");
		CalcButtons[*pRow][*pCol].type = BTN_F;
		CalcButtons[*pRow][*pCol].ButtonNumState = BS_Hex;

		nextCol++;
	}

	nextRow++;

	// Setup sizes and positions small buttons
	for (i = 1; i < g_CalcRows; ++i)
	{
		for (j  = 0; j < g_CalcCols; ++j)
		{
			if (CalcButtons[i][j].ModeButton == FALSE)
			{
				CalcButtons[i][j].enabled = TRUE;

				CalcButtons[i][j].x = calc_x + DefaultButtonHeight * j;
				CalcButtons[i][j].y = calc_y + DefaultButtonHeight * i;

				CalcButtons[i][j].w = DefaultButtonHeight;
				CalcButtons[i][j].h = DefaultButtonHeight;
			}
			else
			{
				CalcButtons[i][j].enabled = TRUE;

				CalcButtons[i][j].x = calc_x + DefaultButtonHeight * j;
				CalcButtons[i][j].y = calc_y + DefaultButtonHeight * i + ModeButtonHeight / 2;

				CalcButtons[i][j].w = DefaultButtonHeight;
				CalcButtons[i][j].h = ModeButtonHeight;
			}
		}
	}
}

/**
  * Gets string representation of the passed operator
  */
char* getStrOp(int operId)
{
	char* op_value_str = "+";

	if (operId == BTN_plus)
		op_value_str = "+";
	else if (operId == BTN_minus)
		op_value_str = "-";
	else if (operId == BTN_multiple)
		op_value_str = "*";
	else if (operId == BTN_div)
		op_value_str = "/";
	else if (operId == BTN_percent)
		op_value_str = "%";
	else if (operId == BTN_x_pow_y)
		op_value_str = "^";
	else if (operId == BTN_logyx)
		op_value_str = "log";
	else if (operId == BTN_and)
		op_value_str = "and";
	else if (operId == BTN_xor)
		op_value_str = "xor";
	else if (operId == BTN_not)
		op_value_str = "not";
	else if (operId == BTN_num_mod)
		op_value_str = "mod";
	else if (operId == BTN_num_div)
		op_value_str = "div";
	else if (operId == BTN_shl)
		op_value_str = "shl";
	else if (operId == BTN_shr)
		op_value_str = "shr";
	else if (operId == BTN_nth_root)
		op_value_str = "rt";
	else if (operId == BTN_or)
		op_value_str = "or";

	return op_value_str;
}

/**
 * Draw string
 */
void DrawFixedString(int x, int y, char* str, int len, int chWidth)
{
	int i = 0;

	char tStr[2] = {0x0, 0x0};

	for (;i < len; ++i)
	{
		tStr[0] = str[i];

		DrawString(x + i*chWidth, y, tStr);
	}
}

/**
  * Draw the button
  */
void DrawButton(int i, int j, int Active)
{
	if ((i == 0) && (j == 0))
	{
		Active = FALSE;
	}

	// Draw button
	if (CalcButtons[i][j].enabled == TRUE)
	{
		int ItemColor = BLACK;
		int BackColor = WHITE;

		// Write gray text on non valid buttons
		if (!(CalcButtons[i][j].ButtonNumState & (1 << g_numSystem)))
		{
			if (Active == TRUE)
			{
				ItemColor = LGRAY;
				BackColor = BLACK;
			}
			else
			{
				ItemColor = LGRAY;
				BackColor = WHITE;
			}
		}
		else
		{
			// Draw contents of the buttons
			if (Active == TRUE)
			{
				ItemColor = WHITE;
				BackColor = BLACK;
			}
		}

		int DrawStatusButton = FALSE;

		// Draw small rect inside of another
		if (CalcButtons[i][j].ModeButton == TRUE)
		{
			if (CalcButtons[i][j].MultiFuncButton == TRUE)
			{
				int activeFunc = g_numSystem;

				if (CalcButtons[i][j].funcs[activeFunc].ModeButtonActive == TRUE)
				{
					DrawStatusButton = TRUE;
				}
			}
			else
			{
				if (CalcButtons[i][j].ModeButtonActive == TRUE)
				{
					DrawStatusButton = TRUE;
				}
			}
		}

		if (DrawStatusButton == TRUE)
		{
			// Draw white box
			FillArea(CalcButtons[i][j].x, CalcButtons[i][j].y, CalcButtons[i][j].w, CalcButtons[i][j].h, BackColor);

			// Draw regions
			FillArea(CalcButtons[i][j].x + 1, CalcButtons[i][j].y + 1, CalcButtons[i][j].w - 2, CalcButtons[i][j].h - 2, ItemColor);

			// Draw white box
			FillArea(CalcButtons[i][j].x + 4, CalcButtons[i][j].y + 4, CalcButtons[i][j].w - 8, CalcButtons[i][j].h - 8, BackColor);
		}
		else
		{
			// Draw white box
			FillArea(CalcButtons[i][j].x, CalcButtons[i][j].y, CalcButtons[i][j].w, CalcButtons[i][j].h, BackColor);

			// Draw regions
			DrawRect(CalcButtons[i][j].x + 1, CalcButtons[i][j].y + 1, CalcButtons[i][j].w - 2, CalcButtons[i][j].h - 2, ItemColor);
		}

		// Set font
		ifont* currentFont = btnFont;

		// Adjsut font for 1st line of buttons
		if (CalcButtons[i][j].ModeButton == TRUE)
		{
			currentFont = modeButtonFont;
		}

		if (CalcButtons[i][j].type == BTN_screen)
		{
			// Draw "M" button if it exists
			DrawMButton(FALSE);

			// Draw results
			if (mainOp.numberLen < ResultFont_MaxChars)
			{
				currentFont = btnResultFont;

				SetFont(currentFont, ItemColor);

				char resultText[MAX_COUNT_OF_DIGITS * 3];

				strcpy(resultText, mainOp.number);

				if (mainOp.operAdded == TRUE)
				{
					strcat(resultText, getStrOp(mainOp.operId));
				}

				// Draw string
				DrawString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - StringWidth(resultText) - 4),
						CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
						resultText
						);
			}
			else if (mainOp.numberLen <= Result2Font_MaxChars)
			{
				currentFont = btnResult2Font;

				SetFont(currentFont, ItemColor);

				char resultText[MAX_COUNT_OF_DIGITS * 3];

				strcpy(resultText, mainOp.number);

				if (mainOp.operAdded == TRUE)
				{
					strcat(resultText, getStrOp(mainOp.operId));
				}

				// Draw string
				DrawString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - StringWidth(resultText) - 4),
						CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
						resultText
						);
			}
			else
			{
				// Draw two lines

				currentFont = btnResult2Font;

				SetFont(currentFont, ItemColor);

				char line1[MAX_COUNT_OF_DIGITS * 3];
				char line2[MAX_COUNT_OF_DIGITS * 3];

				memset(line1, 0x0, sizeof(line1));
				memset(line2, 0x0, sizeof(line2));

				int line2_len = mainOp.numberLen - Result2Font_MaxChars;
				int line1_len = Result2Font_MaxChars;

				strncpy(line2, mainOp.number, line2_len);
				strncpy(line1, mainOp.number + (mainOp.numberLen - Result2Font_MaxChars), line1_len);

				int shift_x = 0;

				if (mainOp.operAdded == TRUE)
				{
					char* strOp = getStrOp(mainOp.operId);

					shift_x = StringWidth(strOp);

					// Draw line1
					DrawString(
							CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - StringWidth(strOp) - 4),
							CalcButtons[i][j].y + (CalcButtons[i][j].h - 1) / 2 + ((CalcButtons[i][j].h - 1) / 2- currentFont->height) / 2,
							strOp
							);
				}

				// Get width of the largest char
				int defCharWidth = CharWidth('0');

				// Draw help message
				if (g_numSystem == NS_Bin)
				{
					DrawString(
							CalcButtons[i][j].x + M_Button_Width + 4,
							CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) / 2 - currentFont->height) / 2,
							"63..32:"
							);
				}

				// Draw line1
				DrawFixedString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - defCharWidth * line2_len - 4) - shift_x,
						CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) / 2 - currentFont->height) / 2,
						line2,
						line2_len,
						defCharWidth
						);

				if (g_numSystem == NS_Bin)
				{
					// Draw help message
					DrawString(
							CalcButtons[i][j].x + M_Button_Width + 4,
							CalcButtons[i][j].y + (CalcButtons[i][j].h - 1) / 2 + ((CalcButtons[i][j].h - 1) / 2- currentFont->height) / 2,
							"31..0:"
							);
				}

				// Draw line2
				DrawFixedString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - defCharWidth * line1_len - 4) - shift_x,
						CalcButtons[i][j].y + (CalcButtons[i][j].h - 1) / 2 + ((CalcButtons[i][j].h - 1) / 2- currentFont->height) / 2,
						line1,
						line1_len,
						defCharWidth
						);
			}
		}
		else if (CalcButtons[i][j].type == BTN_back)
		{
			int x = CalcButtons[i][j].x;
			int y = CalcButtons[i][j].y;
			int w = CalcButtons[i][j].w - 1;
			int h = CalcButtons[i][j].h - 1;

			int dim = w / 6;

			DrawLine(x + dim * 1.5, y + h / 2, x + dim * 4.5, y + h / 2, ItemColor);
			DrawLine(x + dim * 1.5 + 1, y + h / 2 + 1, x + dim * 4.5, y + h / 2 + 1, ItemColor);

			DrawLine(x + dim * 1.5, y + h / 2, x + dim * 2.5, y + h / 2 - dim, ItemColor);
			DrawLine(x + dim * 1.5 + 1, y + h / 2, x + dim * 2.5 + 1, y + h / 2 - dim, ItemColor);

			DrawLine(x + dim * 1.5, y + h / 2, x + dim * 2.5, y + h / 2 + dim, ItemColor);
			DrawLine(x + dim * 1.5 + 1, y + h / 2, x + dim * 2.5 + 1, y + h / 2 + dim, ItemColor);
		}
		else if (CalcButtons[i][j].type == BTN_sqr)
		{
			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			char* str_2 = "2";
			int str_2_width = CharWidth(str_2[0]);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			char* str_1 = "x";
			int str_1_width = CharWidth(str_1[0]);

			int totalStringWidth = str_1_width + str_2_width;

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_1
					);

			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 - (currentFont->height / 4),
					str_2
					);
		}
		else if (CalcButtons[i][j].type == BTN_x_pow_y)
		{
			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			char* str_2 = "y";
			int str_2_width = CharWidth(str_2[0]);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			char* str_1 = "x";
			int str_1_width = CharWidth(str_1[0]);

			int totalStringWidth = str_1_width + str_2_width;

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_1
					);

			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 - (currentFont->height / 4),
					str_2
					);
		}
		else if (CalcButtons[i][j].type == BTN_nth_root)
		{
			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			char* str_2 = "y";
			int str_2_width = StringWidth(str_2);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			char* str_1 = "H";
			int str_1_width = StringWidth(str_1);

			char* str_3 = "x";
			int str_3_width = StringWidth(str_3);

			int totalStringWidth = str_1_width + str_2_width + str_3_width;

			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 - (currentFont->height / 4),
					str_2
					);

			//int x = CalcButtons[i][j].x;
			int y = CalcButtons[i][j].y;
			int w = CalcButtons[i][j].w - 1;
			int h = CalcButtons[i][j].h - 1;

			int y_x = CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2;
			int y_y = CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 - (currentFont->height / 4);
			//int y_w = str_2_width;
			int y_h = currentFont->height;

			int dim = w / 6;

			DrawLine(y_x, y_y + y_h + 1, y_x + str_2_width, y_y + y_h + 1, ItemColor);
			DrawLine(y_x + str_2_width, y_y + y_h + 1, y_x + str_2_width + str_1_width / 2.5, y + h - dim, ItemColor);
			DrawLine(y_x + str_2_width + str_1_width / 2.5, y + h - dim, y_x + str_2_width + str_1_width, y + (dim * 1.5), ItemColor);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width + str_2_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_3
					);
		}
		else if (CalcButtons[i][j].type == BTN_sqrt)
		{
			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			char* str_2 = "y";
			int str_2_width = StringWidth(str_2);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			char* str_1 = "H";
			int str_1_width = StringWidth(str_1);

			char* str_3 = "x";
			int str_3_width = StringWidth(str_3);

			int totalStringWidth = str_1_width + str_2_width + str_3_width;

			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			//int x = CalcButtons[i][j].x;
			int y = CalcButtons[i][j].y;
			int w = CalcButtons[i][j].w - 1;
			int h = CalcButtons[i][j].h - 1;

			int y_x = CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2;
			int y_y = CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 - (currentFont->height / 4);
			//int y_w = str_2_width;
			int y_h = currentFont->height;

			int dim = w / 6;

			DrawLine(y_x, y_y + y_h + 1, y_x + str_2_width, y_y + y_h + 1, ItemColor);
			DrawLine(y_x + str_2_width, y_y + y_h + 1, y_x + str_2_width + str_1_width / 2.5, y + h - dim, ItemColor);
			DrawLine(y_x + str_2_width + str_1_width / 2.5, y + h - dim, y_x + str_2_width + str_1_width, y + (dim * 1.5), ItemColor);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width + str_2_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_3
					);
		}
		else if (CalcButtons[i][j].type == BTN_logyx)
		{
			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			char* str_2 = "y";
			int str_2_width = StringWidth(str_2);

			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			char* str_1 = "log";
			int str_1_width = StringWidth(str_1);

			char* str_3 = "x";
			int str_3_width = StringWidth(str_3);

			int totalStringWidth = str_1_width + str_2_width + str_3_width;

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_1
					);

			currentFont = sqrFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2 + (currentFont->height / 4),
					str_2
					);
				
			currentFont = btnFont;
			SetFont(currentFont, ItemColor);

			DrawString(
					CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - totalStringWidth) / 2 + str_1_width + str_2_width,
					CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
					str_3
					);
		}
		else
		{
			SetFont(currentFont, ItemColor);

			if (CalcButtons[i][j].MultiFuncButton == FALSE)
			{
				// Draw string
				DrawString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - StringWidth(CalcButtons[i][j].text)) / 2,
						CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
						CalcButtons[i][j].text
						);
			}
			else
			{
				int activeFunc = g_numSystem;

				// Draw string
				DrawString(
						CalcButtons[i][j].x + ((CalcButtons[i][j].w - 1) - StringWidth(CalcButtons[i][j].funcs[activeFunc].text)) / 2,
						CalcButtons[i][j].y + ((CalcButtons[i][j].h - 1) - currentFont->height) / 2,
						CalcButtons[i][j].funcs[activeFunc].text
						);
			}
		}
	}
}

/**
  * The purpose of this functions is to draw calculator on the screen
  */
void Draw(int activeBtnRow, int activeBtnCol)
{
	int i = 0;
	int j = 0;

	// Draw buttons onto the screen
	for (i  = 0; i < g_CalcRows; ++i)
	{
		for (j  = 0; j < g_CalcCols; ++j)
		{
			if ((activeBtnRow == i) && (activeBtnCol == j) )
			{
				DrawButton(i, j, TRUE);
			}
			else
			{
				DrawButton(i, j, FALSE);
			}
		}
	}

	// Draw header
	SetFont(headerFont, WHITE);
	FillArea(0, 0, ww, HeaderHeight, BLACK);
	DrawTextRect(0, 0, ww, HeaderHeight, GetLangText("@Calculator"), ALIGN_CENTER | VALIGN_MIDDLE | DOTS | RTLAUTO);
}

/**
  * Inverts buttons area
  */
void InvertButton(int row, int col, int Active)
{
	{
		InvertAreaBW(CalcButtons[row][col].x, CalcButtons[row][col].y, CalcButtons[row][col].w, CalcButtons[row][col].h);
	}
}

/**
  * This function does all calculation stuff
  */
void HandleCalc(int Type, int ButtonNumState, int* StatusUpdated)
{
	// Check first if we are able to process this button
	if (!(ButtonNumState & (1 << g_numSystem)))
	{
		return;
	}

	// Detect type of the button that was pressed
	if (
		((Type >= BTN_0) && (Type <= BTN_9)) ||
		((Type >= BTN_A) && (Type <= BTN_F)) ||
		(Type == BTN_dot)
		)
	{
		if (write2ndArg == TRUE)
		{
			memset(&mainOp, 0x0, sizeof(mainOp));

			strcpy(mainOp.number, "0");
			mainOp.numberLen = 0;

			mainOp.dotAdded = FALSE;
			mainOp.operAdded = FALSE;

			mainOp.numSystem = g_numSystem;

			write2ndArg = FALSE;
		}
	}

	// Process calculator messages
	switch (Type)
	{
		case BTN_dec:
		case BTN_hex:
		case BTN_oct:
		case BTN_bin:
		case BTN_grads:
		case BTN_radians:
		case BTN_degrees:
		case BTN_16_bit:
		case BTN_32_bit:
		case BTN_64_bit:
		{
			// Set new calc buttons
			int col_idx = 0;

			int startCol = 0;
			int endCol = 0;

			if (g_col >= 4)
			{
				startCol = 4;
				endCol = g_CalcCols;
			}
			else
			{
				startCol = 0;
				endCol = 4;
			}

			for (col_idx = startCol; col_idx < endCol; ++col_idx)
			{
				if (CalcButtons[1][col_idx].MultiFuncButton == TRUE)
				{
					if (CalcButtons[1][col_idx].funcs[g_numSystem].ModeButtonActive == TRUE)
					{
						CalcButtons[1][col_idx].funcs[g_numSystem].ModeButtonActive = FALSE;

						*StatusUpdated = TRUE;
					}
				}
				else
				{
					if (CalcButtons[1][col_idx].ModeButtonActive == TRUE)
					{
						CalcButtons[1][col_idx].ModeButtonActive = FALSE;

						*StatusUpdated = TRUE;
					}
				}
			}

			// Clear old calc buttons
			if (CalcButtons[g_row][g_col].MultiFuncButton == TRUE)
			{
				if (g_col >= 4)
				{
					CalcButtons[g_row][g_col].funcs[g_numSystem].ModeButtonActive = TRUE;

					*StatusUpdated = TRUE;

					if (g_numSystem == NS_Dec)
					{
						g_calcMode = CM_Degrees + (g_col - 4);
					}
					else
					{
						g_operandSize[g_numSystem] = OS_16_bits + (g_col - 4);

						ChangeNumSystem(&mainOp, g_numSystem);
					}
				}
				else
				{
					CalcButtons[1][g_col].funcs[g_numSystem].ModeButtonActive = TRUE;

					*StatusUpdated = TRUE;

					g_numSystem = NS_Dec + (g_col);

					ChangeNumSystem(&mainOp, g_numSystem);
				}
			}
			else
			{
				if (g_col >= 4)
				{
					CalcButtons[g_row][g_col].ModeButtonActive = TRUE;

					*StatusUpdated = TRUE;

					g_calcMode = CM_Degrees + (g_col - 4);

					ChangeNumSystem(&mainOp, g_numSystem);
				}
				else
				{
					CalcButtons[1][g_col].ModeButtonActive = TRUE;

					*StatusUpdated = TRUE;

					g_numSystem = NS_Dec + (g_col);

					ChangeNumSystem(&mainOp, g_numSystem);
				}
			}

			if ((*StatusUpdated))
			{
				Draw(g_row, g_col);
			}
		}
		break;

		case BTN_off:

			CloseApp();

		break;

		case BTN_0:
		case BTN_1:
		case BTN_2:
		case BTN_3:
		case BTN_4:
		case BTN_5:
		case BTN_6:
		case BTN_7:
		case BTN_8:
		case BTN_9:

			if (mainOp.numberLen < (getMaxAllowedChars()))
			{
				if ((mainOp.number[0] == '0') && (mainOp.number[1] == '\0'))
				{
					mainOp.numberLen = 0;
				}

				mainOp.number[mainOp.numberLen++] = '0' + (Type - BTN_0);
				mainOp.number[mainOp.numberLen] = '\0';

				numWritten = TRUE;
			}

		break;

		case BTN_A:
		case BTN_B:
		case BTN_C:
		case BTN_D:
		case BTN_E:
		case BTN_F:

			if (mainOp.numberLen < (getMaxAllowedChars()))
			{
				if ((mainOp.number[0] == '0') && (mainOp.number[1] == '\0'))
				{
					mainOp.numberLen = 0;
				}

				mainOp.number[mainOp.numberLen++] = 'A' + (Type - BTN_A);
				mainOp.number[mainOp.numberLen] = '\0';

				numWritten = TRUE;
			}

		break;

		case BTN_clear:

			clearCalc(TRUE);

		break;

		case BTN_allclear:

			clearCalc(TRUE);

		break;

		case BTN_back:

			if (mainOp.operAdded == TRUE)
			{
				mainOp.operId = BTN_none;
				mainOp.operAdded = FALSE;
			}
			else
			{
				if (mainOp.numberLen > 1)
				{
					if (mainOp.number[mainOp.numberLen - 1] == systemDot)
					{
						mainOp.dotAdded = FALSE;
					}

					mainOp.numberLen--;
					mainOp.number[mainOp.numberLen] = '\0';
				}
				else if (mainOp.numberLen == 1)
				{
					mainOp.number[0] = '0';
					mainOp.number[1] = '\0';
				}
			}

		break;

		case BTN_pi:

			if (mainOp.numberLen < (MaxNumOfChars - 1))
			{
				strcpy(mainOp.number, CALC_PI);

				mainOp.numberLen = strlen(mainOp.number);
				mainOp.numSystem = NS_Dec;

				mainOp.number[1] = systemDot;
				mainOp.dotAdded = TRUE;

				mainOp.operAdded = FALSE;
				mainOp.operId = BTN_none;

				numWritten = TRUE;
			}

		break;

		case BTN_e:

			if (mainOp.numberLen < (MaxNumOfChars - 1))
			{
				strcpy(mainOp.number, CALC_E);

				mainOp.numberLen = strlen(mainOp.number);
				mainOp.numSystem = NS_Dec;

				mainOp.number[1] = systemDot;
				mainOp.dotAdded = TRUE;

				mainOp.operAdded = FALSE;
				mainOp.operId = BTN_none;

				numWritten = TRUE;
			}

		break;

		case BTN_memread:

			if (memOp.valid == TRUE)
			{
				mainOp = memOp;

				mainOp.operAdded = FALSE;
				mainOp.operId = BTN_none;

				numWritten = TRUE;

				ChangeNumSystem(&mainOp, g_numSystem);
			}

		break;

		case BTN_memclear:

			if (memOp.valid == TRUE)
			{
				memset(&memOp, 0x0, sizeof(memOp));
				strcpy(memOp.number, "0");

				memOp.valid = FALSE;
			}

			// Redraw M button on the screen
			if ((g_col == 0) && (g_row == 0))
			{
				DrawMButton(TRUE);
			}
			else
			{
				DrawMButton(FALSE);
			}

		break;

		case BTN_memplus:

			// We should not add zero to memory
			if (strncmp(mainOp.number, "0", 2))
			{
				memOp.operAdded = TRUE;
				memOp.operId = BTN_plus;

				oper_result(&memOp, &mainOp, &memOp, memOp.numSystem, FALSE);

				memOp.valid = TRUE;
			}

			// Redraw M button on the screen
			if ((g_col == 0) && (g_row == 0))
			{
				DrawMButton(TRUE);
			}
			else
			{
				DrawMButton(FALSE);
			}

		break;

		case BTN_memstore:

			if (!strncmp(mainOp.number, "0", 2))
			{
				// Clear memory if user has pressed MS button with zero in buffer
				memset(&memOp, 0x0, sizeof(memOp));
				strcpy(memOp.number, "0");

				memOp.valid = FALSE;
			}
			else
			{
				memOp = mainOp;
				memOp.valid = TRUE;
			}

			// Redraw M button on the screen
			if ((g_col == 0) && (g_row == 0))
			{
				DrawMButton(TRUE);
			}
			else
			{
				DrawMButton(FALSE);
			}

		break;

		case BTN_multiple:
		case BTN_minus:
		case BTN_plus:
		case BTN_div:
		case BTN_x_pow_y:
		case BTN_logyx:
		case BTN_nth_root:
		case BTN_or:
		case BTN_xor:
		case BTN_and:
		case BTN_num_mod:
		case BTN_num_div:
		case BTN_shl:
		case BTN_shr:
		case BTN_percent:
			{
				// Check if there is some number
				if (mainOp.numberLen == 0)
				{
					strcpy(mainOp.number, "0");
					mainOp.numberLen = 1;
					mainOp.valid = TRUE;
				}

				// Add new operator
				mainOp.operId = Type;
				mainOp.operAdded = TRUE;

				// Make calculations
				if ((secondOp.valid == FALSE) || (numWritten == FALSE))
				{
					// Check if this is a percent
					if (Type == BTN_percent)
					{
						double oper_f = 0.0;
						int64_t oper_i = 0;

						// Read numbers
						readNumber(&mainOp, &oper_f, &oper_i);

						oper_f = oper_f / 100.0;
						oper_i = oper_f;

						writeNumber(&mainOp, oper_f, oper_i);

						mainOp.operAdded = FALSE;
					}

					secondOp = mainOp;
					secondOp.valid = TRUE;
				}
				else
				{
					oper_result(&secondOp, &mainOp, &mainOp, mainOp.numSystem, TRUE);

					secondOp = mainOp;
					secondOp.valid = TRUE;

					write2ndArg = TRUE;
				}

				write2ndArg = TRUE;
				numWritten = FALSE;
			}

		break;

		case BTN_result:

			if ((secondOp.valid == TRUE) && (numWritten == TRUE))
			{
				oper_result(&secondOp, &mainOp, &mainOp, mainOp.numSystem, FALSE);

				memset(&secondOp, 0x0, sizeof(secondOp));

				secondOp.valid = FALSE;

				write2ndArg = TRUE;
			}
			else
			{
				mainOp.operAdded = FALSE;
				mainOp.operId = BTN_none;
			}

			numWritten = FALSE;

		break;

		case BTN_dot:

			if (mainOp.numberLen < (MaxNumOfChars - 1))
			{
				if (mainOp.dotAdded == FALSE)
				{
					if (mainOp.numberLen == 0)
					{
						mainOp.number[mainOp.numberLen++] = '0';
						mainOp.valid = TRUE;
					}

					mainOp.number[mainOp.numberLen++] = systemDot;
					mainOp.number[mainOp.numberLen] = '\0';

					mainOp.dotAdded = TRUE;

					numWritten = TRUE;
				}
			}

		break;

		case BTN_plusminus:

			//if (mainOp.numberLen < (MaxNumOfChars - 1))
			{
				double oper_f = 0.0;
				int64_t oper_i = 0;

				// Read numbers
				readNumber(&mainOp, &oper_f, &oper_i);

				oper_f = 0.0 - oper_f;
				oper_i = oper_f;

				writeNumber(&mainOp, oper_f, oper_i);

				mainOp.numberLen = strlen(mainOp.number);

				numWritten = TRUE;
			}

		break;

		case BTN_sqrt:
		case BTN_sqr:
		case BTN_sin:
		case BTN_cos:
		case BTN_tan:
		case BTN_sinh:
		case BTN_cosh:
		case BTN_tanh:
		case BTN_abs:
		case BTN_1_div_x:
		case BTN_lg:
		case BTN_ln:
		case BTN_not:
			{
				if (mainOp.operAdded == TRUE)
				{
					mainOp.operId = BTN_none;
					mainOp.operAdded = FALSE;
				}

				// Calculate results
				double oper_f = 0.0;
				int64_t oper_i = 0;

				// Read numbers
				readNumber(&mainOp, &oper_f, &oper_i);

				if (Type == BTN_sqrt)
				{
					oper_f = sqrt(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_not)
				{
					oper_i = ~oper_i;
					oper_f = oper_i;
				}
				else if (Type == BTN_sqr)
				{
					oper_i = oper_i * oper_i;
					oper_f = oper_f * oper_f;
				}
				else if (Type == BTN_sin)
				{
					if (g_calcMode == CM_Degrees)
					{
						oper_f = oper_f * M_PI / 180.0;
					}
					else if (g_calcMode == CM_Grads)
					{
						oper_f = oper_f / 63.6619772368;
					}

					oper_f = sin(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_cos)
				{
					if (g_calcMode == CM_Degrees)
					{
						oper_f = oper_f * M_PI / 180.0;
					}
					else if (g_calcMode == CM_Grads)
					{
						oper_f = oper_f / 63.6619772368;
					}

					oper_f = cos(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_tan)
				{
					if (g_calcMode == CM_Degrees)
					{
						oper_f = oper_f * M_PI / 180.0;
					}
					else if (g_calcMode == CM_Grads)
					{
						oper_f = oper_f / 63.6619772368;
					}

					oper_f = tan(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_sinh)
				{
					oper_f = sinh(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_cosh)
				{
					oper_f = cosh(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_tanh)
				{
					oper_f = tanh(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_1_div_x)
				{
					oper_f = 1.0 / oper_f;
					oper_i = oper_f;
				}
				else if (Type == BTN_abs)
				{
					oper_f = fabs(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_lg)
				{
					oper_f = log10(oper_f);
					oper_i = oper_f;
				}
				else if (Type == BTN_ln)
				{
					oper_f = log(oper_f);
					oper_i = oper_f;
				}

				writeNumber(&mainOp, oper_f, oper_i);

				// Ok, we have it. Not let's check if there is some variable in stack, and if so,
				// let's add it to the result

				if (secondOp.valid == TRUE)
				{
					oper_result(&secondOp, &mainOp, &mainOp, mainOp.numSystem, FALSE);

					memset(&secondOp, 0x0, sizeof(secondOp));

				}

				write2ndArg = TRUE;
				numWritten = FALSE;
			}

		break;
	}
}

/**
  * Default widget handler
  */
int AppMain(int type, int par1, int par2)
{
	// Handle all messages passed into the widget
	if (type == EVT_SHOW)
	{
		Draw(g_row, g_col);

		FullUpdate();
	}

	if (type == EVT_ORIENTATION)
	{
		SetOrientation(par1);

		wx = 0;
		wy = 0;
		ww = ScreenWidth();
		wh = ScreenHeight();

		if ((par1 == ROTATE90) || (par1 == ROTATE270))
		{
			initCalc(TRUE);
		}
		else
		{
			initCalc(FALSE);
		}

		Draw(g_row, g_col);

		FullUpdate();
	}

	if (type == EVT_POINTERUP)
	{
		// recalculate col and row

		int x = par1;
		int y = par2;

		if (((x >= calc_x) && (x < calc_x + calc_w))
			&&
			((y >= calc_y) && (y < calc_y + calc_h))
			)
		{
			// Back up the values
			int old_row = g_row;
			int old_col = g_col;

			// Detect if it is "=" button
			if (((x >= CalcButtons[3][4].x) && (x < CalcButtons[3][4].x + CalcButtons[3][4].w))
				&&
				((y >= CalcButtons[3][4].y) && (y < CalcButtons[3][4].y + CalcButtons[3][4].h))
				)
			{
				g_row = 3;
				g_col = 4;
			}
			else
			{
				g_col = (x - calc_x) / DefaultButtonHeight;
				g_row = (y - calc_y) / DefaultButtonHeight;
			}

			int StatusUpdated = FALSE;

			// Redraw old button
			if ((old_row != -1) && (old_col != -1))
			{
				InvertButton(old_row, old_col, FALSE);
			}

			// Redraw new button
			InvertButton(g_row, g_col, TRUE);

			// Handle pressed button
			HandleCalc(CalcButtons[g_row][g_col].type, CalcButtons[g_row][g_col].ButtonNumState, &StatusUpdated);

			// Draw results of the calculation
			DrawButton(0, 0, FALSE);

			// Update infromation on the screen
			if ((old_col == -1) || (old_row == -1))
			{
				old_row = g_row;
				old_col = g_col;
			}

			// Calcualte area for old and new pressed buttons
			if (StatusUpdated)
			{
				if (UseFullUpdate == TRUE)
				{
					FullUpdate();
				}
				else
				{
					// Update info on the screen
					PartialUpdate(
						calc_x,
						calc_y,
						calc_w,
						calc_h
						);
				}
			}
			else
			{
				tRect updateRect;
				tRects calcRects;

				memset(&updateRect, 0x0, sizeof(updateRect));
				memset(&calcRects, 0x0, sizeof(calcRects));

				calcRects.rects[calcRects.count].x = CalcButtons[old_row][old_col].x;
				calcRects.rects[calcRects.count].y = CalcButtons[old_row][old_col].y;
				calcRects.rects[calcRects.count].w = CalcButtons[old_row][old_col].w;
				calcRects.rects[calcRects.count].h = CalcButtons[old_row][old_col].h;
				calcRects.count++;

				calcRects.rects[calcRects.count].x = CalcButtons[g_row][g_col].x;
				calcRects.rects[calcRects.count].y = CalcButtons[g_row][g_col].y;
				calcRects.rects[calcRects.count].w = CalcButtons[g_row][g_col].w;
				calcRects.rects[calcRects.count].h = CalcButtons[g_row][g_col].h;
				calcRects.count++;

				calcRects.rects[calcRects.count].x = CalcButtons[0][0].x;
				calcRects.rects[calcRects.count].y = CalcButtons[0][0].y;
				calcRects.rects[calcRects.count].w = CalcButtons[0][0].w;
				calcRects.rects[calcRects.count].h = CalcButtons[0][0].h;
				calcRects.count++;

				detectRegion(&calcRects, &updateRect);

				// Update info on the screen
				PartialUpdateBW(
					updateRect.x,
					updateRect.y,
					updateRect.w,
					updateRect.h
					);
			}
		}
	}

	if (type == EVT_KEYPRESS)
	{
		switch(par1)
		{
			case KEY_LEFT:
			{
				if ((g_col >= 0) && (g_row >= 0))
				{
					// Back up the values
					int old_row = g_row;
					int old_col = g_col;

					int StatusUpdated = FALSE;

					// Redraw old button
					InvertButton(g_row, g_col, FALSE);

					// Change index of the active button
					if (g_col == 0)
					{
						g_col = g_CalcCols - 1;
					}
					else
					{
						g_col--;
					}

					// Redraw new button
					InvertButton(g_row, g_col, TRUE);

					// Update infromation on the screen
					if ((old_col == -1) || (old_row == -1))
					{
						old_row = g_row;
						old_col = g_col;
					}

					// Calcualte area for old and new pressed buttons
					if (StatusUpdated)
					{
						if (UseFullUpdate == TRUE)
						{
							FullUpdate();
						}
						else
						{
							// Update info on the screen
							PartialUpdate(
								calc_x,
								calc_y,
								calc_w,
								calc_h
								);
						}
					}
					else
					{
						tRect updateRect;
						tRects calcRects;

						memset(&updateRect, 0x0, sizeof(updateRect));
						memset(&calcRects, 0x0, sizeof(calcRects));

						calcRects.rects[calcRects.count].x = CalcButtons[old_row][old_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[old_row][old_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[old_row][old_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[old_row][old_col].h;
						calcRects.count++;

						calcRects.rects[calcRects.count].x = CalcButtons[g_row][g_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[g_row][g_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[g_row][g_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[g_row][g_col].h;
						calcRects.count++;

						detectRegion(&calcRects, &updateRect);

						// Update info on the screen
						PartialUpdateBW(
							updateRect.x,
							updateRect.y,
							updateRect.w,
							updateRect.h
							);
					}
				}
			}
			break;

			case KEY_RIGHT:
			{
				if ((g_col < g_CalcCols) && (g_col >= 0) && (g_row >= 0))
				{
					// Back up the values
					int old_row = g_row;
					int old_col = g_col;

					int StatusUpdated = FALSE;

					// Redraw old button
					InvertButton(g_row, g_col, FALSE);

					// Change index of the active button
					if (g_col == g_CalcCols - 1)
					{
						g_col = 0;
					}
					else
					{
						g_col++;
					}

					// Redraw new button
					InvertButton(g_row, g_col, TRUE);

					// Update infromation on the screen
					if ((old_col == -1) || (old_row == -1))
					{
						old_row = g_row;
						old_col = g_col;
					}

					// Calcualte area for old and new pressed buttons
					if (StatusUpdated)
					{
						if (UseFullUpdate == TRUE)
						{
							FullUpdate();
						}
						else
						{
							// Update info on the screen
							PartialUpdate(
								calc_x,
								calc_y,
								calc_w,
								calc_h
								);
						}
					}
					else
					{
						tRect updateRect;
						tRects calcRects;

						memset(&updateRect, 0x0, sizeof(updateRect));
						memset(&calcRects, 0x0, sizeof(calcRects));

						calcRects.rects[calcRects.count].x = CalcButtons[old_row][old_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[old_row][old_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[old_row][old_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[old_row][old_col].h;
						calcRects.count++;

						calcRects.rects[calcRects.count].x = CalcButtons[g_row][g_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[g_row][g_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[g_row][g_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[g_row][g_col].h;
						calcRects.count++;

						detectRegion(&calcRects, &updateRect);

						// Update info on the screen
						PartialUpdateBW(
							updateRect.x,
							updateRect.y,
							updateRect.w,
							updateRect.h
							);
					}
				}
			}
			break;

			case KEY_UP:
			{
				if ((g_row >= 1) && (g_col >= 0))
				{
					// Back up the values
					int old_row = g_row;
					int old_col = g_col;

					int StatusUpdated = FALSE;

					// Redraw old button
					InvertButton(g_row, g_col, FALSE);

					// Change index of the active button
					if (g_row == 1)
					{
						g_row = g_CalcRows - 1;
					}
					else
					{
						g_row--;
					}

					// Redraw new button
					InvertButton(g_row, g_col, TRUE);

					// Update infromation on the screen
					if ((old_col == -1) || (old_row == -1))
					{
						old_row = g_row;
						old_col = g_col;
					}

					// Calcualte area for old and new pressed buttons
					if (StatusUpdated)
					{
						if (UseFullUpdate == TRUE)
						{
							FullUpdate();
						}
						else
						{
							// Update info on the screen
							PartialUpdate(
								calc_x,
								calc_y,
								calc_w,
								calc_h
								);
						}
					}
					else
					{
						tRect updateRect;
						tRects calcRects;

						memset(&updateRect, 0x0, sizeof(updateRect));
						memset(&calcRects, 0x0, sizeof(calcRects));

						calcRects.rects[calcRects.count].x = CalcButtons[old_row][old_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[old_row][old_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[old_row][old_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[old_row][old_col].h;
						calcRects.count++;

						calcRects.rects[calcRects.count].x = CalcButtons[g_row][g_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[g_row][g_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[g_row][g_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[g_row][g_col].h;
						calcRects.count++;

						detectRegion(&calcRects, &updateRect);

						// Update info on the screen
						PartialUpdateBW(
							updateRect.x,
							updateRect.y,
							updateRect.w,
							updateRect.h
							);
					}
				}
			}
			break;

			case KEY_DOWN:
			{
				if ((g_row < g_CalcRows))
				{
					// Back up the values
					int old_row = g_row;
					int old_col = g_col;

					int StatusUpdated = FALSE;

					if ((g_row == -1) && (g_col == -1))
					{
						g_row = 1;
						g_col = 0;
					}
					else
					{
						// Redraw old button
						InvertButton(g_row, g_col, FALSE);

						// Change index of the active button
						if (g_row == (g_CalcRows - 1))
						{
							g_row = 1;
						}
						else
						{
							g_row++;
						}
					}

					// Redraw new button
					InvertButton(g_row, g_col, TRUE);

					// Update infromation on the screen
					if ((old_col == -1) || (old_row == -1))
					{
						old_row = g_row;
						old_col = g_col;
					}

					// Calcualte area for old and new pressed buttons
					if (StatusUpdated)
					{
						if (UseFullUpdate == TRUE)
						{
							FullUpdate();
						}
						else
						{
							// Update info on the screen
							PartialUpdate(
								calc_x,
								calc_y,
								calc_w,
								calc_h
								);
						}
					}
					else
					{
						tRect updateRect;
						tRects calcRects;

						memset(&updateRect, 0x0, sizeof(updateRect));
						memset(&calcRects, 0x0, sizeof(calcRects));

						calcRects.rects[calcRects.count].x = CalcButtons[old_row][old_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[old_row][old_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[old_row][old_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[old_row][old_col].h;
						calcRects.count++;

						calcRects.rects[calcRects.count].x = CalcButtons[g_row][g_col].x;
						calcRects.rects[calcRects.count].y = CalcButtons[g_row][g_col].y;
						calcRects.rects[calcRects.count].w = CalcButtons[g_row][g_col].w;
						calcRects.rects[calcRects.count].h = CalcButtons[g_row][g_col].h;
						calcRects.count++;

						detectRegion(&calcRects, &updateRect);

						// Update info on the screen
						PartialUpdateBW(
							updateRect.x,
							updateRect.y,
							updateRect.w,
							updateRect.h
							);
					}
				}
			}
			break;

			case KEY_BACK:
			{
				CloseApp();
			}
			break;

			case KEY_OK:
			{
				int StatusUpdated = FALSE;

				// Enter button was pressed, so let's calculate results
				HandleCalc(CalcButtons[g_row][g_col].type, CalcButtons[g_row][g_col].ButtonNumState, &StatusUpdated);

				// Draw results of the calculation
				DrawButton(0, 0, FALSE);

				// Update info on the sceen
				if (StatusUpdated)
				{
					if (UseFullUpdate == TRUE)
					{
						FullUpdate();
					}
					else
					{
						// Update info on the screen
						PartialUpdate(
							calc_x,
							calc_y,
							calc_w,
							calc_h
							);
					}
				}
				else
				{
					if (CalcButtons[g_row][g_col].type == BTN_clear)
					{
						PartialUpdate(CalcButtons[0][0].x, CalcButtons[0][0].y, CalcButtons[0][0].w, CalcButtons[0][0].h);
					}
					else
					{
						PartialUpdateBW(CalcButtons[0][0].x, CalcButtons[0][0].y, CalcButtons[0][0].w, CalcButtons[0][0].h);
					}
				}
			}

			break;
		}
	}

	return 0;
}

/**
  * Application's entry point
  */
int main()
{
	// This API is a part of InkView library. It initalizes screen
	OpenScreen();

	// Get device model and check if we are on small 5 inch devices
	const char* devModel = GetDeviceModel();

	if (devModel != NULL)
	{
		if (strstr(devModel, "360") != NULL)
		{
			UseFullUpdate = TRUE;
		}
	}

	// Detect screen size and dimension
	wx = 0;
	wy = 0;
	ww = ScreenWidth();
	wh = ScreenHeight();

	// Cell which should be highlighted when we start our calc
	g_row = 3;
	g_col = 0;

	// Init calculator's memory
	memset(memOp.number, 0x0, sizeof(memOp.number));
	strcpy(memOp.number, "0");
	memOp.valid = FALSE;

	// Enable button that switches calculator to Decimal numeral system
	CalcButtons[1][0].ModeButtonActive = TRUE;

	// By default our calculator should be switched to Decimal numeral system
	g_numSystem = NS_Dec;

	// Configure multi functional buttons.
	CalcButtons[1][4].funcs[0].ModeButtonActive = TRUE;
	CalcButtons[1][5].funcs[1].ModeButtonActive = TRUE;
	CalcButtons[1][5].funcs[2].ModeButtonActive = TRUE;
	CalcButtons[1][5].funcs[3].ModeButtonActive = TRUE;

	// By default we should switch our calculator to Degrees mode
	g_calcMode = CM_Degrees;

	// By default operand size is 32 bits
	g_operandSize[NS_Hex] = OS_32_bits;
	g_operandSize[NS_Dec] = OS_32_bits;
	g_operandSize[NS_Bin] = OS_32_bits;
	g_operandSize[NS_Oct] = OS_32_bits;

	// Clear various internal flags and states
	clearCalc(FALSE);

	// Initialize button's dimensions and sizes
	if ((GetOrientation() == ROTATE90) || (GetOrientation() == ROTATE270))
	{
		initCalc(TRUE);
	}
	else
	{
		initCalc(FALSE);
	}

	// Start our application
	InkViewMain(AppMain);

	return 0;
}
