/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.cc,v 1.11 2008-04-03 07:09:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uisurvmap.h"
#include "survinfo.h"
#include "uicanvas.h"
#include "uifont.h"
#include "uiworld2ui.h"
#include "cubesampling.h"

#include "draw.h"
#include "iodrawtool.h"


uiSurveyMap::uiSurveyMap( uiCanvas* cv )
    : mapcanvas(cv)
{
}


void uiSurveyMap::drawMap( const SurveyInfo* survinfo )
{
    ioDrawTool& dt = mapcanvas->drawTool();
    dt.setDrawAreaBackgroundColor( Color::White );
    if ( survinfo->sampling(false).hrg.totalNr() < 2 )
    	return;

    dt.setPenColor( Color::Black );
    dt.setFont( uiFontList::get(FontData::key(FontData::GraphicsLarge)) ); 
    const char* txt = survinfo->name();
    int w = dt.getDevWidth(); int h = dt.getDevHeight();
    dt.drawText( w/2, h/20, txt, mAlign(Middle,Stop) );

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
    uiSize sz( w, h );
    uiWorld2Ui w2ui( wr, sz );
    uiPoint cpt[4];
    for ( int idx=0; idx<4; idx++ )
    {
        cpt[idx] = w2ui.transform( uiWorldPoint(mapcnr[idx].x, mapcnr[idx].y) );
        dt.drawMarker( cpt[idx], MarkerStyle2D::Square );
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    dt.setLineStyle( ls );
    for ( int idx=0; idx<4; idx++ )
        dt.drawLine( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0] );

    dt.setPenColor( Color::Black );
    dt.setFont( uiFontList::get(FontData::key(FontData::GraphicsSmall)) );
    for ( int idx=0; idx<4; idx++ )
    {
	bool bot = cpt[idx].y > h/2;
	const Alignment al( Alignment::Middle,
			    bot ? Alignment::Middle : Alignment::Start );
        BinID bid = survinfo->transform( mapcnr[idx] );
        int spacing =  bot ? 20 : -20;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
        dt.drawText( cpt[idx].x, cpt[idx].y+spacing, annot, al );
        double xcoord = double( int( mapcnr[idx].x*10 + .5 ) ) / 10;
        double ycoord = double( int( mapcnr[idx].y*10 + .5 ) ) / 10;
        annot = "";
        annot += "("; annot += xcoord; annot += ",";
        annot += ycoord; annot += ")";
        dt.drawText( cpt[idx].x, mNINT(cpt[idx].y+1.5*spacing), annot, al );
    }
}


uiSurveyMapDlg::uiSurveyMapDlg( uiParent* p )
	: uiDialog(p,Setup("Survey Map",0,"0.3.3"))
{
    cv = new uiCanvas( this, "survey map" );
    cv->setPrefHeight( 400 );
    cv->setPrefWidth( 400 );
    cv->setStretch(0,0);
    cv->preDraw.notify( mCB(this,uiSurveyMapDlg,doCanvas) );
}


void uiSurveyMapDlg::doCanvas( CallBacker* )
{
    uiSurveyMap* survmap = new uiSurveyMap( cv );
    SurveyInfo survinfo( SI() );
    survmap->drawMap( &survinfo );
}
