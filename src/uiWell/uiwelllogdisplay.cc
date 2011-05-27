/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc";



#include "uiwelllogdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "coltabsequence.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#include <iostream>


#define mDefZPos(zpos) \
    if ( zdata_.zistime_ && zdata_.d2tm_ )\
	zpos = zdata_.d2tm_->getTime( zpos )*1000;

#define mDefZPosInLoop(val) \
    float zpos = val;\
    mDefZPos(zpos)\
    if ( !zdata_.zrg_.includes( zpos ) )\
        continue;

uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellLogDisplay::Setup& s )
    : wl_(0)
    , unitmeas_(0)
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
    , valrg_(mUdf(float),0)
    , curvenmitm_(0)
    , curveitm_(0)
{
    if ( !isfirst )
	yax_.setup().nogridline(true);
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiWellDahDisplay(p,"Well Log display viewer")
    , setup_(su)
    , ld1_(scene(),true,su)
    , ld2_(scene(),false,su)
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
}


void uiWellLogDisplay::gatherInfo()
{
    setAxisRelations();
    gatherInfo( true );
    gatherInfo( false );

    if ( mIsUdf(zdata_.zrg_.start) && ld1_.wl_ )
    {
	zdata_.zrg_ = ld1_.zrg_;
	if ( ld2_.wl_ )
	    zdata_.zrg_.include( ld2_.zrg_ );
    }
    setAxisRanges( true );
    setAxisRanges( false );

    ld1_.yax_.setup().islog( ld1_.disp_.islogarithmic_ );
    ld1_.xax_.setup().epsaroundzero_ = 1e-5;
    ld1_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_.yax_.setup().islog( ld2_.disp_.islogarithmic_ );
    ld2_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_.xax_.setup().epsaroundzero_ = 1e-5;

    if ( ld1_.wl_ ) ld1_.xax_.setName( ld1_.wl_->name() );
    if ( ld2_.wl_ ) ld2_.xax_.setName( ld2_.wl_->name() );
}


void uiWellLogDisplay::setAxisRelations()
{
    ld1_.xax_.setBegin( &ld1_.yax_ );
    ld1_.yax_.setBegin( &ld2_.xax_ );
    ld2_.xax_.setBegin( &ld1_.yax_ );
    ld2_.yax_.setBegin( &ld2_.xax_ );
    ld1_.xax_.setEnd( &ld2_.yax_ );
    ld1_.yax_.setEnd( &ld1_.xax_ );
    ld2_.xax_.setEnd( &ld2_.yax_ );
    ld2_.yax_.setEnd( &ld1_.xax_ );

    ld1_.xax_.setNewDevSize( width(), height() );
    ld1_.yax_.setNewDevSize( height(), width() );
    ld2_.xax_.setNewDevSize( width(), height() );
    ld2_.yax_.setNewDevSize( height(), width() );
}


void uiWellLogDisplay::gatherInfo( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 )
    {
	if ( !first )
	{
	    ld2_.zrg_ = ld1_.zrg_;
	    ld2_.valrg_ = ld1_.valrg_;
	}
	return;
    }

    if ( ld.disp_.cliprate_ || mIsUdf( ld.disp_.range_.start ) )
    {
	DataClipSampler dcs( sz );
	dcs.add( ld.wl_->valArr(), sz );
	ld.valrg_ = dcs.getRange( ld.disp_.cliprate_ );
    }
    else
	ld.valrg_ = ld.disp_.range_;

    if ( !ld1_.wl_ && !ld2_.wl_ ) 
	ld.valrg_ = ld.disp_.range_ = Interval<float>(0,0);

    float startpos = ld.zrg_.start = ld.wl_->dah( 0 );
    float stoppos = ld.zrg_.stop = ld.wl_->dah( sz-1 );
    if ( zdata_.zistime_ && zdata_.d2tm_ && zdata_.d2tm_->size() > 1  )
    {
	startpos = zdata_.d2tm_->getTime( startpos )*1000;
	stoppos = zdata_.d2tm_->getTime( stoppos )*1000;
    }
    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
    if ( zdata_.dispzinft_ && !zdata_.zistime_)
	ld.zrg_.scale( mToFeetFactor );
}


void uiWellLogDisplay::setAxisRanges( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    if ( !first && setup_.sameaxisrange_ ) return;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
	ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zdata_.zrg_.stop, zdata_.zrg_.start );
    ld.yax_.setBounds( dispzrg );

    if ( first )
    {
    // Set default for 2nd
	ld2_.xax_.setBounds( dispvalrg );
	ld2_.yax_.setBounds( dispzrg );
    }
}


void uiWellLogDisplay::draw()
{
    setAxisRelations();
    if ( mIsUdf(zdata_.zrg_.start) ) return;

    ld1_.xax_.plotAxis(); ld1_.yax_.plotAxis();
    ld2_.xax_.plotAxis(); ld2_.yax_.plotAxis();

    drawMarkers();

    drawCurve( true );
    drawCurve( false );

    drawFilledCurve( true );
    drawFilledCurve( false );

    drawZPicks();
}


void uiWellLogDisplay::drawCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;
    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 || ld.disp_.size_ <=0 ) return;

    TypeSet<uiPoint> pts; 
    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( ld.wl_->dah( idx ) );
	float val = ld.wl_->value( idx );
	int xaxisval = mIsUdf(val) ? mUdf(int) : ld.xax_.getPix(val);
	pts += uiPoint( xaxisval, ld.yax_.getPix(zpos) );
    }
    if ( pts.isEmpty() )
	return;
    if ( !ld.curveitm_ ) 
	ld.curveitm_ = scene().addItem( new uiPolyLineItem() );
    uiPolyLineItem* pli = ld.curveitm_;
    pli->setPolyLine( pts );
    LineStyle ls(LineStyle::Solid);
    ls.width_ = ld.disp_.size_;
    ls.color_ = ld.disp_.color_;
    pli->setPenStyle( ls );
    pli->setZValue( ld.zoverlayval_ );

    Alignment al( Alignment::HCenter,
    first ? Alignment::Top : Alignment::Bottom );
    ld.curvenmitm_ = scene().addItem( new uiTextItem(ld.wl_->name(),al) );
    ld.curvenmitm_->setTextColor( ls.color_ );
    uiPoint txtpt;
    if ( first )
	txtpt = uiPoint( pts[0] );
    else
	txtpt = pts[pts.size()-1];

    ld.curvenmitm_->setPos( txtpt );

    if ( first )
	ld.yax_.annotAtEnd( zdata_.zistime_ ? "(ms)" : 
			    zdata_.dispzinft_ ? "(ft)" : "(m)" );
    if ( ld.unitmeas_ )
	ld.xax_.annotAtEnd( BufferString("(",ld.unitmeas_->symbol(),")") );
}


static const int cMaxNrLogSamples = 2000;
#define mGetLoopSize(nrsamp,step)\
    {\
	if ( nrsamp > cMaxNrLogSamples )\
	{\
	    step = (float)nrsamp/cMaxNrLogSamples;\
	    nrsamp = cMaxNrLogSamples;\
	}\
    }

void uiWellLogDisplay::drawFilledCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    deepErase( ld.curvepolyitms_ );

    if ( !ld.disp_.isleftfill_ && !ld.disp_.isrightfill_ ) return;

    const float rgstop = ld.xax_.range().stop; 
    const float rgstart = ld.xax_.range().start;
    const bool isrev = rgstop < rgstart;

    float colstep = ( rgstop -rgstart ) / 255;
    int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;

    const bool fullpanelfill = ld.disp_.isleftfill_ && ld.disp_.isrightfill_;
    const bool isfillrev = !fullpanelfill &&  
		 ( ( first && ld.disp_.isleftfill_ && !isrev )
		|| ( first && ld.disp_.isrightfill_ && isrev )
		|| ( !first && ld.disp_.isrightfill_ && !isrev )
		|| ( !first && ld.disp_.isleftfill_ && isrev ) );

    float zfirst = ld.wl_->dah(0);
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    closept.x = ( first ) ? isfillrev ? pixstart : pixstop 
			  : isfillrev ? pixstop  : pixstart;
    closept.y = ld.yax_.getPix( zfirst );
    int prevcolidx = 0;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept;

    uiPoint pt;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT(idx*step);
	float dah = ld.wl_->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= ld.wl_->dah(index+1) || dah <= ld.wl_->dah(index-1) )
		continue;
	}
	mDefZPosInLoop( dah )

	float val = ld.wl_->value( index );
	bool iscoltabrev = isrev; 
	if ( ld.disp_.iscoltabflipped_ )
	    iscoltabrev = !iscoltabrev;
	float valdiff = iscoltabrev ? rgstop-val : val-rgstart;
	int colindex = (int)( valdiff/colstep );
	if ( fullpanelfill ) 
	    val = first ? rgstart : rgstop;

	if ( mIsUdf(val) )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
		colorintset += 0;
	    }
	    continue;
	}

	pt.x = ld.xax_.getPix(val);
	pt.y = ld.yax_.getPix(zpos);
	*curpts += pt;

	if ( colindex != prevcolidx )
	{
	    *curpts += uiPoint( closept.x, pt.y );
	    colorintset += colindex;
	    prevcolidx = colindex;
	    pts += curpts;
	    curpts = new TypeSet<uiPoint>;
	    *curpts += uiPoint( closept.x, pt.y );
	}
    }
    if ( pts.isEmpty() ) return;
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y );

    const int tabidx = ColTab::SM().indexOf( ld.disp_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color =
	ld.disp_.issinglecol_ ? ld.disp_.seiscolor_
			      : seq->color( (float)colorintset[idx]/255 );
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


#define mDefHorLineX1X2Y() \
const int x1 = ld1_.xax_.getRelPosPix( 0 ); \
const int x2 = ld1_.xax_.getRelPosPix( 1 ); \
const int y = ld1_.yax_.getPix( zpos )

void uiWellLogDisplay::drawMarkers()
{
    deepErase( markerdraws_ );

    if ( !zdata_.markers_ ) return;

    for ( int idx=0; idx<zdata_.markers_->size(); idx++ )
    {
	const Well::Marker& mrkr = *((*zdata_.markers_)[idx]);
	const Color& col = mrkr.color();
	if ( col == Color::NoColor() || col == Color::White() ) continue;

	mDefZPosInLoop( mrkr.dah() );
	mDefHorLineX1X2Y();

	MarkerDraw* mrkdraw = new MarkerDraw( mrkr );
	markerdraws_ += mrkdraw;

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	LineStyle ls = LineStyle(setup_.markerls_.type_,
	setup_.markerls_.width_,col);
	li->setPenStyle( ls );
	li->setZValue( 2 );
	mrkdraw->lineitm_ = li;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
	new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( col );
	mrkdraw->txtitm_ = ti;
    }
}


uiWellLogDisplay::MarkerDraw* uiWellLogDisplay::getMarkerDraw(
						const Well::Marker& mrk )
{
    for ( int idx=0; idx<markerdraws_.size(); idx++)
    {
	if ( &(markerdraws_[idx]->mrk_) == &mrk )
	    return markerdraws_[idx];
    }
    return 0;
}


void uiWellLogDisplay::drawZPicks()
{
    deepErase( zpickitms_ );

    for ( int idx=0; idx<zpicks_.size(); idx++ )
    {
	const PickData& pd = zpicks_[idx];
	mDefZPosInLoop( pd.dah_ );
	mDefHorLineX1X2Y();

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	Color lcol( setup_.pickls_.color_ );
	if ( pd.color_ != Color::NoColor() )
	lcol = pd.color_;
	li->setPenStyle( LineStyle(setup_.pickls_.type_,setup_.pickls_.width_,
			lcol) );
	li->setZValue( 2 );
	zpickitms_ += li;
    }
}


uiWellLogDisplay::MarkerDraw::~MarkerDraw()
{
    delete txtitm_; delete lineitm_; 
}
