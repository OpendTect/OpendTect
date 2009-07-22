#ifndef uigraphicsview_h
#define uigraphicsview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsview.h,v 1.6 2009-07-22 16:01:23 cvsbert Exp $
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
