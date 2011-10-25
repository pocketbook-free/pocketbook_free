/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ZLTextView.h>
#include <ZLBlockTreeView.h>
#include <assert.h>
#include "FBReader.h"
#include "ScrollingAction.h"
#include "inkview.h"
ScrollingAction::ScrollingAction(
	ZLTextAreaController::ScrollingMode textScrollingMode,
	ZLBlockTreeView::ScrollingMode blockScrollingMode,
	bool forward
) : myTextScrollingMode(textScrollingMode), myBlockScrollingMode(blockScrollingMode), myForward(forward) {
}

int ScrollingAction::scrollingDelay() const {
	return 0;
}

bool ScrollingAction::isEnabled() const {
	return true;
}

bool ScrollingAction::useKeyDelay() const {
	return false;
}

void ScrollingAction::run() {
        FBReader &fbreader = FBReader::Instance();
        shared_ptr<ZLView> view = fbreader.currentView();
        int delay = fbreader.myLastScrollingTime.millisecondsTo(ZLTime());
        if (view.isNull() /*||
                        (delay >= 0 && delay < scrollingDelay())*/) {
                return;
        }

        if (view->isInstanceOf(ZLTextView::TYPE_ID)) {
            ((ZLTextView&)*view).scrollPage(myForward, myTextScrollingMode, textOptionValue());
            FBReader::Instance().refreshWindow();
    } else if (view->isInstanceOf(ZLBlockTreeView::TYPE_ID)) {
            ((ZLBlockTreeView&)*view).scroll(myBlockScrollingMode, !myForward);
    }
    fbreader.myLastScrollingTime = ZLTime();
}

LineScrollingAction::LineScrollingAction(bool forward) : ScrollingAction(ZLTextAreaController::SCROLL_LINES, ZLBlockTreeView::ITEM, forward) {
}

int LineScrollingAction::scrollingDelay() const {
    return FBReader::Instance().KeyScrollingDelayOption.value();
}

size_t LineScrollingAction::textOptionValue() const {
    return FBReader::Instance().LinesToScrollOption.value();
}

PageScrollingAction::PageScrollingAction(bool forward) : ScrollingAction(ZLTextAreaController::NO_OVERLAPPING, ZLBlockTreeView::PAGE, forward) {
}

int PageScrollingAction::scrollingDelay() const {
    return FBReader::Instance().KeyScrollingDelayOption.value();
}

size_t PageScrollingAction::textOptionValue() const {
    return FBReader::Instance().LinesToKeepOption.value();
}

MouseWheelScrollingAction::MouseWheelScrollingAction(bool forward) : ScrollingAction(ZLTextAreaController::SCROLL_LINES, ZLBlockTreeView::ITEM, forward) {
}

size_t MouseWheelScrollingAction::textOptionValue() const {
    return 1;
}

TapScrollingAction::TapScrollingAction(bool forward) : ScrollingAction(ZLTextAreaController::KEEP_LINES, ZLBlockTreeView::NONE, forward) {
}

size_t TapScrollingAction::textOptionValue() const {
    return FBReader::Instance().LinesToKeepOption.value();
}

bool TapScrollingAction::isEnabled() const {
    return FBReader::Instance().EnableTapScrollingOption.value();
}

static long long pack_position(int para, int word, int letter) {

        return ((long long)para << 40) | ((long long)word << 16) | ((long long)letter);

}
static long long get_position(ZLTextView & view) {

        ZLTextWordCursor cr = view.textArea().startCursor();
        int paragraph = !cr.isNull() ? cr.paragraphCursor().index() : 0;
        return pack_position(paragraph, cr.elementIndex(), cr.charIndex());
}
//void ForwardScrollingAction::run(long long calc_current_position, int calc_current_page, TPageList &pageList)

//count == -1 -> do not check on eqaulity
//#include <iostream>
inline bool Add(TPageList & pgs, const TPageList & oldPages, long long pos, int & npages, int & calc_current_page, int & count)
{
    npages = ++calc_current_page;
    pgs.push_back(pos);
    if (oldPages.size() >= pgs.size())
    {
        if (count != -1 && (pgs[pgs.size() - 1]) == oldPages[pgs.size() - 1])
        {
            count = pgs.size();
        }
        else
        {
            count = -1;
        }
    }

    assert((int)pgs.size() == npages);
    return true;
}

#include <sys/time.h>
static unsigned int gettime()
{
    static struct timeval curtv;
    gettimeofday(&curtv, NULL);
    return (curtv.tv_sec * 1000) + (curtv.tv_usec / 1000);
}
bool ForwardScrollingAction::run(ZLTextView & view, long long & cp, int & calc_current_page, int & npages, TPageList & pageList, TPageList & oldPageList, unsigned int TTP, bool &areEqual)
{
    int count = 0;
    //FBReader &fbreader = FBReader::Instance();
    //shared_ptr<ZLView> view = fbreader.currentView();
    //int delay = fbreader.myLastScrollingTime.millisecondsTo(ZLTime());
    //long long cp;
//    if (view.isNull() /*||
//                    (delay >= 0 && delay < scrollingDelay())*/) {
//            return;
//    }
    static int rnd1 = 0, rnd2 = 0;    
    unsigned int start =  gettime();
    if (view.isInstanceOf(ZLTextView::TYPE_ID)) {
    {        
            do
            {
                if (count > checkPagesCount) {
                    //old pages are equal to new calculated
                    areEqual = true;
                    return false;
                }
                view.scrollPage(true, ZLTextAreaController::NO_OVERLAPPING, FBReader::Instance().LinesToKeepOption.value());

                //view.preparePaintInfo();
                //view.myTextAreaController.scrollPage(true, ZLTextAreaController::NO_OVERLAPPING, FBReader::Instance().LinesToKeepOption.value());

                FBReader::Instance().refreshWindow();

                cp = get_position(view);
                if (gettime() - start > TTP || IsAnyEvents())
                {
                    if (cp != pageList[calc_current_page - 1] && Add(pageList, oldPageList, cp, npages, calc_current_page, count))
                    {
                        return true;
                    }
                    else
                    {                        
                        return false;
                    }
                    break;
                }

                //std::cerr << "IsNoKeyEvents\n";
            }
            while (cp != pageList[calc_current_page - 1] &&  Add(pageList, oldPageList, cp, npages, calc_current_page, count));
            return false;
        }
    } else if (view.isInstanceOf(ZLBlockTreeView::TYPE_ID))
    {
        do
        {
            if (count > checkPagesCount) {
                //old pages are equal to new calculated
                areEqual = true;
                return false;
            }
            ((ZLBlockTreeView&)view).scroll(ZLBlockTreeView::PAGE, false);
            cp = get_position(view);
            if (gettime() - start > TTP || IsAnyEvents())
            {
                if (cp != pageList[calc_current_page - 1] && Add(pageList, oldPageList, cp, npages, calc_current_page, count))
                {                   
                    return true;
                }
                else
                {                    
                    return false;
                }
                break;
            }
        }
        while (cp != pageList[calc_current_page - 1] &&  Add(pageList, oldPageList, cp, npages, calc_current_page, count));
    }
    //fbreader.myLastScrollingTime = ZLTime();
}
