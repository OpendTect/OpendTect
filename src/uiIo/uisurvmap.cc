/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uisurvmap.h"

#include "uifont.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uistrings.h"

#include "angles.h"
#include "draw.h"
#include "hiddenparam.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


static HiddenParam<uiSurveyBoxObject,char> asworkarea('0');

uiSurveyBoxObject::uiSurveyBoxObject( BaseMapObject* bmo )
    : uiBaseMapObject(bmo)
    , ls_(OD::LineStyle::Solid,3,Color::Red())
    , showlabels_(true)
{
    for ( int idx=0; idx<4; idx++ )
    {
	uiMarkerItem* markeritem = new uiMarkerItem( MarkerStyle2D::Square );
	graphitem_.addChild( markeritem );
	vertices_ += markeritem;
    }

    frame_ = new uiPolygonItem;
    frame_->setPenColor( ls_.color_ );
    frame_->setPenStyle( ls_ );
    graphitem_.addChild( frame_ );

    const mDeclAlignment( postxtalign, HCenter, VCenter );
    for ( int idx=0; idx<4; idx++ )
    {
	uiTextItem* textitem = new uiTextItem();
	textitem->setTextColor( Color::Black() );
	textitem->setAlignment( postxtalign );
	textitem->setFont( FontList().get(FontData::Graphics2DSmall) );
	textitem->setItemIgnoresTransformations( true );
	graphitem_.addChild( textitem );
	labels_ += textitem;
    }

    graphitem_.setZValue( -1 );

    asworkarea.setParam( this, false );
}


uiSurveyBoxObject::~uiSurveyBoxObject()
{
    asworkarea.removeParam( this );
}


void uiSurveyBoxObject::setAsWorkArea( bool yn )
{
    asworkarea.setParam( this, yn );
}


bool uiSurveyBoxObject::asWorkArea() const
{
    return asworkarea.getParam( this );
}


void uiSurveyBoxObject::showLabels( bool yn )
{
    showlabels_ = yn;
    for ( int idx=0; idx<labels_.size(); idx++ )
	labels_[idx]->setVisible( yn );
}


bool uiSurveyBoxObject::labelsShown() const
{ return !labels_.isEmpty() && labels_[0]->isVisible(); }


void uiSurveyBoxObject::setLineStyle( const OD::LineStyle& ls )
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
    graphItem().setVisible( yn );
}


void uiSurveyBoxObject::update()
{
    if ( !survinfo_ )
	{ setVisibility( false ); return; }

    const SurveyInfo& si = *survinfo_;
    const TrcKeyZSampling& cs = si.sampling( asWorkArea() );
    TypeSet<uiWorldPoint> mapcnr; mapcnr.setSize( 4 );
    mapcnr[0] = si.transform( cs.hsamp_.start_ );
    mapcnr[1] = si.transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()) );
    mapcnr[2] = si.transform( cs.hsamp_.stop_ );
    mapcnr[3] = si.transform(
	BinID(cs.hsamp_.stop_.inl(),cs.hsamp_.start_.crl()) );

    for ( int idx=0; idx<vertices_.size(); idx++ )
	vertices_[idx]->setPos( mapcnr[idx] );

    frame_->setPolygon( mapcnr );

    for ( int idx=0; idx<labels_.size(); idx++ )
    {
	const int oppidx = idx < 2 ? idx + 2 : idx - 2;
	const bool bot = mapcnr[idx].y < mapcnr[oppidx].y;
	const BinID bid = si.transform( mapcnr[idx] );
	const BufferString bidstr = bid.toString();
	Alignment al( Alignment::HCenter,
		      bot ? Alignment::Top : Alignment::Bottom );
	labels_[idx]->setPos( mapcnr[idx] );
	labels_[idx]->setText( toUiString(bidstr.buf()) );
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
    graphitem_.addChild( arrow_ );

    if ( !withangle )
	return;

    angleline_ = new uiLineItem;
    angleline_->setPenStyle(
		OD::LineStyle(OD::LineStyle::Dot,2,Color(255,0,0)) );
    graphitem_.addChild( angleline_ );

    mDeclAlignment( txtalign, Right, Bottom );
    anglelabel_ = new uiTextItem();
    anglelabel_->setAlignment( txtalign );
    graphitem_.addChild( anglelabel_ );
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
    graphItem().setVisible( yn );
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
    uiString angtxt;
    if ( iusrang )
    {
	angtxt = toUiString(iusrang / 100);
	iusrang = iusrang % 100;
	if ( iusrang )
	{
	    angtxt = toUiString("%1.%2").arg(angtxt).arg(iusrang/10);
	    iusrang = iusrang % 10;
	    if ( iusrang )
		angtxt = toUiString("%1 %2").arg(angtxt).arg(iusrang);
	}
    }

    anglelabel_->setPos( mCast(float,lastx), mCast(float,yarrowtop) );
    anglelabel_->setText( angtxt );
    setVisibility( true );
}


// uiSurveyMap
uiSurveyMap::uiSurveyMap( uiParent* p, bool withtitle )
    : uiBaseMap(p)
    , survbox_(0)
    , survinfo_(0)
    , title_(0)
{
    view_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    view_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    survbox_ = new uiSurveyBoxObject( 0 );
    addObject( survbox_ );

    const mDeclAlignment( txtalign, Left, Top );
    if ( withtitle )
    {
	title_ = view_.scene().addItem(
		new uiTextItem(uiPoint(10,10),mJoinUiStrs(sSurvey(),sName()),
								    txtalign) );
	title_->setPenColor( Color::Black() );
	title_->setFont( FontList().get(FontData::Graphics2DLarge) );
    }

    centerWorldItem( true );
    setSurveyInfo( survinfo_ );
}


uiSurveyBoxObject* uiSurveyMap::getSurveyBox() const
{
    return survbox_;
}


SurveyInfo* uiSurveyMap::getEmptySurvInfo()
{
    return new SurveyInfo;
}


void uiSurveyMap::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;

    if ( survbox_ )
	survbox_->setSurveyInfo( survinfo_ );

    if ( title_ )
	title_->setVisible( survinfo_ );

    if ( survinfo_ )
    {
	const Coord mincoord = survinfo_->minCoord( false );
	const Coord maxcoord = survinfo_->maxCoord( false );
	const double diffx = maxcoord.x - mincoord.x;
	const double diffy = maxcoord.y - mincoord.y;
	const uiWorldRect wr( mincoord.x-diffx/4, maxcoord.y+diffy/4,
			      maxcoord.x+diffx/4, mincoord.y-diffy/4 );
	if ( title_ ) title_->setText( toUiString(survinfo_->name()) );
	setView( wr );
    }

    if ( survbox_ )
	survbox_->setVisibility( survinfo_ );
}


void uiSurveyMap::reDraw( bool deep )
{
    uiBaseMap::reDraw( deep );
}
