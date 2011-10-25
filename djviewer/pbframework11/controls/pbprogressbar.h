#ifndef __PBPROGRESSBAR_H__
#define __PBPROGRESSBAR_H__

#include "../framework/pbwindow.h"

class PBProgressBar : public PBWindow
{
public:
	PBProgressBar();
	~PBProgressBar();

        bool Create(PBWindow* pParent, int left, int top, int width, int height);

        void reset();
        void setValue(int value);
        int value() const;
protected:
private:
		int OnDraw(bool force);

        int m_properties;
        int m_rectOffset;
        int m_value;
};

#endif /* __PBPROGRESSBAR_H__ */
