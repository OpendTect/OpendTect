/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.cc,v 1.13 2008-09-01 07:26:20 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uisurvmap.h"
#include "survinfo.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifont.h"
#include "uiworld2ui.h"
#include "cubesampling.h"

#include "draw.h"


uiSurveyMap::uiSurveyMap( uiParent* parent )
    : uiGraphicsView( parent, "Survey map view" )
    , mapscene_(new uiGraphicsScene("Survey map scene"))
{
    setScene( *mapscene_ );
}


void uiSurveyMap::removeItems()
{
    mapscene_->removeAllItems();
}


void uiSurveyMap::drawMap( const SurveyInfo* survinfo )
{
    if ( survinfo->sampling(false).hrg.totalNr() < 2 )
    	return;

    const char* txt = survinfo->name();
    uiTextItem* textitem = mapscene_->addText( txt );
    textitem->setPenColor( Color::Black );
    textitem->setFont( uiFontList::get(FontData::key(FontData::GraphicsLarge)));
    textitem->setPos(  width_/2, height_/20 );
    Alignment al( Alignment::Middle, Alignment::Stop );
    textitem->setAlignment( al );

    const CubeSampling& cs = survinfo->sampling( false );
    Coord mapcnr[4];
    mapcnr[0] = survinfo->transform( cs.hrg.start );
    mapcnr[1] = survinfo->transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    mapcnr[2] = survinfo->transform( cs.hrg.stop );
    mapcnr[3] = survinfo->transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );

    Coord mincoord = mapcnr[0];
    Coord maxcoord = mapcnr[2];
    for ( int idx=0; idx<4; idx++ )
    {
        if ( mapcnr[idx].x < mincoord.x ) mincoord.x = mapcnr[idx].x;
        if ( mapcnr[idx].y < mincoord.y ) mincoord.y = mapcnr[idx].y;
        if ( mapcnr[idx].x > maxcoord.x ) maxcoord.x = mapcnr[idx].x;
        if ( mapcnr[idx].y > maxcoord.y ) maxcoord.y = mapcnr[idx].y;
    }

    const Coord diff( maxcoord - mincoord );
    float canvsz = mMAX(diff.x,diff.y);
    canvsz *= 1.5;
    const Coord center( (mincoord.x+maxcoord.x)/2, (mincoord.y+maxcoord.y)/2 );
    const Coord lowpart( canvsz/2, 0.47*canvsz );
    const Coord hipart( canvsz/2, 0.53*canvsz );
    mincoord = center - lowpart;
    maxcoord = center + hipart;

    uiWorldRect wr( mincoord.x, maxcoord.y, maxcoord.x, mincoord.y );
    uiSize sz( width_, height_ );
    uiWorld2Ui w2ui( wr, sz );
    uiPoint cpt[4];
    for ( int idx=0; idx<4; idx++ )
    {
        cpt[idx] = w2ui.transform( uiWorldPoint(mapcnr[idx].x, mapcnr[idx].y) );
        uiMarkerItem* markeritem =
	    mapscene_->addMarker( MarkerStyle2D::Square );
	markeritem->setPos( cpt[idx].x, cpt[idx].y );
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    for ( int idx=0; idx<4; idx++ )
    {
	uiLineItem* lineitm = mapscene_->addLine( cpt[idx],
						  idx!=3 ? cpt[idx+1]:cpt[0] );
	lineitm->setPenStyle( ls );
    }

    for ( int idx=0; idx<4; idx++ )
    {
	bool bot = cpt[idx].y > height_/2;
	const Alignment al( Alignment::Middle,
			    bot ? Alignment::Middle : Alignment::Start );
        BinID bid = survinfo->transform( mapcnr[idx] );
        int spacing =  bot ? 20 : -20;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
        uiTextItem* textitm1 = mapscene_->addText( annot.buf() );
        textitm1->moveBy( (float)(int)al.hor_, (float)(int)al.ver_ );
	textitm1->setPos( cpt[idx].x, cpt[idx].y+spacing );
	textitm1->setPenColor( Color::Black );
	textitm1->setFont(
		uiFontList::get(FontData::key(FontData::GraphicsSmall)) );
	textitm1->setAlignment( al );
	double xcoord = double( int( mapcnr[idx].x*10 + .5 ) ) / 10;
        double ycoord = double( int( mapcnr[idx].y*10 + .5 ) ) / 10;
        annot = "";
        annot += "("; annot += xcoord; annot += ",";
        annot += ycoord; annot += ")";
        uiTextItem* textitm2 = mapscene_->addText( annot.buf() );
        textitm2->moveBy( (float)(int)al.hor_, (float)(int)al.ver_ );
	textitm2->setPos( cpt[idx].x, mNINT(cpt[idx].y+1.5*spacing) );
	textitm2->setPenColor( Color::Black );
	textitm2->setFont(
		uiFontList::get(FontData::key(FontData::GraphicsSmall)) );
	textitm2->setAlignment( al );
    }
}


uiSurveyMapDlg::uiSurveyMapDlg( uiParent* p )
	: uiDialog(p,Setup("Survey Map",0,"0.3.3"))
{
    scene_ = new uiGraphicsScene( "Scene" );
    
    view_ = new uiGraphicsView( this, "Viewer" );
    view_->setStretch( 0, 0 );
    view_->setPrefWidth( 400 );
    view_->setPrefHeight( 400 );
    view_->setScene( *scene_ );
}


void uiSurveyMapDlg::doCanvas( CallBacker* )
{
    uiSurveyMap* survmap = new uiSurveyMap( this );
    SurveyInfo survinfo( SI() );
    survmap->drawMap( &survinfo );
}
