#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.1 2009-03-26 08:17:42 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigraphicsviewbase.h"

mClass uiGraphicsView : public uiGraphicsViewBase
{
public:
				uiGraphicsView(uiParent*,const char*,
					       bool cansaveimage=false);
				~uiGraphicsView();

protected:
    void 			showSaveDialog(CallBacker*);
};

#endif
