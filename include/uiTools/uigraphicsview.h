#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsviewbase.h"

class uiGraphicsItemGroup;
class uiLineItem;
class uiToolButton;
class uiParent;
class LineStyle;

mExpClass(uiTools) uiCrossHairItem : public CallBacker
{
public:
			uiCrossHairItem(uiGraphicsViewBase&);
			~uiCrossHairItem();

    void		setLineStyle(const LineStyle&);
    const LineStyle&	getLineStyle() const;

    void		show(bool yn);
    bool		isShown() const;

protected:
    void		mouseMoveCB(CallBacker*);

    uiGraphicsItemGroup* itemgrp_;
    uiLineItem*		horline_;
    uiLineItem*		vertline_;

    LineStyle&		ls_;
    uiGraphicsViewBase& view_;
};


mExpClass(uiTools) uiGraphicsView : public uiGraphicsViewBase
{ mODTextTranslationClass(uiGraphicsView);
public:
				uiGraphicsView(uiParent*,const char* nm);

    uiToolButton*		getSaveImageButton(uiParent*);
    uiToolButton*		getPrintImageButton(uiParent*);

    void			enableImageSave();
    void			disableImageSave();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
    void 			printImageCB(CallBacker*);
};

#endif


