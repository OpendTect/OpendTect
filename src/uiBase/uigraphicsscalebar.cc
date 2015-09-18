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
    : uiGraphicsItem()
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
    upperleft_ = new uiRectItem;    addChild( upperleft_ );
    uppermid_ = new uiRectItem;	    addChild( uppermid_ );
    upperright_ = new uiRectItem;   addChild( upperright_ );
    lowerleft_ = new uiRectItem;    addChild( lowerleft_ );
    lowermid_ = new uiRectItem;	    addChild( lowermid_ );
    lowerright_ = new uiRectItem;   addChild( lowerright_ );

    const Alignment cenbot = Alignment( Alignment::HCenter, Alignment::Bottom );
    startnr_ = new uiAdvancedTextItem( "", cenbot );addChild( startnr_ );
    midnr_ = new uiAdvancedTextItem( "", cenbot );  addChild( midnr_ );
    stopnr_ = new uiAdvancedTextItem( "", cenbot ); addChild( stopnr_ );

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

    uiString unit = SI().getXYUnitString( false );
    startnr_->setPlainText( "0" );
    midnr_->setPlainText( toString(length_/2) );
    stopnr_->setPlainText( uiString(toString(length_)).append(unit) );
}


void uiScaleBarItem::setPolygons( int width, int height )
{
    upperleft_->setRect( -4*width, 0, width, height );
    uppermid_->setRect( -3*width, 0, width, height );
    upperright_->setRect( -2*width, 0, 2*width, height );

    lowerleft_->setRect( -4*width, height, width, height );
    lowermid_->setRect( -3*width, height, width, height );
    lowerright_->setRect( -2*width, height, 2*width, height );

    startnr_->setPos( -4.0f*width, 0 );
    midnr_->setPos( -2.0f * width, 0 );
    stopnr_->setPos( 0, 0 );
}
