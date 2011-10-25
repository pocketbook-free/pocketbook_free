#include "pbrecordset.h"

extern "C"
{
    extern const ibitmap brokenImage;
}

static const char* PBRECORDSET_NOTEXT = "Text error";

PBRecordset::PBRecordset()
{
	m_row = 0;
}

PBRecordset::~PBRecordset()
{
	Close();
}

void PBRecordset::SetParam(const char *param, const char *value)
{
	m_paramList[param] = value;
}

const char *PBRecordset::GetParam(const char *param)
{
	return m_paramList[param].c_str();
}

bool PBRecordset::Move(int row)
{
	bool result = false;
	if (row >= 0 && row < RowCount())
	{
		m_row = row;
		result = true;
	}
	return result;
}

void PBRecordset::Close()
{
}

bool PBRecordset::Open()
{
	return true;
}

void PBRecordset::Refresh()
{
}

int PBRecordset::currentRow()
{
	return m_row;
}

int PBRecordset::RowCount()
{
	return 0;
}

int PBRecordset::ColumnCount()
{
	return 0;
}

const char *PBRecordset::GetText(int column)
{
	column = 0;
	return PBRECORDSET_NOTEXT;
}

int PBRecordset::GetInt(int column)
{
	column = 0;
	return -1;
}

ibitmap *PBRecordset::GetImage(int column)
{
	column = 0;
	return GetResource("broken_image", (ibitmap *) &brokenImage);
}

bool PBRecordset::GetBool(int column)
{
	column = 0;
	return false;
}

void *PBRecordset::GetVoid(int column)
{
	column = 0;
	return NULL;
}

// mark item
void PBRecordset::Mark(bool value)
{
	value = false;
}

// set what to do type
int PBRecordset::Delete(PBRecordset::AffectedRecords whatToDo)
{
	whatToDo = PBRecordset::NoRecords;
	return -1;
}
