/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";

#include "uigraphicsscalebar.h"

#include "uigraphicsitemimpl.h"
#include "survinfo.h"

uiScaleBarItem::uiScaleBarItem( int pxwidth, int pxheight )
    : uiGraphicsItemGroup()
    , length_(pxwidth)
    , pxwidth_(pxwidth)
    , pxheight_(pxheight)
{
    initDefaultScale();
}


uiScaleBarItem::~uiScaleBarItem()
{
    removeAll( true );
}


void uiScaleBarItem::initDefaultScale()
{
    upperleft_ = new uiRectItem; add( upperleft_ );
    uppermid_ = new uiRectItem; add( uppermid_ );
    upperright_ = new uiRectItem; add( upperright_ );
    lowerleft_ = new uiRectItem; add( lowerleft_ );
    lowermid_ = new uiRectItem; add( lowermid_ );
    lowerright_ = new uiRectItem; add( lowerright_ );

    const Alignment cenbot = Alignment(Alignment::HCenter,Alignment::Bottom);
    startnr_ = new uiTextItem( "", cenbot ); add( startnr_ );
    midnr_ = new uiTextItem( "", cenbot ); add( midnr_ );
    stopnr_ = new uiTextItem( "", cenbot ); add( stopnr_ );

    // filling with color
    upperleft_->setFillColor( Color::Black(), true );
    uppermid_->setFillColor( Color::White(), true );
    upperright_->setFillColor( Color::Black(), true );
    lowerleft_->setFillColor( Color::White(), true );
    lowermid_->setFillColor( Color::Black(), true );
    lowerright_->setFillColor( Color::White(), true );

    update();
}


void uiScaleBarItem::update()
{
    setPolygons( pxwidth_/4, pxheight_ );

    // TODO: Remove this hack [String("   ")] when OD Text alignment is fixed
    uiString unit = SI().getXYUnitString( false );
    startnr_->setText( uiString("   ").append("0") );
    midnr_->setText( uiString("   ").append(toString(length_/2)) );
    stopnr_->setText( uiString("   ").append(toString(length_)).append(unit) );
}


void uiScaleBarItem::setPolygons( int width, int height )
{
    upperleft_->setRect( 0, 0, width, height );
    uppermid_->setRect( width, 0, width, height );
    upperright_->setRect( 2*width, 0, 2*width, height );

    lowerleft_->setRect( 0, height, width, height );
    lowermid_->setRect( width, height, width, height );
    lowerright_->setRect( 2*width, height, 2*width, height );

    startnr_->setPos( 0, 0 );
    midnr_->setPos( 2*width, 0 );
    stopnr_->setPos( 4*width, 0 );
}
