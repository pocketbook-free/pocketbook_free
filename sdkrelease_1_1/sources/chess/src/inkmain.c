#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>                                                              

#include "common.h"
#include "book.h"
#include <inkview.h>

char *FileOptions;
char *FilePosition;

static const char LETTERS[8] = "abcdefgh";
static const char DIGITS[8] = "87654321";

extern const ibitmap bwpawn, bwknight, bwbishop, bwrook, bwqueen, bwking, bbpawn, bbknight, bbbishop, bbrook, bbqueen, bbking, whitefield, blackfield,
	wwpawn, wwknight, wwbishop, wwrook, wwqueen, wwking, wbpawn, wbknight, wbbishop, wbrook, wbqueen, wbking, StartScreen, PlayerScreen, PicWhite, PicBlack;

struct TGameOptions
{
 int Flag;
 int Players;
 int PlayerColor;
 int ColorMove;
 int ComputerLevel;
}GameOptions;

ifont *font, *cour36;

int cx=4, cy=4;
int moving=0;
int steps=1;
int sdvigx=0;
int PlayerColor=1;
int Players=1;
int LastGameFlag=0;
int WinPlayer=0;
int StepPlayer=0;
int Select=0;
int NewGame=0;
int ComputerLevel=1;
int orient=0;
int no_save=0;
int TouchScreen=0;
char tmpinputstr[10]="";
ibitmap *PicColor;
FILE *f;

int Chess_Player(int type, int par1, int par2);
int game_handler(int type, int par1, int par2);
void draw_board(int full);
void do_move();
int LevelHandler(int type, int par1, int par2);

int SelectPromotionHandler(int type, int par1, int par2)
{
 if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return; }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_UP:
 		 	InvertArea(530, 35+(SelectPromotion-1)*100, 50, 50);
 		 	SelectPromotion--;
 		 	if (SelectPromotion<1)
 		 		SelectPromotion=4;
 		 	InvertArea(530, 35+(SelectPromotion-1)*100, 50, 50);
			PartialUpdateBW(530, 35, 50, 350);
 		 	break;
 		 	
 		 case KEY_DOWN:
 		 	InvertArea(530, 35+(SelectPromotion-1)*100, 50, 50);
 		  	SelectPromotion++;
 		  	if (SelectPromotion>4)
 		  		SelectPromotion=1;
 		 	InvertArea(530, 35+(SelectPromotion-1)*100, 50, 50);
			PartialUpdateBW(530, 35, 50, 350);
			break;
		
		case KEY_OK:
			 if (SelectPromotion==1)
			 	SelectPromotion='q';
			 if (SelectPromotion==2)
			 	SelectPromotion='r';
			 if (SelectPromotion==3)
			 	SelectPromotion='b';
			 if (SelectPromotion==4)
			 	SelectPromotion='n';
			 sprintf(inputstr,"%s",tmpinputstr);
			 do_move();
			 FillArea(525, 30, 60, 360, WHITE);
			 draw_board(1);
		 	 SetEventHandler(game_handler);
		 	 break;
		}
 if (type==EVT_POINTERUP){
 	if ((par1>=530 && par1<=580) && ((par2>=35 && par2<=85)
 		|| (par2>=135 && par2<=185)
 		|| (par2>=235 && par2<=285)
 		|| (par2>=335 && par2<=385))){
 		InvertArea(530, 35, 50, 50);
 		SelectPromotion=((par2-35)/100)%4+1;
 		InvertArea(530, 35+((SelectPromotion-1)%4)*100, 50, 50);
 		PartialUpdateBW(530, 35, 50, 350);
 		SelectPromotionHandler(EVT_KEYPRESS, KEY_OK, 0);
 	}
 }
 return 0;
}

void SetPromotion(void)
{
 SelectPromotion=1;
 FillArea(525, 30, 60, 360, BLACK);
 FillArea(529, 34, 52, 352, WHITE);
 if (board.side == white)
 	{
  	 DrawBitmap(530, 35, &wwqueen);
	 DrawBitmap(530, 135, &wwrook);
	 DrawBitmap(530, 235, &wwbishop);
	 DrawBitmap(530, 335, &wwknight);
	}
	else
	{
  	 DrawBitmap(530, 35, &wbqueen);
	 DrawBitmap(530, 135, &wbrook);
	 DrawBitmap(530, 235, &wbbishop);
	 DrawBitmap(530, 335, &wbknight);
	}
 InvertArea(530, 35, 50, 50);
 PartialUpdateBW(525, 30, 60, 360);
 SelectPromotion=1;
 SetEventHandler(SelectPromotionHandler);
}

void SetComputerLevel()
{
 sprintf(inputstr,"depth %d", 50*ComputerLevel/10);
 parse_input();
 sprintf(inputstr,"level %d 1 1", 60/ComputerLevel);
 parse_input();
}

void SaveGame()
{
 if ((f=fopen(FileOptions,"wb"))!=NULL)
 	 if (WinPlayer==0)
 	 	{
 	 	 GameOptions.Flag=1;
		 GameOptions.Players=Players;
		 GameOptions.PlayerColor=PlayerColor;
		 GameOptions.ColorMove=(steps+1)%2;
		 GameOptions.ComputerLevel=ComputerLevel;
		 fwrite(&GameOptions, 1, sizeof(struct TGameOptions), f);
	 	 fclose(f);
	 	 if ((f=fopen(FilePosition,"wb"))!=NULL);
  	 	 fclose(f);
 		 SaveEPD(FilePosition);
 	 	}
 	 	else
 	 	{
 	 	 GameOptions.Flag=0;
		 GameOptions.Players=0;
		 GameOptions.PlayerColor=0;
		 GameOptions.ColorMove=0;
		 GameOptions.ComputerLevel=0;
		 fwrite(&GameOptions, 1, sizeof(struct TGameOptions), f);
	 	 fclose(f);
 	 	}
}

void LoadGame()
{
 if (NewGame==1)
 	{
 	 sprintf(inputstr,"new");
 	 parse_input();
 	 WinPlayer=0;
 	 if (Players==1)
 	 	{
		 ClearScreen();
		 DrawTextRect(100, 400, 400, 50, GetLangText("@Diff_level"), ALIGN_CENTER);
		 ComputerLevel=1;
		 int i;
		 char s[3];
		 for (i=1; i<11; i++)
		 	{
			 sprintf(s, "%d", i);
			 DrawTextRect(i*50, 510, 50, 50, s, ALIGN_CENTER);
			}
		 InvertArea(50, 500, 50, 50);
		 DrawSymbol(20, 507, ARROW_LEFT);
		 DrawSymbol(560, 507, ARROW_RIGHT);
		 if (PlayerColor==0)
			DrawBitmap(230, 0, &PicWhite);
 		 	else
			DrawBitmap(230, 0, &PicBlack);
		 FullUpdate();
		 SetEventHandler(LevelHandler);
		 return;
 	 	}
	 SetEventHandler(game_handler);
	}
	else
	{
	 f=fopen(FileOptions, "rb");
	 if ( (f==NULL) || (fread(&GameOptions, 1, sizeof(struct TGameOptions), f)!=sizeof(struct TGameOptions)))
	 	LastGameFlag=0;
	 	else
	 	{
		 if ((GameOptions.Flag==1)&&(fopen(FilePosition, "rb")!=NULL))
		 	{
		 	 LastGameFlag=1;
		 	 Players=GameOptions.Players;
		 	 PlayerColor=GameOptions.PlayerColor;
		 	 steps+=GameOptions.ColorMove%2;
		 	 if (Players==1)
		 	 	{
		 	 	 ComputerLevel=GameOptions.ComputerLevel;
		 	 	 SetComputerLevel();
		 	 	}
		  	 LoadEPD(FilePosition);
 		 	}
	  	}
	 if (f!=NULL) fclose(f);
	}
}

void draw_board(int full) {

   int r, c, sq;
   int x, y;
   int i;
   char buf[64], *fig;
   char s1[3], s2[3];
   ibitmap *figure;
   SetFont(font, BLACK);
   y=0;
if (PlayerColor)
{   
   for (r = 56; r >= 0; r -= 8)
   {
      x=0;
      for (c = 0; c < 8; c++)
      {
         sq = r + c;
	if (((x%2==0)&&(y%2==0))||((x%2==1)&&(y%2==1)))
		{
		if (board.b[white][pawn] & BitPosArray[sq])
            		figure=&wwpawn;
        			else 
        			if (board.b[white][knight] & BitPosArray[sq])
            			figure=&wwknight;
         				else 
         				if (board.b[white][bishop] & BitPosArray[sq])
            				figure=&wwbishop;
            				else 
            				if (board.b[white][rook]   & BitPosArray[sq])
					            figure=&wwrook;
         						else 
         						if (board.b[white][queen]  & BitPosArray[sq])
						            figure=&wwqueen;
							else 
							if (board.b[white][king]   & BitPosArray[sq])
							            figure=&wwking;
								else 
								if (board.b[black][pawn]   & BitPosArray[sq])
								            figure=&wbpawn;
									else 
									if (board.b[black][knight] & BitPosArray[sq])
									            figure=&wbknight;
										else 
										if (board.b[black][bishop] & BitPosArray[sq])
										            figure=&wbbishop;
											else 
											if (board.b[black][rook]   & BitPosArray[sq])
											            figure=&wbrook;
												else 
												if (board.b[black][queen]  & BitPosArray[sq])
												            figure=&wbqueen;
													else 
													if (board.b[black][king]   & BitPosArray[sq])
													            figure=&wbking;
														else
													           	figure=&whitefield;
		}
         		else
         		if (board.b[white][pawn] & BitPosArray[sq])
            		figure=&bwpawn;
         			else 
         			if (board.b[white][knight] & BitPosArray[sq])
			            figure=&bwknight;
			            else 
			            if (board.b[white][bishop] & BitPosArray[sq])
			            	figure=&bwbishop;
			            	else 
			            	if (board.b[white][rook]   & BitPosArray[sq])
			            		figure=&bwrook;
			            		else 
			            		if (board.b[white][queen]  & BitPosArray[sq])
				            		figure=&bwqueen;
				            		else 
				            		if (board.b[white][king]   & BitPosArray[sq])
				            			figure=&bwking;
				            			else 
				            			if (board.b[black][pawn]   & BitPosArray[sq])
				            				figure=&bbpawn;
				            				else 
				            				if (board.b[black][knight] & BitPosArray[sq])
				            					figure=&bbknight;
				            					else 
				            					if (board.b[black][bishop] & BitPosArray[sq])
				            						figure=&bbbishop;
				            						else 
				            						if (board.b[black][rook]   & BitPosArray[sq])
				            							figure=&bbrook;
				            							else 
				            							if (board.b[black][queen]  & BitPosArray[sq])
				            								figure=&bbqueen;
				            								else 
				            								if (board.b[black][king]   & BitPosArray[sq])
				            									figure=&bbking;
				            									else
				            									figure=&blackfield;
         DrawBitmap(100+x*50, 10+y*50, figure);
         if (x==cx && y==cy && (!full || (full && !TouchScreen))) InvertArea(100+x*50, 10+y*50, 50, 50);
         x++;
      }
      y++;
   }
   for (i=1; i<9; i++)
	{
	 sprintf(s1, "%c", i+96);
	 sprintf(s2, "%d", 9-i);
	 DrawString(70+i*50, 410, s1);
	 DrawString(85, 30+(i-1)*50, s2);
	}
}
else   
{
   for (r = 56; r >= 0; r -= 8)
   {
      x=0;
      for (c = 0; c < 8; c++)
      {
         sq = r + c;
	if (((x%2==0)&&(y%2==0))||((x%2==1)&&(y%2==1)))
		{
		if (board.b[white][pawn] & BitPosArray[sq])
            		figure=&wwpawn;
        			else 
        			if (board.b[white][knight] & BitPosArray[sq])
            			figure=&wwknight;
         				else 
         				if (board.b[white][bishop] & BitPosArray[sq])
            				figure=&wwbishop;
            				else 
            				if (board.b[white][rook]   & BitPosArray[sq])
					            figure=&wwrook;
         						else 
         						if (board.b[white][queen]  & BitPosArray[sq])
						            figure=&wwqueen;
							else 
							if (board.b[white][king]   & BitPosArray[sq])
							            figure=&wwking;
								else 
								if (board.b[black][pawn]   & BitPosArray[sq])
								            figure=&wbpawn;
									else 
									if (board.b[black][knight] & BitPosArray[sq])
									            figure=&wbknight;
										else 
										if (board.b[black][bishop] & BitPosArray[sq])
										            figure=&wbbishop;
											else 
											if (board.b[black][rook]   & BitPosArray[sq])
											            figure=&wbrook;
												else 
												if (board.b[black][queen]  & BitPosArray[sq])
												            figure=&wbqueen;
													else 
													if (board.b[black][king]   & BitPosArray[sq])
													            figure=&wbking;
														else
													           	figure=&whitefield;
		}
         		else
         		if (board.b[white][pawn] & BitPosArray[sq])
            		figure=&bwpawn;
         			else 
         			if (board.b[white][knight] & BitPosArray[sq])
			            figure=&bwknight;
			            else 
			            if (board.b[white][bishop] & BitPosArray[sq])
			            	figure=&bwbishop;
			            	else 
			            	if (board.b[white][rook]   & BitPosArray[sq])
			            		figure=&bwrook;
			            		else 
			            		if (board.b[white][queen]  & BitPosArray[sq])
				            		figure=&bwqueen;
				            		else 
				            		if (board.b[white][king]   & BitPosArray[sq])
				            			figure=&bwking;
				            			else 
				            			if (board.b[black][pawn]   & BitPosArray[sq])
				            				figure=&bbpawn;
				            				else 
				            				if (board.b[black][knight] & BitPosArray[sq])
				            					figure=&bbknight;
				            					else 
				            					if (board.b[black][bishop] & BitPosArray[sq])
				            						figure=&bbbishop;
				            						else 
				            						if (board.b[black][rook]   & BitPosArray[sq])
				            							figure=&bbrook;
				            							else 
				            							if (board.b[black][queen]  & BitPosArray[sq])
				            								figure=&bbqueen;
				            								else 
				            								if (board.b[black][king]   & BitPosArray[sq])
				            									figure=&bbking;
				            									else
				            									figure=&blackfield;
         DrawBitmap(450-x*50, 360-y*50, figure);
         if (x==7-cx && y==7-cy && (!full || (full && !TouchScreen))) InvertArea(450-x*50, 360-y*50, 50, 50);
         x++;
      }
      y++;
   }
  for (i=1; i<9; i++)
	{
	 sprintf(s1, "%c", 105-i);
	 sprintf(s2, "%d", i);
	 DrawString(70+i*50, 410, s1);
	 DrawString(85, 30+(i-1)*50, s2);
	}
}
for(i=0; i<strlen(SANmv); i++)
	if (SANmv[i]=='#')
		{
		 WinPlayer=1;
		 FillArea(200, 430, 200, 25, WHITE);
		 if ((sdvigx*30+steps)%2==(PlayerColor+1)%2) 
		 	 if ((steps%2)==0)
		 	 	DrawTextRect(200, 430, 200, 20, GetLangText("@CheckMate_B"), ALIGN_CENTER);
		 	 	else
		 	 	DrawTextRect(200, 430, 200, 20, GetLangText("@CheckMate_W"), ALIGN_CENTER);
		 	 else
		 	 if ((steps%2)==1)
			 	DrawTextRect(200, 430, 200, 20, GetLangText("@CheckMate_B"), ALIGN_CENTER);
		 	 	else	
		 	 	DrawTextRect(200, 430, 200, 20, GetLangText("@CheckMate_W"), ALIGN_CENTER);
		 PartialUpdateBW(200, 430, 200, 25);
		 }		 	
		 	
   if (full) {
   	if (steps>31) 
   		{
   		 steps=2;
   		 sdvigx++;
   		}
   	if (steps/2>0)
   		{
   		 if ((sdvigx>1)&&((sdvigx*30+steps)%2==0)) 
   		 	FillArea(100+(sdvigx%2)*200, 450+(steps/2)*20, 200, 20, WHITE);
   		 sprintf(buf,"%d. ",(sdvigx*30+steps)/2);
		 strcat(buf, SANmv);
   		 DrawString(100+(sdvigx%2)*200+(steps%2)*100, 450+(steps/2)*20, buf);
   		}
	 if (WinPlayer==0)
		 FillArea(280, 430, 50, 25, WHITE);
	inputstr[0]='\0';
	FullUpdate();
   	}
   	else
   	{
	 if (WinPlayer==0)
	 	{
	 	 PartialUpdateBW(85, 10, 415, 410);
	 	 FillArea(280, 430, 50, 25, WHITE);
	 	 DrawString(280, 430, inputstr);
	 	 PartialUpdateBW(280, 430, 50, 25);
		}	
	}	
}

int ExitHandler(int type, int par1, int par2)
{
 if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return; }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_BACK:
			ClearScreen();
			draw_board(1);
			SetEventHandler(game_handler);
 		 	break;
 		 
 		 case KEY_UP:
 		 	InvertArea(100, 195+((Select)%4)*100, 400, 50);
 		 	Select--;
 		 	if (Select<0)
 		 		Select=3;
 		 	InvertArea(100, 195+((Select)%4)*100, 400, 50);
 		 	PartialUpdateBW(0, 190, 600, 400);
 		 	break;
 		 
 		 case KEY_DOWN:
 		 	InvertArea(100, 195+((Select)%4)*100, 400, 50);
 		 	Select++;
 		 	Select=Select%4;
 		 	InvertArea(100, 195+((Select)%4)*100, 400, 50);
 		 	PartialUpdateBW(0, 190, 600, 400);
 		 	break;
 		 
 		 case KEY_OK:
 			ClearScreen();
			if (Select==0)
				{
				 SANmv[0]='\0';
				 NewGame=1;
				 steps=1;
				 moving=0;
				 sdvigx=0;
				 PlayerColor=1;
				 Players=1;
				 LastGameFlag=0;
				 WinPlayer=0;
				 StepPlayer=0;
				 SetFont(cour36, BLACK);
				 ClearScreen();
				 DrawBitmap(205, 30, &PlayerScreen);
				 DrawTextRect(100, 400, 400, 50, GetLangText("@One_Player"), ALIGN_CENTER);
				 DrawTextRect(100, 500, 400, 50, GetLangText("@Two_Players"), ALIGN_CENTER);

				 InvertArea(100, 395, 400, 50);
				 FullUpdate();
				 SetEventHandler(Chess_Player);
				}
			if (Select==1)
				{
				 draw_board(1);
			 	 SetEventHandler(game_handler);
				}
			if (Select==2)
				{
				 CloseApp();
				}
			if (Select==3)
				{
				 no_save = 1;
				 CloseApp();			
				}
 		 	break;
 		}
  if (type==EVT_POINTERUP){
 	if ((par1>=100 && par1<=500) && ((par2>=195 && par2<=245)
 		|| (par2>=295 && par2<=345)
 		|| (par2>=395 && par2<=445)
 		|| (par2>=495 && par2<=545))){
 		InvertArea(100, 195, 400, 50);
 		Select=((par2-195)/100)%4;
 		InvertArea(100, 195+((Select)%4)*100, 400, 50);
 		PartialUpdateBW(100, 195, 400, 450);
 		ExitHandler(EVT_KEYPRESS, KEY_OK, 0);
 	}
 }
 	return 0;
}

void do_move() {
    input_status = INPUT_AVAILABLE;
    //    wait_for_input();
    dbg_printf("Parsing input...\n");
    sprintf(tmpinputstr, "%s", inputstr);
    parse_input();
    input_status = INPUT_NONE;
    dbg_printf("input_status = %d\n", input_status);
    if ((flags & THINK) && !(flags & MANUAL) && !(flags & ENDED)) 
	{
	 steps++;
      	 if (!(flags & XBOARD)) 
		{
	 	 printf("Thinking...\n");
	 	 if (Players==1) draw_board(1);
		}
      	if (Players==1) 
      		{
      		 Iterate();
      		 steps++;
 	 	 draw_board(1);
      		};
      	CLEAR (flags, THINK);
    	}
    RealGameCnt = GameCnt;
    RealSide = board.side;
//    dbg_printf("Waking up input...\n");
//    dbg_printf("input_status = %d\n", input_status);
    //    input_wakeup();
 //   dbg_printf("input_status = %d\n", input_status);
    /* Ponder only after first move */
    /* Ponder or (if pondering disabled) just wait for input */
 //   if ((flags & HARD) && !(flags & QUIT) ) {
    //      ponder();
//    }

}

void StartNewGame()
{
 SetFont(cour36, BLACK);
 ClearScreen();
 DrawBitmap(205, 30, &PlayerScreen);
 DrawTextRect(100, 400, 400, 50, GetLangText("@One_Player"), ALIGN_CENTER);
 DrawTextRect(100, 500, 400, 50, GetLangText("@Two_Players"), ALIGN_CENTER);
 InvertArea(100, 395, 400, 50);
 FullUpdate();
 SetEventHandler(Chess_Player);
 font = OpenFont("arial", 18, 0);
}

void PointerGameHandler(int type, int par1, int par2){
	int px=par1-100;
	int py=par2-10;

	if (type==EVT_POINTERUP && px/50>=0 && px/50<8 && py/50>=0 && py/50<8){
		cx=px/50;
		cy=py/50;
		TouchScreen=1;
		game_handler(EVT_KEYPRESS, KEY_OK, 0);
		TouchScreen=0;
	}
}

int game_handler(int type, int par1, int par2) {

	switch (type) {

		case EVT_INIT:
			cour36=OpenFont(DEFAULTFONTB, 30, 0); 
			font = OpenFont(DEFAULTFONTB, 18, 0);
			ClearScreen();
			DrawBitmap(110, 60, &StartScreen);
			FullUpdate();
			sleep(3);
			if (LastGameFlag==0)
				StartNewGame(); 
				else
				{
				 ClearScreen();
				 draw_board(1);
				}
				break;

		case EVT_EXIT:
			if (! no_save) {
				fprintf(stderr, "chess: saving game\n");
				 SaveGame();
				fprintf(stderr, "chess: saving game done\n");
			}
			break;

		case EVT_SHOW:
			break;

		case EVT_KEYREPEAT:
			if (par1 == KEY_OK) break;
			if (par1 == KEY_PREV) { CloseApp(); return; }
		case EVT_KEYPRESS:
			switch (par1) {
				case KEY_LEFT:
					cx--; 
					if (cx<0) cx=7;
					draw_board(0);
					break;
				case KEY_RIGHT:
					cx++; 
					if (cx>7) cx=0;
					draw_board(0);
					break;
				case KEY_UP:
					cy--; 
					if (cy<0) cy=7;
					draw_board(0);
					break;
				case KEY_DOWN:
					cy++; 
					if (cy>7) cy=0;
					draw_board(0);
					break;
				case KEY_OK:
					PromotionFlag=0;
					if (!moving) 
						if (PlayerColor)
							{
							 inputstr[0] = LETTERS[cx];
							 inputstr[1] = DIGITS[cy];
							 inputstr[2] = 0;
							 moving = 1;
							 draw_board(0);
							}
							else
							{
							 inputstr[0] = LETTERS[7-cx];
							 inputstr[1] = DIGITS[7-cy];
							 inputstr[2] = 0;
							 moving = 1;
							 draw_board(0);
							}
						else 
						 if (PlayerColor)
						 	{
						 	 inputstr[2] = ' ';
						 	 inputstr[3] = LETTERS[cx];
						 	 inputstr[4] = DIGITS[cy];
						 	 inputstr[5] = 0;
						 	 //draw_board(0);
						 	 do_move();
						 	 moving = 0;
						 	 if (PromotionFlag==0) draw_board(1);
						 	 }
						 	 else
						 	 {
						 	 inputstr[2] = ' ';
						 	 inputstr[3] = LETTERS[7-cx];
						 	 inputstr[4] = DIGITS[7-cy];
						 	 inputstr[5] = 0;
						 	 //draw_board(0);
						 	 do_move();
						 	 moving = 0;
						 	 if (PromotionFlag==0) draw_board(1);
						 	 }
					break;
				case KEY_BACK:
					if (moving) 
						{
						 moving = 0;
						 inputstr[0]='\0';
					 	 draw_board(0);
						} 
						else 
						{
						 Select=0;
						 ClearScreen();
						 SetFont(cour36, BLACK);
						 DrawTextRect(100, 195, 400, 50, GetLangText("@New_game"), ALIGN_CENTER);
						 DrawTextRect(100, 295, 400, 50, GetLangText("@Continue_game"), ALIGN_CENTER);
						 DrawTextRect(100, 395, 400, 50, GetLangText("@SaveExit"), ALIGN_CENTER);
						 DrawTextRect(100, 495, 400, 50, GetLangText("@ExitNoSave"), ALIGN_CENTER);
						 InvertArea(100, 195, 400, 50);
						 FullUpdate();
						 SetEventHandler(ExitHandler);
						}
					break;
			}
			break;
		
		case EVT_POINTERUP:
			PointerGameHandler(type, par1, par2);
			break;
		
	}
	return 0;
}

int LevelHandler(int type, int par1, int par2)
{
 char buf[5];
 if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return; }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_BACK:
 		 	CloseApp();
 		 	break;
 
 		 case KEY_RIGHT:
 		 	InvertArea(ComputerLevel*50, 500, 50, 50);
 		 	PartialUpdateBW(ComputerLevel*50, 500, 50, 50);
			ComputerLevel++;
			if (ComputerLevel>10)
				ComputerLevel=1;
 		 	InvertArea(ComputerLevel*50, 500, 50, 50);
 		 	PartialUpdateBW(ComputerLevel*50, 500, 50, 50);
 		 	break;
 
 		 case KEY_LEFT:
 		 	InvertArea(ComputerLevel*50, 500, 50, 50);
 		 	PartialUpdateBW(ComputerLevel*50, 500, 50, 50);
			ComputerLevel--;
			if (ComputerLevel<1)
				ComputerLevel=10;
 		 	InvertArea(ComputerLevel*50, 500, 50, 50);
			PartialUpdateBW(ComputerLevel*50, 500, 50, 50);
 		 	break;

		 case KEY_OK:
		 	ClearScreen();
		 	SetComputerLevel();
			draw_board(1);
			if (!PlayerColor) 
				if (Players==1) 
			 		{
			 	 	Iterate();
			 	 	steps++;
				 	draw_board(1);
				 	}
			SetEventHandler(game_handler);
			break;
		}
 if (type==EVT_POINTERUP){
 	if (par1>=50 && par1<=550 && par2>=500 && par2<=550){
 		InvertArea(50, 500, 50, 50);
 		ComputerLevel=par1/50;
 		InvertArea(ComputerLevel*50, 500, 50, 50);
 		PartialUpdateBW(50, 500, 500, 50);
 		LevelHandler(EVT_KEYPRESS, KEY_OK, 0);
 	}
 }
 return 0;
}


int Chess_Start(int type, int par1, int par2)
{
 if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return; }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_BACK:
 		 	CloseApp();
 		 	break;
 		 
 		 case KEY_UP:
 		 	InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PlayerColor+=1;
 		 	PlayerColor=PlayerColor%2;
 		 	InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	if (PlayerColor==1)
 		 		DrawBitmap(230, 0, &PicWhite);
 		 		else
 		 		DrawBitmap(230, 0, &PicBlack);
 		 	PartialUpdateBW(230, 0, 140, 400);
 		 	break;
 		 
 		 case KEY_DOWN:
 		 	InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PlayerColor+=1;
 		 	PlayerColor=PlayerColor%2;
 		 	InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		 	if (PlayerColor==1)
 		 		DrawBitmap(230, 0, &PicWhite);
 		 		else
 		 		DrawBitmap(230, 0, &PicBlack);
 		 	PartialUpdateBW(230, 0, 140, 400);
 		 	break;
 		 
 		 case KEY_OK:
 			ClearScreen();
			if (NewGame==0)
				{
				 ClearScreen();
				 DrawTextRect(100, 400, 400, 50, GetLangText("@Diff_level"), ALIGN_CENTER);
				 ComputerLevel=1;
				 int i;
				 char s[3];
				 for (i=1; i<11; i++)
				 	{
			 		 sprintf(s, "%d", i);
					 DrawTextRect(i*50, 510, 50, 50, s, ALIGN_CENTER);
			 		}
				 InvertArea(50, 500, 50, 50);
				 DrawSymbol(20, 507, ARROW_LEFT);
				 DrawSymbol(560, 507, ARROW_RIGHT);
				 if (PlayerColor==0)
					DrawBitmap(230, 0, &PicWhite);
 				 	else
			 		DrawBitmap(230, 0, &PicBlack);
				 FullUpdate();
				 SetEventHandler(LevelHandler);
				}
				else
				LoadGame();
 		 	break;
 		}
 if (type==EVT_POINTERUP){
 	if ((par1>=100 && par1<=500 && par2>=495 && par2<=545)
 		|| (par1>=100 && par1<=500 && par2>=595 && par2<=645)){
 		InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		PlayerColor=((par2-495)/100+1)%2;
 		InvertArea(100, 495+((PlayerColor+1)%2)*100, 400, 50);
 		PartialUpdateBW(100, 495, 400, 200);
 		Chess_Start(EVT_KEYPRESS, KEY_OK, 0);
 	}
 }
 return 0;
}

int Chess_Player(int type, int par1, int par2) 
{
 if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return; }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_BACK:
 		 case KEY_PREV:
 		 	CloseApp();
 		 	break;
 		 
 		 case KEY_UP:
 		 	InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		 	Players+=1;
 		 	Players=Players%2;
 		 	InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 395, 400, 200);
 		 	break;
 		 
 		 case KEY_DOWN:
 		 	InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		 	Players+=1;
 		 	Players=Players%2;
 		 	InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		 	PartialUpdateBW(100, 395, 400, 200);
 		 	break;
 		 
 		 case KEY_OK:
 			ClearScreen();
	 		SetFont(cour36, BLACK);
			if (Players==1)
				{
	 			 DrawTextRect(100, 400, 400, 50, GetLangText("@You_play"), ALIGN_CENTER);
	 			 DrawTextRect(200, 500, 200, 50, GetLangText("@Chess_white"), ALIGN_CENTER);
	 		 	 DrawTextRect(200, 600, 200, 50, GetLangText("@Chess_black"), ALIGN_CENTER);
	 			 InvertArea(100, 495, 400, 50);
	 			 DrawBitmap(230, 0, &PicWhite);
	 			 FullUpdate();
				 SetEventHandler(Chess_Start);
				}
				else
				{
				 PlayerColor=1;
				 LoadGame();
				 draw_board(1);
				 SetEventHandler(game_handler);
				}
			break;
 		}
 if (type==EVT_POINTERUP){
 	if ((par1>=100 && par1<=500 && par2>=395 && par2<=445)
 		|| (par1>=100 && par1<=500 && par2>=495 && par2<=545)){
 		InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		Players=((par2-395)/100+1)%2;
 		InvertArea(100, 395+((Players+1)%2)*100, 400, 50);
 		PartialUpdateBW(100, 395, 400, 200);
 		Chess_Player(EVT_KEYPRESS, KEY_OK, 0);
 	}
 }
 return 0;
}

void ink_main(int argc, char **argv)
{
 OpenScreen();
 SetOrientation(orient=0);
 FileOptions = GetAssociatedFile(argv[0], 0);
 FilePosition = GetAssociatedFile(argv[0], 1);
fprintf(stderr, "[%s]\n",FileOptions);
 LoadGame();
 InkViewMain(game_handler);
}

/*
fprintf(stderr, "OK\n");
sleep(1);
*/