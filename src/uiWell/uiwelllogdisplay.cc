/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc,v 1.61 2010-09-13 14:05:09 cvsjaap Exp $";

#include "uiwelllogdisplay.h"

#include "uicolor.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwellinfopanels.h"
#include "uiwellstratdisplay.h"

#include "coltabsequence.h"
#include "dataclipper.h"
#include "randcolor.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellman.h"

#include <iostream>


#define mDefZPos(zpos)\
    if ( data_.zistime_ && data_.d2tm_ )\
	zpos = data_.d2tm_->getTime( zpos )*1000;\
    if ( data_.dispzinft_ && !data_.zistime_)\
	zpos *= mToFeetFactor;
#define mDefZPosInLoop(val)\
    float zpos = val;\
    mDefZPos(zpos)\
    if ( !data_.zrg_.includes( zpos ) )\
	continue;
uiWellLogDisplay::LineData::LineData( uiGraphicsScene& scn, Setup su )
    : zrg_(mUdf(float),0)
    , setup_(su)  
    , xax_(&scn,uiAxisHandler::Setup(uiRect::Top)
					    .nogridline(su.noxgridline_)
					    .border(su.border_)
					    .noborderspace(su.noborderspace_)
					    .noaxisline(su.noxaxisline_)
					    .ticsz(su.xaxisticsz_))
    , yax_(&scn,uiAxisHandler::Setup(uiRect::Left)
					    .nogridline(su.noygridline_)
					    .noaxisline(su.noyaxisline_)
					    .border(su.border_)
					    .noborderspace(su.noborderspace_))
    , valrg_(mUdf(float),0)
    , curvenmitm_(0)
{
}


uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool first, Setup s )
    : LineData(scn,s)
    , wl_(0)
    , wld_(Well::DisplayProperties::Log())		   
    , unitmeas_(0)
    , xrev_(false)
    , isyaxisleft_(true)	     
{
    if ( !first )
    {
	xax_.setup().side_ = uiRect::Bottom;
	yax_.setup().nogridline(true);
    }
}


uiWellLogDisplay::MarkerItem::MarkerItem( Well::Marker& mrk )
    : mrk_(mrk) 
{
}


uiWellLogDisplay::MarkerItem::~MarkerItem()
{
    delete itm_; delete txtitm_;
}



uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiGraphicsView(p,"Well Log display viewer")
    , setup_(su)
    , highlightedmrk_(0)	     
    , highlightedMarkerItemChged(this)
    , ld1_(scene(),true,LineData::Setup()
	    				.noxaxisline(su.noxaxisline_)
	    				.noyaxisline(su.noyaxisline_)
					.xaxisticsz(su.axisticsz_)
	    				.noygridline(su.noygridline_)
	    				.noxgridline(su.noxgridline_)
					.noborderspace(su.noborderspace_))
    , ld2_(scene(),false,LineData::Setup()
	    				.noxaxisline(su.noxaxisline_)
					.noyaxisline(su.noyaxisline_)
					.xaxisticsz(su.axisticsz_)
					.noborderspace(su.noborderspace_)
	    				.noygridline(su.noygridline_)
	    				.noxgridline(su.noxgridline_))
{
    if ( su.nobackground_ )
	setNoBackGround();

    data_.dispzinft_ = SI().depthsInFeetByDefault();
    reSize.notify( mCB(this,uiWellLogDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


void uiWellLogDisplay::reSized( CallBacker* )
{
    draw();
}


void uiWellLogDisplay::init( CallBacker* )
{
    dataChanged();
    show();
}


void uiWellLogDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiWellLogDisplay::gatherInfo()
{
    setAxisRelations();
    
    gatherInfo( true );
    gatherInfo( false );

    if ( mIsUdf(data_.zrg_.start)  && ld1_.wl_ )
    {
	data_.zrg_ = ld1_.zrg_;
	if ( ld2_.wl_ )
	    data_.zrg_.include( ld2_.zrg_ );
    }
    setAxisRanges( true );
    setAxisRanges( false );
	
    BufferString znm;
    if ( data_.zistime_ )
	znm += "TWT", "(ms)";
    else
	znm += "MD ", data_.dispzinft_ ? "(ft)" : "(m)";

    ld1_.yax_.setup().side_ = ld1_.isyaxisleft_ ? uiRect::Left : uiRect::Right;
    ld2_.yax_.setup().side_ = ld2_.isyaxisleft_ ? uiRect::Left : uiRect::Right;
    ld1_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld1_.xax_.setup().epsaroundzero_ = 1e-5;
    ld1_.yax_.setup().islog( ld1_.wld_.islogarithmic_ );
    ld1_.yax_.setup().name( znm );
    ld2_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_.xax_.setup().epsaroundzero_ = 1e-5;
    ld2_.yax_.setup().islog( ld2_.wld_.islogarithmic_ );
    ld2_.yax_.setup().name( znm );
    if ( ld1_.wl_ ) ld1_.xax_.setup().name( ld1_.wl_->name() );
    if ( ld2_.wl_ ) ld2_.xax_.setup().name( ld2_.wl_->name() );
}


void uiWellLogDisplay::setAxisRelations()
{
    ld1_.xax_.setBegin( setup_.noxpixbefore_ ? 0 : &ld1_.yax_ );
    ld1_.yax_.setBegin( setup_.noypixbefore_ ? 0 : &ld1_.xax_ );
    ld1_.xax_.setEnd(  setup_.noxpixafter_ ? 0 : &ld1_.yax_ );
    ld1_.yax_.setEnd( setup_.noypixafter_ ? 0 : &ld1_.xax_ );
    ld2_.xax_.setBegin( setup_.noxpixbefore_ ? 0 : &ld2_.yax_ );
    ld2_.yax_.setBegin( setup_.noypixbefore_ ? 0 : &ld2_.xax_ );
    ld2_.xax_.setEnd(  setup_.noxpixafter_ ? 0 : &ld2_.yax_ );
    ld2_.yax_.setEnd( setup_.noypixafter_ ? 0 : &ld2_.xax_ );
    
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
	    ld2_.copySetupFrom( ld1_ );
	    ld2_.zrg_ = ld1_.zrg_;
	    ld2_.valrg_ = ld1_.valrg_;
	}
	return;
    }

    if ( ld.wld_.cliprate_ || mIsUdf( ld.wld_.range_.start ) )
    {
	DataClipSampler dcs( sz );
	dcs.add( ld.wl_->valArr(), sz );
	ld.valrg_ = dcs.getRange( ld.wld_.cliprate_ );
    }
    else
	ld.valrg_ = ld.wld_.range_;

    float startpos = ld.wl_->dah( 0 );
    float stoppos = ld.wl_->dah( sz-1 );
    if ( data_.zistime_ && data_.d2tm_ )
    {
	startpos = data_.d2tm_->getTime( startpos )*1000;
	stoppos = data_.d2tm_->getTime( stoppos )*1000;
    }

    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
}


void uiWellLogDisplay::setDispProperties( Well::DisplayProperties& prop )
{
    disp_ = prop;
    logData( true ).wld_ = disp_.left_;
    logData( false ).wld_ = disp_.right_;
    dataChanged();
}


void uiWellLogDisplay::setAxisRanges( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
    ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( data_.zrg_.stop, data_.zrg_.start );
    if ( data_.dispzinft_ && !data_.zistime_ )
	dispzrg.scale( mToFeetFactor );
    ld.yax_.setBounds( dispzrg );

    if ( first )
    {
	// Set default for 2nd
	ld2_.xax_.setBounds( dispvalrg );
	ld2_.yax_.setBounds( dispzrg );
    }
}


#define mRemoveSet( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
    scene().removeItem( itms[idx] ); \
    deepErase( itms );
void uiWellLogDisplay::draw()
{
    setAxisRelations();
    if ( mIsUdf(data_.zrg_.start) ) return;

    ld1_.xax_.plotAxis(); ld1_.yax_.plotAxis();
    ld2_.xax_.plotAxis(); ld2_.yax_.plotAxis();

    drawMarkers();
    drawZPicks();
    drawLog( true );
    drawLog( false );
}


void uiWellLogDisplay::drawLog( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    ld.al_ = Alignment(Alignment::HCenter, first ? 
	    			Alignment::Top : Alignment::Bottom );

    mRemoveSet( ld.curvepolyitms_ );
    if ( ld.wld_.islogfill_ )
	drawFilling( first );
    
    drawLine( ld, ld.wl_ );
    
    if ( ld.unitmeas_ )
	ld.xax_.annotAtEnd( BufferString("(",ld.unitmeas_->symbol(),")") );
}


void uiWellLogDisplay::drawLine( LogData& ld, const Well::DahObj* dahobj )
{
    mRemoveSet( ld.curveitms_ );
    scene().removeItem( ld.curvenmitm_ );
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;

    if ( !dahobj ) return;
    const int sz = dahobj ? dahobj->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;

    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( dahobj->dah( idx ) )
	float val = dahobj->value( idx );

	if ( mIsUdf(val) )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
	    }
	    continue;
	}
	*curpts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(zpos) );
    }
    if ( curpts->isEmpty() )
	delete curpts;
    else
	pts += curpts;
    if ( pts.isEmpty() ) return;

    LineStyle ls(LineStyle::Solid);
    ls.width_ = ld.wld_.size_;
    ls.color_ = ld.wld_.color_;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem(*pts[idx]) );
	pli->setPenStyle( ls );
	pli->setZValue( 1 );
	ld.curveitms_ += pli;
    }
    ld.curvenmitm_ = scene().addItem( new uiTextItem(dahobj->name(),ld.al_) );
    ld.curvenmitm_->setTextColor( ls.color_ );
    uiPoint txtpt;
    if ( ld.al_.vPos() == Alignment::Top )
	txtpt = uiPoint( (*pts[0])[0] );
    else
    {
	TypeSet<uiPoint>& lastpts( *pts[pts.size()-1] );
	txtpt = lastpts[lastpts.size()-1];
    }
    ld.curvenmitm_->setPos( txtpt );
    
    deepErase( pts );
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
void uiWellLogDisplay::drawFilling( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    mRemoveSet( ld.curvepolyitms_ );

    float colstep = ( ld.xax_.range().stop - ld.xax_.range().start ) / 255;
    int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;

    float zfirst = ld.wl_->dah(0);
    mDefZPos( zfirst )
    closept.x = ( first || ld.xrev_ ) ? ld.xax_.getPix(ld.xax_.range().stop) 
				      : ld.xax_.getPix(ld.xax_.range().start);
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

	float valdiff = ld.xrev_ ? ld.xax_.range().stop-val 
	    			 : val-ld.xax_.range().start;
	int colindex = (int)( valdiff/colstep );
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

    const int tabidx = ColTab::SM().indexOf( ld.wld_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color = 
	    ld.wld_.issinglecol_ ? ld.wld_.seiscolor_
				 : seq->color( (float)colorintset[idx]/255 );
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
    }

    deepErase( pts );
}


#define mDefHorLineX1X2Y() \
    const int x1 = ld1_.xax_.getRelPosPix( 0 ); \
    const int x2 = ld1_.xax_.getRelPosPix( 1 ); \
    const int y = ld1_.yax_.getPix( zpos )
void uiWellLogDisplay::drawMarkers()
{
    for ( int idx=0; idx<markeritms_.size(); idx++ ) \
    {
	scene().removeItem( markeritms_[idx]->itm_ ); 
	scene().removeItem( markeritms_[idx]->txtitm_ ); 
    }
    deepErase( markeritms_ );
    if ( !data_.markers_ ) return;

    for ( int idx=0; idx<data_.markers_->size(); idx++ )
    {
	Well::Marker& mrkr = *((*data_.markers_)[idx]);
	if ( mrkr.color() == Color::NoColor() || 
		mrkr.color() == Color::White()  ) continue;

	mDefZPosInLoop( mrkr.dah() )
	mDefHorLineX1X2Y();

	MarkerItem* markeritm = new MarkerItem( mrkr );
	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(setup_.markerls_.type_,
		    		   setup_.markerls_.width_,mrkr.color()) );
	li->setZValue( 2 );
	markeritm->itm_ = li;
	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	    mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
			 new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( mrkr.color() );
	markeritm->color_ = mrkr.color();
	markeritm->txtitm_ = ti;
	markeritms_ += markeritm;
    }
    highlightedmrk_ = 0;
}


void uiWellLogDisplay::reDrawMarkers( CallBacker* )
{
    drawMarkers();
}



uiWellLogDisplay::MarkerItem* uiWellLogDisplay::getMarkerItem( 
						const Well::Marker* mrk )
{
    for ( int idx=0; idx<markeritms_.size(); idx++ )
    {
	if ( markeritms_[idx]->mrk_ == mrk )
	    return markeritms_[idx];
    }
    return 0;
}


void uiWellLogDisplay::highlightMarkerItem( const Well::Marker* mrk, 
					    bool trigchg  )
{
    if ( highlightedmrk_ )
    {
	uiWellLogDisplay::MarkerItem* mitm = getMarkerItem( highlightedmrk_ );
	if ( mitm ) 
	    mitm->itm_->setPenStyle( LineStyle(setup_.markerls_.type_,
				     setup_.markerls_.width_, 
				     mitm->color_) );
    }
    MarkerItem* mrkitm = mrk ? getMarkerItem( mrk ) : 0;
    if ( mrkitm )
    {
	mrkitm->itm_->setPenStyle( LineStyle(setup_.markerls_.type_,
				    setup_.markerls_.width_+2, 
				    mrkitm->color_) );
    }
    highlightedmrk_ = mrk;
    if ( trigchg )
	highlightedMarkerItemChged.trigger();
}


void uiWellLogDisplay::drawZPicks()
{
    mRemoveSet( zpickitms_ );

    for ( int idx=0; idx<zpicks_.size(); idx++ )
    {
	const PickData& pd = zpicks_[idx];
	mDefZPosInLoop( pd.dah_ )
	mDefHorLineX1X2Y();

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	Color lcol( setup_.pickls_.color_ );
	if ( pd.color_ != Color::NoColor() )
	    lcol = pd.color_;
	li->setPenStyle( LineStyle(setup_.pickls_.type_,
		    		   setup_.pickls_.width_,
		    		   lcol) );
	zpickitms_ += li;
    }
}


float uiWellLogDisplay::mousePos() const
{
    const MouseEventHandler& meh = scene_->getMouseEventHandler();
    if ( !meh.hasEvent() ) return 0;
    const MouseEvent& ev = meh.event();
    return logData(true).yax_.getVal( ev.pos().y );
}



uiWellDisplay::Params::Params( Well::Data* wd )
    : wd_(wd)
{
    data_.zistime_ = ( wd && wd->haveD2TModel() );		    
}


#define mGetWD(act) Well::Data* wd = getWD(); if ( !wd ) act;
uiWellDisplay::uiWellDisplay( uiParent* p, const Setup& s, const MultiID& wid )
    	: uiGroup(p,"")
	, stratdisp_(0)
	, wellid_(wid) 
	, control_(0)	       
	, pms_(Params(s.wd_))			   
{
    mGetWD(return);

    if ( s.nobackground_ )
	setNoBackGround();
    
    for ( int idx=0; idx<s.nrpanels_; idx++ )
    {
	addLogPanel( s.noborderspace_, idx==0 );
	setDispProperties( wd->displayProperties(), idx );
    }

    if ( s.withstratdisp_ )
	setStratDisp();
    
    control_ = new uiWellDisplayControl( *logDisplay(0) );

    setHSpacing( 0 );
    setStretch( 2, 2 );
    addWDNotifiers( *wd );
    dataChanged( 0 );
}


uiWellDisplay::uiWellDisplay( uiWellDisplay& orgdisp, const ShapeSetup& su )
    : uiGroup(su.parent_,"")
    , stratdisp_(orgdisp.stratDisp())
    , pms_(orgdisp.params())				     
    , control_(orgdisp.control_)				    
    , wellid_(orgdisp.getWellID())       	       
{
    mGetWD(return);
    orgdisp.removeWDNotifiers( *wd );
    setNoBackGround();

    for ( int idx=0; idx<orgdisp.nrLogDisp(); idx++ )
    {
	if ( su.nrlogpanels_ > idx )
	{
	    uiWellLogDisplay* log = orgdisp.logDisplay(idx);
	    log->reParent( this );
	    if ( idx ) log->attach( rightOf, logdisps_[idx-1] );
	    log->setStretch( 2, 2 );
	    logdisps_ += log;
	}
	else if ( control_ )
	    control_->removeLogDisplay(*orgdisp.logDisplay(idx));

	orgdisp.removeLogDisplay( orgdisp.logDisplay(idx) ); 
    }
    
    for ( int idx=logdisps_.size(); idx<su.nrlogpanels_; idx++ )
    {
	addLogPanel( true, false );
	logdisps_[idx]->data().copyFrom( pms_.data_ );
	logdisps_[idx]->doDataChange();
	logdisps_[idx]->setStretch( 2, 2 );
	if ( control_ ) control_->addLogDisplay( *logdisps_[idx] ); 
    }
   
    if ( su.withstrat_ )
	setStratDisp();
    else 
    { delete stratdisp_; stratdisp_ = 0; }

    setHSpacing( 0 );
    setStretch( 2, 2 );
    addWDNotifiers( *wd );
}


uiWellDisplay::~uiWellDisplay()
{
    mGetWD(return);
    removeWDNotifiers( *wd );
    pms_.wd_ = 0;
    Well::MGR().release( wellid_ );
}


void uiWellDisplay::setInitialSize( uiSize sz )
{
    setPrefWidth( sz.width() ); setPrefHeight( sz.height() );
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	logdisps_[idx]->setPrefWidth( (int)(sz.width()/logdisps_.size()) ); 
	logdisps_[idx]->setPrefHeight( sz.height() );
    }
    if ( stratdisp_ )
    {
	stratdisp_->setPrefWidth( (int)(sz.width()/(2*logdisps_.size()+1)));
	stratdisp_->setPrefHeight( sz.height() );
    }
}


void uiWellDisplay::addWDNotifiers( Well::Data& wd )
{
    uiWellDisplay* self = const_cast<uiWellDisplay*>( this );
    wd.d2tchanged.notify( mCB(self,uiWellDisplay,dataChanged) );
    wd.markerschanged.notify( mCB(self,uiWellDisplay,dataChanged) );
    wd.tobedeleted.notify(mCB(self,uiWellDisplay,welldataDelNotify) );
}


void uiWellDisplay::removeWDNotifiers( Well::Data& wd )
{
    uiWellDisplay* self = const_cast<uiWellDisplay*>( this );
    wd.d2tchanged.remove( mCB(self,uiWellDisplay,dataChanged) );
    wd.markerschanged.remove( mCB(self,uiWellDisplay,dataChanged) );
    wd.tobedeleted.remove(mCB(self,uiWellDisplay,welldataDelNotify) );
}


void uiWellDisplay::welldataDelNotify( CallBacker* )
{
    pms_.wd_ = 0;
    pms_.data_.d2tm_ = 0;
    pms_.data_.markers_ = 0;
    getWD();
}


Well::Data* uiWellDisplay::getWD() const
{
    if ( !pms_.wd_ )
    {
	uiWellDisplay* self = const_cast<uiWellDisplay*>( this );
	if ( !self ) pErrMsg( "Huh" );
	Well::Data* wd = Well::MGR().get( wellid_, false );
	self->pms_.wd_ = wd;
	if ( wd )
	{
	    self->addWDNotifiers( *wd );
	    self->dataChanged( 0 );
	}
    }
    return pms_.wd_;
}


int uiWellDisplay::getDispWidth()
{
    return (nrLogDisp()? logdisps_[0]->width() : 0) 
         + (hasStratDisp() ? stratdisp_->width() : 0);
}


void uiWellDisplay::setStratDisp()
{
    mGetWD( return );
    if ( stratdisp_ )
	stratdisp_->reParent(this);
    else
	stratdisp_ = new uiWellStratDisplay( this, true, pms_.data_ );
    if ( nrLogDisp() )
	stratdisp_->attach( rightOf, logdisps_[nrLogDisp()-1] );
    stratdisp_->setStretch( 2, 2 );
}


void uiWellDisplay::addLogPanel( bool noborderspace, bool isleft )
{
    uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
    wldsu.noxpixafter_ = isleft; wldsu.noxpixbefore_ = !isleft;
    if ( noborderspace ) 
    { 
	wldsu.nobackground_ = true;
	wldsu.axisticsz_ = -15; 
	wldsu.noborderspace_ = true; 
	wldsu.noygridline_ = true;
	wldsu.noxgridline_ = false;
	wldsu.noyaxisline_ = true; 
    }
    wldsu.border_ = uiBorder(0);
    uiWellLogDisplay* logdisp = new uiWellLogDisplay( this, wldsu );
    if ( logdisps_.size() )
	logdisp->attach( rightOf, logdisps_[logdisps_.size()-1] );
    logdisps_ += logdisp;
}


void uiWellDisplay::setLog( const char* logname, int logidx, bool left )
{
    mGetWD( return )
    const Well::Log* l = wd->logs().getLog( logname );
    if ( !logdisps_[logidx] ) return;

    uiWellLogDisplay::LogData& ld = logdisps_[logidx]->logData(left);
    ld.wl_ 	    = l;
    ld.xrev_        = left;
    ld.isyaxisleft_ = left;
}


void uiWellDisplay::dataChanged( CallBacker* cb )
{
    mGetWD( return )
    pms_.data_.d2tm_ = wd->d2TModel();
    pms_.data_.markers_ = &wd->markers();
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	uiWellLogDisplay* logdisp = logdisps_[idx];
	if ( !logdisp ) continue;
	logdisp->data().copyFrom( pms_.data_ );
	logdisp->doDataChange();
    }
    if ( hasStratDisp() )
    {
	stratdisp_->dispData().copyFrom( pms_.data_ );
	stratdisp_->doDataChange();
    }
}


void uiWellDisplay::setDispProperties( Well::DisplayProperties& disp, int pannr)
{
    if ( pannr < logdisps_.size() )
    {
	uiWellLogDisplay* ld = logdisps_[pannr];
	if ( !ld ) return;
	setLog( disp.left_.name_, pannr, true );
	setLog( disp.right_.name_, pannr, false );
	ld->setDispProperties( disp );
    }
}


bool uiWellDisplay::isPresent( const uiWellLogDisplay* ld ) const
{
    for ( int idl=0; idl<logdisps_.size(); idl++ )
    {
	if ( logdisps_[idl] == ld )
	    return true;
    }
    return false;
}


uiWellDisplayWin::uiWellDisplayWin( uiParent* p, Well::Data& wd )
    : uiMainWin(p,"")
    , welldisp_(*new uiWellDisplay(this,uiWellDisplay::Setup()
							.withstratdisp(false)
							.wd(&wd)
							,0))
    , wd_(wd)				
    , wellinfo_(0)		    			
{
    setStretch( 2, 2 );
    welldisp_.setInitialSize( uiSize(50,600) );
    const_cast<uiWellDisplayControl*>(
	welldisp_.control())->posChanged.notify(
		mCB(this,uiWellDisplayWin,dispInfoMsg) );
    BufferString msg( "2D Viewer " );
    msg += wd.name();
    setCaption( msg );
    wd_.tobedeleted.notify( mCB(this,uiWellDisplayWin,closeWin) );
    wd_.dispparschanged.notify( mCB(this,uiWellDisplayWin,updateProperties) );
    finaliseDone.notify(mCB(this,uiWellDisplayWin,mkInfoPanel)); 
}


void uiWellDisplayWin::mkInfoPanel( CallBacker* )
{
    wellinfo_ = new uiWellDispInfoPanel( this, welldisp_ );
    wellinfo_->setInitialSize( uiSize(50,100) );
    wellinfo_->attach( alignedAbove, &welldisp_ );
}


void uiWellDisplayWin::closeWin( CallBacker* )
{ close(); }


void uiWellDisplayWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mesg.buf() );
}


void uiWellDisplayWin::updateProperties( CallBacker* )
{
    welldisp_.setDispProperties( wd_.displayProperties() );
}
