/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurvmap.cc,v 1.29 2009-10-16 10:58:27 cvsbert Exp $";

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
#include "angles.h"


uiSurveyMap::uiSurveyMap( uiParent* parent )
    : uiGraphicsView( parent, "Survey map view" )
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


void uiSurveyMap::drawMap( const SurveyInfo* survinfo )
{
    scene().removeAllItems();

    if ( survinfo->sampling(false).hrg.totalNr() < 2 )
    	return;

    drawAngleN( survinfo->computeAngleXInl() );

    const mDeclAlignment( txtalign, Left, Top );

    uiTextItem* textitem = scene().addItem(
	    new uiTextItem(uiPoint(10,10),survinfo->name(),txtalign) );
    textitem->setPenColor( Color::Black() );
    textitem->setFont( FontList().get(FontData::key(FontData::GraphicsLarge)));

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
    uiWorld2Ui w2ui( sz, wr );
    uiPoint cpt[4];
    for ( int idx=0; idx<4; idx++ )
    {
        cpt[idx] = w2ui.transform( uiWorldPoint(mapcnr[idx].x, mapcnr[idx].y) );
        uiMarkerItem* markeritem = scene().addItem(
	    new uiMarkerItem(cpt[idx],MarkerStyle2D::Square) );
	markeritem->setZValue( 1 );
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    for ( int idx=0; idx<4; idx++ )
    {
	uiLineItem* lineitm =
	    new uiLineItem( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0], true );
	scene().addItem( lineitm );
	lineitm->setPenStyle( ls );
    }

    bool printxy = false;
    for ( int idx=0; idx<4; idx++ )
    {
	const bool bot = cpt[idx].y > height()/2;
        BinID bid = survinfo->transform( mapcnr[idx] );
        const int spacing =  bot ? 10 : -10;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
	uiPoint txtpos( cpt[idx].x, cpt[idx].y+spacing );
        uiTextItem* textitm1 = scene().addItem(
		new uiTextItem(txtpos,annot.buf(),txtalign) );
	textitm1->setPenColor( Color::Black() );
	textitm1->setFont(
		FontList().get(FontData::key(FontData::GraphicsSmall)) );
	textitm1->setZValue( 1 );

	if ( printxy )
	{
	    double xcoord = double( int( mapcnr[idx].x*10 + .5 ) ) / 10;
	    double ycoord = double( int( mapcnr[idx].y*10 + .5 ) ) / 10;
	    annot = "("; annot += xcoord; annot += ",";
	    annot += ycoord; annot += ")";
	    uiPoint txtpos( cpt[idx].x, mNINT(cpt[idx].y+1.5*spacing) );
	    uiTextItem* textitm2 = scene().addItem(
		new uiTextItem(txtpos,annot.buf(),txtalign) );
	    textitm2->setPenColor( Color::Black() );
	    textitm2->setFont(
		    FontList().get(FontData::key(FontData::GraphicsSmall)) );
	}
    }

    setViewArea( 0, 0, scene().width(), scene().height() );
}


void uiSurveyMap::drawAngleN( float mathang )
{
    static const float halfpi = M_PI * .5;
    static const float quartpi = M_PI * .25;

	    // To [0,pi]
    if ( mathang < 0 )			mathang += M_PI;
    if ( mathang > M_PI )		mathang -= M_PI;
	    // Find angle closest to N, not necessarily X vs inline
    if ( mathang < quartpi )		mathang += halfpi;
    if ( mathang > halfpi+quartpi )	mathang -= halfpi;

    float usrang = Angle::rad2usrdeg( mathang );
    if ( usrang > 180 ) usrang = 360 - usrang;
    if ( mIsEqual(usrang,0,0.01) ) return;

    const bool northisleft = mathang < halfpi;
    const int arrowlen = 30;
    const int sideoffs = 10;
    const int yarrowtop = 20;

    float dx = arrowlen * tan( halfpi-mathang );
    const int dxpix = mNINT( dx );
    const int lastx = width() - 1 - sideoffs;
    const uiPoint origin( lastx - (northisleft?dxpix:0),
	    		  arrowlen + yarrowtop );
    const uiPoint arrowtop( origin.x, yarrowtop );

    ArrowStyle arrowstyle( 3, ArrowStyle::HeadOnly );
    arrowstyle.linestyle_.width_ = 3;
    uiArrowItem* northarrow = new uiArrowItem( origin, arrowtop, arrowstyle );
    scene().addItem( northarrow );

    uiLineItem* li = new uiLineItem( origin, uiPoint(origin.x+dxpix,yarrowtop),
	    			     true );
    li->setPenStyle( LineStyle(LineStyle::Dot,2,Color(255,0,0)) );
    scene().addItem( li );

    float usrang100 = usrang * 100;
    if ( usrang100 < 0 ) usrang100 = -usrang100;
    int iusrang = (int)(usrang100 + .5);
    BufferString angtxt; angtxt += iusrang / 100;
    iusrang = iusrang % 100;
    if ( iusrang )
    {
	angtxt += ".";
	angtxt += iusrang / 10; iusrang = iusrang % 10;
	if ( iusrang )
	    angtxt += iusrang;
    }
    mDeclAlignment( txtalign, Right, Bottom );
    uiTextItem* ti = scene().addItem(
	    new uiTextItem(uiPoint(lastx,yarrowtop),angtxt,txtalign) );
}
