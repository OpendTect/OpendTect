/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "survinfo.h"
#include "uicanvas.h"
#include "uifont.h"
#include "uiworld2ui.h"

#include "uidialog.h"
#include "draw.h"
#include "iodrawtool.h"


class uiSurveyMap
{
public:

uiSurveyMap( uiCanvas* cv, SurveyInfo* si )
	: mapcanvas(cv)
	, survinfo(si)
{}

void drawMap( SurveyInfo* survinfo )
{
    ioDrawTool& dt = *mapcanvas->drawTool();
    dt.beginDraw();
    dt.clear();
    if ( !survinfo->rangeUsable() ) { dt.endDraw(); return; }

    dt.setPenColor( Color::Black );
    dt.setFont( uiFontList::get(FontData::defaultKeys()[3]) ); // Graphics large
    Alignment al = Alignment(Alignment::Middle,Alignment::Stop);
    const char* txt = survinfo->name();
    int w = dt.getDevWidth(); int h = dt.getDevHeight();
    dt.drawText( w/2, h/20, txt, al );

    BinIDRange br = survinfo->range();
    Coord mapcnr[4];
    mapcnr[0] = survinfo->transform( br.start );
    mapcnr[1] = survinfo->transform( BinID(br.start.inl,br.stop.crl) );
    mapcnr[2] = survinfo->transform( br.stop );
    mapcnr[3] = survinfo->transform( BinID(br.stop.inl,br.start.crl) );

    Coord mincoord = mapcnr[0];
    Coord maxcoord = mapcnr[2];
    for ( int idx=0; idx<4; idx++ )
    {
        if ( mapcnr[idx].x < mincoord.x ) mincoord.x = mapcnr[idx].x;
        if ( mapcnr[idx].y < mincoord.y ) mincoord.y = mapcnr[idx].y;
        if ( mapcnr[idx].x > maxcoord.x ) maxcoord.x = mapcnr[idx].x;
        if ( mapcnr[idx].y > maxcoord.y ) maxcoord.y = mapcnr[idx].y;
    }

    Coord mapsize = ( maxcoord - mincoord );
    Coord border(mapsize.x * 0.25, mapsize.y * 0.25);
    mincoord -= border;
    maxcoord += border;

    uiWorldRect wr( mincoord.x, maxcoord.y, maxcoord.x, mincoord.y );
    uiSize sz( w, h );
    uiWorld2Ui w2ui( wr, sz );
    uiPoint cpt[4];
    MarkerStyle ms = MarkerStyle::Square;
    for ( int idx=0; idx<4; idx++ )
    {
        cpt[idx] = w2ui.transform( uiWorldPoint(mapcnr[idx].x, mapcnr[idx].y) );
        dt.drawMarker( cpt[idx], ms );
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    dt.setLineStyle( ls );
    for ( int idx=0; idx<4; idx++ )
    {
        dt.drawLine( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0] );
    }

    dt.setPenColor( Color::Black );
    dt.setFont( uiFontList::get(FontData::defaultKeys()[2]) ); // Graphics small
    BufferString ann;
    for ( int idx=0; idx<4; idx++ )
    {
        BinID bid = survinfo->transform( mapcnr[idx] );
        int spacing =  cpt[idx].y() > h/2 ? 20 : -20;
        ann = "";
        ann += bid.inl; ann += "/"; ann += bid.crl;
        dt.drawText( cpt[idx].x(), cpt[idx].y()+spacing, ann,
            Alignment::Middle );
        double xcoord = double( int( mapcnr[idx].x*10 + .5 ) ) / 10;
        double ycoord = double( int( mapcnr[idx].y*10 + .5 ) ) / 10;
        ann = "";
        ann += "("; ann += xcoord; ann += ",";
        ann += ycoord; ann += ")";
        dt.drawText( cpt[idx].x(), cpt[idx].y()+1.5*spacing, ann,
            Alignment::Middle );
    }

    dt.endDraw();
}

    SurveyInfo*		survinfo;
    uiCanvas*		mapcanvas;

};


class uiSurveyMapDlg : public uiDialog
{
public:

uiSurveyMapDlg( uiParent* p )
	: uiDialog(p,"Survey Map")
{
    cv = new uiCanvas( this, "survey map" );
    cv->setPrefHeight( 400 );
    cv->setPrefWidth( 400 );
    cv->setStretch(0,0);
    cv->preDraw.notify( mCB(this,uiSurveyMapDlg,doCanvas) );
}

void doCanvas( CallBacker* )
{
    SurveyInfo* survinfo = new SurveyInfo( SI() );
    uiSurveyMap* survmap = new uiSurveyMap( cv, survinfo );
    survmap->drawMap( survinfo );
}

uiCanvas*		cv;

};		
