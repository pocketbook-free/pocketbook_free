#ifndef PBIMAGE_H
#define PBIMAGE_H

#include "../framework/pbwindow.h"

class PBImage : public PBWindow
{
public:
	PBImage();
	virtual ~PBImage();

	void Create(PBWindow *pParent, int left, int top, int width, int height);
	void Create(PBWindow *pParent, int left, int top, int width, int height, const ibitmap *bmp);
	void Create(PBWindow *pParent, int left, int top, int width, int height, const ibitmap *bmp, int properties);
	void Create(PBWindow *pParent, int left, int top, int width, int height, const char *path);
	void Create(PBWindow *pParent, int left, int top, int width, int height, const char *path, int properties);

	void SetBitmap(ibitmap *bmp);
	bool LoadImage(const char *path);
	void SetProperties(int properties);
	void SetAlign(int align);
	void SetValign(int valign);
	void SetStretch(bool stretch);
	void SetTile(bool tile);
	void ImageType(bool bw);
	void FillRegion(bool fill);

	ibitmap *GetBitmap() const;
	char *GetImagePath() const;
	int GetProperties() const;
	int GetAlign() const;
	int GetValign() const;
	bool GetStretch() const;
	bool GetTile() const;

	int OnDraw(bool force);

private:
	char *m_pPath;
	ibitmap *m_pImage;
	int m_Properties;	// 4-bits - align, 3-bits - valign, 13-s bit - stretch, 14-s bit tile

	int m_Fill;

	bool m_ImgLoad;
	bool m_BW;

	void FreeData();

};

#endif // PBIMAGE_H
