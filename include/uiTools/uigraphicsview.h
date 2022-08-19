#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsviewbase.h"

class uiAction;
class uiGraphicsItemGroup;
class uiLineItem;
class uiToolButton;
class uiParent;
namespace OD { class LineStyle; }

mExpClass(uiTools) uiCrossHairItem : public CallBacker
{
public:
				uiCrossHairItem(uiGraphicsViewBase&);
				~uiCrossHairItem();

    void			setLineStyle(const OD::LineStyle&);
    const OD::LineStyle&	getLineStyle() const;

    void			show(bool yn);
    bool			isShown() const;

    void			showLine(OD::Orientation,bool yn);
    bool			isLineShown(OD::Orientation) const;

protected:
    void			mouseMoveCB(CallBacker*);

    uiGraphicsItemGroup*	itemgrp_;
    uiLineItem*			horline_;
    uiLineItem*			vertline_;

    OD::LineStyle&		ls_;
    uiGraphicsViewBase&		view_;
};


mExpClass(uiTools) uiGraphicsView : public uiGraphicsViewBase
{ mODTextTranslationClass(uiGraphicsView);
public:
				uiGraphicsView(uiParent*,const char* nm);
				~uiGraphicsView();

    uiAction*			getSaveImageAction();
    uiAction*			getPrintImageAction();
    uiToolButton*		getSaveImageButton(uiParent*);
    uiToolButton*		getPrintImageButton(uiParent*);

    void			enableImageSave();
    void			disableImageSave();

    uiCrossHairItem*		getCrossHairItem();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
    void 			printImageCB(CallBacker*);

    uiCrossHairItem*		crosshairitem_;
};
