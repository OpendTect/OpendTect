/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.5 2009-06-16 04:36:48 cvsnanne Exp $";


#include "uigraphicsview.h"

#include "uibutton.h"
#include "uigraphicssaveimagedlg.h"
#include "uigraphicsscene.h"
#include "pixmap.h"


uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(true)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getSaveImageButton( uiParent* p )
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
