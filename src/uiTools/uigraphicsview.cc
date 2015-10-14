/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uigraphicsview.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"
#include "uigraphicsscene.h"
#include "uitoolbutton.h"


uiCrossHairItem::uiCrossHairItem( uiGraphicsViewBase& vw )
    : view_(vw)
    , ls_(*new LineStyle(LineStyle::Dot,1,Color::LightGrey()))
{
    itemgrp_ = view_.scene().addItemGrp( new uiGraphicsItemGroup );
    horline_ = new uiLineItem; itemgrp_->add( horline_ );
    vertline_ = new uiLineItem; itemgrp_->add( vertline_ );
    horline_->setPenStyle( ls_ );
    vertline_->setPenStyle( ls_ );
    view_.getMouseEventHandler().movement.notify(
	mCB(this,uiCrossHairItem,mouseMoveCB) );
}


uiCrossHairItem::~uiCrossHairItem()
{
    delete &ls_;
    view_.scene().removeItem( itemgrp_ );
}


void uiCrossHairItem::mouseMoveCB( CallBacker* )
{
    const uiRect rect = view_.getViewArea();
    const MouseEvent& ev = view_.getMouseEventHandler().event();
    horline_->setLine( rect.left(), ev.y(), rect.right(), ev.y() );
    vertline_->setLine( ev.x(), rect.top(), ev.x(), rect.bottom() );
}


void uiCrossHairItem::setLineStyle( const LineStyle& ls )
{
    horline_->setPenStyle( ls );
    vertline_->setPenStyle( ls );
}

const LineStyle& uiCrossHairItem::getLineStyle() const
{ return ls_; }

void uiCrossHairItem::show( bool yn )
{ itemgrp_->setVisible( yn ); }

bool uiCrossHairItem::isShown() const
{ return itemgrp_->isVisible(); }




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

