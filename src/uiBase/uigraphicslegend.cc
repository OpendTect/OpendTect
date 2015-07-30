/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		July 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";

#include "uigraphicslegend.h"

#include "uigraphicsitemimpl.h"

uiLegendItem::uiLegendItem()
    : uiRectItem()
{
    setRect( 0, 0, 200, 150 );

    buildLayout();
    setProperties();

    init();
}


uiLegendItem::~uiLegendItem()
{}


void uiLegendItem::buildLayout()
{
    uiAdvancedTextItem* country = new uiAdvancedTextItem( "Country" );
    uiAdvancedTextItem* block = new uiAdvancedTextItem( "Block" );
    uiAdvancedTextItem* license = new uiAdvancedTextItem( "License" );
    uiAdvancedTextItem* modelname = new uiAdvancedTextItem( "Model Name" );
    uiAdvancedTextItem* horizonname = new uiAdvancedTextItem( "Horizon Name" );
    uiAdvancedTextItem* mapscale = new uiAdvancedTextItem( "Scale" );
    uiAdvancedTextItem* contourinc = new uiAdvancedTextItem( "Contour Inc" );
    uiAdvancedTextItem* username = new uiAdvancedTextItem( "User Name" );
    uiAdvancedTextItem* date = new uiAdvancedTextItem( "Date" );
    uiAdvancedTextItem* sign = new uiAdvancedTextItem( "Signature" );

    uiRectItem* rec0 = new uiRectItem( 0, 0, 200, 30 );
    uiRectItem* separator = new uiRectItem( 0, 0, 200, 3 );
    uiRectItem* rec1 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec2 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec3 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec4 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec5 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec6 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec7 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec8 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec9 = new uiRectItem( 0, 0, 100, 30 );
    uiRectItem* rec10 = new uiRectItem( 0, 0, 100, 30 );

    country->setParent( this );
    block->setParent( this );
    license->setParent( this );
    modelname->setParent( this );
    horizonname->setParent( this );
    mapscale->setParent( this );
    contourinc->setParent( this );
    username->setParent( this );
    date->setParent( this );
    sign->setParent( this );

    rec0->setParent( this );
    separator->setParent( this );
    rec1->setParent( this );
    rec2->setParent( this );
    rec3->setParent( this );
    rec4->setParent( this );
    rec5->setParent( this );
    rec6->setParent( this );
    rec7->setParent( this );
    rec8->setParent( this );
    rec9->setParent( this );
    rec10->setParent( this );

// setting position
    rec0->setPos(0,0);

    separator->setPos(0,27);

    country->setPos(0,30);	mapscale->setPos(100,30);
    rec1->setPos(0,30);		rec6->setPos(100,30);

    block->setPos(0,60);	contourinc->setPos(100,60);
    rec2->setPos(0,60);		rec7->setPos(100,60);

    license->setPos(0,90);	username->setPos(100,90);
    rec3->setPos(0,90);		rec8->setPos(100,90);

    modelname->setPos(0,120);	date->setPos(100,120);
    rec4->setPos(0,120);	rec9->setPos(100,120);

    horizonname->setPos(0,150); sign->setPos(100,150);
    rec5->setPos(0,150);	rec10->setPos(100,150);

    LineStyle ls; ls.width_ = 2;
}


void uiLegendItem::setProperties()
{
    setAcceptHoverEvents( true );
    setItemIgnoresTransformations( true );
    setMovable( true );
}


void uiLegendItem::init()
{
    title_ = new uiAdvancedTextItem( "Map" );
    country_ = new uiAdvancedTextItem( "Country" );
    block_ = new uiAdvancedTextItem( "Block" );
    license_ = new uiAdvancedTextItem( "License" );
    modelname_ = new uiAdvancedTextItem( "Model Name" );
    horizonname_ = new uiAdvancedTextItem( "Horizon Name" );
    mapscale_ = new uiAdvancedTextItem( "Scale" );
    contourinc_ = new uiAdvancedTextItem( "Contour Inc" );
    username_ = new uiAdvancedTextItem( "User Name" );
    date_ = new uiAdvancedTextItem( "Date" );
    sign_ = new uiAdvancedTextItem( "Signature" );

    title_->setParent( this );
    country_->setParent( this );
    block_->setParent( this );
    license_->setParent( this );
    modelname_->setParent( this );
    horizonname_->setParent( this );
    mapscale_->setParent( this );
    contourinc_->setParent( this );
    username_->setParent( this );
    date_->setParent( this );
    sign_->setParent( this );

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


    title_->setPos(0,0);

    country_->setPos(0,30);	mapscale_->setPos(100,30);

    block_->setPos(0,60);	contourinc_->setPos(100,60);

    license_->setPos(0,90);	username_->setPos(100,90);

    modelname_->setPos(0,120);	date_->setPos(100,120);

    horizonname_->setPos(0,150);	sign_->setPos(100,150);
}

