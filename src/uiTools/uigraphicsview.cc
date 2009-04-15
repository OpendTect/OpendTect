/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.3 2009-04-15 12:12:01 cvssatyaki Exp $";


#include "uibutton.h"
#include "pixmap.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicssaveimagedlg.h"

uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm, bool cansaveimage )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(cansaveimage)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getSaveImageTB( uiParent* p )
{
    if ( !enableimagesave_ )
	return 0;
    uiToolButton* savetb =
	new uiToolButton( p, "save Image as", ioPixmap("snapshot.png"),
			  mCB(this,uiGraphicsView,saveImageCB) );
    return savetb;
}


void uiGraphicsView::enableImageSave()	{ enableimagesave_ = true; }
void uiGraphicsView::disableImageSave()	{ enableimagesave_ = false; }

void uiGraphicsView::saveImageCB( CallBacker* )
{
    if ( !enableimagesave_ ) return;

    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}
