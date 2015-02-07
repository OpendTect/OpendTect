/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uigraphicsview.h"

#include "uigraphicssaveimagedlg.h"
#include "uigraphicsscene.h"
#include "uitoolbutton.h"


uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(true)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getSaveImageButton( uiParent* p )
{
    if ( !enableimagesave_ ) return 0;

    return new uiToolButton( p, "snapshot", tr("Save image"),
			mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getPrintImageButton( uiParent* p )
{
    return new uiToolButton( p, "printer", tr("Print image"),
			mCB(this,uiGraphicsView,printImageCB) );
}


void uiGraphicsView::enableImageSave()	{ enableimagesave_ = true; }
void uiGraphicsView::disableImageSave()	{ enableimagesave_ = false; }

void uiGraphicsView::saveImageCB( CallBacker* )
{
    if ( !enableimagesave_ ) return;

    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}


void uiGraphicsView::printImageCB( CallBacker* )
{
    print();
}
