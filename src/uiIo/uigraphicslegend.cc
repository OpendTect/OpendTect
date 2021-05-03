/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		July 2015
________________________________________________________________________

-*/


#include "uigraphicslegend.h"

#include "uifont.h"
#include "uigraphicsitemimpl.h"

uiLegendItem::uiLegendItem()
    : uiRectItem()
    , headerfont_(*new FontData(8))
    , infofont_(*new FontData(8))
{
    buildLayout();
    setProperties();
}


uiLegendItem::~uiLegendItem()
{
    delete &headerfont_;
    delete &infofont_;
}


void uiLegendItem::buildLayout()
{
    uiAdvancedTextItem* country = new uiAdvancedTextItem( sCountry() );
    uiAdvancedTextItem* block = new uiAdvancedTextItem( sBlock() );
    uiAdvancedTextItem* license = new uiAdvancedTextItem( sLicense() );
    uiAdvancedTextItem* modelname = new uiAdvancedTextItem( sModelNm() );
    uiAdvancedTextItem* horizonname = new uiAdvancedTextItem( sHorNm() );
    uiAdvancedTextItem* mapscale = new uiAdvancedTextItem( sScale() );
    uiAdvancedTextItem* contourinc = new uiAdvancedTextItem( sContourInc() );
    uiAdvancedTextItem* username = new uiAdvancedTextItem( sUserNm() );
    uiAdvancedTextItem* date = new uiAdvancedTextItem( sDate() );
    uiAdvancedTextItem* sign = new uiAdvancedTextItem( sSignature() );

    const int width = 100;
    const int height = 40;
    uiRectItem* rec0 = new uiRectItem( 0, 0, 2*width, height );
    uiRectItem* separator = new uiRectItem( 0, 0, 2*width, height/10 );
    uiRectItem* rec1 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec2 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec3 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec4 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec5 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec6 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec7 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec8 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec9 = new uiRectItem( 0, 0, width, height );
    uiRectItem* rec10 = new uiRectItem( 0, 0, width, height );

    addChild( country );
    addChild( block );
    addChild( license );
    addChild( modelname );
    addChild( horizonname );
    addChild( mapscale );
    addChild( contourinc );
    addChild( username );
    addChild( date );
    addChild( sign );

    addChild( rec0 );
    addChild( separator );
    addChild( rec1 );
    addChild( rec2 );
    addChild( rec3 );
    addChild( rec4 );
    addChild( rec5 );
    addChild( rec6 );
    addChild( rec7 );
    addChild( rec8 );
    addChild( rec9 );
    addChild( rec10 );

    country->setFont( headerfont_ );
    block->setFont( headerfont_ );
    license->setFont( headerfont_ );
    modelname->setFont( headerfont_ );
    horizonname->setFont( headerfont_ );
    mapscale->setFont( headerfont_ );
    contourinc->setFont( headerfont_ );
    username->setFont( headerfont_ );
    date->setFont( headerfont_ );
    sign->setFont( headerfont_ );

// interactive text
    title_ = new uiAdvancedTextItem( sMap() );

    Alignment al( Alignment::HCenter, Alignment::Bottom );
    country_ = new uiAdvancedTextItem( sCountry(), al, true );
    block_ = new uiAdvancedTextItem( sBlock(), al, true );
    license_ = new uiAdvancedTextItem( sLicense(), al, true );
    modelname_ = new uiAdvancedTextItem( sModelNm(), al, true );
    horizonname_ = new uiAdvancedTextItem( sHorNm(), al, true );
    mapscale_ = new uiAdvancedTextItem( sScale(), al, true );
    contourinc_ = new uiAdvancedTextItem( sContourInc(), al, true );
    username_ = new uiAdvancedTextItem( sUserNm(), al, true );
    date_ = new uiAdvancedTextItem( sDate(), al, true );
    sign_ = new uiAdvancedTextItem( sSignature(), al, true );

    addChild( title_ );
    addChild( country_ );
    addChild( block_ );
    addChild( license_ );
    addChild( modelname_ );
    addChild( horizonname_ );
    addChild( mapscale_ );
    addChild( contourinc_ );
    addChild( username_ );
    addChild( date_ );
    addChild( sign_ );

    title_->setTextIteraction( true );
    country_->setTextIteraction( true );
    block_->setTextIteraction( true );
    license_->setTextIteraction( true );
    modelname_->setTextIteraction( true );
    horizonname_->setTextIteraction( true );
    mapscale_->setTextIteraction( true );
    contourinc_->setTextIteraction( true );
    username_->setTextIteraction( true );
    date_->setTextIteraction( true );
    sign_->setTextIteraction( true );

    country_->setFont( infofont_ );
    block_->setFont( infofont_ );
    license_->setFont( infofont_ );
    modelname_->setFont( infofont_ );
    horizonname_->setFont( infofont_ );
    mapscale_->setFont( infofont_ );
    contourinc_->setFont( infofont_ );
    username_->setFont( infofont_ );
    date_->setFont( infofont_ );
    sign_->setFont( infofont_ );

    const float w = (float)width;
    const float h = (float)height;

    country_->setTextWidth( w );
    block_->setTextWidth( w );
    license_->setTextWidth( w );
    modelname_->setTextWidth( w );
    horizonname_->setTextWidth( w );
    mapscale_->setTextWidth( w );
    contourinc_->setTextWidth( w );
    username_->setTextWidth( w );
    date_->setTextWidth( w );
    sign_->setTextWidth( w );

// setting position

    rec0->setPos(0,0);

    separator->setPos(0,h-3.f);

    country->setPos(0,h);	mapscale->setPos(w,h);
    rec1->setPos(0,h);		rec6->setPos(w,h);

    block->setPos(0,2*h);	contourinc->setPos(w,2*h);
    rec2->setPos(0,2*h);	rec7->setPos(w,2*h);

    license->setPos(0,3*h);	username->setPos(w,3*h);
    rec3->setPos(0,3*h);	rec8->setPos(w,3*h);

    modelname->setPos(0,4*h);	date->setPos(w,4*h);
    rec4->setPos(0,4*h);	rec9->setPos(w,4*h);

    horizonname->setPos(0,5*h); sign->setPos(w,5*h);
    rec5->setPos(0,5*h);	rec10->setPos(w,5*h);

    title_->setPos(0,0);

    country_->setPos(w/2,2*h);	    mapscale_->setPos(1.5f*w,2*h);

    block_->setPos(w/2,3*h);	    contourinc_->setPos(1.5f*w,3*h);

    license_->setPos(w/2,4*h);	    username_->setPos(1.5f*w,4*h);

    modelname_->setPos(w/2,5*h);    date_->setPos(1.5f*w,5*h);

    horizonname_->setPos(w/2,6*h);  sign_->setPos(1.5f*w,6*h);

    setRect( 0, 0, 2*width, 5*height );
}


void uiLegendItem::setProperties()
{
    setAcceptHoverEvents( true );
    setItemIgnoresTransformations( true );
    setMovable( true );
}
