/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc,v 1.20 2010-02-08 05:54:16 cvsranojay Exp $";

#include "uiwelllogdisplay.h"
#include "uiwelldisppropdlg.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "coltabsequence.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "uibutton.h"

#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welldisp.h"

#include <iostream>



#define mDefZPosInLoop(val)\
    float zpos = val;\
    if ( zintime_ && d2tm_ )\
	zpos = d2tm_->getTime( zpos )*1000;\
    if ( zpos < zrg_.start )\
	continue;\
    else if ( zpos > zrg_.stop )\
	break;\
    if ( dispzinft_ && !zintime_)\
	zpos *= mToFeetFactor;
uiWellLogDisplay::LineData::LineData( uiGraphicsScene& scn, const uiBorder& b )
    : zrg_(mUdf(float),0)
    , xax_(&scn,uiAxisHandler::Setup(uiRect::Top).border(b))
    , yax_(&scn,uiAxisHandler::Setup(uiRect::Left).border(b))
    , valrg_(mUdf(float),0)
    , curvenmitm_(0)
{
}


uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, int idx,
       				    const uiBorder& b )
    : LineData(scn,b)
    , wl_(0)
    , wld_(Well::DisplayProperties::Log())		   
    , unitmeas_(0)
    , xrev_(false)
    , isyaxisleft_(true)	     
{
    if ( idx )
    {
	xax_.setup().side_ = uiRect::Bottom;
	yax_.setup().nogridline(true);
    }
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiGroup(p,"Well Log display viewer")
    , viewer_(new uiGraphicsView(this,"Well Log display viewer"))
    , setup_(su)
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeetByDefault())
    , zintime_(false)
    , markers_(0)
    , d2tm_(0)
{
    setStretch( 2, 2 );
    setPrefWidth( 250 );
    setPrefHeight( 700 );

    viewer_->getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiWellLogDisplay,mouseRelease) );

    viewer_->reSize.notify( mCB(this,uiWellLogDisplay,reSized) );
    viewer_->setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    viewer_->setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
    deepErase( lds_ );
    delete viewer_;
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
    viewer_->show();
}


void uiWellLogDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiWellLogDisplay::gatherInfo()
{
    if ( !lds_.size() ) return;
	
    setAxisRelations();
    for ( int idx=0; idx<lds_.size(); idx++ )
	gatherInfo( idx );

    if ( mIsUdf(zrg_.start)  && lds_[0]->wl_ )
    {
	zrg_ = lds_[0]->zrg_;
	for ( int idx=0; idx<lds_.size(); idx++ )
	{
	    if ( lds_[idx]->wl_ )
		zrg_.include( lds_[idx]->zrg_ );
	}
    }
	
    BufferString znm;
    if ( zintime_ )
	znm += "TWT", "(ms)";
    else
	znm += "MD ", dispzinft_ ? "(ft)" : "(m)";

    for ( int idx=0; idx<lds_.size(); idx++ )
    {
	setAxisRanges( idx );
	
	lds_[idx]->yax_.setup().side_ = lds_[idx]->isyaxisleft_ ? 
					uiRect::Left : uiRect::Right;

	lds_[idx]->xax_.setup().maxnumberdigitsprecision_ = 3;
	lds_[idx]->xax_.setup().epsaroundzero_ = 1e-5;
	lds_[idx]->yax_.setup().islog( lds_[idx]->wld_.islogarithmic_ );
	lds_[idx]->yax_.setup().name( znm );

	if ( lds_[idx]->wl_ ) 
	    lds_[idx]->xax_.setup().name( lds_[idx]->wl_->name() );
    }
}


void uiWellLogDisplay::setAxisRelations()
{
    for ( int idx=0; idx<lds_.size(); idx++ )
    {
	lds_[idx]->xax_.setBegin( setup_.noxpixbefore_ ? 0 : &lds_[idx]->yax_ );
	lds_[idx]->yax_.setBegin( setup_.noypixbefore_ ? 0 : &lds_[idx]->xax_ );
	lds_[idx]->xax_.setEnd(  setup_.noxpixafter_ ? 0 : &lds_[idx]->yax_ );
	lds_[idx]->yax_.setEnd( setup_.noypixafter_ ? 0 : &lds_[idx]->xax_ );
	
	lds_[idx]->xax_.setNewDevSize( viewer_->width(), viewer_->height() );
	lds_[idx]->yax_.setNewDevSize( viewer_->height(), viewer_->width() );
    }
}


void uiWellLogDisplay::gatherInfo( int idx )
{
    uiWellLogDisplay::LogData& ld = *lds_[idx];

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 )
    {
	if ( idx != 0 )
	{
	    ld.copySetupFrom( *lds_[0] );
	    ld.zrg_ = lds_[0]->zrg_;
	    ld.valrg_ = lds_[0]->valrg_;
	}
	return;
    }

    DataClipSampler dcs( sz );
    dcs.add( ld.wl_->valArr(), sz );
    ld.valrg_ = dcs.getRange( ld.wld_.cliprate_ );

    float startpos = ld.wl_->dah( 0 );
    float stoppos = ld.wl_->dah( sz-1 );
    if ( zintime_ && d2tm_ )
    {
	startpos = d2tm_->getTime( startpos )*1000;
	stoppos = d2tm_->getTime( stoppos )*1000;
    }

    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
}


void uiWellLogDisplay::setAxisRanges( int idx )
{
    uiWellLogDisplay::LogData& ld = *lds_[idx];
    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
    ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zrg_.stop, zrg_.start );
    if ( dispzinft_ && !zintime_ )
	dispzrg.scale( mToFeetFactor );
    ld.yax_.setBounds( dispzrg );

    if ( idx == 0 )
    {
	// Set default for others
	for ( int idx=1; idx<lds_.size(); idx++)
	{
	    lds_[idx]->xax_.setBounds( dispvalrg );
	    lds_[idx]->yax_.setBounds( dispzrg );
	}
    }
}


uiWellLogDisplay::LogData& uiWellLogDisplay::logData( int idx )
{
    if ( lds_.size()-1 < idx ) addLogData();
    return *lds_[idx];
}


uiWellLogDisplay::LogData& uiWellLogDisplay::addLogData()
{
    LogData* ld = new LogData( scene(), lds_.size(), setup_.border_ );
    lds_ += ld;
    return *ld;
}


void uiWellLogDisplay::draw()
{
    setAxisRelations();
    if ( mIsUdf(zrg_.start) ) return;

    for ( int idx=0; idx<lds_.size(); idx++)
    {
	lds_[idx]->xax_.plotAxis(); lds_[idx]->yax_.plotAxis();

	drawLog( idx );

	if ( lds_[idx]->wld_.islogfill_ )
	    drawFilling( idx );
    }

    drawMarkers();
    drawZPicks();
}


#define mRemoveSet( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
    scene().removeItem( itms[idx] ); \
    deepErase( itms );
void uiWellLogDisplay::drawLog( int logidx )
{
    uiWellLogDisplay::LogData& ld = *lds_[logidx];
    ld.al_ = Alignment(Alignment::HCenter, (logidx==-1) ? 
	    			Alignment::Top : Alignment::Bottom );
    drawLine( ld, ld.wl_ );
    
    if ( ld.unitmeas_ )
	ld.xax_.annotAtEnd( BufferString("(",ld.unitmeas_->symbol(),")") );
}


void uiWellLogDisplay::drawLine( LogData& ld, const Well::DahObj* wd )
{
    if ( !wd ) return;

    mRemoveSet( ld.curveitms_ );
    scene().removeItem( ld.curvenmitm_ );
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;

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


void uiWellLogDisplay::drawFilling( int idx )
{
    uiWellLogDisplay::LogData& ld = *lds_[idx];

    mRemoveSet( ld.curvepolyitms_ );
    float colstep = ( ld.xax_.range().stop - ld.xax_.range().start ) / 255;

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;
    closept.x = ld.xrev_ ? ld.xax_.getPix(ld.xax_.range().stop) 
		     	 : ld.xax_.getPix(ld.xax_.range().start);
    closept.y = ld.xrev_ ? ld.yax_.getPix(ld.yax_.range().stop) 
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

	const int colindex = (int)((val-ld.xax_.range().start)/colstep);
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
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y ); 
    if ( pts.isEmpty() ) return;

    const int tabidx = ColTab::SM().indexOf( ld.wld_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color = seq->color( (float)colorintset[idx]/255 );
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
    }

    deepErase( pts );
}


#define mDefHorLineX1X2Y() \
	const int x1 = lds_.size() ? lds_[0]->xax_.getRelPosPix( 0 ) : 0; \
	const int x2 = lds_.size()>1 ? lds_[1]->xax_.getRelPosPix( 1 ) \
				     : lds_[0]->xax_.getRelPosPix( 1 ); \
	const int y = lds_[0]->yax_.getPix( zpos )

void uiWellLogDisplay::drawMarkers()
{
    mRemoveSet( markeritms_ );
    mRemoveSet( markertxtitms_ );
    if ( !markers_ ) return;

    for ( int idx=0; idx<markers_->size(); idx++ )
    {
	const Well::Marker& mrkr = *((*markers_)[idx]);
	if ( mrkr.color() == Color::NoColor() ) continue;

	mDefZPosInLoop( mrkr.dah() )
	mDefHorLineX1X2Y();

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(setup_.markerls_.type_,
		    		   setup_.markerls_.width_,mrkr.color()) );
	markeritms_ += li;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	    mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
			 new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( mrkr.color() );
	markertxtitms_ += ti;
    }
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


void uiWellLogDisplay::removeLog( const char* logname )
{
    int logidx = 0;
    for ( int idx=0; idx<lds_.size(); idx++ )
    {
	if ( !strcmp( lds_[idx]->wl_->name(), logname ) )
	{ logidx=idx; break; }
    }
    if ( !lds_[logidx] ) return;

    mRemoveSet( lds_[logidx]->curveitms_ )
    scene().removeItem( lds_[logidx]->curvenmitm_ );
    delete lds_[logidx]->curvenmitm_; lds_[logidx]->curvenmitm_ = 0;
    lds_[logidx]->wl_ = 0;
}


void uiWellLogDisplay::removeAllLogs()
{
    for ( int logidx=0; logidx<lds_.size(); logidx++ )
    {
	mRemoveSet( lds_[logidx]->curveitms_ )
	scene().removeItem( lds_[logidx]->curvenmitm_ );
    }
    deepErase( lds_ );
}


void uiWellLogDisplay::mouseRelease( CallBacker* )
{}



uiWellDisplay::uiWellDisplay( uiParent* p, const Setup& s, const Well::Data& w )
    	: uiGraphicsView(p,"Well Log Viewer")
	, leftlogdisp_(0)
	, rightlogdisp_(0)
	, leftlogitm_(0)
	, rightlogitm_(0)
	, wd_(Well::Data(w))
	, td_(scene(),uiBorder(0))
	, zrg_(mUdf(float),0)
	, d2tm_(wd_.d2TModel())
   	, zintime_(false)		     
   	, dispzinft_(false)		     
{
    setStretch( 2, 2 );
    setPrefWidth( 2*mPanelWidth + 20 );
    setPrefHeight( mPanelHeight + 20 );

    if ( s.left_ )  addLogPanel( true );
    if ( s.right_ ) addLogPanel( false );

    addLog( wd_.displayProperties().left_.name_, true );
    addLog( wd_.displayProperties().right_.name_, false );

    propdlg_ = new uiWellDispPropDlg( p, &wd_ );

    uiPushButton* propbut = new uiPushButton( 0, "&Property",
			     mCB(this,uiWellDisplay,propButPushed), true );
    uiObjectItem* propitm  = scene_->addItem( new uiObjectItem( propbut ) );
    propitm->setPos( mPanelWidth, mPanelHeight );
    
    td_.wt_ = &wd_.track(); 
    td_.wtd_ = wd_.displayProperties().track_; 

    wd_.dispparschanged.notify(mCB(this,uiWellDisplay,updateProperties));
    updateProperties(0);
}


uiWellDisplay::~uiWellDisplay()
{
    delete leftlogdisp_; 
    delete rightlogdisp_; 
    delete propdlg_;
}


void uiWellDisplay::addLogPanel( bool isleft )
{
    uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
    wldsu.noxpixafter_ = isleft; 	wldsu.noxpixbefore_ = !isleft;
    wldsu.border_.setLeft(0); wldsu.border_.setRight(0);
    uiWellLogDisplay* logdisp = new uiWellLogDisplay( 0, wldsu );
    uiObjectItem* logitm  = scene_->addItem( new uiObjectItem( logdisp ) );

    if ( isleft )
    { leftlogdisp_ = logdisp;  leftlogitm_  = logitm; }
    else
    { rightlogdisp_ = logdisp; rightlogitm_ = logitm; }

    logdisp->setWellData( wd_ );
    
    logitm->setPos( ( isleft || !leftlogdisp_ ) ? 0 : mPanelWidth );
    logitm->setSelectable( true );
    logitm->setObjectSize( mPanelWidth, mPanelHeight );
}


void uiWellDisplay::removeLogPanel( bool isleft )
{
    uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
    wldsu.noxpixafter_ = isleft; 	wldsu.noxpixbefore_ = !isleft;
    wldsu.border_.setLeft(0); wldsu.border_.setRight(0);
    uiWellLogDisplay* logdisp = new uiWellLogDisplay( 0, wldsu );
    uiObjectItem* logitm  = scene_->addItem( new uiObjectItem( logdisp ) );

    if ( isleft )
    { leftlogdisp_ = logdisp;  leftlogitm_  = logitm; }
    else
    { rightlogdisp_ = logdisp; rightlogitm_ = logitm; }

    logdisp->setWellData( wd_ );
    
    logitm->setPos( ( isleft || !leftlogdisp_ ) ? 0 : mPanelWidth );
    logitm->setSelectable( true );
    logitm->setObjectSize( mPanelWidth, mPanelHeight );
}


void uiWellDisplay::setAxisRanges()
{
    Interval<float> dispzrg( zrg_.stop, zrg_.start );
    if ( dispzinft_ && !zintime_ )
	dispzrg.scale( mToFeetFactor );

    if ( td_.wt_ )
    {
	td_.yax_.setBounds( dispzrg  );
	td_.xax_.setBounds( Interval<float>(0,0) );
    }
}


void uiWellDisplay::addLog( const char* logname, bool left )
{
    const Well::Log* l = wd_.logs().getLog( logname );
    if ( !l ) return;
	
    if ( left && leftlogdisp_ )
    {
	uiWellLogDisplay::LogData& wldld = leftlogdisp_->addLogData();
	wldld.wl_ = l;
	wldld.xrev_ = true;
    }
    else if ( !left && rightlogdisp_ )
    {
	uiWellLogDisplay::LogData& wldld = rightlogdisp_->addLogData();
	wldld.wl_ = l;
	wldld.xrev_ = false;
	wldld.isyaxisleft_ = false;
    }
}


void uiWellDisplay::removeLog( const char* logname, bool left )
{
    if ( left && leftlogdisp_ )
	leftlogdisp_->removeLog( logname );
    else if ( !left && rightlogdisp_ )
	rightlogdisp_->removeLog( logname );
}


void uiWellDisplay::dataChanged( CallBacker* cb )
{
    //gatherInfo();
    //drawTrack();

    if ( leftlogdisp_ ) leftlogdisp_->dataChanged();
    if ( rightlogdisp_ ) rightlogdisp_->dataChanged();
}


void uiWellDisplay::gatherInfo()
{
    float startpos = td_.wt_->dah( 0 );
    float stoppos = td_.wt_->dah( td_.wt_->size()-1 );
    
    if ( zintime_ && d2tm_ )
    {
	startpos = d2tm_->getTime( startpos )*1000;
	stoppos = d2tm_->getTime( stoppos )*1000;
    }

    td_.zrg_.start = startpos;
    td_.zrg_.stop = stoppos;
    
    setAxisRanges();
}

//TODO put together with drawLogLine
void uiWellDisplay::drawTrack()
{
    mRemoveSet( td_.curveitms_ );
    scene().removeItem( td_.curvenmitm_ );
    delete td_.curvenmitm_; td_.curvenmitm_ = 0;

    const Well::DahObj* wd = td_.wt_;

    const int sz = wd ? wd->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;

    LineStyle ls(LineStyle::Solid);
    ls.width_ = td_.wtd_.size_;
    ls.color_ = td_.wtd_.color_;
    
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
	*curpts += uiPoint( td_.xax_.getPix(val), td_.yax_.getPix(zpos) );
    }
    if ( curpts->isEmpty() )
	delete curpts;
    else
	pts += curpts;
    if ( pts.isEmpty() ) return;

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem(*pts[idx]) );
	pli->setPenStyle( ls );
	td_.curveitms_ += pli;
    }

    td_.yax_.annotAtEnd( zintime_ ? "(ms)" : dispzinft_ ? "(ft)" : "(m)" );
}


void uiWellDisplay::propButPushed( CallBacker* )
{
    propdlg_->go();
}


void uiWellDisplay::updateProperties( CallBacker* cb )
{
    //TODO link to several tabstacks...
    td_.wtd_ = wd_.displayProperties().track_;
    if ( leftlogdisp_ && leftlogdisp_->lds_.size() ) 
	leftlogdisp_->lds_[0]->wld_ = wd_.displayProperties().left_;
    if ( rightlogdisp_ && rightlogdisp_->lds_.size() ) 
	rightlogdisp_->lds_[0]->wld_ = wd_.displayProperties().right_;
    dataChanged( cb );
}




uiWellDisplayWin::uiWellDisplayWin( uiParent* p, const Well::Data& wd )
	: uiMainWin(p,"2D Well Log Viewer")
{
    logviewer_ = new uiWellDisplay( this, uiWellDisplay::Setup(), wd );

    finaliseDone.notify( mCB(logviewer_,uiWellDisplay,dataChanged) );
}


uiWellDisplayWin::~uiWellDisplayWin()
{
    delete logviewer_;
}


