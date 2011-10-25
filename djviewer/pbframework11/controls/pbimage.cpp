 #include "pbimage.h"

PBImage::PBImage():
		PBWindow(),
		m_pPath(NULL),
		m_pImage(NULL),
		m_Properties(ALIGN_LEFT | VALIGN_TOP),
		m_Fill(true),
		m_ImgLoad(false),
		m_BW(false)
{
}

PBImage::~PBImage()
{
	FreeData();
}

void PBImage::Create(PBWindow *pParent, int left, int top, int width, int height)
{
	PBWindow::Create(pParent, left, top, width, height);

	FreeData();
	SetProperties(0);

	MarkAsChanged();

}

void PBImage::Create(PBWindow *pParent, int left, int top, int width, int height, const ibitmap *bmp)
{
	PBImage::Create(pParent, left, top, width, height);

	if (bmp != NULL)
	{
		m_ImgLoad = false;
		m_pImage = (ibitmap*)bmp;
	}
}

void PBImage::Create(PBWindow *pParent, int left, int top, int width, int height, const ibitmap *bmp, int properties)
{

	PBImage::Create(pParent, left, top, width, height, bmp);

	SetProperties(properties);

	MarkAsChanged();

}

void PBImage::Create(PBWindow *pParent, int left, int top, int width, int height, const char *path)
{

	PBWindow::Create(pParent, left, top, width, height);

	LoadImage(path);

	MarkAsChanged();

}

void PBImage::Create(PBWindow *pParent, int left, int top, int width, int height, const char *path, int properties)
{

	PBImage::Create(pParent, left, top, width, height, path);

	SetProperties(properties);

	MarkAsChanged();

}

void PBImage::SetBitmap(ibitmap *bmp)
{
	FreeData();
	if (bmp != NULL)
	{
		m_ImgLoad = false;
		m_pImage = bmp;
	}

	MarkAsChanged();
}

bool PBImage::LoadImage(const char *path)
{

	if (m_pPath != NULL && path != NULL && strcmp(m_pPath, path) == 0)
	{
		return true;
	}

	FreeData();

	MarkAsChanged();

	if (path != NULL)
	{
		char *p = strrchr((char *)path, '.');

		if (p != NULL && (
				strncmp(p, ".jpg", 4) == 0 ||
				strncmp(p, ".jpeg", 5) == 0 ||
				strncmp(p, ".JPG", 4) == 0 ||
				strncmp(p, ".JPEG", 5) == 0
				))
		{
			m_ImgLoad = true;
			m_pImage = LoadJPEG(path, GetClientWidth(), GetClientHeight(), 64, 128, 1);
			if (m_pImage != NULL)
			{
				m_pPath = strdup(path);
				return true;
			}
			return false;
		}

		if (p != NULL && (
				strncmp(p, ".bmp", 4) == 0 ||
				strncmp(p, ".BMP", 4) == 0
				))
		{
			m_ImgLoad = true;
			m_pImage = LoadBitmap(path);
			if (m_pImage != NULL)
			{
				m_pPath = strdup(path);
				return true;
			}
			return false;
		}
	}

	return false;

}

void PBImage::SetProperties(int properties)
{
	SetAlign(properties);
	SetValign(properties);
	SetStretch(((properties & STRETCH) > 0));
	SetTile(((properties & TILE) > 0));
}

void PBImage::SetAlign(int align)
{
	align &= (ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT | ALIGN_FIT);

	if ((align & ALIGN_LEFT) > 0)
	{
		align &= ALIGN_LEFT;
	}
	else if ((align & ALIGN_CENTER) > 0)
	{
		align &= ALIGN_CENTER;
	}
	else if ((align &= ALIGN_RIGHT) > 0)
	{
		align = ALIGN_RIGHT;
	}
	else if ((align &= ALIGN_FIT) == 0)
	{
		align = ALIGN_LEFT;
	}

	m_Properties &= (~(ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT | ALIGN_FIT));
	m_Properties |= align;

	MarkAsChanged();

}

void PBImage::SetValign(int valign)
{
	valign &= (VALIGN_TOP | VALIGN_MIDDLE | VALIGN_BOTTOM);

	if ((valign & VALIGN_TOP) > 0)
	{
		valign &= VALIGN_TOP;
	}
	else if ((valign & VALIGN_MIDDLE) > 0)
	{
		valign &= VALIGN_MIDDLE;
	}
	else if ((valign &= VALIGN_BOTTOM) == 0)
	{
		valign = VALIGN_TOP;
	}

	m_Properties &= (~(VALIGN_TOP | VALIGN_MIDDLE | VALIGN_BOTTOM));
	m_Properties |= valign;

	MarkAsChanged();

}

void PBImage::SetStretch(bool stretch)
{
	if (stretch)
	{
		m_Properties |= STRETCH;
	}
	else
	{
		m_Properties &= (~STRETCH);
	}

	MarkAsChanged();

}

void PBImage::SetTile(bool tile)
{
	if (tile)
	{
		m_Properties |= TILE;
	}
	else
	{
		m_Properties &= (~TILE);
	}

	MarkAsChanged();

}

void PBImage::ImageType(bool bw)
{
	m_BW = bw;
}

void PBImage::FillRegion(bool fill)
{
	m_Fill = fill;
}

ibitmap *PBImage::GetBitmap() const
{
	ibitmap *result;
	if (m_pRecordset != NULL)
	{
		m_pRecordset->Move(m_RecordsetRow);
		result = m_pRecordset->GetImage(m_RecordsetCol);
	}
	else
	{
		result = m_pImage;
	}
	return result;
}

char *PBImage::GetImagePath() const
{
	return m_pPath;
}

int PBImage::GetProperties() const
{
	return m_Properties;
}

int PBImage::GetAlign() const
{
	return (m_Properties & (ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT | ALIGN_FIT));
}

int PBImage::GetValign() const
{
	return (m_Properties & (VALIGN_TOP | VALIGN_MIDDLE | VALIGN_BOTTOM));
}

bool PBImage::GetStretch() const
{
	return ((m_Properties & STRETCH) > 0);
}

bool PBImage::GetTile() const
{
	return ((m_Properties & TILE) > 0);
}

int PBImage::OnDraw(bool force)
{
	if (IsVisible() && (IsChanged() || force))
	{
		PBGraphics *graphics = GetGraphics();
		if (m_Fill)
			graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), m_Background);
		if (m_BW)
		{
			graphics->DrawBitmapRectBW(GetClientLeft(), GetClientTop(), GetClientWidth(), GetClientHeight(), GetBitmap(), GetProperties());
		}
		else
		{
			graphics->DrawBitmapRect(GetClientLeft(), GetClientTop(), GetClientWidth(), GetClientHeight(), GetBitmap(), GetProperties());
		}
	}
	
	return 0;
}

void PBImage::FreeData()
{
	if (m_pPath != NULL)
	{
		free(m_pPath);
		m_pPath = NULL;
	}

	if (m_ImgLoad && m_pImage != NULL)
	{
		free(m_pImage);
	}

	m_pImage = NULL;
	m_ImgLoad = false;
}
