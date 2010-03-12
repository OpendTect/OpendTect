/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc,v 1.27 2010-03-12 14:14:44 cvsbruno Exp $";

#include "uiwelllogdisplay.h"

#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimenuhandler.h"

#include "coltabsequence.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welldisp.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"

#include <iostream>


#define mDefZPosInLoop(val)\
    float zpos = val;\
    if ( zistime_ && d2tm_ )\
	zpos = d2tm_->getTime( zpos )*1000;\
    if ( zpos < zrg_.start )\
	continue;\
    else if ( zpos > zrg_.stop )\
	break;\
    if ( dispzinft_ && !zistime_)\
	zpos *= mToFeetFactor;
uiWellLogDisplay::LineData::LineData( uiGraphicsScene& scn, Setup su )
    : zrg_(mUdf(float),0)
    , setup_(su)  
    , xax_(&scn,uiAxisHandler::Setup(uiRect::Top)
					    .nogridline(su.nogridline_)
					    .border(su.border_)
					    .noborderspace(su.noborderspace_)
					    .noaxisline(su.noxaxisline_)
					    .ticsz(su.xaxisticsz_))
    , yax_(&scn,uiAxisHandler::Setup(uiRect::Left)
					    .nogridline(su.nogridline_)
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


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiGraphicsView(p,"Well Log display viewer")
    , setup_(su)
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeetByDefault())
    , zistime_(false)
    , markers_(0)
    , markeredit_(false)   
    , mousepressed_(false)			   
    , selmarker_(0)			   
    , d2tm_(0)
    , markerchged(this)	   
    , ld1_(scene(),true,LineData::Setup()
	    				.noxaxisline(su.noxaxisline_)
	    				.noyaxisline(su.noyaxisline_)
					.xaxisticsz(su.axisticsz_)
	    				.nogridline(su.nogridline_)
					.noborderspace(su.noborderspace_))
    , ld2_(scene(),false,LineData::Setup()
	    				.noxaxisline(su.noxaxisline_)
					.noyaxisline(su.noyaxisline_)
					.xaxisticsz(su.axisticsz_)
					.noborderspace(su.noborderspace_)
	    				.nogridline(su.nogridline_))
{
    if ( su.nobackground_ )
    {
	setNoSytemBackGroundAttribute();
	uisetBackgroundColor( Color( 255, 255, 255, 0 )  );
	scene().setBackGroundColor( Color( 255, 255, 255, 0 )  );
    }
    setStretch( 2, 2 );

    getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiWellLogDisplay,mouseRelease) );
    getMouseEventHandler().buttonPressed.notify(
			    mCB(this,uiWellLogDisplay,mousePressed) );
    getMouseEventHandler().movement.notify(
			    mCB(this,uiWellLogDisplay,mouseMoved) );

    markerchged.notify( mCB(this,uiWellLogDisplay,reDrawMarkers) );
    reSize.notify( mCB(this,uiWellLogDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


void uiWellLogDisplay::setZRange( const Interval<float>& rg )
{
    zrg_ = rg;
    dataChanged();
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

    if ( mIsUdf(zrg_.start)  && ld1_.wl_ )
    {
	zrg_ = ld1_.zrg_;
	if ( ld2_.wl_ )
	    zrg_.include( ld2_.zrg_ );
    }
    setAxisRanges( true );
    setAxisRanges( false );
	
    BufferString znm;
    if ( zistime_ )
	znm += "TWT", "(ms)";
    else
	znm += "MD ", dispzinft_ ? "(ft)" : "(m)";

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
    if ( zistime_ && d2tm_ )
    {
	startpos = d2tm_->getTime( startpos )*1000;
	stoppos = d2tm_->getTime( stoppos )*1000;
    }

    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
}


void uiWellLogDisplay::setAxisRanges( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
    ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zrg_.stop, zrg_.start );
    if ( dispzinft_ && !zistime_ )
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
    if ( mIsUdf(zrg_.start) ) return;

    drawLog( true );
    drawLog( false );
    
    mRemoveSet( ld1_.curvepolyitms_ );
    if ( ld1_.wld_.islogfill_ )
	drawFilling( true );
    mRemoveSet( ld2_.curvepolyitms_ );
    if ( ld2_.wld_.islogfill_ )
	drawFilling( false );

    ld1_.xax_.plotAxis(); ld1_.yax_.plotAxis();
    ld2_.xax_.plotAxis(); ld2_.yax_.plotAxis();

    drawMarkers();
    drawZPicks();
}


void uiWellLogDisplay::drawLog( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    ld.al_ = Alignment(Alignment::HCenter, first ? 
	    			Alignment::Top : Alignment::Bottom );
    drawLine( ld, ld.wl_ );
    
    if ( ld.unitmeas_ )
	ld.xax_.annotAtEnd( BufferString("(",ld.unitmeas_->symbol(),")") );
}


void uiWellLogDisplay::drawLine( LogData& ld, const Well::DahObj* wd )
{
    mRemoveSet( ld.curveitms_ );
    scene().removeItem( ld.curvenmitm_ );
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;

    if ( !wd ) return;
    const int sz = wd ? wd->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;

    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( wd->dah( idx ) )
	float val = wd->value( idx );

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
	ld.curveitms_ += pli;
    }
    ld.curvenmitm_ = scene().addItem( new uiTextItem(wd->name(),ld.al_) );
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


void uiWellLogDisplay::drawFilling( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    mRemoveSet( ld.curvepolyitms_ );
    float colstep = ( ld.xax_.range().stop - ld.xax_.range().start ) / 255;

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;
    closept.x = ( first || ld.xrev_ ) ? ld.xax_.getPix(ld.xax_.range().stop) 
				      : ld.xax_.getPix(ld.xax_.range().start);
    closept.y = ( first || ld.xrev_ ) ? ld.yax_.getPix(ld.yax_.range().stop) 
				      : ld.yax_.getPix(ld.yax_.range().stop);
    int prevcolidx = 0;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept; 

    uiPoint pt;
    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( ld.wl_->dah( idx ) )
	float val = ld.wl_->value( idx );

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

	int colindex = ld.xrev_ ? (int)(ld.xax_.range().stop-val)
				: (int)(val-ld.xax_.range().start);
	colindex = (int)( colindex/colstep );
	if ( colindex != prevcolidx )
	{
	    *curpts += uiPoint( closept.x, pt.y ); 
	    if ( !curpts->isEmpty() )
	    {
		colorintset += colindex;
		prevcolidx = colindex;
		pts += curpts;
	    }
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
    if ( !markers_ ) return;

    for ( int idx=0; idx<markers_->size(); idx++ )
    {
	Well::Marker& mrkr = *((*markers_)[idx]);
	if ( mrkr.color() == Color::NoColor() ) continue;

	mDefZPosInLoop( mrkr.dah() )
	mDefHorLineX1X2Y();

	MarkerItem* markeritm = new MarkerItem( mrkr );
	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(setup_.markerls_.type_,
		    		   setup_.markerls_.width_,mrkr.color()) );
	MouseCursor cursor( markeredit_ ? 
				MouseCursor::SizeVer : MouseCursor::Arrow );
	li->setCursor( cursor );
	markeritm->itm_ = li;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	    mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
			 new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( mrkr.color() );
	markeritm->txtitm_ = ti;

	markeritms_ += markeritm;
    }
}


void uiWellLogDisplay::reDrawMarkers( CallBacker* )
{
    drawMarkers();
}


void uiWellLogDisplay::setEditMarkers( bool edit )
{
    markeredit_ = edit;
    drawMarkers();
}


void uiWellLogDisplay::mousePressed( CallBacker* cb )
{
    mousepressed_ = true;
    if ( mousepressed_ && markeredit_ )
       selmarker_ = selectMarker(false);	
}


void uiWellLogDisplay::mouseMoved( CallBacker* cb )
{
    if ( mousepressed_ )
	changeMarkerPos( selmarker_ );
}


void uiWellLogDisplay::mouseRelease( CallBacker* )
{
    mousepressed_ = false;
    selmarker_ = 0; 
}


Well::Marker* uiWellLogDisplay::selectMarker( bool allowrightclk)
{
    if ( !markeredit_ ) return 0;
    if ( getMouseEventHandler().isHandled() )
	return 0;

    const MouseEvent& ev = getMouseEventHandler().event();
    if ( (ev.buttonState() & OD::MidButton ) ) return 0;
    if ( !allowrightclk )
    {	
	 if ( !(ev.buttonState() & OD::LeftButton ) ||
	       (ev.buttonState() & OD::RightButton ) )
	return 0;
    }

    int mousepos = ev.pos().y;
    uiLineItem* mrkitm = 0; uiTextItem* mrktxtitm;
    for ( int idx=0; idx<markeritms_.size(); idx++ )
    {
	if ( fabs( markeritms_[idx]->itm_->getPos().y - mousepos )<2 )
	    return ( &markeritms_[idx]->mrk_ );
    }
    return 0;
}


void uiWellLogDisplay::changeMarkerPos( Well::Marker* mrk )
{
    if ( getMouseEventHandler().isHandled() )
	return;

    if ( !selmarker_ ) return;

    const MouseEvent& ev = getMouseEventHandler().event();
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    selmarker_->setDah( mousePos() );
    markerchged.trigger(); 
}


float uiWellLogDisplay::mousePos()  
{
    const MouseEvent& ev = getMouseEventHandler().event();
    float mousepos = ld1_.yax_.getVal( ev.pos().y );
    if ( zistime_ && d2tm_ )
	mousepos = d2tm_->getDepth( mousepos )/1000;
    return mousepos;
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
	li->setPenStyle( LineStyle(setup_.pickls_.type_,setup_.pickls_.width_,
		    		   lcol) );
	zpickitms_ += li;
    }
}




uiWellDisplay::uiWellDisplay( uiParent* p, const Setup& s, Well::Data& wd)
    	: uiGroup(p, wd.name() )
	, logwidth_(s.logwidth_)		 
	, logheight_(s.logheight_)		 
	, wd_(wd)
	, zrg_(mUdf(float),0)
	, d2tm_(wd_.d2TModel())
   	, zistime_(wd_.haveD2TModel())		     
	, noborderspace_(s.noborderspace_)				    
	, menu_(*new uiMenuHandler(p,-1))
	, remmrkmnuitem_("Remove marker...",100)      				
	, addmrkmnuitem_("Add marker...",101)      			 
{
    if ( s.nobackground_ )
	setNoBackGround();
    
    setStretch( 2, 2 );
    logwidth_ -= s.noborderspace_ ? 50 : 0;
    wd_.dispparschanged.notify( mCB(this,uiWellDisplay,updateProperties) );

    for ( int idx=0; idx<s.nrpanels_; idx++ )
	setLogPanel( s.noborderspace_, idx==0 );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiWellDisplay,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiWellDisplay,handleMenuCB));

    setHSpacing( 0 );
    setPrefWidth( s.nrpanels_*logwidth_ );
    setPrefHeight( mLogHeight );
    updateProperties(0);
    //setInitialZRange();
}


uiWellDisplay::~uiWellDisplay()
{
    menu_.unRef();
}


void uiWellDisplay::setInitialZRange()
{
    Interval<float> zrg(0,0);
    const Well::Log* wl = 0;
    for ( int dispidx=0; dispidx<logdisps_.size();  dispidx++ )
    {
	if ( !logdisps_[dispidx] ) continue;
	Interval<float> dahrg;
	uiWellLogDisplay::LogData ld = logdisps_[dispidx]->logData( true );
	wl = ld.wl_;
	if ( wl ) dahrg = wl->dahRange();
	if ( !dahrg.overlaps( zrg, false ) )
	    zrg = dahrg;

	ld = logdisps_[dispidx]->logData( false );
	wl = ld.wl_;
	if ( wl ) dahrg = wl->dahRange();
	if ( !dahrg.overlaps( zrg, false ) )
	    zrg = dahrg;
    }
    //Swap( zrg.start, zrg.stop );
    if ( zistime_ && d2tm_ ) 
	zrg.set( d2tm_->getTime(zrg.start)*1000,d2tm_->getTime(zrg.stop)*1000 );
  
    setZRange( zrg );
}


void uiWellDisplay::setLogPanel( bool noborderspace, bool isleft )
{
    uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
    wldsu.noxpixafter_ = isleft; wldsu.noxpixbefore_ = !isleft;
    if ( noborderspace_ ) 
    { 
	wldsu.nobackground_ = true;
	wldsu.axisticsz_ = -15; 
	wldsu.noborderspace_ = true; 
	wldsu.nogridline_ = true;
	wldsu.noyaxisline_ = true; 
    }
    wldsu.border_ = uiBorder(0);
    wldsu.border_.setLeft(0); wldsu.border_.setRight(0);
    uiWellLogDisplay* logdisp = new uiWellLogDisplay( this, wldsu );
    if ( logdisps_.size() )
	logdisp->attach( rightOf, logdisps_[logdisps_.size()-1] );

    logdisps_ += logdisp;
    logdisp->setZInTime( zistime_ && d2tm_ );
    logdisp->setD2TModel( d2tm_ );
    logdisp->setMarkers( &wd_.markers() );
    logdisp->setEditMarkers( true );
    logdisp->markerchged.notify(mCB(this,uiWellDisplay,trigMarkersChanged));
    logdisp->getMouseEventHandler().buttonReleased.notify(
				    mCB(this,uiWellDisplay,usrClickCB) );
}


void uiWellDisplay::setAxisRanges()
{
    Interval<float> dispzrg( zrg_.stop, zrg_.start );
    for ( int idx=0; idx<logdisps_.size(); idx++ )
	if ( logdisps_[idx] ) logdisps_[idx]->setZRange( dispzrg );
}


void uiWellDisplay::setZRange( Interval<float> rg )
{
    zrg_.set( rg.stop, rg.start );
    setAxisRanges();
}


void uiWellDisplay::setLog( const char* logname, int logidx, bool left )
{
    const Well::Log* l = wd_.logs().getLog( logname );
    if ( !logdisps_[logidx] ) return;

    uiWellLogDisplay::LogData& ld = logdisps_[logidx]->logData(left);
    ld.wl_ 	    = l;
    ld.xrev_        = false;
    ld.isyaxisleft_ = left;
}


void uiWellDisplay::dataChanged( CallBacker* cb )
{
    for ( int idx=0; idx<logdisps_.size(); idx++ )
	if ( logdisps_[idx] ) logdisps_[idx]->dataChanged();
}


void uiWellDisplay::updateProperties( CallBacker* cb )
{
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	uiWellLogDisplay* ld = logdisps_[idx];
	if ( !ld ) continue;
	setLog( wd_.displayProperties().left_.name_, idx, true );
	setLog( wd_.displayProperties().right_.name_, idx, false );
	logdisps_[idx]->logData(true).wld_ = wd_.displayProperties().left_;
	logdisps_[idx]->logData(false).wld_ = wd_.displayProperties().right_;
    }
    dataChanged( 0 );
}


void uiWellDisplay::trigMarkersChanged( CallBacker* )
{
    wd_.markerschanged.trigger();
}


void uiWellDisplay::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return; 
    if ( !mevh->hasEvent() )
	return;
    if ( mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


bool uiWellDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() )
    {
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


class uiWellDispAddMarkerDlg : public uiDialog
{
public : 
    uiWellDispAddMarkerDlg( uiParent* p, float dah )
	: uiDialog(p,uiDialog::Setup("Add marker",
				     "Specify properties",mNoHelpID))
	, marker_(0)							     
    {
	namefld_ = new uiGenInput( this, "Name", StringInpSpec("Marker") );
	depthfld_ = new uiGenInput( this, "Depth", FloatInpSpec(dah) );
	depthfld_->attach( alignedBelow, namefld_ );
	uiColorInput::Setup csu( Color::DgbColor() );
	csu.lbltxt( "Color" ).withalpha(false);
	colorfld_ = new uiColorInput( this, csu, "Color" );
	colorfld_->attach( alignedBelow, depthfld_ );
    }

    bool acceptOK( CallBacker* )
    {
	const char* nm = namefld_->text();
	float dpt = depthfld_->getfValue();
	marker_ = new Well::Marker( nm, dpt );
	marker_->setColor( colorfld_->color() );
	return true;
    }

    Well::Marker* marker() { return marker_; }

protected :
    Well::Marker*	marker_;
    uiGenInput*		namefld_;
    uiGenInput*		depthfld_;
    uiColorInput*	colorfld_;
};


void uiWellDisplay::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;
    if ( !logdisps_.size() ) return;
    if ( mnuid==addmrkmnuitem_.id )
    {
	uiWellDispAddMarkerDlg dlg( this, logdisps_[0]->mousePos() );
	if ( dlg.go() )
	{
	    Well::Marker* newmrk = dlg.marker();
	    if ( !newmrk ) return;
	    Well::MarkerSet& mrkset = wd_.markers();
	    for ( int idx=0; idx<mrkset.size(); idx++ )
	    {
		Well::Marker& mrk = *mrkset[idx]; 
		if ( newmrk->dah() > mrk.dah() )
		    continue;
		else 
		{ 
		    mrkset.insertAt( newmrk, idx );
		    logdisps_[0]->markerchged.trigger(); 
		    return;
		}
	    }
	}
    }
    else if ( mnuid==remmrkmnuitem_.id  && logdisps_[0]->selectMarker(true) )
    {
	ObjectSet<Well::Marker>& mrkset = wd_.markers();
	delete mrkset.remove( 
		mrkset.indexOf( logdisps_[0]->selectMarker(true) ), true );
	logdisps_[0]->markerchged.trigger(); 
    }
    else
	ishandled = false;

    menu->setIsHandled( ishandled );
}


void uiWellDisplay::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    if ( logdisps_.size() && logdisps_[0]->selectMarker(true) ) 
	mAddMenuItem( menu, &remmrkmnuitem_, true, false );
    mAddMenuItem( menu, &addmrkmnuitem_, true, false );
}
