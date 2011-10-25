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

#include <algorithm>

#include <ZLMirroredPaintContext.h>

#include <ZLTextModel.h>

#include "ZLTextArea.h"
#include "ZLTextAreaStyle.h"
#include "ZLTextLineInfo.h"
#include "ZLTextSelectionModel.h"
#include "../bookmodel/FBTextKind.h"

#include <stdio.h>
#include <stdlib.h>

ZLTextArea::ZLTextArea(ZLPaintContext &context, const Properties &properties) : myContext(context), myProperties(properties), myWidth(0), myHeight(0) {
}

ZLTextArea::~ZLTextArea() {
}

int ZLTextArea::realX(int x) const {
	return myMirroredContext.isNull() ? x : myMirroredContext->mirroredX(x);
}

void ZLTextArea::setModel(shared_ptr<ZLTextModel> model) {
	clear();

	if (model.isNull() || model->paragraphsNumber() == 0) {
		myModel = 0;
		return;
	}

	myModel = model;
	if (model->isRtl()) {
		myMirroredContext = new ZLMirroredPaintContext(myContext);
	} else {
		myMirroredContext.reset();
	}
	myStartCursor = ZLTextParagraphCursor::cursor(*model);
	myEndCursor = 0;
}

void ZLTextArea::clear() {
	myStartCursor = 0;
	myEndCursor = 0;

	myLineInfos.clear();
	myTextElementMap.clear();
	myTreeNodeMap.clear();

	if (!mySelectionModel.isNull()) {
		mySelectionModel->clear();
	}
}

bool ZLTextArea::_getHyperlinkInfo(const ZLTextElementRectangle &rectangle, std::string &id, std::string &type, ZLTextKind& hyperlinkKind, int& x1, int&y1, int& x2, int& y2) const {
	if ((rectangle.Kind != ZLTextElement::WORD_ELEMENT) &&
			(rectangle.Kind != ZLTextElement::IMAGE_ELEMENT)) {
		return false;
	}
	ZLTextWordCursor cursor = startCursor();
	cursor.moveToParagraph(rectangle.ParagraphIndex);
	cursor.moveToParagraphStart();
	hyperlinkKind = REGULAR;
	for (int i = 0; i < rectangle.ElementIndex; ++i) {
		const ZLTextElement &element = cursor.element();
		if (element.kind() == ZLTextElement::CONTROL_ELEMENT) {
			const ZLTextControlEntry &control = ((const ZLTextControlElement&)element).entry();
			if (control.isHyperlink()) {
				hyperlinkKind = control.kind();
				id = ((const ZLTextHyperlinkControlEntry&)control).label();
				type = ((const ZLTextHyperlinkControlEntry&)control).hyperlinkType();
				x1 = rectangle.XStart;
				y1 = rectangle.YStart;
				x2 = rectangle.XEnd;
				y2 = rectangle.YEnd;
			} else if (!control.isStart() && (control.kind() == hyperlinkKind)) {
				hyperlinkKind = REGULAR;
			}
		}
		cursor.nextWord();
	}

	return hyperlinkKind != REGULAR;
}

int ZLTextArea::get_xxx_links(std::vector<struct xxx_link> & links) const {

	links.clear();

	for (ZLTextElementIterator it = myTextElementMap.begin(); it != myTextElementMap.end(); ++it) {
		ZLTextElementRectangle rectangle = *it;

		std::string id;
		std::string type;
		ZLTextKind kind = REGULAR;
		xxx_link link;
		bool alreadyExists = false;

		//fprintf(stderr, "ParagraphIndex - %d, ElementIndex - %d, Kind - %d\n", rectangle.ParagraphIndex, rectangle.ElementIndex, rectangle.Kind);

		if (_getHyperlinkInfo(rectangle, id, type, kind, link.x1, link.y1, link.x2, link.y2)) {

			for (std::vector<struct xxx_link>::iterator it = links.begin(); it != links.end(); it++) {
				if ( (*it).id == id && (*it).y1 ==link. y1 ){
					(*it).x2 = link.x2;
					alreadyExists = true;
					break;
				}
			}

			if (!alreadyExists) {
				link.id = id;
				link.kind = kind;
				link.type = type;

				//fprintf(stderr, "found link. ParagraphIndex - %d, ElementIndex - %d, Kind - %d\n", rectangle.ParagraphIndex, rectangle.ElementIndex, rectangle.Kind);

				links.push_back(link);
			}
		}
	}

	return links.size();
}

int ZLTextArea::paragraphIndexByCoordinates(int x, int y, bool absolute) const {
	if (absolute) {
		x -= myHOffset;
		y -= myVOffset;
	}

	int paragraphIndex = -1;
	int yBottom = -1;
	int xLeft = context().width() + 1;
	int xRight = -1;

	for (ZLTextElementIterator it = myTextElementMap.begin(); it != myTextElementMap.end(); ++it) {
		if (it->YEnd < y) {
			paragraphIndex = it->ParagraphIndex;
			if (it->YStart > yBottom) {
				yBottom = it->YEnd;
				xLeft = it->XStart;
				xRight = -1;
			}
			xRight = it->XEnd;
			continue;
		}
		if (it->YStart > y) {
			return
				((paragraphIndex == it->ParagraphIndex) &&
				 (xLeft <= x) && (x <= xRight)) ?
				paragraphIndex : -1;
		}
		if (it->XEnd < x) {
			paragraphIndex = it->ParagraphIndex;
			if (it->YStart > yBottom) {
				yBottom = it->YEnd;
				xLeft = it->XStart;
				xRight = -1;
			}
			xRight = it->XEnd;
			continue;
		}
		if (it->XStart > x) {
			return
				((paragraphIndex == it->ParagraphIndex) &&
				 (it->YStart <= yBottom) && (xLeft < x)) ?
				paragraphIndex : -1;
		}
		return it->ParagraphIndex;
	}
	return -1;
}

const ZLTextElementRectangle *ZLTextArea::elementByCoordinates(int x, int y, bool absolute) const {
	if (absolute) {
		x -= myHOffset;
		y -= myVOffset;
	}

	ZLTextElementIterator it =
		std::find_if(myTextElementMap.begin(), myTextElementMap.end(), ZLTextRectangle::RangeChecker(x, y));
	return (it != myTextElementMap.end()) ? &*it : 0;
}

const ZLTextTreeNodeRectangle *ZLTextArea::treeNodeByCoordinates(int x, int y, bool absolute) const {
	if (absolute) {
		x -= myHOffset;
		y -= myVOffset;
	}

	ZLTextTreeNodeMap::const_iterator it =
		std::find_if(myTreeNodeMap.begin(), myTreeNodeMap.end(), ZLTextRectangle::RangeChecker(x, y));
	return (it != myTreeNodeMap.end()) ? &*it : 0;
}

void ZLTextArea::drawSelectionRectangle(int left, int top, int right, int bottom) {
	left = std::max(left, 0);
	right = std::min(right, (int)width() - 1);
	if (left < right) {
		context().setFillColor(myProperties.color(ZLTextStyle::SELECTION_BACKGROUND));
		context().fillRectangle(
			myHOffset + left,
			myVOffset + top + (bottom - top) / 6,
			myHOffset + right,
			myVOffset + top + 5 * (bottom - top) / 6
		);
	}
}

ZLTextSelectionModel &ZLTextArea::selectionModel() {
	if (mySelectionModel.isNull()) {
		mySelectionModel = new ZLTextSelectionModel(*this);
	}
	return *mySelectionModel;
}

void ZLTextArea::paint() {
	myTextElementMap.clear();
	myTreeNodeMap.clear();

	std::vector<size_t> labels;
	labels.reserve(myLineInfos.size() + 1);
	labels.push_back(0);

	ZLTextArea::Style style(*this, myProperties.baseStyle());

	int y = 0;
	for (std::vector<ZLTextLineInfoPtr>::const_iterator it = myLineInfos.begin(); it != myLineInfos.end(); ++it) {
		const ZLTextLineInfo &info = **it;
		prepareTextLine(style, info, y);
		y += info.Height + info.Descent + info.VSpaceAfter;
		labels.push_back(myTextElementMap.size());
	}

	if (!mySelectionModel.isNull()) {
		if (!myProperties.isSelectionEnabled()) {
			mySelectionModel->clear();
		} else {
			mySelectionModel->update();
		}
	}

	y = 0;
	int index = 0;
	for (std::vector<ZLTextLineInfoPtr>::const_iterator it = myLineInfos.begin(); it != myLineInfos.end(); ++it) {
		const ZLTextLineInfo &info = **it;
		drawTextLine(style, info, y, labels[index], labels[index + 1]);
		y += info.Height + info.Descent + info.VSpaceAfter;
		++index;
	}
}
