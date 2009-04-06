#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.2 2009-04-06 17:59:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsviewbase.h"

mClass uiGraphicsView : public uiGraphicsViewBase
{
public:
				uiGraphicsView(uiParent*,const char*,
					       bool cansaveimage=false);

    void			enableImageSave();
    void			disableImageSave();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
};

#endif
