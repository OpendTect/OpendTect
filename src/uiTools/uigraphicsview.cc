/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.2 2009-04-06 17:59:45 cvsnanne Exp $";


#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicssaveimagedlg.h"

uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm, bool cansaveimage )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(cansaveimage)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


void uiGraphicsView::enableImageSave()	{ enableimagesave_ = true; }
void uiGraphicsView::disableImageSave()	{ enableimagesave_ = false; }

void uiGraphicsView::saveImageCB( CallBacker* )
{
    if ( !enableimagesave_ ) return;

    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}
