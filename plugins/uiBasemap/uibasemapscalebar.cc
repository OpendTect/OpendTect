/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapscalebar.h"

#include "uigraphicsitemimpl.h"
#include "survinfo.h"


// uiMapScaleObject
uiMapScaleObject::uiMapScaleObject( BaseMapObject* bmo )
    : uiBaseMapObject(bmo)
    , ls_(LineStyle::Solid,1,Color::Black())
{
    scalelen_ = (float)( 0.05 * ( SI().maxCoord(false).x -
				  SI().minCoord(false).x ) );
    scalelen_ = (float)( 100 * mCast(int,scalelen_ / 100) );

    scaleline_ = new uiLineItem;
    itemgrp_.add( scaleline_ );

    leftcornerline_ = new uiLineItem;
    itemgrp_.add( leftcornerline_ );

    rightcornerline_ = new uiLineItem;
    itemgrp_.add( rightcornerline_ );

    mDeclAlignment( txtalign, HCenter, Top );

    scalelabelorigin_ = new uiTextItem;
    scalelabelorigin_->setAlignment( txtalign );
    itemgrp_.add( scalelabelorigin_ );

    scalelabelend_ = new uiTextItem;
    scalelabelend_->setAlignment( txtalign );
    itemgrp_.add( scalelabelend_ );
}


void uiMapScaleObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiMapScaleObject::setPixelPos( int x, int y )
{
    uistartposition_.setXY( x, y );
}


void uiMapScaleObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiMapScaleObject::update()
{
    if ( !survinfo_ )
	{ setVisibility( false ); return; }

    const float worldscalelen = scalelen_;
    const int sideoffs = 80;
    const int scalecornerlen = 2;

    const int xmax = uistartposition_.x;
    const int ymin = uistartposition_.y;

    const float worldref = xmax - worldscalelen;
    const float uiscalelen = (float)xmax - worldref;

    const int lastx = xmax - 1 - sideoffs;
    const int firsty = ymin - 70;

    const Geom::Point2D<float> origin( (float)lastx - uiscalelen,
				       (float)firsty );
    const Geom::Point2D<float> end( (float)lastx, (float)firsty );
    scaleline_->setLine( origin, end );
    scaleline_->setPenStyle( ls_ );

    leftcornerline_->setLine( origin, 0.0f, (float)scalecornerlen,
				      0.0f, (float)scalecornerlen );
    leftcornerline_->setPenStyle( ls_ );

    rightcornerline_->setLine( end, 0.0f, (float)scalecornerlen,
				    0.0f, (float)scalecornerlen );
    rightcornerline_->setPenStyle( ls_ );

    BufferString label_origin = "0";
    BufferString label_end; label_end.set( worldscalelen, 0 );
    label_end += survinfo_->getXYUnitString( false );

    scalelabelorigin_->setPos( origin );
    scalelabelorigin_->setText( label_origin );

    scalelabelend_->setPos( end );
    scalelabelend_->setText( label_end );

    setVisibility( true );
}


void uiMapScaleObject::setScaleLen( float scalelen )
{
    scalelen_ = scalelen;
    update();
}


void uiMapScaleObject::setLineStyle( const LineStyle& ls )
{
    ls_ = ls;
    update();
}

