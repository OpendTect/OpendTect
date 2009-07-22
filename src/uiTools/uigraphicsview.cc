/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.6 2009-07-22 16:01:42 cvsbert Exp $";


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
