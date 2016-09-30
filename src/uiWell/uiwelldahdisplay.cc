/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiwelldahdisplay.cc";


#include "uiwelldahdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "coltabsequence.h"
#include "dataclipper.h"
#include "mouseevent.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "wellman.h"
#include "refcount.h"


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
    , zrg_(mUdf(float),mUdf(float))
    , cliprate_(0)
    , valrg_(mUdf(float),mUdf(float))
    , col_(Color::Black())
    , pointsz_(5)
    , curvesz_(1)
    , drawascurve_(true)
    , drawaspoints_(false)
    , xaxprcts_(0)
{
    if ( !isfirst )
	yax_.setup().nogridline(true);
    if ( s.xannotinpercents_ )
    {
	xaxprcts_ = new uiAxisHandler( &scn, uiAxisHandler::Setup(
				    isfirst? uiRect::Top : uiRect::Bottom )
				    .border(s.border_)
				    .annotinside(s.annotinside_)
				    .noannot(s.noxannot_)
				    .noaxisannot(true));
	xaxprcts_->setBounds( StepInterval<float>(0,100,25) );
	xax_.setup().noannot( true );
    }
}


uiWellDahDisplay::DahObjData::~DahObjData()
{
    RefCount::unRefIfObjIsReffed( dahobj_ );
    delete xaxprcts_;
}


void uiWellDahDisplay::DahObjData::setData( const Well::DahObj* dahobj )
{
    if ( dahobj != dahobj_ )
    {
	RefCount::unRefIfObjIsReffed( dahobj_ );
	dahobj_ = dahobj;
	RefCount::refIfObjIsReffed( dahobj_ );
    }
}


void uiWellDahDisplay::DahObjData::plotAxis()
{
    xax_.updateScene(); yax_.updateScene();
    if ( xaxprcts_ )
	xaxprcts_->updateScene();
}


void uiWellDahDisplay::DahObjData::getInfoForDah( float dah,
						BufferString& msg ) const
{
    if ( !dahobj_ )
	return;
    const float val = dahobj_->valueAt( dah );
    if ( mIsUdf(val) )
	return;

    msg.add( dahobj_->name() ).add( ":" ).add( val );
}


uiWellDahDisplay::uiWellDahDisplay( uiParent* p, const Setup& su )
    : uiGraphicsView(p,"Well Dah display viewer")
    , setup_(su)
    , ld1_(new DahObjData(scene(),true,su))
    , ld2_(new DahObjData(scene(),false,su))
    , zdata_(0)
{
    disableScrollZoom();
    setStretch( 2, 2 );
    reSize.notify( mCB(this,uiWellDahDisplay,reSized) );
    postFinalise().notify( mCB(this,uiWellDahDisplay,init) );
}


uiWellDahDisplay::~uiWellDahDisplay()
{
    detachAllNotifiers();
    delete ld1_; delete ld2_;
    deepErase( markerdraws_ );
}


void uiWellDahDisplay::setData( const Data& data )
{
    zdata_.copyFrom( data );
    dataChanged();
}


void uiWellDahDisplay::gatherInfo()
{
    setAxisRelations();
    gatherDataInfo( true );
    gatherDataInfo( false );

    if ( !ld1_->dahobj_ && !ld2_->dahobj_ )
	ld1_->valrg_ = ld2_->valrg_ = Interval<float>(mUdf(float),mUdf(float));

    setAxisRanges( true );
    setAxisRanges( false );

    ld1_->xax_.setup().maxnrchars_ = 8;
    ld2_->xax_.setup().maxnrchars_ = 8;
    ld1_->xax_.setup().nmcolor_ = ld1_->dahobj_ ? ld1_->col_
				: ld2_->dahobj_ ? ld2_->col_ : Color::Black();
    ld2_->xax_.setup().nmcolor_ = ld2_->dahobj_ ? ld2_->col_
				: ld1_->dahobj_ ? ld1_->col_ : Color::Black();

    BufferString axis1nm = ld1_->dahobj_ ? ld1_->dahobj_->name().str()
			 : ld2_->dahobj_ ? ld2_->dahobj_->name().str() : 0;
    BufferString axis2nm = ld2_->dahobj_ ? ld2_->dahobj_->name().str()
			 : ld1_->dahobj_ ? ld1_->dahobj_->name().str() : 0;
    ld1_->xax_.setCaption( toUiString(axis1nm) );
    ld2_->xax_.setCaption( toUiString(axis2nm) );
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

    if ( ld2_->xaxprcts_ )
    {
	ld2_->xaxprcts_->setBegin( &ld1_->yax_ );
	ld2_->xaxprcts_->setEnd( &ld2_->yax_ );
	ld2_->xaxprcts_->setNewDevSize( width(), height() );
    }
    if ( ld1_->xaxprcts_ )
    {
	ld1_->xaxprcts_->setBegin( &ld1_->yax_ );
	ld1_->xaxprcts_->setEnd( &ld2_->yax_ );
	ld1_->xaxprcts_->setNewDevSize( width(), height() );
    }
}


void uiWellDahDisplay::gatherDataInfo( bool first )
{
    uiWellDahDisplay::DahObjData& ld = first ? *ld1_ : *ld2_;
    if ( !ld.dahobj_ )
	return;
    const Well::DahObj& dahobj = *ld.dahobj_;
    MonitorLock ml( dahobj );
    const int sz = dahobj.size();
    if ( sz < 2 )
	return;

    if ( mIsUdf( ld.valrg_.start ) )
    {
	DataClipSampler dcs( sz );
	for ( int idx=0; idx<sz; idx++ )
	    dcs.add( dahobj.valueByIdx( idx ) );
	ld.valrg_ = dcs.getRange( ld.cliprate_ );
    }

    float startpos = ld.zrg_.start = dahobj.firstDah();
    float stoppos = ld.zrg_.stop = dahobj.lastDah();
    if ( zdata_.zistime_ && d2T() && d2T()->size() > 1 && track() )
    {
	startpos = d2T()->getTime( startpos, *track() ) * 1000;
	stoppos = d2T()->getTime( stoppos, *track() ) * 1000;
    }
    else if ( !zdata_.zistime_ && track() )
    {
	startpos = (float) track()->getPos( startpos ).z_;
	stoppos = (float) track()->getPos( stoppos ).z_;
    }
    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
}


void uiWellDahDisplay::setAxisRanges( bool first )
{
    uiWellDahDisplay::DahObjData& ld = first ? *ld1_ : *ld2_;
    uiWellDahDisplay::DahObjData& otherld = first ? *ld2_ : *ld1_;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( mIsUdf( dispvalrg.start ) )
	dispvalrg = otherld.valrg_ ;

    if ( setup_.samexaxisrange_ && !mIsUdf( otherld.valrg_.start ) )
	dispvalrg.include( otherld.valrg_ );

    if ( setup_.symetricalxaxis_ )
    {
	const float max = mMAX(fabs(dispvalrg.start),fabs(dispvalrg.stop));
	dispvalrg.start = -max;
	dispvalrg.stop  =  max;
    }

    if ( ld.xrev_ )
	Swap( dispvalrg.start, dispvalrg.stop );

    ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zdata_.zrg_.stop, zdata_.zrg_.start );
    if ( mIsUdf(zdata_.zrg_.start) )
    {
	dispzrg = ld1_->zrg_;
	if ( mIsUdf( dispzrg.start ) )
	    dispzrg = ld2_->zrg_;
	if ( !mIsUdf( ld2_->zrg_.start ) )
	    dispzrg.include( ld2_->zrg_ );
    }
    if ( dispzrg.start < dispzrg.stop )
	dispzrg.sort( false );

    ld.yax_.setBounds( dispzrg );
}


void uiWellDahDisplay::draw()
{
    setAxisRelations();

    ld1_->plotAxis();
    ld2_->plotAxis();

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
    if ( !ld.dahobj_ )
	return;
    const Well::DahObj& dahobj = *ld.dahobj_;
    MonitorLock ml( dahobj );
    const int sz = dahobj.size();
    if ( sz < 2 )
	return;

    TypeSet<uiPoint> pts;
    pts.setCapacity( sz, false );
    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( dahobj.dahByIdx( idx ) );
	float val = dahobj.valueByIdx( idx );
	int xaxisval = mIsUdf(val) ? mUdf(int) : ld.xax_.getPix(val);
	pts += uiPoint( xaxisval, ld.yax_.getPix(zpos) );
    }
    if ( pts.isEmpty() )
	return;
    ml.unlockNow();

    OD::LineStyle ls(OD::LineStyle::Solid); ls.color_ = ld.col_;
    if ( ld.drawascurve_ )
    {
	TypeSet<uiPoint> ptsforspikes;
	const bool isreflectivity =
	    dahobj.name().isEqual( "reflectivity", CaseInsensitive );
	if ( isreflectivity )
	{
	    ptsforspikes.setCapacity( 3 * sz, false );
	    for ( int idx=0; idx<pts.size(); idx++ )
	    {
		const uiPoint extrapt =
			uiPoint( ld.xax_.getPix(0), pts[idx].y_ );
		ptsforspikes += extrapt;
		ptsforspikes += pts[idx];
		ptsforspikes += extrapt;
	    }
	}

	uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem() );
	pli->setPolyLine( !isreflectivity ? pts : ptsforspikes );
	ls.width_ = ld.curvesz_;
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
	    ls.width_ = ld.pointsz_;
	    ci->setPenStyle( ls );
	    ci->setZValue( ld.zoverlayval_+1 );
	}
    }

    if ( setup_.drawcurvenames_ )
    {
	OD::Alignment al( OD::Alignment::HCenter, first ? OD::Alignment::Top
						: OD::Alignment::Bottom );
	uiTextItem* ti = scene().addItem(new uiTextItem(toUiString(
						       dahobj.name()),al));
	ti->setTextColor( ls.color_ );
	uiPoint txtpt;
	txtpt = first ? uiPoint( pts[0] ) : pts[pts.size()-1];
	ti->setPos( txtpt );
	ld.curveitms_.add( ti );
    }
}


#define mDefHorLineX1X2Y() \
const int x1 = ld1_->xax_.getRelPosPix( 0 ); \
const int x2 = ld1_->xax_.getRelPosPix( 1 ); \
const int y = ld1_->yax_.getPix( zpos )

void uiWellDahDisplay::drawMarkers()
{
    deepErase( markerdraws_ );
    if ( !markers() ) return;

    const BufferStringSet selmrkrnms( mrkdisp_.selMarkerNames() );
    const Well::MarkerSet& mrkrs = *markers();
    Well::MarkerSetIter miter( mrkrs );
    while( miter.next() )
    {
	const Well::Marker& mrkr = miter.get();
	if ( !selmrkrnms.isPresent( mrkr.name() ) )
	    continue;

	const Color col= mrkdisp_.singleColor() ? mrkdisp_.color()
						: mrkr.color();
	const Color nmcol = mrkdisp_.sameNameCol() ? col
						   : mrkdisp_.nameColor();

	if ( col == Color::NoColor() || col == Color::White() )
	    continue;

	mDefZPosInLoop( mrkr.dah() );
	mDefHorLineX1X2Y();

	MarkerDraw* mrkdraw = new MarkerDraw( miter.get() );
	markerdraws_ += mrkdraw;

	uiPoint p1( x1, y ), p2( x2, y );
	uiLineItem* li = scene().addItem( new uiLineItem(p1,p2) );

	const int shapeint = mrkdisp_.shapeType();
	const int drawsize = mrkdisp_.size();
	OD::LineStyle ls = OD::LineStyle( OD::LineStyle::Dot, drawsize, col );
	if ( shapeint == 1 )
	    ls.type_ =  OD::LineStyle::Solid;
	if ( shapeint == 2 )
	    ls.type_ = OD::LineStyle::Dash;

	li->setPenStyle( ls );
	li->setZValue( 2 );
	li->setAcceptHoverEvents( true );

	mrkdraw->lineitm_ = li;
	mrkdraw->ls_ = ls;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
	    new uiTextItem(toUiString(mtxt),mAlignment(Right,VCenter) ));
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( nmcol );
	ti->setAcceptHoverEvents( true );
	mrkdraw->txtitm_ = ti;
    }
}


uiWellDahDisplay::MarkerDraw* uiWellDahDisplay::getMarkerDraw(
					const Well::Marker& mrkr )
{
    for ( int idx=0; idx<markerdraws_.size(); idx++ )
    {
	if ( (markerdraws_[idx]->mrkr_) == mrkr )
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
	    li = scene().addItem( new uiLineItem(x1,y,x2,y) );
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
	li->setPenStyle(
		    OD::LineStyle(setup_.pickls_.type_,setup_.pickls_.width_,
			lcol) );
	li->setZValue( 2 );
	zpickitms_.add( li );
    }
}


bool uiWellDahDisplay::MarkerDraw::contains( const Geom::Point2D<int>& pt )const
{
    const bool contns = lineitm_->boundingRect().contains( pt )
		    || txtitm_->boundingRect().contains( pt );
    return contns;
}


void uiWellDahDisplay::MarkerDraw::highlight()
{
    lineitm_->highlight();
}


void uiWellDahDisplay::MarkerDraw::unHighlight()
{
    lineitm_->unHighlight();
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
