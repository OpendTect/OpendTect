/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldahdisplay.cc";


#include "uiwelldahdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "coltabsequence.h"
#include "dataclipper.h"
#include "math.h"
#include "mouseevent.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#include <iostream>


uiWellDahDisplay::DahObjData::DahObjData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellDahDisplay::Setup& s )
    : dahobj_(0)
    , zoverlayval_(2)		  
    , xax_(&scn,uiAxisHandler::Setup( isfirst? uiRect::Top : uiRect::Bottom )
				.border(s.border_)
				.annotinside(s.annotinside_)
				.noannot(s.noxannot_))
    , yax_(&scn,uiAxisHandler::Setup( isfirst? uiRect::Left : uiRect::Right )
				.border(s.border_)
				.annotinside(s.annotinside_)
				.noannot(s.noyannot_))
    , xrev_(false)
    , zrg_(mUdf(float),0)
    , cliprate_(0.05)
    , valrg_(mUdf(float),0)
    , col_(Color::Black())
    , drawascurve_(true)
    , drawaspoints_(false)
{
    if ( !isfirst )
	yax_.setup().nogridline(true);
}


void uiWellDahDisplay::DahObjData::getInfoForDah( float dah, 
						BufferString& msg ) const
{
    if ( !dahobj_ )
	return;

    msg += dahobj_->name();

    const int idx = dahobj_->indexOf( dah );
    if ( idx < 0 ) return;
    msg += ":";
    msg += toString( dahobj_->value( idx ) );
}


uiWellDahDisplay::uiWellDahDisplay( uiParent* p, const Setup& su )
    : uiGraphicsView(p,"Well Dah display viewer")
    , setup_(su)
    , ld1_(new DahObjData(scene(),true,su))
    , ld2_(new DahObjData(scene(),false,su))
{
    disableScrollZoom();
    setStretch( 2, 2 );

    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    reSize.notify( mCB(this,uiWellDahDisplay,reSized) );
    postFinalise().notify( mCB(this,uiWellDahDisplay,init) );
}


uiWellDahDisplay::~uiWellDahDisplay()
{
    delete ld1_; delete ld2_;
}


void uiWellDahDisplay::gatherInfo()
{
    setAxisRelations();
    gatherDataInfo( true );
    gatherDataInfo( false );

    if ( !ld1_->dahobj_ && !ld2_->dahobj_ ) 
	ld1_->valrg_ = ld2_->valrg_ = Interval<float>(0,0);

    if ( mIsUdf(zdata_.zrg_.start) && ld1_->dahobj_ )
    {
	zdata_.zrg_ = ld1_->zrg_;
	if ( ld2_->dahobj_ )
	    zdata_.zrg_.include( ld2_->zrg_ );
    }
    setAxisRanges( true );
    setAxisRanges( false );

    ld1_->xax_.setup().epsaroundzero_ = 1e-5;
    ld1_->xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_->xax_.setup().epsaroundzero_ = 1e-5;
    ld2_->xax_.setup().maxnumberdigitsprecision_ = 3;

    ld1_->xax_.setup().nmcolor_ = ld1_->dahobj_ ? ld1_->col_
				: ld2_->dahobj_ ? ld2_->col_ : Color::Black();
    ld2_->xax_.setup().nmcolor_ = ld2_->dahobj_ ? ld2_->col_
				: ld1_->dahobj_ ? ld1_->col_ : Color::Black();

    BufferString axis1nm = ld1_->dahobj_ ? ld1_->dahobj_->name() 
			 : ld2_->dahobj_ ? ld2_->dahobj_->name() : 0; 
    BufferString axis2nm = ld2_->dahobj_ ? ld2_->dahobj_->name() 
			 : ld1_->dahobj_ ? ld1_->dahobj_->name() : 0; 
    ld1_->xax_.setName( axis1nm );
    ld2_->xax_.setName( axis2nm );
}


void uiWellDahDisplay::setAxisRelations()
{
    ld1_->xax_.setBegin( &ld1_->yax_ );
    ld1_->yax_.setBegin( &ld2_->xax_ );
    ld2_->xax_.setBegin( &ld1_->yax_ );
    ld2_->yax_.setBegin( &ld2_->xax_ );
    ld1_->xax_.setEnd( &ld2_->yax_ );
    ld1_->yax_.setEnd( &ld1_->xax_ );
    ld2_->xax_.setEnd( &ld2_->yax_ );
    ld2_->yax_.setEnd( &ld1_->xax_ );

    ld1_->xax_.setNewDevSize( width(), height() );
    ld1_->yax_.setNewDevSize( height(), width() );
    ld2_->xax_.setNewDevSize( width(), height() );
    ld2_->yax_.setNewDevSize( height(), width() );
}


void uiWellDahDisplay::gatherDataInfo( bool first )
{
    uiWellDahDisplay::DahObjData& ld = first ? *ld1_ : *ld2_;

    const int sz = ld.dahobj_ ? ld.dahobj_->size() : 0;
    if ( sz < 2 )
    {
	if ( !first )
	{
	    ld2_->zrg_ = ld1_->zrg_;
	    ld2_->valrg_ = ld1_->valrg_;
	}
	return;
    }

    if ( ld.cliprate_ || mIsUdf( ld.valrg_.start ) )
    {
	DataClipSampler dcs( sz );
	for ( int idx=0; idx<sz; idx++ )
	    dcs.add(  ld.dahobj_->value( idx ) );

	ld.valrg_ = dcs.getRange( ld.cliprate_ );
    }

    float startpos = ld.zrg_.start = ld.dahobj_->dah( 0 );
    float stoppos = ld.zrg_.stop = ld.dahobj_->dah( sz-1 );
    if ( zdata_.zistime_ && d2T() && d2T()->size() > 1  )
    {
	startpos = d2T()->getTime( startpos )*1000;
	stoppos = d2T()->getTime( stoppos )*1000;
    }
    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
    if ( zdata_.dispzinft_ && !zdata_.zistime_)
	ld.zrg_.scale( mToFeetFactor );
}


void uiWellDahDisplay::setAxisRanges( bool first )
{
    uiWellDahDisplay::DahObjData& ld = first ? *ld1_ : *ld2_;
    if ( !first && setup_.samexaxisrange_ ) return;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( setup_.symetricalxaxis_ )
    {
	const float max = mMAX(fabs(dispvalrg.start),fabs(dispvalrg.stop));
	dispvalrg.start = -max;
	dispvalrg.stop  =  max;
    }

    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
	ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zdata_.zrg_.stop, zdata_.zrg_.start );
    ld.yax_.setBounds( dispzrg );

    if ( first )
    {
    // Set default for 2nd
	ld2_->xax_.setBounds( dispvalrg );
	ld2_->yax_.setBounds( dispzrg );
    }
}


void uiWellDahDisplay::draw()
{
    setAxisRelations();
    if ( mIsUdf(zdata_.zrg_.start) ) return;

    ld1_->xax_.plotAxis(); ld1_->yax_.plotAxis();
    ld2_->xax_.plotAxis(); ld2_->yax_.plotAxis();

    drawMarkers();
    drawCurve( true );
    drawCurve( false );

    drawZPicks();
}


static const int cMaxNrDahSamples = 2000;
#define mGetLoopSize(nrsamp,step)\
    {\
	if ( nrsamp > cMaxNrDahSamples )\
	{\
	    step = (float)nrsamp/cMaxNrDahSamples;\
	    nrsamp = cMaxNrDahSamples;\
	}\
    }

void uiWellDahDisplay::drawCurve( bool first )
{
    uiWellDahDisplay::DahObjData& ld = first ? *ld1_ : *ld2_;
    deepErase( ld.curveitms_ ); ld.curvepolyitm_ = 0; 
    const int sz = ld.dahobj_ ? ld.dahobj_->size() : 0;
    if ( sz < 2 ) return;

    TypeSet<uiPoint> pts; 
    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( ld.dahobj_->dah( idx ) );
	float val = ld.dahobj_->value( idx );
	int xaxisval = mIsUdf(val) ? mUdf(int) : ld.xax_.getPix(val);
	pts += uiPoint( xaxisval, ld.yax_.getPix(zpos) );
    }
    if ( pts.isEmpty() )
	return;

    LineStyle ls(LineStyle::Solid); ls.color_ = ld.col_; 
    if ( ld.drawascurve_ )
    {
	uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem() );
	pli->setPolyLine( pts );
	pli->setPenStyle( ls );
	pli->setZValue( ld.zoverlayval_ );
	ld.curveitms_.add( pli );
	ld.curvepolyitm_ = pli;
    }
    if ( ld.drawaspoints_ )
    {
	for ( int idx=0; idx<pts.size(); idx++ )
	{
	    uiCircleItem* ci = scene().addItem(new uiCircleItem( pts[idx], 1) );
	    ld.curveitms_.add( ci );
	    ci->setPenStyle( ls );
	    ci->setZValue( ld.zoverlayval_+1 );
	}
    }

    if ( setup_.drawcurvenames_ )
    {
	Alignment al( Alignment::HCenter, first ? Alignment::Top 
						: Alignment::Bottom );
	uiTextItem* ti = scene().addItem(new uiTextItem(ld.dahobj_->name(),al));
	ti->setTextColor( ls.color_ );
	uiPoint txtpt;
	txtpt = first ? uiPoint( pts[0] ) : pts[pts.size()-1];
	ti->setPos( txtpt );
	ld.curveitms_.add( ti );
    }

    if ( first )
	ld.yax_.annotAtEnd( zdata_.zistime_ ? "(ms)" : 
			    zdata_.dispzinft_ ? "(ft)" : "(m)" );
}


#define mMrkrScale2DFac 1./(float)5
#define mDefHorLineX1X2Y() \
const int x1 = ld1_->xax_.getRelPosPix( 0 ); \
const int x2 = ld1_->xax_.getRelPosPix( 1 ); \
const int y = ld1_->yax_.getPix( zpos )

void uiWellDahDisplay::drawMarkers()
{
    deepErase( markerdraws_ );

    if ( !markers() ) return;

    for ( int idx=0; idx<markers()->size(); idx++ )
    {
	const Well::Marker& mrkr = *((*markers())[idx]);
	if ( !mrkdisp_.selmarkernms_.isPresent( mrkr.name() ) )
	    continue;

	const Color& col= mrkdisp_.issinglecol_? mrkdisp_.color_ : mrkr.color();
	const Color& nmcol = mrkdisp_.samenmcol_ ? col :  mrkdisp_.nmcol_;

	if ( col == Color::NoColor() || col == Color::White() )
	    continue;

	mDefZPosInLoop( mrkr.dah() );
	mDefHorLineX1X2Y();

	MarkerDraw* mrkdraw = new MarkerDraw( mrkr );
	markerdraws_ += mrkdraw;

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	const int shapeint = mrkdisp_.shapeint_;
	const int drawsize = (int)(mrkdisp_.size_*mMrkrScale2DFac);
	LineStyle ls = LineStyle( LineStyle::Dot, drawsize, col );
	if ( shapeint == 1 )
	    ls.type_ =  LineStyle::Solid;
	if ( shapeint == 2 )
	    ls.type_ = LineStyle::Dash;

	li->setPenStyle( ls );
	li->setZValue( 2 );
	mrkdraw->lineitm_ = li;
	mrkdraw->ls_ = ls;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
	new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( nmcol );
	mrkdraw->txtitm_ = ti;
    }
}


uiWellDahDisplay::MarkerDraw* uiWellDahDisplay::getMarkerDraw(
						const Well::Marker& mrk )
{
    for ( int idx=0; idx<markerdraws_.size(); idx++)
    {
	if ( &(markerdraws_[idx]->mrk_) == &mrk )
	    return markerdraws_[idx];
    }
    return 0;
}


void uiWellDahDisplay::drawZPicks()
{
    deepErase( zpickitms_ );
    for ( int idx=0; idx<zpicks_.size(); idx++ )
    {
	const PickData& pd = zpicks_[idx];
	mDefZPosInLoop( pd.dah_ );
	const float& val = pd.val_;
	uiGraphicsItem* li;
	if ( mIsUdf(val) )
	{
	    mDefHorLineX1X2Y();
	    li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	}
	else
	{
	    int xpos = ld1_->xax_.getPix(val);
	    int pos = ld1_->yax_.getPix(zpos);
	    li = scene().addItem( new uiCircleItem( uiPoint(xpos,pos), 1 ) );
	}

	Color lcol( setup_.pickls_.color_ );
	if ( pd.color_ != Color::NoColor() )
	    lcol = pd.color_;
	li->setPenStyle( LineStyle(setup_.pickls_.type_,setup_.pickls_.width_,
			lcol) );
	li->setZValue( 2 );
	zpickitms_.add( li );
    }
}


uiWellDahDisplay::MarkerDraw::~MarkerDraw()
{
    delete txtitm_; delete lineitm_; 
}



void uiWellDahDisplay::init( CallBacker* )
{
    dataChanged();
    show();
}


void uiWellDahDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiWellDahDisplay::reSized( CallBacker* )
{
    draw();
}
