#ifndef PBGridContainer_H
#define PBGridContainer_H

#include "pbwindow.h"
#include "pbframewindow.h"

typedef struct rowinfo_s
{
	int top;
	int height;
	int ratioheight;
	bool ratio;
} rowinfo;

typedef struct columninfo_s
{
	int left;
	int width;
	int ratiowidth;
	bool ratio;
} columninfo;

class PBGridContainer : public PBWindow
{
public:
	PBGridContainer();
	~PBGridContainer();

	int OnCreate();
	virtual void SetGrid(int nrow, int ncol);
	void SetColumnWidth(int index, int width, bool ratio);
	void SetRowHeight(int index, int height, bool ratio);
	
	PBRect GetCell(int row, int col);

protected:

	int m_RowCount;
	int m_ColCount;

	std::vector<rowinfo> m_Rows;
	std::vector<columninfo> m_Cols;

	void ResizeGrid();
	void Clear();
};

#endif // PBGridContainer_H
