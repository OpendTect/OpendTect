#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.3 2009-04-15 12:12:01 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigraphicsviewbase.h"

class uiToolButton;
class uiParent;

mClass uiGraphicsView : public uiGraphicsViewBase
{
public:
				uiGraphicsView(uiParent*,const char*,
					       bool cansaveimage=false);

    uiToolButton*		getSaveImageTB(uiParent*);
    void			enableImageSave();
    void			disableImageSave();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
};

#endif
