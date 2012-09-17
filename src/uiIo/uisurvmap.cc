/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurvmap.cc,v 1.41 2012/07/10 13:06:07 cvskris Exp $";

#include "uisurvmap.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifont.h"
#include "uiworld2ui.h"

#include "cubesampling.h"
#include "draw.h"
#include "survinfo.h"
#include "angles.h"


uiSurveyBoxObject::uiSurveyBoxObject( BaseMapObject* bmo, bool withlabels )
    : uiBaseMapObject(bmo)
{
    for ( int idx=0; idx<4; idx++ )
    {
        uiMarkerItem* markeritem = new uiMarkerItem( MarkerStyle2D::Square );
	markeritem->setZValue( 1 );
	itemgrp_->add( markeritem );
	vertices_ += markeritem;
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    for ( int idx=0; idx<4; idx++ )
    {
	uiLineItem* lineitem = new uiLineItem();
	lineitem->setPenColor( red );
	lineitem->setPenStyle( ls );
	itemgrp_->add( lineitem );
	edges_ += lineitem;
    }

    if ( !withlabels )
	return;

    const mDeclAlignment( postxtalign, HCenter, VCenter );
    for ( int idx=0; idx<4; idx++ )
    {
        uiTextItem* textitem = new uiTextItem();
	textitem->setTextColor( Color::Black() );
	textitem->setAlignment( postxtalign );
	textitem->setFont(
		FontList().get(FontData::key(FontData::GraphicsSmall)) );
	textitem->setZValue( 1 );
	itemgrp_->add( textitem );
	labels_ += textitem;
    }
}


void uiSurveyBoxObject::setSurveyInfo( const SurveyInfo& si )
{
    survinfo_ = &si;
}


void uiSurveyBoxObject::update()
{
    if ( !survinfo_ || !transform_ )
	return;

    const SurveyInfo& si = *survinfo_;
    const CubeSampling& cs = si.sampling( false );
    Coord mapcnr[4];
    mapcnr[0] = si.transform( cs.hrg.start );
    mapcnr[1] = si.transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    mapcnr[2] = si.transform( cs.hrg.stop );
    mapcnr[3] = si.transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );

    uiPoint cpt[4];
    for ( int idx=0; idx<vertices_.size(); idx++ )
    {
        cpt[idx] = transform_->transform( uiWorldPoint(mapcnr[idx].x,
		    				      mapcnr[idx].y) );
	vertices_[idx]->setPos( cpt[idx] );
    }

    for ( int idx=0; idx<edges_.size(); idx++ )
	edges_[idx]->setLine( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0], true );

    for ( int idx=0; idx<labels_.size(); idx++ )
    {
	const int oppidx = idx < 2 ? idx + 2 : idx - 2;
	const bool bot = cpt[idx].y > cpt[oppidx].y;
        BinID bid = si.transform( mapcnr[idx] );
        const int spacing =  bot ? 10 : -10;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
	uiPoint txtpos( cpt[idx].x, cpt[idx].y+spacing );
	labels_[idx]->setPos( txtpos );
	labels_[idx]->setText( annot.buf() );
    }
}


uiNorthArrowObject::uiNorthArrowObject( BaseMapObject* bmo, bool withangle )
    : uiBaseMapObject(bmo)
    , angleline_(0),anglelabel_(0)
{   
    ArrowStyle arrowstyle( 3, ArrowStyle::HeadOnly );
    arrowstyle.linestyle_.width_ = 3;
    arrow_ = new uiArrowItem;
    arrow_->setArrowStyle( arrowstyle );
    itemgrp_->add( arrow_ );

    if ( !withangle )
	return;

    angleline_ = new uiLineItem;
    angleline_->setPenStyle( LineStyle(LineStyle::Dot,2,Color(255,0,0)) );
    itemgrp_->add( angleline_ );

    mDeclAlignment( txtalign, Right, Bottom );
    anglelabel_ = new uiTextItem();
    anglelabel_->setAlignment( txtalign );
    itemgrp_->add( anglelabel_ );
}


void uiNorthArrowObject::setSurveyInfo( const SurveyInfo& si )
{
    survinfo_ = &si;
}


void uiNorthArrowObject::update()
{
    if ( !survinfo_ || !transform_ )
	return;

    static const float halfpi = M_PI * .5;
    static const float quartpi = M_PI * .25;

    float mathang = survinfo_->computeAngleXInl();

	    // To [0,pi]
    if ( mathang < 0 )			mathang += M_PI;
    if ( mathang > M_PI )		mathang -= M_PI;
	    // Find angle closest to N, not necessarily X vs inline
    if ( mathang < quartpi )		mathang += halfpi;
    if ( mathang > halfpi+quartpi )	mathang -= halfpi;

    float usrang = Angle::rad2usrdeg( mathang );
    if ( usrang > 180 ) usrang = 360 - usrang;

    const bool northisleft = mathang < halfpi;
    const int arrowlen = 30;
    const int sideoffs = 10;
    const int yarrowtop = 20;

    float dx = arrowlen * tan( halfpi-mathang );
    const int dxpix = mNINT32( dx );
    float worldxmin, worldxmax;
    transform_->getWorldXRange( worldxmin, worldxmax );
    const int xmax = transform_->toUiX( worldxmax );
    const int lastx = xmax - 1 - sideoffs;
    const uiPoint origin( lastx - (northisleft?dxpix:0), arrowlen + yarrowtop );
    const uiPoint arrowtop( origin.x, yarrowtop );

    arrow_->setTailHeadPos( origin, arrowtop );
    if ( !angleline_ || !anglelabel_ )
	return;

    angleline_->setLine( origin, uiPoint(origin.x+dxpix,yarrowtop), true );
    float usrang100 = usrang * 100;
    if ( usrang100 < 0 ) usrang100 = -usrang100;
    int iusrang = (int)(usrang100 + .5);
    BufferString angtxt;
    if ( iusrang )
    {
	angtxt += iusrang / 100;
	iusrang = iusrang % 100;
	if ( iusrang )
	{
	    angtxt += ".";
	    angtxt += iusrang / 10; iusrang = iusrang % 10;
	    if ( iusrang )
		angtxt += iusrang;
	}
    }
	
    anglelabel_->setPos( lastx, yarrowtop );
    anglelabel_->setText( angtxt );
}


uiSurveyMap::uiSurveyMap( uiParent* p, bool withtitle )
    : uiBaseMap(p)
    , survbox_(0),northarrow_(0)
    , survinfo_(0)
    , title_(0)
{
    view_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    view_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    const mDeclAlignment( txtalign, Left, Top );

    if ( !withtitle )
	return;

    title_ = view_.scene().addItem( new uiTextItem(uiPoint(10,10),"Survey name",
						   txtalign) );
    title_->setPenColor( Color::Black() );
    title_->setFont( FontList().get(FontData::key(FontData::GraphicsLarge)));
}


void uiSurveyMap::drawMap( const SurveyInfo* si )
{
    if ( !survbox_ )
    {
	survbox_ = new uiSurveyBoxObject( 0, true );
	addObject( survbox_ );
	if ( title_ )
	{
	    northarrow_ = new uiNorthArrowObject( 0, true );
	    addObject( northarrow_ );
	}
    }

    if ( !si )
	return;

    if ( si != survinfo_ )
	survinfo_ = si;

    view_.setViewArea( 0, 0, view_.scene().width(), view_.scene().height() );

    uiBorder border( 20, title_ ? 70 : 20, 20, 20 );
    uiSize sz( (int)view_.scene().width(), (int)view_.scene().height() );
    uiRect rc = border.getRect( sz );
    w2ui_.set( rc, *si );
    if ( title_ )
	title_->setText( si->name() );

    survbox_->setSurveyInfo( *si );
    if ( northarrow_ )
	northarrow_->setSurveyInfo( *si );

    uiBaseMap::reDraw();
}


void uiSurveyMap::reDraw( bool )
{
    if ( !survinfo_ )
	return;

    drawMap( survinfo_ );
}

