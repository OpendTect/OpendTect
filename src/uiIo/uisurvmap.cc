/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.cc,v 1.16 2008-10-09 17:43:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uisurvmap.h"

#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifont.h"
#include "uiworld2ui.h"

#include "cubesampling.h"
#include "draw.h"
#include "survinfo.h"


uiSurveyMap::uiSurveyMap( uiParent* parent )
    : uiGraphicsView( parent, "Survey map view" )
    , mapscene_(new uiGraphicsScene("Survey map scene"))
{
    setScene( *mapscene_ );
    setScrollBar( false );
}


void uiSurveyMap::drawMap( const SurveyInfo* survinfo )
{
    mapscene_->removeAllItems();

    if ( survinfo->sampling(false).hrg.totalNr() < 2 )
    	return;

    uiTextItem* textitem = mapscene_->addText( survinfo->name() );
    textitem->setPenColor( Color::Black );
    textitem->setFont( uiFontList::get(FontData::key(FontData::GraphicsLarge)));
    textitem->setPos( width()/2, 10 );
    Alignment al( Alignment::Middle, Alignment::Start );
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
    uiSize sz( width(), height() );
    uiWorld2Ui w2ui; w2ui.set( uiRect(5,5,width()-10,height()-10), wr );
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
	uiLineItem* lineitm =
	    mapscene_->addLine( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0] );
	lineitm->setPenStyle( ls );

	uiPointItem* ptitm = new uiPointItem();
	ptitm->setPos( cpt[idx].x, cpt[idx].y );
	ptitm->setZValue( 1 );
	mapscene_->addItem( ptitm );
    }

    bool printxy = false;
    for ( int idx=0; idx<4; idx++ )
    {
	bool bot = cpt[idx].y > height()/2;
	const Alignment al( Alignment::Middle, Alignment::Middle );
        BinID bid = survinfo->transform( mapcnr[idx] );
        int spacing =  bot ? 10 : -10;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
        uiTextItem* textitm1 = mapscene_->addText( annot.buf() );
	textitm1->setPos( cpt[idx].x, cpt[idx].y+spacing );
	textitm1->setPenColor( Color::Black );
	textitm1->setFont(
		uiFontList::get(FontData::key(FontData::GraphicsSmall)) );
	textitm1->setAlignment( al );
	textitm1->setZValue( 1 );

	if ( printxy )
	{
	    double xcoord = double( int( mapcnr[idx].x*10 + .5 ) ) / 10;
	    double ycoord = double( int( mapcnr[idx].y*10 + .5 ) ) / 10;
	    annot = "("; annot += xcoord; annot += ",";
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

    setViewArea( 0, 0, mapscene_->width(), mapscene_->height() );
}
