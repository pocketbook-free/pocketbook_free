#include "pbgridcontainer.h"

PBGridContainer::PBGridContainer():PBWindow()
{
}

PBGridContainer::~PBGridContainer()
{
	Clear();
}

int PBGridContainer::OnCreate()
{
	rowinfo row;
	columninfo col;

	m_RowCount = 1;
	m_ColCount = 1;

	row.top = 0;
	row.ratio = true;
	row.ratioheight = 100;
	row.height = 0;

	col.left = 0;
	col.ratio = true;
	col.ratiowidth = 100;
	col.width = 0;

	m_Rows.push_back(row);
	m_Cols.push_back(col);
	ResizeGrid();

	return 0;
}

void PBGridContainer::SetGrid(int nrow, int ncol)
{
	int i, j;
	rowinfo row;
	columninfo col;

	Clear();

	m_RowCount = nrow;
	m_ColCount = ncol;

	row.top = GetClientTop() - GetTop();
	row.ratio = false;
	row.ratioheight = -1;
	row.height = GetClientHeight() / m_RowCount;

	col.left = GetClientLeft() - GetLeft();
	col.ratio = false;
	col.ratiowidth = -1;
	col.width = GetClientWidth() / m_ColCount;

	for (j = 0; j < m_ColCount; j++)
	{
		m_Cols.push_back(col);
		col.left += col.width;
	}

	for (i = 0; i < m_RowCount; i++)
	{
		m_Rows.push_back(row);
		row.top += row.height;
	}
}

void PBGridContainer::SetColumnWidth(int index, int width, bool ratio)
{

	if (width > 0)
	{
		if (index >= 0 && index < m_ColCount)
		{
			if (ratio)
			{
				m_Cols[index].ratiowidth = width;
			}
			else
			{
				m_Cols[index].width = width;
			}
			m_Cols[index].ratio = ratio;
			ResizeGrid();
		}
	}
}

void PBGridContainer::SetRowHeight(int index, int height, bool ratio)
{
	if (height > 0)
	{
		if (index >= 0 && index < m_RowCount)
		{
			if (ratio)
			{
				m_Rows[index].ratioheight = height;
			}
			else
			{
				m_Rows[index].height = height;
			}
			m_Rows[index].ratio = ratio;
			ResizeGrid();
		}
	}
}

PBRect PBGridContainer::GetCell(int row, int col)
{
	if (row >= 0 && row < m_RowCount && col >= 0 && col < m_ColCount)
	{
		return PBRect(m_Cols[col].left, m_Rows[row].top, m_Cols[col].width, m_Rows[row].height);
	}

	return PBRect(0, 0, 0, 0);
}

void PBGridContainer::ResizeGrid()
{
	int i;

	int ratio = 0;
	int size = 0;
	int pos = 0;

	for (i = 0; i < m_RowCount; i++)
	{
		if (m_Rows[i].ratio)
		{
			if (m_Rows[i].ratioheight > 0)
			{
				ratio += m_Rows[i].ratioheight;
			}
		}
		else
		{
			size += m_Rows[i].height;
		}
	}

	for (i = 0; i < m_RowCount; i++)
	{
		if (m_Rows[i].ratio)
		{
			if (m_Rows[i].ratioheight > 0)
			{
				m_Rows[i].top = pos;
				m_Rows[i].height = (GetHeight() - size) * m_Rows[i].ratioheight / ratio;
			}
			else
			{
				m_Rows[i].top = pos;
				m_Rows[i].height = 0;
			}
		}
		else
		{
			m_Rows[i].top = pos;
		}
		pos += m_Rows[i].height;
	}

	ratio = 0;
	size = 0;
	pos = 0;

	for (i = 0; i < m_ColCount; i++)
	{
		if (m_Cols[i].ratio)
		{
			if (m_Cols[i].ratiowidth > 0)
			{
				ratio += m_Cols[i].ratiowidth;
			}
		}
		else
		{
			size += m_Cols[i].width;
		}
	}

	for (i = 0; i < m_ColCount; i++)
	{
		if (m_Cols[i].ratio)
		{
			if (m_Cols[i].ratiowidth > 0)
			{
				m_Cols[i].left = pos;
				m_Cols[i].width = (GetWidth() - size) * m_Cols[i].ratiowidth / ratio;
			}
			else
			{
				m_Cols[i].left = pos;
				m_Cols[i].width = 0;
			}
		}
		else
		{
			m_Cols[i].left = pos;
		}
		pos += m_Cols[i].width;
	}
}

void PBGridContainer::Clear()
{
	while (!m_Rows.empty())
		m_Rows.pop_back();

	while (!m_Cols.empty())
		m_Cols.pop_back();
}
