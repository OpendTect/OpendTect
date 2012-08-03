#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.8 2012-08-03 13:01:13 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsviewbase.h"

class uiToolButton;
class uiParent;

mClass(uiTools) uiGraphicsView : public uiGraphicsViewBase
{
public:
				uiGraphicsView(uiParent*,const char* nm);

    uiToolButton*		getSaveImageButton(uiParent*);
    void			enableImageSave();
    void			disableImageSave();

protected:
    bool			enableimagesave_;
    void 			saveImageCB(CallBacker*);
};

#endif

