/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsview.cc,v 1.1 2009-03-26 08:17:42 cvssatyaki Exp $";


#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicssaveimagedlg.h"

uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm, bool cansaveimage )
    : uiGraphicsViewBase(p,nm)
{
    if ( cansaveimage )
	scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,showSaveDialog) );
}


uiGraphicsView::~uiGraphicsView()
{}


void uiGraphicsView::showSaveDialog( CallBacker* )
{
    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}
