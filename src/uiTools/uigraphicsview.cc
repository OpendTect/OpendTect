/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigraphicsview.h"

#include "uiaction.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"
#include "uigraphicsscene.h"
#include "uitoolbutton.h"


uiCrossHairItem::uiCrossHairItem( uiGraphicsViewBase& vw )
    : view_(vw)
    , ls_(*new OD::LineStyle(OD::LineStyle::Dot,1,OD::Color::LightGrey()))
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


void uiCrossHairItem::setLineStyle( const OD::LineStyle& ls )
{
    ls_ = ls;
    horline_->setPenStyle( ls );
    vertline_->setPenStyle( ls );
}


const OD::LineStyle& uiCrossHairItem::getLineStyle() const
{ return ls_; }

void uiCrossHairItem::show( bool yn )
{ itemgrp_->setVisible( yn ); }

bool uiCrossHairItem::isShown() const
{ return itemgrp_->isVisible(); }


void uiCrossHairItem::showLine( OD::Orientation orient, bool yn )
{
    uiLineItem* itm = orient==OD::Horizontal ? horline_ : vertline_;
    itm->setVisible( yn );
}


bool uiCrossHairItem::isLineShown( OD::Orientation orient ) const
{
    uiLineItem* itm = orient==OD::Horizontal ? horline_ : vertline_;
    return itm->isVisible();
}


// uiGraphicsView
uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(true)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
    crosshairitem_ = new uiCrossHairItem( *this );
    crosshairitem_->show( false );
}


uiGraphicsView::~uiGraphicsView()
{
    delete crosshairitem_;
}


uiCrossHairItem* uiGraphicsView::getCrossHairItem()
{ return crosshairitem_; }


uiAction* uiGraphicsView::getSaveImageAction()
{
    if ( !enableimagesave_ )
	return nullptr;

    auto* action = new uiAction( uiStrings::sEmptyString(),
				 mCB(this,uiGraphicsView,saveImageCB),
				 "snapshot" );
    action->setToolTip( tr("Save image") );
    return action;
}


uiToolButton* uiGraphicsView::getSaveImageButton( uiParent* p )
{
    if ( !enableimagesave_ )
	return nullptr;

    return new uiToolButton( p, "snapshot", tr("Save image"),
			mCB(this,uiGraphicsView,saveImageCB) );
}


uiAction* uiGraphicsView::getPrintImageAction()
{
    auto* action = new uiAction( uiStrings::sEmptyString(),
				 mCB(this,uiGraphicsView,printImageCB),
				 "printer" );
    action->setToolTip( tr("Print image") );
    return action;
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
