/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
________________________________________________________________________

-*/


#include "uigraphicsscalebar.h"

#include "uigraphicsitemimpl.h"
#include "survinfo.h"
#include "uistrings.h"

uiScaleBarItem::uiScaleBarItem( int pxwidth, int pxheight )
    : uiGraphicsItem()
    , preferablepxwidth_(pxwidth)
    , pxwidth_(pxwidth)
    , pxheight_(pxheight)
    , w2ui_(uiWorld2Ui())
    , unitstr_(SI().getUiXYUnitString(true,false))
    , worldwidth_((float)pxwidth)
    , annotpos_(OD::Top)
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
    startnr_ = new uiAdvancedTextItem( uiStrings::sEmptyString(), cenbot );
    addChild( startnr_ );
    midnr_ = new uiAdvancedTextItem( uiStrings::sEmptyString(), cenbot );
    addChild( midnr_ );
    stopnr_ = new uiAdvancedTextItem( uiStrings::sEmptyString(), cenbot );
    addChild( stopnr_ );

    // filling with color
    upperleft_->setFillColor( Color::Black(), true );
    uppermid_->setFillColor( Color::White(), true );
    upperright_->setFillColor( Color::Black(), true );
    lowerleft_->setFillColor( Color::White(), true );
    lowermid_->setFillColor( Color::Black(), true );
    lowerright_->setFillColor( Color::White(), true );

    update();
}


void uiScaleBarItem::setWorld2Ui( const uiWorld2Ui& w2ui )
{
    w2ui_ = w2ui;
}


void uiScaleBarItem::update()
{
    adjustValues();
    setPolygons( pxwidth_/4, pxheight_ );

    startnr_->setPlainText( toUiString("0") );
    midnr_->setPlainText( toUiString(worldwidth_/2) );
    stopnr_->setPlainText( (toUiString("%1 %2")).arg(toUiString(worldwidth_))
								.arg(unitstr_));
}


void uiScaleBarItem::setPolygons( int width, int height )
{
    upperleft_->setRect( -4*width, 0, width, height );
    uppermid_->setRect( -3*width, 0, width, height );
    upperright_->setRect( -2*width, 0, 2*width, height );

    lowerleft_->setRect( -4*width, height, width, height );
    lowermid_->setRect( -3*width, height, width, height );
    lowerright_->setRect( -2*width, height, 2*width, height );

    const Alignment al = Alignment( Alignment::HCenter,
		annotpos_==OD::Top ? Alignment::Bottom : Alignment::Top );
    const int ypos = annotpos_==OD::Top ? 1 : 2*height;
    startnr_->setPos( -4.0f*width, ypos );
    midnr_->setPos( -2.0f * width, ypos );
    stopnr_->setPos( 0, ypos );
    startnr_->setAlignment( al );
    midnr_->setAlignment( al );
    stopnr_->setAlignment( al );
}


void uiScaleBarItem::adjustValues()
{
    float scalex, scaley;
    getScale( scalex, scaley );
    worldwidth_ = w2ui_.toWorldX(preferablepxwidth_) - w2ui_.toWorldX(0);
    worldwidth_ *= scalex;
    worldwidth_ = Math::Abs( worldwidth_ );

    float rval = 1.f;
    while ( worldwidth_/10.f > rval )
	rval *= 10.f;

    const float vroundedtotenth = Math::Floor(worldwidth_/rval+.5f)*rval;
    if ( worldwidth_ != vroundedtotenth )
    {
	worldwidth_ = vroundedtotenth;
	pxwidth_ = mNINT32( Math::Abs( worldwidth_/
				(w2ui_.worldPerPixel().x*scalex) ) );
    }
}


void uiScaleBarItem::setAnnotPos( OD::Edge pos )
{
    annotpos_ = pos;
}
