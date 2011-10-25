#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/times.h>
#include <limits.h>
#include <unistd.h>
#include "inkview.h"

typedef struct 
{
	int value;
	int cand[9];
	int ncand;
	unsigned char stat;
} t_field;

typedef struct 
{
	t_field grid[9][9];
	float rating;
	int viewcells;
} t_sudoku;

#define PLAYCELLSIZE 60
#define LABELCELLSIZE 45
#define MAINLINEWIDTH 3
#define BORDERSIZE 10
#define STARTY (400-(PLAYCELLSIZE*9)/2)
#define STARTX (300-(PLAYCELLSIZE*9)/2)
#define VIEWCELLSIZE (PLAYCELLSIZE-2*BORDERSIZE)
#define STARTYLABEL (390+(PLAYCELLSIZE*9)/2)
#define MAXSUDOKUCREATE 20

imenu LevelMenu[]=
{
	{ ITEM_ACTIVE, 601, "@Level1", NULL},
	{ ITEM_ACTIVE, 602, "@Level2", NULL},
	{ ITEM_ACTIVE, 603, "@Level3", NULL},
	{ ITEM_ACTIVE, 604, "@Level4", NULL},
	//		{ ITEM_ACTIVE, 605, "@Level5", NULL},
	{ 0, 0, NULL, NULL}
};

imenu PlayMenu[]=
{
	{ ITEM_HEADER, 0, "@Menu", NULL},
	{ ITEM_SUBMENU, 101, "@New_game", LevelMenu},
	{ ITEM_INACTIVE, 102, "@Restart_game", NULL},
	{ ITEM_SEPARATOR, 0, NULL, NULL},
	{ ITEM_INACTIVE, 103, "@Load_game", NULL},
	{ ITEM_INACTIVE, 104, "@Save_game", NULL},
	{ ITEM_SEPARATOR, 0, NULL, NULL},
	{ ITEM_ACTIVE, 105, "@Exit"},
	{ 0, 0, NULL, NULL}
};

int RatingLevel[6]={0, 2, 20, 70, 200, 1000};
int Candidates[10], SelectCandidates[10];

int LabelCandidates[10];
int MenuX, MenuY;

ifont *cour10, *cour12, *cour22, *cour36, *cour36bd, *cour72, *cour96, *cour16bd, *bottom_font, *newmenu_font, *newmenu_lfont, *mlabel_font;

extern const ibitmap Sudoku;
ibitmap *UnderMenu;
FILE *f;
int marknum=0;
long long marktime=0;

char *ProgramName;
char *FileSudoku;

t_sudoku GameSudoku, AnswerSudoku, TmpSudoku;

int Level, x, y, GameFlag, LoadFlag, Candidat, LabelsFlag;

int MustLink(t_sudoku *sudoku, int *fcl);
//int LevelHandler(int type, int par1, int par2);
int MenuHandler(int type, int par1, int par2);
int PlayHandler(int type, int par1, int par2);
void LoadGame();

long long timeinms() {

	struct timeval tv;

	gettimeofday(&tv, NULL);
	return ((long long)(tv.tv_sec) * 1000LL) + ((long long)(tv.tv_usec) / 1000LL);

}

void UpdatenCand(t_sudoku *sudoku) 
{
	int r, c, cc;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
 		{
		sudoku->grid[r][c].ncand = 0;
		for (cc=0; cc<9; cc++)
			if (sudoku->grid[r][c].cand[cc] != 0)
				sudoku->grid[r][c].ncand += 1;
	}
}

void RemoveCand(t_sudoku *sudoku, int row, int col) 
{
	int r, c;
	int br = (row / 3);
	int bc = (col / 3);
	int cc = sudoku->grid[row][col].value-1;
	for (c=0; c<9; c++)
		sudoku->grid[row][c].cand[cc] = 0;
	for (r=0; r<9; r++)
		sudoku->grid[r][col].cand[cc] = 0;
	for (r=0; r<9; r++)
		sudoku->grid[row][col].cand[r] = 0;
	for (r=br*3; r<(br+1)*3; r++)
		for (c=bc*3; c<(bc+1)*3; c++)
			sudoku->grid[r][c].cand[cc] = 0;
	UpdatenCand(sudoku);
}

void GetCand(t_sudoku *sudoku) 
{
	int c, r, cc;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			for (cc=0; cc<9; cc++)
				sudoku->grid[r][c].cand[cc] = cc+1;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (sudoku->grid[r][c].value != 0)
				RemoveCand(sudoku,r,c);
	UpdatenCand(sudoku);
}

void GetNearby(int row, int col, int nb[20][2]) 
{
	int r, c, i=0;
	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
	 	{
		nb[i][0] = (row / 3) * 3 + r;
		nb[i][1] = (col / 3) * 3 + c;
		if (! ((nb[i][0] == row) && (nb[i][1] == col)))
			i++;
	}
	for (c = 3; c < 9; c++)
	{
		nb[i][0] = row;
		nb[i][1] = ((col / 3) * 3 + c) % 9;
		i++;
	}
	for (r = 3; r < 9; r++)
 	{
		nb[i][0] = ((row / 3) * 3 + r) % 9;
		nb[i][1] = col;
		i++;
	}
}

int GetGeneral(int rc1[2], int rc2[2], int rc3[2], int nb[20][2], int NN) 
{
	int i, j, k, l, ncn;
	int nb1[20][2];
	int nb2[20][2];
	int nb3[30][2];
	GetNearby(rc1[0], rc1[1], nb1);
	GetNearby(rc2[0], rc2[1], nb2);
	if (NN==3) GetNearby(rc3[0], rc3[1], nb2);
	k = 0;
	for (i = 0; i < 20; i++)
 	{
		for (j = 0; j < 20; j++)
			if (NN==2){
			if ((nb1[i][0] == nb2[j][0]) && (nb1[i][1] == nb2[j][1])) 
			{
				nb[k][0] = nb1[i][0];
				nb[k][1] = nb1[i][1];
				k++;
				break;
			}
			else
				if ((nb3[i][0] == nb1[j][0]) && (nb3[i][1] == nb1[j][1]))
					for (l = 0; l < 20; l++)
						if ((nb3[i][0] == nb2[l][0]) && (nb3[i][1] == nb2[l][1]))
						{
				nb[k][0] = nb3[i][0];
				nb[k][1] = nb3[i][1];
				k++;
				goto next;
			}
		}
		next: ;
	}
	ncn = k;
	for (; k < 20; k++)
		nb[k][0] = nb[k][1] = -1;
	return(ncn);
}

void TrainProc(t_sudoku *sudoku, int train, int *cc, int c, char ent, int doElim, int *ntup, int hide) 
{
	int r, iqc, row=0, col=0, nelim;
	int nTC, nTCWC, nTCWOTC, nEC, nNTCWTC;
	int rows[9], cols[9], tupCands[9];
	nTC = nTCWOTC = nEC = nNTCWTC = 0;
	for (r = 0; r < 9; r++)
 	{
		switch (ent)
	 	{
		case 'c':
			col=c; row=r; break;
		case 'r':
			col=r; row=c; break;
		case 'b':
			row = (c / 3) * 3 + (r / 3);
			col = (c % 3) * 3 + (r % 3);
			break;
		}
		nTCWC = 0;
		for (iqc = 0; iqc < train; iqc++)
			if (sudoku->grid[row][col].cand[cc[iqc]] != 0)
		 	{
			if (nTCWC == 0)
			{
				if (hide==1)
				{
					rows[nTC] = row;
					cols[nTC] = col;
				}
				nTC++;
			}
			nTCWC++;
		}
		if (nTCWC > 0)
		{
			if (hide==1)
				nNTCWTC +=sudoku->grid[row][col].ncand - nTCWC;
			else
				if (sudoku->grid[row][col].ncand == nTCWC)
					nTCWOTC++;
			else
			{
				rows[nEC] = row;
				cols[nEC] = col;
				nEC++;
			}
		}
	}
	if (((nTC > train)&& (nTCWOTC == train)&&(hide==0))
		|| ((nTC == train)&& (nNTCWTC > 0)&&(hide==1)))
		{
		(*ntup)++;
		if (doElim == 1)
	 	{
			for (r = 0; r < 9; r++)
				tupCands[r] = 0;
			for (iqc=0; iqc<train; iqc++)
				tupCands[cc[iqc]] = 1;
			nelim = 0;
			if (hide==0)
		 	{
				for (iqc=0; iqc<nEC; iqc++)
					for (r = 0; r < 9; r++)
						if ((tupCands[r] == 1) && (sudoku->grid[rows[iqc]][cols[iqc]].cand[r] != 0))
						{
					nelim++;
					sudoku->grid[rows[iqc]][cols[iqc]].cand[r]  = 0;
					sudoku->grid[rows[iqc]][cols[iqc]].ncand   -= 1;
				}
			}
			else
				for (iqc=0; iqc<train; iqc++)
					for (r = 0; r < 9; r++)
						if ((tupCands[r] == 0) && (sudoku->grid[rows[iqc]][cols[iqc]].cand[r] != 0))
				 		{
				nelim++;
				sudoku->grid[rows[iqc]][cols[iqc]].cand[r]  = 0;
				sudoku->grid[rows[iqc]][cols[iqc]].ncand   -= 1;
			}
		}
	}
}

void FindTrain(t_sudoku *sudoku,int train, int itup, int c, int *cc,int colVals[9], int rowVals[9], int blkVals[9],int doElim,int *ntup, int hide) 
{
	int iqc, checktrain;
	for (cc[itup] = cc[itup-1]+1; cc[itup] < 9-train+itup+1; cc[itup]++)
		if (itup+1 < train)
			FindTrain(sudoku,train,itup+1,c,cc,colVals,rowVals,blkVals,doElim,ntup, hide);
	else
	{
		checktrain = 1;
		for (iqc = 0; iqc < train; iqc++)
		 	if (colVals[cc[iqc]] != 0)
				checktrain = 0;
		if (checktrain == 1) 
		{
			TrainProc(sudoku,train,cc,c,'c',doElim,ntup, hide);
			if ((doElim == 1) && (*ntup > 0))
			 	return;
		}
		checktrain = 1;
		for (iqc = 0; iqc < train; iqc++) 
			if (rowVals[cc[iqc]] != 0)
				checktrain = 0;
		if (checktrain == 1) 
		{
			TrainProc(sudoku,train,cc,c,'r',doElim,ntup, hide);
			if ((doElim == 1) && (*ntup > 0))
			 	return;
		}
		checktrain = 1;
		for (iqc = 0; iqc < train; iqc++) 
			if (blkVals[cc[iqc]] != 0)
				checktrain = 0;
		if (checktrain == 1) 
		{
			TrainProc(sudoku,train,cc,c,'b',doElim,ntup, hide);
			if ((doElim == 1) && (*ntup > 0))
			 	return;
		}
	}
}

void ReversSudoku(t_sudoku *sudoku) 
{
	int r, c;
	t_sudoku sudoku0;
	sudoku0=*sudoku;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			sudoku->grid[r][c] = sudoku0.grid[c][r];
}

int OverlayCells(t_sudoku *sudoku, int overlay, int cc,int *r, int *c, const char ent[12], int doElim) 
{
	int fc[9], fr[9];
	int i, j, nelim=0, noverlay=0;
	for (i=0; i < 9; i++)
		fr[i] = fc[i] = 0;
	for (i=0; i < overlay; i++)
		fr[r[i]] = fc[c[i]] = 1;
	if (strcmp(ent,"row") != 0) ReversSudoku(sudoku);
	for (i=0; i<9; i++)
		if (fc[i] == 0)


			for (j=0; j<overlay; j++)
				if (sudoku->grid[r[j]][i].cand[cc] != 0)
					goto done;
	for (i=0; i<9; i++)
		if (fr[i] == 0)
			for (j=0; j<overlay; j++)
				if (sudoku->grid[i][c[j]].cand[cc] != 0)
				{ 
		if (doElim == 1)
		{
			nelim++;
			sudoku->grid[i][c[j]].cand[cc] = 0;
			sudoku->grid[i][c[j]].ncand -=1;
		}
		else
		{
			noverlay = 1;
			goto done;
		}
	}
	done:
	if (strcmp(ent,"row") != 0) ReversSudoku(sudoku);
	return(noverlay);
}

void FindOverlay(t_sudoku *sudoku, int overlay, int ioverlay, int *r, int *c, int cc, int doElim, int *noverlay) 
{
	int rcc[4], ccc[4], nc, i, j;
	for (r[ioverlay]=r[ioverlay-1]+1; r[ioverlay]<9-overlay+ioverlay+1; r[ioverlay]++)
		for (c[ioverlay]=c[ioverlay-1]+1; c[ioverlay]<9-overlay+ioverlay+1; c[ioverlay]++)
			if (ioverlay+1 < overlay)
				FindOverlay(sudoku,overlay,ioverlay+1,r,c,cc,doElim,noverlay);
	else
	{
		for (i=0; i<overlay; i++)
			rcc[i] = ccc[i] = 0;
		for (i=0; i<overlay; i++)
			for (j=0; j<overlay; j++)
				if (sudoku->grid[r[i]][c[j]].cand[cc] > 0)
					rcc[i] = ccc[j] = 1;
		for (i=nc=0; i<overlay; i++)
			nc += rcc[i] + ccc[i];
		if (nc == 2*overlay)
		{
			*noverlay += OverlayCells(sudoku,overlay,cc,r,c,"row",doElim);
			if ((*noverlay > 0) && (doElim == 1)) return;
			*noverlay += OverlayCells(sudoku,overlay,cc,c,r,"column",doElim);
			if ((*noverlay > 0) && (doElim == 1)) return;
		}
	}
}

int SolveTechnic1(t_sudoku *sudoku, int doElim) 
{
	int r, c, cc, nc, br, bc, nhs=0, row=0, col=0, cnd;
	for (cc = 0; cc < 9; cc++)
		for (c = 0; c < 9; c++)
	 	{
		r = nc = 0;
		while((nc < 2) && (r < 9))
		{
		 	if (sudoku->grid[r][c].cand[cc] != 0) 
			{
				nc++;
				row = r;
			}
			r++;
		}
		if (nc == 1)
		{
			nhs++;
			if (doElim == 1)
			{
				col = c;
				cnd = cc;
				goto eliminate;
			}
		}
		r = nc = 0;
		while((nc < 2) && (r < 9)) 
		{
			if (sudoku->grid[c][r].cand[cc] != 0)
			{
				nc++;
				col = r;
			}
			r++;
		}
		if (nc == 1) 
		{
			nhs++;
			if (doElim == 1)
			{
				row = c;
				cnd = cc;
				goto eliminate;
			}
		}
		r = nc = 0;
		while((nc < 2) && (r < 9)) 
		{
			br = (c / 3) * 3 + (r / 3);
			bc = (c % 3) * 3 + (r % 3);
			if (sudoku->grid[br][bc].cand[cc] != 0)
			{
				nc++;
				row = br; col = bc;
			}
			r++;
		}
		if (nc == 1)
		{
			nhs++;
			if (doElim == 1)
			{
				cnd = cc;
				goto eliminate;
			}
		}
	}
	if (doElim == 0)
		return(nhs);
	else
		return(-1);

	eliminate:
	sudoku->grid[row][col].value = sudoku->grid[row][col].cand[cnd];
	if (sudoku->grid[row][col].stat == 0) sudoku->grid[row][col].stat = 3;
	RemoveCand(sudoku,row,col);
	return(1);
}

int SolveTechnic2(t_sudoku *sudoku, int doElim) 
{
	int r, c, cc, ns=0;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (sudoku->grid[r][c].ncand == 1)
		 	{
		if (doElim == 1)
		{
			for (cc=0; cc<9; cc++)
				if (sudoku->grid[r][c].cand[cc] > 0)
				{
				sudoku->grid[r][c].value = sudoku->grid[r][c].cand[cc];
				if (sudoku->grid[r][c].stat == 0) sudoku->grid[r][c].stat  = 3;
				RemoveCand(sudoku,r,c);
				return(1);
			}
		}
		else
			ns++;
	}
	if (doElim == 1)
		return(-1);
	else
		return(ns);
}

int SolveTechnic3(t_sudoku *sudoku, int train, int doElim, int hide) 
{
	int r, c, br, bc, *cc, ntup = 0;
	int rowVals[9], colVals[9], blkVals[9];
	cc = (int *) malloc(sizeof(int)*9);
	for (c = 0; c < 9; c++)
		cc[c] = 0;
	for (c = 0; c < 9; c++)
 	{
		for (r = 0; r < 9; r++)
			rowVals[r] = colVals[r] = blkVals[r] = 0;
		for (r = 0; r < 9; r++)
	 	{
			if (sudoku->grid[r][c].value != 0)
				colVals[sudoku->grid[r][c].value-1] = 1;
			if (sudoku->grid[c][r].value != 0)
				rowVals[sudoku->grid[c][r].value-1] = 1;
			br = (c / 3) * 3 + (r / 3);
			bc = (c % 3) * 3 + (r % 3);
			if (sudoku->grid[br][bc].value != 0)
				blkVals[sudoku->grid[br][bc].value-1] = 1;
		}
		for (cc[0] = 0; cc[0] < 9-train+1; cc[0]++)
		{
			FindTrain(sudoku,train,1,c,cc,colVals,rowVals,blkVals,doElim,&ntup, hide);
			if ((doElim == 1) && (ntup > 0))
				goto done;
		}
	}
	done:
	free(cc);
	return(ntup);
}

int SolveTechnic4(t_sudoku *sudoku, int doElim) 
{
	int br, bc, r, c, sr=0, sc=0, bbr, bbc, cc, nc, nr, ccc[3], rrr[3];
	int nelim, nlb = 0;
	for (br=0; br<3; br++)
		for (bc=0; bc<3; bc++)
			for (cc=0; cc < 9; cc++)
		 	{
		for (r=0; r<3; r++)
			ccc[r] = rrr[r] = -1;
		for (r=br*3, bbr=0; r<(br+1)*3; r++, bbr++)
			for (c=bc*3, bbc=0; c<(bc+1)*3; c++, bbc++)
				if (sudoku->grid[r][c].cand[cc] > 0)
				{
			ccc[bbc] = c;
			rrr[bbr] = r;
		}
		for (nc = nr = bbr = 0; bbr<3; bbr++)
		{
			if (rrr[bbr] >= 0)
				sr = rrr[bbr], nr++;
			if (ccc[bbr] >= 0)
				sc = ccc[bbr], nc++;
		}
		nelim = 0;
		if (nr == 1)
		{
			for (bbc=0; bbc<bc; bbc++)
				for (c=bbc*3; c<(bbc+1)*3; c++)
					if (sudoku->grid[sr][c].cand[cc] > 0)
					{
				if (doElim == 1)
				{
					nelim++;
					sudoku->grid[sr][c].cand[cc] = 0;
					sudoku->grid[sr][c].ncand -= 1;
				}
				else
				{
					nlb++;
					goto check_column;
				}
			}
			for (bbc=bc+1; bbc<3; bbc++)
				for (c=bbc*3; c<(bbc+1)*3; c++)
					if (sudoku->grid[sr][c].cand[cc] > 0)
					{
				if (doElim == 1)
				{
					nelim++;
					sudoku->grid[sr][c].cand[cc] = 0;
					sudoku->grid[sr][c].ncand -= 1;
				}
				else
				{
					nlb++;
					goto check_column;
				}
			}
		}
		check_column:
		if (nc == 1)
		{
			for (bbr=0; bbr<br; bbr++)
				for (r=bbr*3; r<(bbr+1)*3; r++)
					if (sudoku->grid[r][sc].cand[cc] > 0)
					{
				if (doElim == 1)
				{
					nelim++;
					sudoku->grid[r][sc].cand[cc] = 0;
					sudoku->grid[r][sc].ncand -= 1;
				}
				else
				{
					nlb++;
					goto next_candidate;
				}
			}
			for (bbr=br+1; bbr<3; bbr++)
				for (r=bbr*3; r<(bbr+1)*3; r++)
					if (sudoku->grid[r][sc].cand[cc] > 0)
					{
				if (doElim == 1)
				{
					nelim++;
					sudoku->grid[r][sc].cand[cc] = 0;
					sudoku->grid[r][sc].ncand -= 1;
				}
				else
				{
					nlb++;
					goto next_candidate;
				}
			}
		}
		next_candidate: continue;
		}
	return(nlb);
}

int SolveTechnic5(t_sudoku *sudoku, int doElim) 
{
	int r, c, b, bc, r0, c0, row, col, cc, nbc, blkc[3];
	int nelim = 0, nlb = 0;
	for (c = 0; c < 9; c++)
		for (cc=0; cc < 9; cc++)
	 	{
		for (nbc = b = 0; b < 3; b++)
		{
			blkc[b] = 0;
			for (bc = 0; bc < 3; bc++)
				if (sudoku->grid[b*3+bc][c].cand[cc] > 0)
				{
				blkc[b] = 1;
				nbc++;
				break;
			}
		}
		c0 = (c / 3) * 3;
		if (nbc == 1) 
			for (b = 0; b < 3; b++)
				if (blkc[b] == 1)
				{
			r0 = b * 3;
			for (r=0; r<9; r++)
			{
				row = r0 + r / 3;
				col = c0 + r % 3;
				if ((col != c) && (sudoku->grid[row][col].cand[cc]) != 0)
				{
					if (doElim == 1)
					{
						nelim++;
						sudoku->grid[row][col].cand[cc] = 0;
						sudoku->grid[row][col].ncand -= 1;
					}
					else
					{
						nlb++;
						goto check_rows;
					}
				}
			}
		}
		check_rows:
		for (nbc = b = 0; b < 3; b++) 
		{
			blkc[b] = 0;
			for (bc = 0; bc < 3; bc++)
				if (sudoku->grid[c][b*3+bc].cand[cc] > 0)
				{
				blkc[b] = 1;
				nbc++;
				break;
			}
		}
		r0 = (c / 3) * 3;
		if (nbc == 1) 
			for (b = 0; b < 3; b++)
				if (blkc[b] == 1)
				{
			c0 = b * 3;
			for (r=0; r<9; r++)
			{
				row = r0 + r / 3;
				col = c0 + r % 3;
				if ((row != c) && (sudoku->grid[row][col].cand[cc]) != 0)
				{
					if (doElim == 1)
					{
						nelim++;
						sudoku->grid[row][col].cand[cc] = 0;
						sudoku->grid[row][col].ncand -= 1;
					}
					else
					{
						nlb++;
						goto next_candidate;
					}
				}
			}
		}
		next_candidate: continue;
		}
	return(nlb);
}

int SolveTechnic6(t_sudoku *sudoku, int doElim, int NN) 
{
	int p[2], cc, x, y, z, a, b, i, nwing = 0, nelim = 0, nb, nbp1, nbp2, ncn, k, tmp;
	int neighb[20][2];
	int comNeighb[20][2];
	t_field fld;
	for (p[0] = 0; p[0] < 9; p[0]++)
		for (p[1] = 0; p[1] < 9; p[1]++)
			if (sudoku->grid[p[0]][p[1]].ncand == NN)
		 	{
		cc = x = y = z = 0;

		while (x == 0)
			x = sudoku->grid[p[0]][p[1]].cand[cc++];
		while (y == 0)
			y = sudoku->grid[p[0]][p[1]].cand[cc++];
		if (NN==3)
			while (z == 0)
				z = sudoku->grid[p[0]][p[1]].cand[cc++];
		GetNearby(p[0], p[1], neighb);
		for (k=0; k<3; k++)
		{
			if (NN==2)
				k=3;
			nbp1 = nbp2 = -1;
			for (nb = 0; nb < 20; nb++)
			{
				fld = sudoku->grid[neighb[nb][0]][neighb[nb][1]];
				if (NN==2)
				{
					if (fld.ncand == 2)
					{
						cc = a = b = z = 0;
						while (a == 0)
							a = fld.cand[cc++];
						while (b == 0)
							b = fld.cand[cc++];
						if ((a == x) && (b != y))
						{
							z = b;
							nbp1 = nb;
						}
						else
							if ((a != x) && (b == y))
							{
							z = a;
							y = x;
							x = b;
							nbp1 = nb;
						}
					}
				}
				else
					if ((fld.ncand == 2) && (fld.cand[x-1] != 0) && (fld.cand[z-1] != 0))
						nbp1 = nb;
				if (nbp1 >= 0)
				{
					for (nb = 0; nb < 20; nb++)
					{
						fld = sudoku->grid[neighb[nb][0]][neighb[nb][1]];
						if (NN==2)
						{
							if (fld.ncand == 2)
							{
								cc = a = b = 0;
								while (a == 0)
									a = fld.cand[cc++];
								while (b == 0)
									b = fld.cand[cc++];
								if (((a == y) && (b == z)) || ((a == z) && (b == y)))
									nbp2 = nb;
							}
						}
						else
							if ((fld.ncand == 2) && (fld.cand[y-1] != 0) && (fld.cand[z-1] != 0))
								nbp2 = nb;
						if (nbp2 >= 0)
						{
							ncn =GetGeneral(neighb[nbp1],neighb[nbp2], p, comNeighb, NN);
							if (((ncn <= 6) && (NN==2)) || ((ncn > 0) && (ncn < 3)))
							{
								i = nelim = 0;
								while(comNeighb[i][0] != -1)
								{
									if (sudoku->grid[comNeighb[i][0]][comNeighb[i][1]].cand[z-1] != 0)
									{
										if (doElim == 1)
										{
											sudoku->grid[comNeighb[i][0]][comNeighb[i][1]].cand[z-1] = 0;
											sudoku->grid[comNeighb[i][0]][comNeighb[i][1]].ncand--;
										}
										nelim++;
									}
									i++;
								}
								if (nelim > 0)
								{
									nwing++;
									if (doElim == 1)
										goto done;
								}
							}
						}
					}
					nbp1 = -1;
				}
			}
			tmp = x;
			x = y;
			y = z;
			z = tmp;
		}
	}
	done:
	return(nwing);
}

int SolveTechnic7(t_sudoku *sudoku, int overlay, int doElim) 
{
	int *r, *c, cc, noverlay=0;
	r = (int *) malloc(sizeof(int)*overlay);
	c = (int *) malloc(sizeof(int)*overlay);
	for (cc=0; cc<9; cc++)
		for (r[0]=0; r[0]<9-overlay+1; r[0]++)
			for (c[0]=0; c[0]<9-overlay+1; c[0]++)
			{
		FindOverlay(sudoku,overlay,1,r,c,cc,doElim,&noverlay);
		if ((doElim == 1) && (noverlay > 0))
			goto done;
	}
	done:
	free(c);
	free(r);
	return(noverlay);
}

void SetZeroGrid(t_field grid[9][9]) 
{
	int i, j, k;
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
		{
		grid[i][j].value = 0;
		for (k=0; k<9; k++)
		 	grid[i][j].cand[k] = 0;
		grid[i][j].ncand = 0;
		grid[i][j].stat = 0;
	}
}

void CreateDifferentGrid(t_field grid0[9][9], t_field grid1[9][9], t_field egrid[9][9]) 
{
	int r, c, cc;
	SetZeroGrid(egrid);
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
		{
		for (cc=0; cc<9; cc++)
			if (grid0[r][c].cand[cc] != grid1[r][c].cand[cc])
				egrid[r][c].cand[cc] = grid0[r][c].cand[cc] + grid1[r][c].cand[cc];
		if (grid0[r][c].value != grid1[r][c].value)
			egrid[r][c].value = grid0[r][c].value + grid1[r][c].value;
	}
}

void CreateGridValue(t_field grid0[9][9], t_field grid1[9][9], t_field egrid[9][9]) 
{
	int r, c;
	SetZeroGrid(egrid);
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if ((grid0[r][c].value == grid1[r][c].value) && (grid0[r][c].value != 0))
				egrid[r][c].value = grid0[r][c].value;
}

int CheckSudoku(t_sudoku sudoku) 
{
	int r, c, v, nv, ret=0, nc=0, br, bc;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
 		{
		nc += sudoku.grid[r][c].ncand;
		if ((sudoku.grid[r][c].value == 0) && (sudoku.grid[r][c].ncand < 1))
			ret--;
	}
	if (ret != 0) return(ret);
	for (r=0; r<9; r++)
		for (v=0; v<9; v++)
	 	{
		for (nv=c=0; c<9; c++)
			if (sudoku.grid[r][c].value == (v+1))
			 	nv++;
		if (nv > 1)
			ret--;
		for (nv=c=0; c<9; c++)
			if (sudoku.grid[c][r].value == (v+1))
 		 	 	nv++;
		if (nv > 1)
			ret--;
	}
	for (br=0; br<3; br++)
		for (bc=0; bc<3; bc++)
			for (v=0; v<9; v++)
			{
		nv = 0;
		for (r=br*3; r<(br+1)*3; r++)
			for (c=bc*3; c<(bc+1)*3; c++)
				if (sudoku.grid[r][c].value == (v+1))
					nv++;
		if (nv > 1)
			ret--;
	}
	if (ret != 0) return(ret);
	return(nc);
}

int ApplySolveTechnic(t_sudoku *sudoku, int elim[18], int depth) 
{
	int new=0, fcl, iet;
	if (CheckSudoku(*sudoku) == 0) return(-1);
	for (iet=1; iet<17; iet++)
	{
		if (depth >= iet)
			switch(iet)
		 	{
			case 1:
			if (SolveTechnic1(sudoku,1) > 0)
			{
				new = 1;
				elim[iet] += 1;
			}
			break;
			 case 2:
			if (SolveTechnic2(sudoku,1) > 0)
			{
				new = 1;
				elim[iet] += 1;
			}
			break;
			case 3:
			if ((new = SolveTechnic3(sudoku,2,1,1)) > 0)
				elim[3] += 1;
			break;
			case 4:
			if ((new = SolveTechnic3(sudoku,2,1,0)) > 0)
				elim[4] += 1;
			break;
			case 5:
			if ((new = SolveTechnic4(sudoku,1)) > 0)
				elim[iet] += 1;
			break;
			case 6:
			if ((new = SolveTechnic5(sudoku,1)) > 0)
				elim[iet] += 1;
			break;
			case 7:
			if ((new = SolveTechnic3(sudoku,3,1,1)) > 0)
				elim[7] += 1;
			break;
			case 8:
			if ((new = SolveTechnic3(sudoku,3,1,0)) > 0)
				elim[8] += 1;
			break;
			case 9:
			if ((new = SolveTechnic3(sudoku,4,1,1)) > 0)
				elim[9] += 1;
			break;
			case 10:
			if ((new = SolveTechnic3(sudoku,4,1,0)) > 0)
				elim[10] += 1;
			break;
			case 11:
			if ((new = SolveTechnic6(sudoku,1,2)) > 0)
				elim[11] += 1;
			break;
			case 12:
			if ((new = SolveTechnic6(sudoku,1,3)) > 0)
				elim[12] += 1;
			break;
			case 13:
			if ((new = SolveTechnic7(sudoku,2,1)) > 0)
				elim[13] += 1;
			break;
			case 14:
			if ((new = SolveTechnic7(sudoku,3,1)) > 0)
				elim[14] += 1;
			break;
			case 15:
			if ((new = SolveTechnic7(sudoku,4,1)) > 0)
				elim[15] += 1;
			break;
			case 16:
			if ((new = MustLink(sudoku,&fcl)) != 0)
				switch(new)
				{
				case 1:
				elim[17] += 1;
				break;
				case 2:
				elim[16] += 1;
				break;
				case 3:
				elim[16] += 1;
				break;
			}
			break;
		}
		if (new > 0) return(new);
	}
	return(new);
}

int MustLink(t_sudoku *sudoku, int *fcl) 
{
	int r, c, cc, row, col, cand, c0, c1, elim[18], ret, eofc0, eofc1;
	int minr=0, minc=0, mincand=0, minfcl, minrv, minstartr, minstartc;
	int fcrow=0, fccol=0, fccand=0, fclen, fcrv, fcstartr, fcstartc;
	unsigned char minstat=0, fcstat=0;
	t_sudoku sudokuc0, sudokuc1;

	t_field diffgridc0[9][9];
	t_field diffgridc1[9][9];
	t_field intgrid[9][9];

	minfcl = 81;
	minrv = 0;



	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (sudoku->grid[r][c].ncand == 2)
		 	{
		sudokuc0=*sudoku;
		sudokuc1=*sudoku;
		cc = 0;
		while ((cc < 9) && (sudoku->grid[r][c].cand[cc] == 0))
			cc++;
		c0 = cc;
		sudokuc0.grid[r][c].value = sudoku->grid[r][c].cand[cc];
		sudokuc0.grid[r][c].stat = 4;
		RemoveCand(&sudokuc0,r,c);
		cc++;
		while ((cc < 9) && (sudoku->grid[r][c].cand[cc] == 0))
			cc++;
		c1 = cc;
		sudokuc1.grid[r][c].value = sudoku->grid[r][c].cand[cc];
		sudokuc1.grid[r][c].stat = 4;
		RemoveCand(&sudokuc1,r,c);
		fclen = eofc0 = eofc1 = fcrv = 0;
		do
		{
			if ((eofc0 == 0) && (ApplySolveTechnic(&sudokuc0,elim,2) <= 0))
			{
				ret = CheckSudoku(sudokuc0);
				if (ret == 0)
				{
					fcrow = r;
					fccol = c;
					fccand = c0+1;
					fcstat = 4;
					fcrv = 1;
					eofc0 = eofc1 = 1;
				}
				else
					if (ret < 0)
					{
					fcrow = r;
					fccol = c;
					fccand = c1+1;
					fcstat = 3;
					fcrv = 2;
					eofc0 = eofc1 = 1;
				}
				else
					eofc0 = 1;
			}
			if ((eofc1 == 0) && (ApplySolveTechnic(&sudokuc1,elim,2) <= 0))
			{
				ret = CheckSudoku(sudokuc1);
				if (ret == 0)
				{
					fcrow = r;
					fccol = c;
					fccand = c1+1;

					fcstat = 4;
					fcrv = 1;
					eofc0 = eofc1 = 1;
				}
				else
					if (ret < 0)
					{
					fcrow = r;
					fccol = c;
					fccand = c0+1;
					fcstat = 3;
					fcrv = 2;
					eofc0 = eofc1 = 1;
				}
				else
					eofc1 = 1;
			}
			fclen++;
			if ((eofc0 == 0) || (eofc1 == 0))
			{
				CreateDifferentGrid(sudoku->grid,sudokuc0.grid,diffgridc0);
				CreateDifferentGrid(sudoku->grid,sudokuc1.grid,diffgridc1);
				CreateGridValue(diffgridc0,diffgridc1,intgrid);
				intgrid[r][c].value = 0;
				for (row=0; row<9; row++)
					for (col=0; col<9; col++)
						if ((cand = intgrid[row][col].value) != 0)
						{
					fcrow = row;
					fccol = col;
					fccand = cand;
					fcstat = 3;
					fcstartr = r;
					fcstartc = c;
					fcrv = 3;
					eofc0 = eofc1 = 1;
				}
			}
		} while ((eofc0 == 0) || (eofc1 == 0));
		if ((fcrv > 0) && (fclen < minfcl))
		{
			minr = fcrow;
			minc = fccol;
			minfcl = fclen;
			mincand = fccand;
			minstat = fcstat;
			minstartr = fcstartr;
			minstartc = fcstartc;
			minrv = fcrv;
		}
	}
	if (minfcl < 81)
	{
		sudoku->grid[minr][minc].value = mincand;
		sudoku->grid[minr][minc].stat = minstat;
		RemoveCand(sudoku,minr,minc);
		*fcl = minfcl;
		sudoku->rating += 4 * minfcl;
	}
	return(minrv);
}

void FindCellCand(t_field grid[9][9], int *r, int *c, int *cc) 
{
	int nc[9], i, ir, ic;
	int min_ncand = 10, min_nc = 82;
	if ( (*r > -1) && (*c > -1) && (grid[*r][*c].stat == 4) && (grid[*r][*c].ncand > 0) )
 	{
		*cc = 0;
		while ( (*cc < 9) && (grid[*r][*c].cand[*cc] == 0) ) (*cc) += 1;
		return;
	}
	for (i=0; i<9; i++)
		nc[i] = 0;
	for (ir=0; ir<9; ir++)
		for (ic=0; ic<9; ic++)
			for (i=0; i<9; i++)
				if (grid[ir][ic].cand[i] != 0)
					nc[i]++;
	for (i=0; i<9; i++)
		if ((nc[i] < min_nc) && (nc[i] > 0))
		{
		min_nc = nc[i];
		*cc = i;
	}
	for (ir=0; ir<9; ir++)
		for (ic=0; ic<9; ic++)
			if ( (grid[ir][ic].cand[*cc] != 0) && (grid[ir][ic].ncand < min_ncand) )
			{
		min_ncand = grid[ir][ic].ncand;
		*r = ir;
		*c = ic;
	}
}

int CountCellWithStat(t_field grid[9][9], unsigned char stat) 
{
	int r, c, nstat=0;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (grid[r][c].stat == stat)
				nstat++;
	return(nstat);
}

int Erase(t_sudoku *sudoku, int elim[18], int depth, int doRate) 
{
	int i, j, e, ne = 3;
	do
 	{
		i = e = 0;
		if (doRate)
	 	{
			for (j = 1; ((j <= 15) && (i < ne)); j++)
		 	{
				switch(j)
			 	{
				case 1:
					e = SolveTechnic1(sudoku,0);
					break;
				case 2:
					e = SolveTechnic2(sudoku,0);
					break;
				case 3:
					e = SolveTechnic3(sudoku,2,0,1);
					break;
				case 4:
					e = SolveTechnic3(sudoku,2,0,0);
					break;
				case 5:
					e = SolveTechnic4(sudoku,0);
					break;
				case 6:
					e = SolveTechnic5(sudoku,0);
					break;
				case 7:
					e = SolveTechnic3(sudoku,3,0,1);
					break;
				case 8:
					e = SolveTechnic3(sudoku,3,0,0);
					break;
				case 9:
					e = SolveTechnic3(sudoku,4,0,1);
					break;
				case 10:
					e = SolveTechnic3(sudoku,4,0,0);
					break;
				case 11:
					e = SolveTechnic6(sudoku,0,2);
					break;
				case 12:
					e = SolveTechnic6(sudoku,0,3);
					break;
				case 13:
					e = SolveTechnic7(sudoku,2,0);
					break;
				case 14:
					e = SolveTechnic7(sudoku,3,0);
					break;
				case 15:
					e = SolveTechnic7(sudoku,4,0);
					break;
				}
				e = ( (e+i) > ne ? (ne-i) : e );
				i += e;
				sudoku->rating += (e * j) / ((float) ne);
			}
			if ((i < ne) && (CountCellWithStat((*sudoku).grid,0) > 0))
				sudoku->rating += ((ne - i) * 16) / ((float) ne);
		}
	} while (ApplySolveTechnic(sudoku,elim,depth) > 0);
	return (CheckSudoku(*sudoku));
}

int SolveSudokuRect(t_sudoku *sudoku, t_sudoku *dsudoku, int elim[18], int r, int c, int cc, int depth, int doRate) 
{
	t_sudoku esudoku;
	int ret = 1, r1=-1, c1=-1, cc1=-1;
	int backtrack_depth = depth < 15 ? depth : 15;
	while(ret != 0)
 	{
		ret = Erase(dsudoku, elim, depth, doRate);
		if (ret > 0)
		{
			FindCellCand(dsudoku->grid,&r1,&c1,&cc1);
			if ( (r1 >= 9) || (r1 < 0) || (c1 >= 9) || (c1 < 0) || (cc1 >= 9) || (cc1 < 0) )
				return(0);
			dsudoku->grid[r1][c1].stat  = 4;
			esudoku=*dsudoku;
			esudoku.grid[r1][c1].value = cc1+1;
			elim[17] += 1;
			RemoveCand(&esudoku,r1,c1);
			ret = SolveSudokuRect(dsudoku, &esudoku, elim, r1, c1, cc1, backtrack_depth, doRate);
			dsudoku->rating = esudoku.rating;
		} 
		else 
			if (ret < 0)
			{
			if ( (r >= 9) || (r  < 0) || (c >= 9) || (c  < 0) || (cc >= 9) || (cc < 0) )
				return(0);			
			if (sudoku->grid[r][c].stat == 4) 
			{
				sudoku->grid[r][c].cand[cc] = 0;
				sudoku->grid[r][c].ncand -= 1;
				elim[0] += 1;
			}
			return(ret);
		}
		if (ret == 0) *sudoku=*dsudoku;
	}
	return(ret);
}

void SolveSudoku(int elim[18], int depth, int doRate)
{
	int i, j, ret, rating;
	t_sudoku tmpSudoku;
	for (i=0; i<18; i++)
		elim[i] = 0;
	GetCand(&TmpSudoku);
	tmpSudoku=TmpSudoku;
	ret = SolveSudokuRect(&TmpSudoku,&tmpSudoku,elim,0,0,0,depth, doRate);
	if (ret == 0 && doRate == 1)
 	{
		if ((elim[16]+elim[13]+elim[14]+elim[15]+elim[17]+elim[11]+elim[12]) > 0)
			rating = 100;
		else 
			if ((elim[5]+elim[6]+elim[4]+elim[8]+elim[7]+elim[10]+elim[9]) > 0)
				rating = 10;
		else
			rating = 1;         
		for (i=0; i<9; i++)
			for (j=0; j<9; j++)
				if (TmpSudoku.grid[i][j].stat != 1) rating--;
		TmpSudoku.rating += rating;
	}
}

int CheckAlone(t_sudoku sudoku, t_sudoku ssudoku) 
{
	int r, c, ret, elim[18];
	t_sudoku csudoku;
	t_sudoku dsudoku;
	t_sudoku esudoku;
	csudoku=sudoku;
	GetCand(&csudoku);
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (ssudoku.grid[r][c].stat == 4)
	 		{
		dsudoku=csudoku;
		dsudoku.grid[r][c].cand[ssudoku.grid[r][c].value-1] = 0;
		dsudoku.grid[r][c].ncand -= 1;
		esudoku=dsudoku;
		ret = SolveSudokuRect(&dsudoku,&esudoku,elim,4,0,0,2,0);
		if (ret == 0)
			return(-1);
	}
	return(0);
}

void ResetSudoku(t_sudoku *sudoku) 
{
	int i, j, k;
	sudoku->rating = 0;
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
		{
		sudoku->grid[i][j].value = 0;
		for (k=0; k<9; k++) sudoku->grid[i][j].cand[k] = k+1;
		sudoku->grid[i][j].ncand = 9;
		sudoku->grid[i][j].stat = 0;
	}
}

int FindNext(int *r, int *c, int *cc) 
{
	int nc[9], i, ir, ic;
	int max_ncand = -1, max_nc = -1;
	int ng, index;
	ng = CountCellWithStat(TmpSudoku.grid,1);
	if (ng < 8)
 	{
		i=CountCellWithStat(TmpSudoku.grid, 0);
		index=rand();
		index = index % i;
		for (ir=0; ir<9; ir++)
			for (ic=0; ic<9; ic++)
				if (TmpSudoku.grid[ir][ic].stat == 0)
				{
			if (index > 0)
				index--;
			else
			{
				index=ir*9+ic;
				goto ext;
			}
		}
		ext:
		*r = index / 9;
		*c = index % 9;
		*cc = ng;
		return(0);
	}
	for (i=0; i<9; i++)
		nc[i] = 0;
	for (ir=0; ir<9; ir++)
		for (ic=0; ic<9; ic++)
			for (i=0; i<9; i++)
				if (TmpSudoku.grid[ir][ic].cand[i] != 0)
					nc[i]++;
	for (i=0; i<9; i++)
		if ((nc[i] > max_nc) && (nc[i] > 0))
		{
		max_nc = nc[i];
		*cc = i;
	}
	if (max_nc < 1)
		return(-1);
	for (ir=0; ir<9; ir++)
		for (ic=0; ic<9; ic++)
			if ( (TmpSudoku.grid[ir][ic].cand[*cc] != 0) && (TmpSudoku.grid[ir][ic].ncand > max_ncand) )
			{
		max_ncand = TmpSudoku.grid[ir][ic].ncand;
		*r = ir;
		*c = ic;
	}
	return(0);
}

void CreateSudoku() 
{
	int r, c, cc, ng, ngiv, gr, gc;
	int elim[18], ret;
	t_sudoku csudoku;
	do
	{
		ResetSudoku(&TmpSudoku);
		gr=gc=-1;
		ng=0;
		ret=1;
		do
		{
			if (FindNext(&r,&c,&cc) < 0)
		 	{
				ret = -1;
				break;
			}
			TmpSudoku.grid[r][c].stat = 1;
			TmpSudoku.grid[r][c].value = cc+1;
			ng++;
			GetCand(&TmpSudoku);
			if (ng > 8) ret = Erase(&TmpSudoku,elim,4,0);
		} 
		while(ret > 0);
	} 
	while (ret < 0);
	AnswerSudoku=TmpSudoku;
	ResetSudoku(&GameSudoku);
	for (ng=r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (TmpSudoku.grid[r][c].stat == 1)
			{
		GameSudoku.grid[r][c].value = TmpSudoku.grid[r][c].value;
		GameSudoku.grid[r][c].stat = TmpSudoku.grid[r][c].stat;
		ng++;
	}
	csudoku=TmpSudoku;
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (((r == 4) || (c == 4)) && (GameSudoku.grid[r][c].stat == 1))
			{
		csudoku=GameSudoku;
		csudoku.grid[r][c].value = 0;
		csudoku.grid[r][c].stat = 0;
		TmpSudoku=csudoku;
		SolveSudoku(elim,4,0);
		if (CheckAlone(csudoku,TmpSudoku) == 0)
		{
			GameSudoku.grid[r][c].value = 0;
			GameSudoku.grid[r][c].stat = 0;
		}
	}
	for (r=0; r<9; r++)
		for (c=0; c<9; c++)
			if (((r != 4) && (c != 4)) && (GameSudoku.grid[r][c].stat == 1))
			{
		csudoku=GameSudoku;
		csudoku.grid[r][c].value = 0;
		csudoku.grid[r][c].stat = 0;
		TmpSudoku=csudoku;
		SolveSudoku(elim,4,0);
		if (CheckAlone(csudoku,TmpSudoku) == 0)
		{
			GameSudoku.grid[r][c].value = 0;
			GameSudoku.grid[r][c].stat = 0;
		}
	}
	for (r=ngiv=0; r<9; r++)
		for (c=0; c<9; c++)
			if (GameSudoku.grid[r][c].stat == 1)
				ngiv++;
	GameSudoku.viewcells=ngiv;
	TmpSudoku=GameSudoku;
	SolveSudoku(elim,17,1);
	GameSudoku.rating = TmpSudoku.rating;
}

int LoadSudoku()
{
	FILE *f;
	if ((f=fopen(FileSudoku,"rb"))==NULL)
		return 0;
 	else
		if (fread(&TmpSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
 		{
		fclose(f);
		return 0;
	}
	else
	 	if (fread(&GameSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
		{
		fclose(f);
		return 0;
	}
	else
		if (fread(&AnswerSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
		{
		fclose(f);
		return 0;
	}
	else
	{
		fclose(f);
		return 1;
	}

}

void panel_update_proc() {

	DrawPanel(NULL, "", NULL, -1);
	PartialUpdate(0, ScreenHeight()-PanelHeight(), ScreenWidth(), PanelHeight());
	SetHardTimer("PANELUPDATE", panel_update_proc, 110000);

}

void DrawSudoku()
{
	int i, j, k;
	char s[16];
	ClearScreen();
	FillArea(STARTX-MAINLINEWIDTH/2, STARTY-MAINLINEWIDTH/2, PLAYCELLSIZE*9+(MAINLINEWIDTH/2)*2, PLAYCELLSIZE*9+(MAINLINEWIDTH/2)*2, BLACK);
	FillArea(STARTX+MAINLINEWIDTH/2, STARTY+MAINLINEWIDTH/2, PLAYCELLSIZE*9-(MAINLINEWIDTH/2)*2, PLAYCELLSIZE*9-(MAINLINEWIDTH/2)*2, WHITE);
	for (i=1; i<9; i++)
		DrawLine(STARTX+i*PLAYCELLSIZE, STARTY, STARTX+i*PLAYCELLSIZE, STARTY+9*PLAYCELLSIZE, BLACK);
	for (i=1; i<9; i++)
		DrawLine(STARTX, STARTY+i*PLAYCELLSIZE, STARTX+9*PLAYCELLSIZE, STARTY+i*PLAYCELLSIZE, BLACK);
	for (i=1; i<3; i++)
		FillArea(STARTX+3*i*PLAYCELLSIZE-2, STARTY-MAINLINEWIDTH/2, MAINLINEWIDTH, 9*PLAYCELLSIZE, BLACK);
	for (i=1; i<3; i++)
		FillArea(STARTX-MAINLINEWIDTH/2, STARTY+3*i*PLAYCELLSIZE-2, 9*PLAYCELLSIZE, MAINLINEWIDTH, BLACK);
	DrawBitmap(150, 10, &Sudoku);
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
			if (TmpSudoku.grid[i][j].stat==1)
 			{
				sprintf(s, "%d", TmpSudoku.grid[i][j].value);
				if (TmpSudoku.grid[i][j].value>0)
				{
					if (GameSudoku.grid[i][j].stat==1)
					{
						SetFont(cour36bd, BLACK);
						DrawTextRect(STARTX+j*PLAYCELLSIZE+BORDERSIZE, STARTY+i*PLAYCELLSIZE+BORDERSIZE-3, VIEWCELLSIZE, VIEWCELLSIZE, s, ALIGN_CENTER);
					}
					else
					{
						SetFont(cour36, BLACK);
						DrawTextRect(STARTX+j*PLAYCELLSIZE+BORDERSIZE, STARTY+i*PLAYCELLSIZE+BORDERSIZE-3, VIEWCELLSIZE, VIEWCELLSIZE, s, ALIGN_CENTER);
					}
				}
			}
	SetFont(cour16bd, BLACK);
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
			if (TmpSudoku.grid[i][j].ncand>0)
 			{
				s[0]='\0';
				// 			 sprintf(s, "");
				for (k=0; k<9; k++)
					if (TmpSudoku.grid[i][j].cand[k]!=0)
						sprintf(s, "%s%d", s, k+1);
				DrawTextRect(STARTX+j*PLAYCELLSIZE+3, STARTY+i*PLAYCELLSIZE+1, PLAYCELLSIZE-3, (3*BORDERSIZE)/2, s, ALIGN_CENTER);
				if (strlen(s)>5)
				{
					DrawTextRect(STARTX+j*PLAYCELLSIZE+3, STARTY+(i+1)*PLAYCELLSIZE-2*BORDERSIZE, PLAYCELLSIZE-3, (3*BORDERSIZE)/2, &s[6], ALIGN_CENTER);
					InvertArea(STARTX+j*PLAYCELLSIZE+3, STARTY+(i+1)*PLAYCELLSIZE-2*BORDERSIZE, PLAYCELLSIZE-4, (3*BORDERSIZE)/2);
				}
			}
	x=4;
	y=4;
	InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
	SetHardTimer("PANELUPDATE", panel_update_proc, 110000);
	DrawPanel(NULL, "", NULL, -1);
	FullUpdate();
}

void CreateSudokuLevel()
{
	char s[64];
	int i, j, k ,rating, maxrating;
	rating=-1;
	ClearScreen();
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
 		{
		TmpSudoku.grid[i][j].stat=0;
		TmpSudoku.grid[i][j].value=0;
		TmpSudoku.grid[i][j].ncand=0;
		for (k=0; k<9; k++)
 		 	TmpSudoku.grid[i][j].cand[k]=0;
	}
	DrawSudoku();
	SetFont(bottom_font, BLACK);
	sprintf(s, "%s.", GetLangText("@CreatingSudoku"));
	DrawTextRect(50, STARTY+9*PLAYCELLSIZE+25, 500, 50, s, ALIGN_LEFT);
	PartialUpdateBW(50, STARTY+9*PLAYCELLSIZE+25, 500, 100);
	ShowHourglass();
	j=1;
	k=0;
	maxrating=-1;
	while ( ((rating<RatingLevel[Level]) || (rating>=RatingLevel[Level+1])) && (k<MAXSUDOKUCREATE) )
 	{
		CreateSudoku();
		rating=(int)GameSudoku.rating;
		fprintf(stderr, "%i\n", rating);
		if (rating>maxrating && rating<RatingLevel[Level+1])
	 	{
			TmpSudoku=GameSudoku;
			maxrating=rating;
	 	}
		sprintf(s, "%s.", GetLangText("@CreatingSudoku"));
		FillArea(50, STARTY+9*PLAYCELLSIZE+25, 500, 50, WHITE);
		for (i=0; i<j; i++)
			sprintf(s, "%s.", s);
		DrawTextRect(50, STARTY+9*PLAYCELLSIZE+25, 500, 100, s, ALIGN_LEFT);
		PartialUpdateBW(50, STARTY+9*PLAYCELLSIZE+25, 500, 100);
		j++;
		if (j>4) j=0;
		k++;
 	}
	if (k==MAXSUDOKUCREATE)
		GameSudoku=TmpSudoku;
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
 		{
		if (GameSudoku.grid[i][j].stat!=1)
		{
			GameSudoku.grid[i][j].stat=0;
			GameSudoku.grid[i][j].value=0;
		}
		GameSudoku.grid[i][j].ncand=0;
		for (k=0; k<9; k++)
 		 	GameSudoku.grid[i][j].cand[k]=0;
	}
	HideHourglass();
	TmpSudoku=GameSudoku;
	DrawSudoku();
	SetEventHandler(PlayHandler);
}

void SaveSudoku()
{
	FILE *f;
	if ((f=fopen(FileSudoku,"wb"))==NULL)
		return;
	else if (fwrite(&TmpSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
	{
		fclose(f);
		return;
	}
	else if (fwrite(&GameSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
	{
		fclose(f);
		return;
	}
	else if (fwrite(&AnswerSudoku, 1, sizeof(t_sudoku), f)!=sizeof(t_sudoku))
	{
		fclose(f);
		return;
	}
	else
	{
		fclose(f);
		return;
	}
}

void DialogHandler(int Button)
{
	Button==1 ? LoadGame() : CloseApp();
}

void PlayMenuHandler(int index)
{
	switch(index)
 	{
	case 102:
		TmpSudoku=GameSudoku;
		DrawSudoku();
		SetEventHandler(PlayHandler);
		break; 	 	

	case 103:
	 	GameFlag=1;
	 	LoadSudoku();
 	 	DrawSudoku();
 	 	SetEventHandler(PlayHandler);
	 	break;
	 	
	case 104:
	 	LoadFlag=1;
	 	SaveSudoku();
	 	break;
	 	
	case 105:
	 	CloseApp();
	 	break;
	 	
	case 601:
	 	GameFlag=1;
	 	Level=0;
	 	CreateSudokuLevel();
	 	break;

	case 602:
	 	GameFlag=1;
	 	Level=1;
	 	CreateSudokuLevel();
	 	break;

	case 603:
	 	GameFlag=1;
	 	Level=2;
	 	CreateSudokuLevel();
	 	break;

	case 604:
	 	GameFlag=1;
	 	Level=3;
	 	CreateSudokuLevel();
	 	break;

	case 605:
	 	GameFlag=1;
	 	Level=4;
	 	CreateSudokuLevel();
	 	break;

	default:
	 	if (GameFlag==0)
	 	{
//	 		OpenMenu(PlayMenu, 0, 250, 350, PlayMenuHandler);
			CloseApp();
	 	}
	 	break;
 	}
}

int VerifyWin()
{
	int i, j, v1, v2, v3;

	for (i=0; i<9; i++) {
		v1=v2=v3=0;
		for (j=0; j<9; j++) {
			v1 |= (1 << TmpSudoku.grid[i][j].value);
			v2 |= (1 << TmpSudoku.grid[j][i].value);
			v3 |= (1 << TmpSudoku.grid[(i%3)*3+(j%3)][(i/3)*3+(j/3)].value);
		}
		if (v1 != 0x3fe || v2 != 0x3fe || v3 != 0x3fe) return 0;
	}
	return 1;
}

void CreateMenu()
{
	PlayMenu[5].type=ITEM_ACTIVE;
	PlayMenu[2].type=ITEM_ACTIVE;
}

int PointerMenuHandler(int type, int par1, int par2){

	int px=par1-130;
	int py=par2-345;
	
	if (type == EVT_POINTERUP && px>0 && px<8*LABELCELLSIZE && py>0 && py<3*LABELCELLSIZE){
	 	if (MenuX==3) 
			InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
		if (MenuX<3) 
			InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
		if (MenuX>3) 
			InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);

		if (px<=3*LABELCELLSIZE){
			MenuX=px/LABELCELLSIZE;
		} else if (px<=3*LABELCELLSIZE+65){
			MenuX=3;
		} else {
			MenuX=(px-65)/LABELCELLSIZE+1;
		}		
		MenuY=py/LABELCELLSIZE;

	 	if (MenuX==3) 
			InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
		if (MenuX<3) 
			InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
		if (MenuX>3) 
			InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);

		PartialUpdateBW(110, 300, 380, 200);
		MenuHandler(EVT_KEYPRESS, KEY_OK, 0);
		return 1;
	}
	return 0;
}

int MenuHandler(int type, int par1, int par2)
{
	char s[15];
	int i, n, vcells;
	if (type==EVT_KEYPRESS){
		int res = 0;
		switch (par1)
 		{
		case KEY_LEFT:
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
 		 	MenuX--;
 		 	if (MenuX<0)
 		 		MenuX=6;
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			PartialUpdateBW(110, 300, 380, 200);
			res = 1;
 		 	break;
 		 	
		case KEY_RIGHT:
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
 		 	MenuX++;
 		 	if (MenuX>6)
 		 		MenuX=0;
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			PartialUpdateBW(110, 300, 380, 200);
			res = 1;
 		 	break;
 		 	
		case KEY_UP:
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
 		 	MenuY--;
 		 	if (MenuY<0)
 		 		MenuY=2;
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			PartialUpdateBW(110, 300, 380, 200);
			res = 1;
 		 	break;
 		 	
		case KEY_DOWN:
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
 		 	MenuY++;
 		 	if (MenuY>2)
 		 		MenuY=0;
 		 	if (MenuX==3) 
				InvertArea(266, 341+MenuY*LABELCELLSIZE, 64, LABELCELLSIZE-1);
			if (MenuX<3) 
				InvertArea(131+MenuX*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			if (MenuX>3) 
				InvertArea(331+(MenuX-4)*LABELCELLSIZE, 341+MenuY*LABELCELLSIZE, LABELCELLSIZE-1, LABELCELLSIZE-1);
			PartialUpdateBW(110, 300, 380, 200);
			res = 1;
 		 	break;

		case KEY_OK:
 		 	if (MenuX==3)
			{
				if (MenuY==2)
				{
					CreateMenu();
					OpenMenu(PlayMenu, 0, 250, 350, PlayMenuHandler);
				}
				if (MenuY==1)
				{
					DrawBitmap(110, 300, UnderMenu);
					if (marknum != 0) {
						n=0;
						TmpSudoku.grid[y][x].value = 0;
						for(i=1; i<10; i++)
						{
							TmpSudoku.grid[y][x].cand[i-1]=LabelCandidates[i];
							if (TmpSudoku.grid[y][x].cand[i-1]==1)
								n++;
						}
						TmpSudoku.grid[y][x].ncand=n;
						FillArea(STARTX+x*PLAYCELLSIZE+2, STARTY+y*PLAYCELLSIZE+2, PLAYCELLSIZE-4, PLAYCELLSIZE-4, BLACK);
						//FillArea(STARTX+x*PLAYCELLSIZE+3, STARTY+(y+1)*PLAYCELLSIZE-2*BORDERSIZE, PLAYCELLSIZE-5, (3*BORDERSIZE)/2, WHITE);
						s[0]='\0';
						//						 sprintf(s, "");
						for (i=0; i<9; i++)
							if (TmpSudoku.grid[y][x].cand[i]!=0)
								sprintf(s, "%s%d", s, i+1);
						SetFont(cour16bd, WHITE);
						DrawTextRect(STARTX+x*PLAYCELLSIZE+3, STARTY+y*PLAYCELLSIZE+1, PLAYCELLSIZE-5, (3*BORDERSIZE)/2, s, ALIGN_CENTER);
						//InvertArea(STARTX+x*PLAYCELLSIZE+3, STARTY+y*PLAYCELLSIZE+1, PLAYCELLSIZE-5, (3*BORDERSIZE)/2);
						//if (strlen(s)>5)
						// 	 DrawTextRect(STARTX+x*PLAYCELLSIZE+3, STARTY+(y+1)*PLAYCELLSIZE-2*BORDERSIZE, PLAYCELLSIZE-3, (3*BORDERSIZE)/2, &s[6], ALIGN_CENTER);
						//InvertArea(STARTX+x*PLAYCELLSIZE+3, STARTY+(y+1)*PLAYCELLSIZE-2*BORDERSIZE, PLAYCELLSIZE-5, (3*BORDERSIZE)/2);
					}
					PartialUpdateBW(0, 0, 600, 800);
					SetEventHandler(PlayHandler);
				}
			}
 		 	if (MenuX<3 || (MenuX==3 && MenuY==0))
			{
				DrawBitmap(110, 300, UnderMenu);
				TmpSudoku.viewcells++;
				for(i=1; i<10; i++)
				{
					TmpSudoku.grid[y][x].cand[i-1]=0;
					TmpSudoku.grid[y][x].ncand=0;
				}
				LabelCandidates[0]=MenuY*3+MenuX;
				if (MenuX==3) LabelCandidates[0] = -1;
				TmpSudoku.grid[y][x].value=LabelCandidates[0]+1;
				if (TmpSudoku.grid[y][x].value>0)
				{
					TmpSudoku.grid[y][x].stat=1;
					if (GameSudoku.grid[y][x].stat==1)
						SetFont(cour36bd, BLACK);
					else
						SetFont(cour36, BLACK);
					sprintf(s, "%d", TmpSudoku.grid[y][x].value);
					FillArea(STARTX+x*PLAYCELLSIZE+2, STARTY+y*PLAYCELLSIZE+2, PLAYCELLSIZE-4, PLAYCELLSIZE-4, BLACK);
					//FillArea(STARTX+x*PLAYCELLSIZE+BORDERSIZE, STARTY+y*PLAYCELLSIZE+BORDERSIZE+, VIEWCELLSIZE, VIEWCELLSIZE, BLACK);
					InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
					DrawTextRect(STARTX+x*PLAYCELLSIZE+BORDERSIZE, STARTY+y*PLAYCELLSIZE+BORDERSIZE-3, VIEWCELLSIZE, VIEWCELLSIZE, s, ALIGN_CENTER);
					InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
					PartialUpdateBW(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
				}
				else
				{
					InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
					FillArea(STARTX+x*PLAYCELLSIZE+2, STARTY+y*PLAYCELLSIZE+2, PLAYCELLSIZE-4, PLAYCELLSIZE-4, WHITE);
					InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
					PartialUpdateBW(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
				}
				PartialUpdateBW(110, 300, 380, 200);
				vcells = 0;
				for (i=0; i<81; i++) {
					if (TmpSudoku.grid[i/9][i%9].value != 0) vcells++;
				}
				if (vcells == 81) {
					if (VerifyWin()) {
						Message(ICON_INFORMATION, GetLangText("@SudokuSolved"), GetLangText("@SudokuSolved"), 5000);
					}
				}
				SetEventHandler(PlayHandler);
			}

			if (MenuX>3)
			{
				if (marknum==MenuY*3+MenuX-3 && timeinms()-marktime <= 700) {
					MenuX=3;
					MenuY=1;
					MenuHandler(EVT_KEYPRESS, KEY_OK, 0);
				} else {
					marknum=MenuY*3+MenuX-3;
					marktime=timeinms();
					InvertArea(348+(MenuX-4)*LABELCELLSIZE, 344+MenuY*LABELCELLSIZE, LABELCELLSIZE-24, LABELCELLSIZE-24);
					//InvertArea(330+(MenuX-4)*LABELCELLSIZE, 340+MenuY*LABELCELLSIZE, LABELCELLSIZE, LABELCELLSIZE);
					LabelCandidates[MenuY*3+MenuX-3]=1-LabelCandidates[MenuY*3+MenuX-3];
					PartialUpdateBW(110, 300, 380, 200);
				}
			}
			res = 1;
 		 	break;
 		}
		return res;
	}
	if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return 0; }
	if (type == EVT_POINTERDOWN || type == EVT_POINTERUP || type == EVT_POINTERMOVE || type == EVT_POINTERHOLD || type == EVT_POINTERLONG){
		return PointerMenuHandler(type, par1, par2);
	}
	return 0;
}

void DrawNewMenu()
{
	int i, j, k;
	char s[16];
	if (UnderMenu==NULL)
		UnderMenu=NewBitmap(380, 200);
	UnderMenu=BitmapFromScreen(110, 300, 380, 200);
	FillArea(110, 300, 380, 200, WHITE);
	DrawRect(110, 301, 380, 198, BLACK);
	DrawRect(111, 300, 378, 200, BLACK);
	SetFont(newmenu_lfont, BLACK);
	for(i=0; i<4; i++)
		DrawLine(270, 340+LABELCELLSIZE*i, 330, 340+LABELCELLSIZE*i, BLACK);
	DrawTextRect(130, 309, 140, 50, GetLangText("@Sudoku_select"), ALIGN_CENTER);
	DrawTextRect(330, 309, 140, 50, GetLangText("@Sudoku_labels"), ALIGN_CENTER);
	DrawTextRect(265, 350, 65, 50, "x", ALIGN_CENTER);
	DrawTextRect(265, 395, 65, 50, GetLangText("@OK"), ALIGN_CENTER);
	DrawTextRect(265, 440, 65, 50, GetLangText("@Menu"), ALIGN_CENTER);
	for(i=0; i<4; i++)
		DrawLine(130+i*LABELCELLSIZE, 340, 130+i*LABELCELLSIZE, 475, BLACK);
	for(i=0; i<4; i++)
		DrawLine(330+i*LABELCELLSIZE, 340, 330+i*LABELCELLSIZE, 475, BLACK);
	for(i=0; i<4; i++)
		DrawLine(130, 340+i*LABELCELLSIZE, 270, 340+i*LABELCELLSIZE, BLACK);
	for(i=0; i<4; i++)
		DrawLine(330, 340+i*LABELCELLSIZE, 465, 340+i*LABELCELLSIZE, BLACK);
	SetFont(newmenu_font, BLACK);
	for(i=0;i<3; i++)
		for(j=0; j<3; j++)
		{
		sprintf(s, "%d", i*3+j+1);
		DrawTextRect(130+j*LABELCELLSIZE, 345+i*LABELCELLSIZE, LABELCELLSIZE, LABELCELLSIZE, s, ALIGN_CENTER);
	}
	InvertArea(266, 386, 64, LABELCELLSIZE-1);
	SetFont(mlabel_font, BLACK);
	for(i=0;i<3; i++)
		for(j=0; j<3; j++)
		{
		sprintf(s, "%d", i*3+j+1);
		DrawTextRect(337+j*LABELCELLSIZE, 345+i*LABELCELLSIZE, LABELCELLSIZE, LABELCELLSIZE, s, ALIGN_CENTER);
	}
	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
		{
		if (TmpSudoku.grid[y][x].cand[i*3+j]==1)
		{
			LabelCandidates[i*3+j+1]=1;
			InvertArea(348+j*LABELCELLSIZE, 344+i*LABELCELLSIZE, LABELCELLSIZE-24, LABELCELLSIZE-24);
		}
		else
		 	LabelCandidates[i*3+j+1]=0;
	}
	k=TmpSudoku.grid[y][x].value-1;
	LabelCandidates[0]=k;
	PartialUpdateBW(110, 300, 380, 200);
	MenuX=3;
	MenuY=1;
	marknum=0;
	SetEventHandler(MenuHandler);
}

int PointerPlayHandler(int type, int par1, int par2){

	int px=par1-STARTX;
	int py=par2-STARTY;
	
	if (type == EVT_POINTERUP && px/PLAYCELLSIZE>=0 && px/PLAYCELLSIZE<9 && py/PLAYCELLSIZE>=0 && py/PLAYCELLSIZE<9){
		InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
		x=px/PLAYCELLSIZE;
		y=py/PLAYCELLSIZE;
		InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
		PartialUpdateBW(STARTX, STARTY, 9*PLAYCELLSIZE, 9*PLAYCELLSIZE);
		PlayHandler(EVT_KEYPRESS, KEY_OK, 0);
	}
	return 0;
}

int PlayHandler(int type, int par1, int par2)
{
	if (type==EVT_KEYPRESS){
		int res = 0;
		switch(par1)
		{
		case KEY_LEFT:
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			x--;
			if (x<0) 
				x=8;
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			PartialUpdateBW(STARTX, STARTY, 9*PLAYCELLSIZE, 9*PLAYCELLSIZE);
			res = 1;
			break;

		case KEY_RIGHT:
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			x++;
			if (x>8) 
				x=0;
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			PartialUpdateBW(STARTX, STARTY, 9*PLAYCELLSIZE, 9*PLAYCELLSIZE);
			res = 1;
			break;

		case KEY_UP:
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			y--;
			if (y<0) 
				y=8;
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			PartialUpdateBW(STARTX, STARTY, 9*PLAYCELLSIZE, 9*PLAYCELLSIZE);
			res = 1;
			break;

		case KEY_DOWN:
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			y++;
			if (y>8) 
				y=0;
			InvertArea(STARTX+x*PLAYCELLSIZE, STARTY+y*PLAYCELLSIZE, PLAYCELLSIZE, PLAYCELLSIZE);
			PartialUpdateBW(STARTX, STARTY, 9*PLAYCELLSIZE, 9*PLAYCELLSIZE);
			res = 1;
			break;

		case KEY_OK:
			if (GameSudoku.grid[y][x].stat!=1) {
				DrawNewMenu();
			}
			res = 1;
			break;

		case KEY_BACK:
			CloseApp();
			res = 1;
			break;		 	
		}
		return res;
	}

	if (type==EVT_KEYREPEAT && par1==KEY_PREV) { CloseApp(); return 0; }

	if (type == EVT_POINTERDOWN || type == EVT_POINTERUP || type == EVT_POINTERMOVE || type == EVT_POINTERHOLD || type == EVT_POINTERLONG){
		return PointerPlayHandler(type, par1, par2);
	}
	return 0;
}

void LoadGame()
{
	int i, j, k;
	for (i=0; i<9; i++)
		for (j=0; j<9; j++)
 		{
		TmpSudoku.grid[i][j].stat=0;
		TmpSudoku.grid[i][j].value=0;
		TmpSudoku.grid[i][j].ncand=0;
		for (k=0; k<9; k++)
 		 	TmpSudoku.grid[i][j].cand[k]=0;
	}
	DrawSudoku();
	if (LoadSudoku()==1)
 	{
		PlayMenu[4].type=ITEM_ACTIVE;
		LoadFlag=1;
 	}
 	else
 	{
		PlayMenu[4].type=ITEM_INACTIVE;
		LoadFlag=0;
 	}
	PlayMenu[5].type=ITEM_INACTIVE;
	PlayMenu[7].type=ITEM_ACTIVE;
	GameFlag=0;
	OpenMenu(PlayMenu, 0, 250, 350, PlayMenuHandler);
}

int MainHandler(int type, int par1, int par2)
{
	if (type==EVT_INIT)
 	{
		FileSudoku=GetAssociatedFile(ProgramName, 0);
		//fprintf(stderr, "[%s]\n", FileSudoku);
		srand(time(NULL));
		cour10=OpenFont("LiberationMono", 10, 0);
		cour16bd=OpenFont("LiberationMono-Bold", 16, 0);
		cour12=OpenFont("LiberationMono", 12, 0);
		cour22=OpenFont("LiberationMono", 22, 0);
		cour36=OpenFont("Arbat", 42, 0);
		cour36bd=OpenFont("LiberationMono-Bold", 48, 0);
		cour72=OpenFont("LiberationMono", 72, 0);
		cour96=OpenFont("LiberationMono", 96, 0);
		newmenu_font=OpenFont("LiberationMono", 32, 0);
		newmenu_lfont=OpenFont("LiberationSans", 18, 0);
		bottom_font=OpenFont("LiberationSans", 32, 0);
		mlabel_font=OpenFont("LiberationMono-Bold", 16, 0);
		LoadGame();
		return 1;
 	} 
	return 0;
}

int main(int argc, char *argv[]) 
{
	ProgramName = argv[0];
	OpenScreen();
	SetOrientation(0);
	InkViewMain(MainHandler);
	return 0;
} 

/*
fprintf(stderr, "OK - 1\n");
sleep(1);
*/
