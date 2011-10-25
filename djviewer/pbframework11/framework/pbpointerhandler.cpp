#include "pbpointerhandler.h"
#include "inkview.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>


// для доступа из статической callback-функции таймера
static PBPointerHandler* pd = NULL;

PBPointerHandler::~PBPointerHandler()
{
}

PBPointerHandler::PBPointerHandler()
    : _curPosX(0),
	_curPosY(0),
	_prevPosX(0),
	_prevPosY(0),
	_nextPosX(0),
	_nextPosY(0),
	_screenWidth(0),
	_screenHeight(0),
	_pageWidth(0),
	_pageHeight(0),
	_pointerRunner(0),
	_phAddBookMark(false),
	_isLongMotion(false),
	_isLongInternal(false),
	_isHoldMotion(false),
	_isHoldInternal(false),
	_isClick(false)
{
    // для доступа из статической callback-функции таймера
    pd = this;
}

// регистрировать обработчик событий
int PBPointerHandler::registrationRunner(PBIEventHandler* pRunner)
{
    if (_pointerRunner)
        return 0;

    _pointerRunner = pRunner;
    return 1;
}

// нажатие в правом верхнем углу
bool PBPointerHandler::checkClickTopRigth()
{
    if ((_curPosX > _screenWidth - BOOKMARK_MARGIN) &&
        (_curPosY < BOOKMARK_MARGIN)                &&
        (_curPosX - _prevPosX < BAR_HORZ)           &&
        ((_curPosY - _prevPosY) < BAR_VERT))
        return true;

    return false;
}

//  центральный жест  сверху вниз
bool PBPointerHandler::checkMotionCenterTopToCenterBottom()
{
    if ((_curPosX > ((_screenWidth / 2) - MENU_MARGIN)) &&
        (_curPosX < ((_screenWidth / 2) + MENU_MARGIN)) &&
        ((_curPosX - _prevPosX) < BAR_HORZ)             &&
        (_curPosY - _prevPosY > BAR_HORZ))
        return true;

    return false;
}

//  центральный жест  снизу вверх
bool PBPointerHandler::checkMotionCenterBottomToCenterTop()
{
    if ((_curPosX > ((_screenWidth / 2) - MENU_MARGIN)) &&
        (_curPosX < ((_screenWidth / 2) + MENU_MARGIN)) &&
        ((_curPosX - _prevPosX) < BAR_HORZ)             &&
        ((_prevPosY - _curPosY) > BAR_HORZ))
        return true;

    return false;
}

//  центральный жест  слева направо
bool PBPointerHandler::checkMotionCenterLeftToCenterRight()
{
    if ((_curPosY > (_screenHeight / 2 - MENU_MARGIN))  &&
        (_curPosY < (_screenHeight / 2 + MENU_MARGIN))  &&
        ((_curPosX - _prevPosX) > BAR_HORZ))
        return true;

    return false;
}

//  центральный жест  справа налево
bool PBPointerHandler::checkMotionCenterRightToCenterLeft()
{
    if ((_curPosY > (_screenHeight / 2 - MENU_MARGIN))  &&
        (_curPosY < (_screenHeight / 2 + MENU_MARGIN))  &&
        ((_prevPosX - _curPosX) > BAR_HORZ))
        return true;

    return false;
}

//  нижний жест справа налево
bool PBPointerHandler::checkMotionLeftBottomToRigthBottom()
{
    if ((_prevPosY > _screenHeight - PAGE_MARGIN_HEIGHT) &&
        (_curPosY > _screenHeight - PAGE_MARGIN_HEIGHT)  &&
        ((_curPosX - _prevPosX) > BAR_HORZ))
        return true;

    return false;
}

//  нижний жест слева направо
bool PBPointerHandler::checkMotionRigthBottomToLeftBottom()
{
    if ((_prevPosY > _screenHeight - PAGE_MARGIN_HEIGHT) &&
        (_curPosY > _screenHeight - PAGE_MARGIN_HEIGHT)  &&
        ((_prevPosX - _curPosX) > BAR_HORZ))
        return true;

    return false;
}

//  правый жест сверху вниз
bool PBPointerHandler::checkMotionTopRigthToBottomRight()
{
    if ((_prevPosX > _screenWidth - PAGE_MARGIN_WIDTH)  &&
        (_curPosX > (_screenWidth - PAGE_MARGIN_WIDTH)) &&
        (_curPosY - _prevPosY > BAR_HORZ))
        return true;

    return false;
}

//  правый жест снизу вверх
bool PBPointerHandler::checkMotionBottomRigthToTopRight()
{
    if ((_prevPosX > _screenWidth - PAGE_MARGIN_WIDTH)  &&
        (_curPosX > (_screenWidth - PAGE_MARGIN_WIDTH)) &&
        (_prevPosY - _curPosY > BAR_HORZ))
        return true;

    return false;
}

// нажатие в левом верхнем углу
bool PBPointerHandler::checkBookMark2()
{
    if (_phAddBookMark)
        if ((_curPosX > (_screenWidth - BOOKMARK_MARGIN)) &&
            (_curPosY < BOOKMARK_MARGIN))
            return true;

    return false;
}

bool PBPointerHandler::checkClickTopLeft()
{
    if (_curPosX < BACKLINK_BUTTON_WIDTH  &&
		_curPosY < BACKLINK_BUTTON_HEIGHT)
        return true;

    return false;
}

bool PBPointerHandler::checkClickCenter()
{
	if ((_curPosX > (_screenWidth / 2 - MENU_MARGIN))   &&
        (_curPosX < (_screenWidth / 2 + MENU_MARGIN))   &&
        ((_curPosX - _prevPosX) < BAR_HORZ)             &&
        (_curPosY > (_screenHeight / 2 - MENU_MARGIN))  &&
        (_curPosY < (_screenHeight / 2 + MENU_MARGIN))  &&
        ((_curPosY - _prevPosY) < BAR_VERT))
        return true;

    return false;
}

// вычислить разницу во времени в миллисекундах
int PBPointerHandler::diffTime(struct timeb* sStart, struct timeb* sEnd)
{
    int tempTime;

    tempTime = sEnd->time - sStart->time;
    tempTime *= 1000;
    tempTime += sEnd->millitm;
    tempTime -= sStart->millitm;

    return tempTime;
}

int PBPointerHandler::HandleEvent(int type_event, int posiX, int posiY)
{
    assert(_pointerRunner);

    // сохранить входящие координаты
    _curPosX = posiX;
    _curPosY = posiY;

    _screenWidth = ScreenWidth();
    _screenHeight = ScreenHeight();

    if (type_event == EVT_POINTERDOWN)
    {
		_prevPosX = _curPosX;
        _prevPosY = _curPosY;
        _nextPosX = _curPosX;
        _nextPosY = _curPosY;

		_isClick = true;
		_pointerDownPoint = PBPoint(posiX, posiY);

        return 1;
    }
    else if (type_event == EVT_POINTERUP)
    {
        _nextPosX = _prevPosX;
        _nextPosY = _prevPosY;

        // сбросить признак начального удержания
        _isLongInternal = _isLongMotion;
        _isLongMotion   = false;

        // сбросить признак конечного удержания
        _isHoldInternal = _isHoldMotion;
        _isHoldMotion   = false;

		_pointerUpPoint = PBPoint(posiX, posiY);

		return processPointerUpEvent();
    }
    // вызывается после удержания с жестом
    else if (type_event == EVT_POINTERLONG)
    {
        _isLongMotion = true;       // установить признак начального удержания
        _phAddBookMark = false;
		_isClick = false;

        if ((_curPosX > _screenWidth - BOOKMARK_MARGIN)  && (_curPosY < BOOKMARK_MARGIN))
            _phAddBookMark = true;
        else
            return 1;
    }
    // вызывается после жеста с удержанием
	else if ((type_event == EVT_POINTERHOLD))
    {
        _isHoldMotion = true;       // установить признак конечного удержания
		_isClick = false;

        _nextPosX = _curPosX;
        _nextPosY = _curPosY;

        return 1;
    }
    else if (type_event == EVT_POINTERMOVE)
    {
        return 1;
    }

    return 0;
}

int PBPointerHandler::processPointerUpEvent()
{
	int ret = 0;

	if (checkBookMark2())
	{
		int ret = _pointerRunner->onBookMark2();
		_phAddBookMark = false;

		return ret;
	}

	if (_isClick && _pointerUpPoint.Distance(_pointerDownPoint) < 10)
		ret = _pointerRunner->onTouchClick(_curPosX,_curPosY);
	if (ret)
		return ret;

	if (checkClickCenter())
		return _pointerRunner->onMenu();
	if (checkClickTopRigth())
		return _pointerRunner->onBookMarkOpen();

	if (checkClickTopLeft())
		return _pointerRunner->onLinkBack();
	if (checkMotionRigthBottomToLeftBottom())
		return IsBookRTL() == false ? _pointerRunner->onPageNext() : _pointerRunner->onPagePrev();
	if (checkMotionLeftBottomToRigthBottom())
		return IsBookRTL() == false ? _pointerRunner->onPagePrev() : _pointerRunner->onPageNext();
	if (checkMotionTopRigthToBottomRight())
		return _pointerRunner->onZoomOut();
	if (checkMotionBottomRigthToTopRight())
		return _pointerRunner->onZoomIn();

	if (!_isHoldInternal)
	{
		if (checkMotionCenterTopToCenterBottom())
			return _pointerRunner->onScreenDown(_prevPosY, _curPosY);
		if (checkMotionCenterBottomToCenterTop())
			return _pointerRunner->onScreenUp(_prevPosY, _curPosY);
		if (checkMotionCenterLeftToCenterRight())
			return _pointerRunner->onScreenRight(_prevPosX, _curPosX);
		if (checkMotionCenterRightToCenterLeft())
			return _pointerRunner->onScreenLeft(_prevPosX, _curPosX);
	}
	else
	{
		if (checkMotionCenterTopToCenterBottom())
			return _pointerRunner->onScrollDown(_prevPosY, _curPosY);
		if (checkMotionCenterBottomToCenterTop())
			return _pointerRunner->onScrollUp(_prevPosY, _curPosY);
	}

	return 0;
}
