/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.4 2009-04-20 06:20:58 cvsnanne Exp $";


#include "uibutton.h"
#include "pixmap.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicssaveimagedlg.h"

uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(true)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getSaveImageTB( uiParent* p )
{
    return enableimagesave_ ? new uiToolButton( p, "Save image",
					ioPixmap("snapshot.png"),
					mCB(this,uiGraphicsView,saveImageCB) )
			    : 0;
}


void uiGraphicsView::enableImageSave()	{ enableimagesave_ = true; }
void uiGraphicsView::disableImageSave()	{ enableimagesave_ = false; }

void uiGraphicsView::saveImageCB( CallBacker* )
{
    if ( !enableimagesave_ ) return;

    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}
