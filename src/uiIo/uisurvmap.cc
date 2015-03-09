/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvmap.h"

#include "uifont.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uipixmap.h"
#include "uiworld2ui.h"

#include "angles.h"
#include "draw.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


uiSurveyBoxObject::uiSurveyBoxObject( BaseMapObject* bmo )
    : uiBaseMapObject(bmo)
    , ls_(LineStyle::Solid,3,Color::Red())
    , showlabels_(true)
{
    for ( int idx=0; idx<4; idx++ )
    {
        uiMarkerItem* markeritem = new uiMarkerItem( MarkerStyle2D::Square );
	itemgrp_.add( markeritem );
	vertices_ += markeritem;
    }

    frame_ = new uiPolygonItem;
    frame_->setPenColor( ls_.color_ );
    frame_->setPenStyle( ls_ );
    itemgrp_.add( frame_ );

    const mDeclAlignment( postxtalign, HCenter, VCenter );
    for ( int idx=0; idx<4; idx++ )
    {
        uiTextItem* textitem = new uiTextItem();
	textitem->setTextColor( Color::Black() );
	textitem->setAlignment( postxtalign );
	textitem->setFont( FontList().get(FontData::Graphics2DSmall) );
	itemgrp_.add( textitem );
	labels_ += textitem;
    }

    itemgrp_.setZValue( 2 );
}


void uiSurveyBoxObject::showLabels( bool yn )
{
    showlabels_ = yn;
    for ( int idx=0; idx<labels_.size(); idx++ )
	labels_[idx]->setVisible( yn );
}


bool uiSurveyBoxObject::labelsShown() const
{ return !labels_.isEmpty() && labels_[0]->isVisible(); }


void uiSurveyBoxObject::setLineStyle( const LineStyle& ls )
{
    ls_ = ls;
    frame_->setPenColor( ls.color_ );
    frame_->setPenStyle( ls );
}


void uiSurveyBoxObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiSurveyBoxObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiSurveyBoxObject::update()
{
    if ( !survinfo_ )
	{ setVisibility( false ); return; }

    const SurveyInfo& si = *survinfo_;
    const TrcKeyZSampling& cs = si.sampling( false );
    TypeSet<uiWorldPoint> mapcnr; mapcnr.setSize( 4 );
    mapcnr[0] = si.transform( cs.hrg.start );
    mapcnr[1] = si.transform( BinID(cs.hrg.start.inl(),cs.hrg.stop.crl()) );
    mapcnr[2] = si.transform( cs.hrg.stop );
    mapcnr[3] = si.transform( BinID(cs.hrg.stop.inl(),cs.hrg.start.crl()) );

    for ( int idx=0; idx<vertices_.size(); idx++ )
	vertices_[idx]->setPos( mapcnr[idx] );

    frame_->setPolygon( mapcnr );

    for ( int idx=0; idx<labels_.size(); idx++ )
    {
	const int oppidx = idx < 2 ? idx + 2 : idx - 2;
	const bool bot = mapcnr[idx].y > mapcnr[oppidx].y;
        BinID bid = si.transform( mapcnr[idx] );
	Alignment al( Alignment::HCenter,
		      bot ? Alignment::Top : Alignment::Bottom );
	labels_[idx]->setPos( mapcnr[idx] );
	labels_[idx]->setText( bid.toString() );
	labels_[idx]->setAlignment( al );
	labels_[idx]->setVisible( showlabels_ );
    }
}


// uiNorthArrowObject
uiNorthArrowObject::uiNorthArrowObject( BaseMapObject* bmo, bool withangle )
    : uiBaseMapObject(bmo)
    , angleline_(0), anglelabel_(0)
{
    ArrowStyle arrowstyle( 3, ArrowStyle::HeadOnly );
    arrowstyle.linestyle_.width_ = 3;
    arrow_ = new uiArrowItem;
    arrow_->setArrowStyle( arrowstyle );
    itemgrp_.add( arrow_ );

    if ( !withangle )
	return;

    angleline_ = new uiLineItem;
    angleline_->setPenStyle( LineStyle(LineStyle::Dot,2,Color(255,0,0)) );
    itemgrp_.add( angleline_ );

    mDeclAlignment( txtalign, Right, Bottom );
    anglelabel_ = new uiTextItem();
    anglelabel_->setAlignment( txtalign );
    itemgrp_.add( anglelabel_ );
}


void uiNorthArrowObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiNorthArrowObject::setPixelPos(int x,int y)
{
    uistartposition_.setXY( x, y );
}


void uiNorthArrowObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiNorthArrowObject::update()
{
    if ( !survinfo_ )
	{ setVisibility( false ); return; }

    float mathang = survinfo_->angleXInl();
    if ( mIsUdf(mathang) ) return;

	    // To [0,pi]
    if ( mathang < 0 )			mathang += M_PIf;
    if ( mathang > M_PIf )		mathang -= M_PIf;
	    // Find angle closest to N, not necessarily X vs inline
    if ( mathang < M_PI_4f )		mathang += M_PI_2f;
    if ( mathang > M_PI_2f+M_PI_4f )	mathang -= M_PI_2f;

    float usrang = Angle::rad2usrdeg( mathang );
    if ( usrang > 180 ) usrang = 360 - usrang;

    const bool northisleft = mathang < M_PI_2f;
    const int arrowlen = 30;
    const int sideoffs = 80;
    const int yarrowtop = 20;

    float dx = arrowlen * tan( M_PI_2f-mathang );
    const int dxpix = mNINT32( dx );

    const int xmax = uistartposition_.x;
    const int lastx = xmax - 1 - sideoffs;
    const uiPoint origin( lastx - (northisleft?dxpix:0), arrowlen + yarrowtop );
    const uiPoint arrowtop( origin.x, yarrowtop );

    arrow_->setTailHeadPos( origin, arrowtop );
    if ( !angleline_ || !anglelabel_ )
	return;

    angleline_->setLine( origin, uiPoint(origin.x+dxpix,yarrowtop) );
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

    anglelabel_->setPos( mCast(float,lastx), mCast(float,yarrowtop) );
    anglelabel_->setText( angtxt );
    setVisibility( true );
}


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


// uiSurveyMap
uiSurveyMap::uiSurveyMap( uiParent* p, bool withtitle,
			  bool withnortharrow, bool withmapscale )
    : uiBaseMap(p)
    , survbox_(0)
    , northarrow_(0)
    , mapscale_(0)
    , survinfo_(0)
    , title_(0)
{
    view_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    view_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    const mDeclAlignment( txtalign, Left, Top );

    survbox_ = new uiSurveyBoxObject( 0 );
    addObject( survbox_ );

    if ( withnortharrow )
    {
	const uiPixmap pm( "northarrow" );
	northarrow_ = view().scene().addItem( new uiPixmapItem(pm) );
	northarrow_->setScale( 0.5, 0.5 );
    }

    if ( withmapscale )
    {
	mapscale_ = new uiMapScaleObject( 0 );
	addObject( mapscale_ );
    }

    if ( withtitle )
    {
	title_ = view_.scene().addItem(
		new uiTextItem(uiPoint(10,10),"Survey name",txtalign) );
	title_->setPenColor( Color::Black() );
	title_->setFont( FontList().get(FontData::Graphics2DLarge) );
    }

    setSurveyInfo( survinfo_ );
}


uiMapScaleObject* uiSurveyMap::getMapScale()	const	{ return mapscale_; }
uiGraphicsItem* uiSurveyMap::getNorthArrow() const	{ return northarrow_; }
uiSurveyBoxObject* uiSurveyMap::getSurveyBox() const	{ return survbox_; }


void uiSurveyMap::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;

    const int width = view().width();
    const int height = view().height();

    if ( survbox_ )
	survbox_->setSurveyInfo( survinfo_ );

    if ( northarrow_ )
	northarrow_->setPos( 10, 10 );

    if ( mapscale_ )
    {
	mapscale_->setSurveyInfo( survinfo_ );
	mapscale_->setPixelPos( width, height );
    }

    if ( title_ )
	title_->setVisible( survinfo_ );

    if ( survinfo_ )
    {
    /*
	uiBorder border( 20, title_ ? 70 : 20, 20, mapscale_ ? 70 : 20 );
	uiSize sz( (int)view_.scene().width(), (int)view_.scene().height() );
	uiRect rc = border.getRect( sz );
    */

	const Coord mincoord = survinfo_->minCoord( false );
	const Coord maxcoord = survinfo_->maxCoord( false );
	const double diffx = maxcoord.x - mincoord.x;
	const double diffy = maxcoord.y - mincoord.y;
	const uiWorldRect wr( mincoord.x-diffx/4, maxcoord.y+diffy/4,
			      maxcoord.x+diffx/4, mincoord.y-diffy/4 );
	if ( title_ ) title_->setText( survinfo_->name().buf() );
	setView( wr );
    }
}


void uiSurveyMap::reDraw( bool deep )
{
    uiBaseMap::reDraw( deep );
}
