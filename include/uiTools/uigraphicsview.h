#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.5 2009-06-16 04:36:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsviewbase.h"

class uiToolButton;
class uiParent;

mClass uiGraphicsView : public uiGraphicsViewBase
{
public:
				uiGraphicsView(uiParent*,const char* nm);

    uiToolButton*		getSaveImageButton(uiParent* p=0);
    void			enableImageSave();
    void			disableImageSave();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
};

#endif
