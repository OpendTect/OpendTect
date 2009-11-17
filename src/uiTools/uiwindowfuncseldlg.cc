/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.37 2009-11-17 08:25:18 cvsbruno Exp $";


#include "uiwindowfuncseldlg.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "uiaxishandler.h"
#include "uicanvas.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uiworld2ui.h"
#include "iodraw.h"
#include "randcolor.h"
#include "scaler.h"
#include "survinfo.h"
#include "windowfunction.h"

#define mTransHeight    250
#define mTransWidth     500

uiFunctionDrawer::uiFunctionDrawer( uiParent* p, const Setup& su )
    : uiGraphicsView( p, "" )
    , transform_(new uiWorld2Ui())
    , polyitemgrp_(0)
    , borderrectitem_(0)
    , funcrg_(su.funcrg_)
{
    setPrefHeight( mTransHeight );
    setPrefWidth( mTransWidth );
    setStretch( 2, 2 );

    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect( su.xaxrg_.start, su.yaxrg_.stop, 
				  su.xaxrg_.stop, su.yaxrg_.start ) );

    uiAxisHandler::Setup asu( uiRect::Bottom, width(), height() );
    asu.style( LineStyle::None );
    asu.maxnumberdigitsprecision_ = 3;
    asu.epsaroundzero_ = 1e-3;
    asu.border_ = uiBorder(10,10,10,10);

    float annotstart = -1;
    xax_ = new uiAxisHandler( &scene(), asu );
    xax_->setRange( su.xaxrg_, &annotstart );

    asu.side( uiRect::Left ); asu.islog_ = false;
    yax_ = new uiAxisHandler( &scene(), asu );
    yax_->setRange( StepInterval<float>(0,1,0.25),0 );

    xax_->setBegin( yax_ ); 		yax_->setBegin( xax_ );
    xax_->setBounds( su.xaxrg_ ); 	yax_->setBounds( su.yaxrg_ );
    xax_->setName( su.xaxname_ ); 	yax_->setName( su.yaxname_ );

    reSize.notify( mCB( this, uiFunctionDrawer, draw ) );
}


void uiFunctionDrawer::setUpAxis()
{
    xax_->setNewDevSize( width(), height() );
    yax_->setNewDevSize( height(), width() );
    xax_->plotAxis();
    yax_->plotAxis();
}


uiFunctionDrawer::~uiFunctionDrawer()
{
    delete transform_;
    clearFunctions();
}


void uiFunctionDrawer::setFrame()
{
    uiRect borderrect( xax_->getPix( xax_->range().start ), 
	    	       yax_->getPix( yax_->range().stop ), 
		       xax_->getPix( xax_->range().stop ), 
		       yax_->getPix( yax_->range().start ) );
    if ( !borderrectitem_ )
	borderrectitem_ = scene().addRect(
		borderrect.left(), borderrect.top(), borderrect.width(),
		borderrect.height() );
    else
	borderrectitem_->setRect( borderrect.left(), borderrect.top(),
				  borderrect.width(), borderrect.height() );
    borderrectitem_->setPenStyle( LineStyle() );
    borderrect.setTop( borderrect.top() + 3 );
    transform_->resetUiRect( borderrect );
}


void uiFunctionDrawer::draw( CallBacker* )
{
    setUpAxis();
    setFrame();

    if ( !polyitemgrp_ )
    {
	polyitemgrp_ = new uiGraphicsItemGroup();
	scene().addItemGrp( polyitemgrp_ );
    }
    else
	polyitemgrp_->removeAll( true );

    if ( !selitemsidx_.size() && functions_.size() )
	selitemsidx_ += 0;
    
    for ( int idx=0; idx<selitemsidx_.size(); idx++ )
    {
	const int selidx = selitemsidx_[idx];
	DrawFunction* func = functions_[selidx];
	if ( !func ) return;
	createLine( func );
	uiPolyLineItem* polyitem = new uiPolyLineItem();
	polyitem->setPolyLine( func->pointlist_ );
	LineStyle ls;
	ls.width_ = 2;
	ls.color_ = func->color_;
	polyitem->setPenStyle( ls );
	polyitemgrp_->add( polyitem );
    }
}


void uiFunctionDrawer::createLine( DrawFunction* func )
{
    if ( !func ) return;
    TypeSet<uiPoint>& pointlist = func->pointlist_;
    pointlist.erase();

    LinScaler scaler( funcrg_.start, xax_->range().start,
		      funcrg_.stop, xax_->range().stop );

    StepInterval<float> xrg( funcrg_ );
    xrg.step = 0.0001;
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	float x = xrg.atIndex( idx );
	const float y = func->mathfunc_->getValue( x );
	x = scaler.scale( x );
	const int xpix = xax_->getPix( x );
	const int ypix = yax_->getPix( y );
	pointlist += uiPoint( transform_->transform( uiWorldPoint(x,y) ) );
    }
}



uiFuncSelDraw::uiFuncSelDraw( uiParent* p, const uiFunctionDrawer::Setup& su )
    : uiGroup(p)
    , funclistselChged(this)
{
    funclistfld_ = new uiListBox( this );
    funclistfld_->attach( topBorder, 0 );
    funclistfld_->setMultiSelect();
    funclistfld_->selectionChanged.notify( mCB(this,uiFuncSelDraw,funcSelChg) );
    
    view_ = new uiFunctionDrawer( this, su );
    view_->attach( rightOf, funclistfld_ );
}


int uiFuncSelDraw::getListSize() const
{
    return funclistfld_->size();
}


int uiFuncSelDraw::getNrSel() const
{
    return funclistfld_->nrSelected();
}


void uiFuncSelDraw::setAsCurrent( const char* fcname )
{
    funclistfld_->setCurrentItem( fcname );
}


int uiFuncSelDraw::removeLastItem()
{
    const int curidx = funclistfld_->size()-1;
    funclistfld_->removeItem( curidx );
    mathfunc_.remove( curidx );
    view_->clearFunction( curidx );
    return curidx;
}


void uiFuncSelDraw::removeItem( int idx )
{
    funclistfld_->removeItem( idx );
    mathfunc_.remove( idx );
    view_->clearFunction( idx );
}


void uiFuncSelDraw::funcSelChg( CallBacker* cb )
{
    funclistselChged.trigger();
    
    TypeSet<int> selecteditems;
    funclistfld_->getSelectedItems( selecteditems );

    view_->setSelItems( selecteditems );
    view_->draw( cb );
}


void uiFuncSelDraw::addFunction( const char* fcname, FloatMathFunction* mfunc, 					bool withcolor )
{ 
    if ( !mfunc ) return; 
    mathfunc_ += mfunc;

    const int curidx = funclistfld_->size();
    const Color& col = withcolor ? Color::stdDrawColor( curidx ) 
				 : Color::Black();
    colors_ += col;
    funclistfld_->addItem( fcname, col );

    uiFunctionDrawer::DrawFunction* drawfunction = 
			new uiFunctionDrawer::DrawFunction( mfunc );
    drawfunction->color_ = colors_[curidx];  
    view_->addFunction( drawfunction );
}


void uiFuncSelDraw::getSelectedItems( TypeSet<int>& selitems ) const
{ 
    return funclistfld_->getSelectedItems( selitems );
}


bool uiFuncSelDraw::isSelected( int idx) const
{ 
    return funclistfld_->isSelected(idx);
}


void uiFuncSelDraw::setSelected( int idx ) 
{ 
    funclistfld_->setSelected(idx);
}


const char* uiFuncSelDraw::getCurrentListName() const
{
    if ( funclistfld_->nrSelected() == 1 )
	return funclistfld_->textOfItem( funclistfld_->currentItem() );
    return 0;
}



const char* tapertxt_[] = { "Taper Length (%)", "Slope (dB/Octave)", 0 };
uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p, const char* winname, 
					float variable )
    : uiDialog( p, uiDialog::Setup("Window/Taper display",0,mNoHelpID) )
    , variable_(variable)
    , funcdrawer_(0)			 
{
    setCtrlStyle( LeaveOnly );
  
    uiFunctionDrawer::Setup su;
    funcdrawer_ = new uiFuncSelDraw( this, su );
    funcnames_ = WinFuncs().getNames();

    for ( int idx=0; idx<funcnames_.size(); idx++ )
    {
	winfunc_ += WinFuncs().create( funcnames_[idx]->buf() );
	funcdrawer_->addFunction( funcnames_[idx]->buf(), winfunc_[idx] );
    }

    funcdrawer_->funclistselChged.notify(mCB(this,uiWindowFuncSelDlg,funcSelChg));

    varinpfld_ = new uiGenInput( this, tapertxt_[0], FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, funcdrawer_ );
    varinpfld_->setValue( variable_ * 100 );
    varinpfld_->valuechanged.notify( mCB(this,uiWindowFuncSelDlg,funcSelChg) );

    setCurrentWindowFunc( winname, variable_ );
}


void uiWindowFuncSelDlg::funcSelChg( CallBacker* )
{
    NotifyStopper nsf( funcdrawer_->funclistselChged );
    bool isvartappresent = false;

    TypeSet<int> selitems; 
    funcdrawer_->getSelectedItems( selitems );
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const BufferString& winname = funcnames_[selitems[idx]]->buf();
 	WindowFunction* wf = getWindowFuncByName( winname );
	if ( wf && wf->hasVariable() )
	{
	    isvartappresent = true;
	    float prevvariable = variable_;
	    variable_ = mIsUdf(variable_) ? 0.05 : varinpfld_->getfValue(0)/100;
	    if ( variable_ > 1 || mIsUdf(variable_) )
		variable_ = prevvariable; 
	    wf->setVariable( 1.0 - variable_ );
	    varinpfld_->setValue( variable_ *100 );
	}
    }

    varinpfld_->display( isvartappresent );
    funcdrawer_->funcSelChg(0);
}


float uiWindowFuncSelDlg::getVariable()
{
    const WindowFunction* wf = getWindowFuncByName( getCurrentWindowName() );
    if ( wf && wf->hasVariable() )
	return variable_;
    return -1;
}


void uiWindowFuncSelDlg::setVariable( float variable )
{
    float prevvariable = variable_;
    variable_ = variable;
    if ( variable_ > 1 )
    {
	varinpfld_->setValue( prevvariable * 100 );
	variable_ = prevvariable; 
    }
    else
	varinpfld_->setValue( variable_ * 100 );

    funcSelChg(0);
}


void uiWindowFuncSelDlg::setCurrentWindowFunc( const char* nm, float variable )
{
    variable_ = variable;
    funcdrawer_->setAsCurrent( nm );
    funcSelChg(0); 
}


WindowFunction* uiWindowFuncSelDlg::getWindowFuncByName( const char* nm )
{
    const int idx = funcnames_.indexOf(nm);
    if ( idx >=0 )
	return winfunc_[funcnames_.indexOf(nm)];
    return 0;
}


const char* uiWindowFuncSelDlg::getCurrentWindowName() const
{
    return funcdrawer_->getCurrentListName();
}


static const char* winname = "CosTaper";
#define mGetData() isminactive_ ? dd1_ : dd2_; 
#define mCheckLimitRanges()\
    dd1_.freqrg_.limitTo( Interval<float>( 0.01, dd1_.freqrg_.stop ) );\
    dd2_.freqrg_.limitTo( Interval<float>( dd2_.freqrg_.start+0.01,datasz_) );\
    if ( mIsZero(dd2_.freqrg_.start-dd2_.freqrg_.stop,0.1))\
	dd2_.freqrg_.stop += 0.1;\
    if ( mIsZero(dd2_.freqrg_.start-dd2_.freqrg_.stop,0.1))\
	dd2_.freqrg_.stop += 0.1;
uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const Setup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mNoHelpID) )
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , datasz_((int)(0.5/(SI().zStep())))
    , winvals_(0)				
{
    setCtrlStyle( LeaveOnly );
    setHSpacing( 35 );

    dd1_.freqrg_ = s.minfreqrg_;   
    dd2_.freqrg_ = s.maxfreqrg_; 
    mCheckLimitRanges();

    uiFunctionDisplay::Setup su;
    su.noxgridline_ = true;  su.noygridline_ = true; 
    su.ywidth_ = 2; 

    drawer_ = new uiFunctionDisplay( this, su );
    drawer_->xAxis()->setName( "Frequency (Hz)" ); 	
    drawer_->yAxis(false)->setName( "Gain (dB)" );
    
    varinpfld_ = new uiGenInput( this, tapertxt_[1], FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, drawer_ );
    varinpfld_->setTitleText ( tapertxt_[1] );
    varinpfld_->setValue( dd1_.variable_ );
    varinpfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, getFromScreen));
    varinpfld_->valuechanged.notify( 
	    		mCB( this, uiFreqTaperDlg, setFreqFromPercents ) );
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperDlg, taperChged) );

    freqrgfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency"),
				    FloatInpSpec().setName("Max frequency") );
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, getFromScreen));
    freqrgfld_->valuechanged.notify( mCB( 
			    this, uiFreqTaperDlg, setPercentsFromFreq ) );
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, taperChged  ) );
    freqrgfld_->attach( rightOf, varinpfld_ );
    
    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperDlg,freqChoiceChged) );
	freqinpfld_->valuechanged.notify( mCB(this,uiFreqTaperDlg,taperChged) );
	freqinpfld_->attach( ensureBelow, freqrgfld_ );
	freqinpfld_->attach( centeredBelow, drawer_ );
    }
 
    setPercentsFromFreq(0);
    winvals_ = new Array1DImpl<float>(datasz_);					
    dd1_.winsz_ = 2*( (int)dd1_.freqrg_.stop );
    dd2_.winsz_ = 2*( datasz_ - (int)dd2_.freqrg_.start );
    dd1_.window_ = new ArrayNDWindow( 
			    Array1DInfoImpl(dd1_.winsz_), false, winname,0 );
    dd2_.window_ = new ArrayNDWindow( 
			    Array1DInfoImpl(dd2_.winsz_), false, winname,0 );
   
    freqChoiceChged(0);
    finaliseDone.notify( mCB(this, uiFreqTaperDlg, taperChged ) );
}


uiFreqTaperDlg::~uiFreqTaperDlg()
{
    delete winvals_;
    delete dd1_.window_; 
    delete dd2_.window_; 
}


void uiFreqTaperDlg::getFromScreen( CallBacker* )
{
    DrawData& dd = mGetData();
    setFreqFromSlope( varinpfld_->getfValue() );
    dd.freqrg_ = freqrgfld_->getFInterval();
    setPercentsFromFreq(0);
    
    mCheckLimitRanges();
}


#define setToNearestInt(val)\
{\
    int ifr = mNINT( val  );\
    if ( mIsZero(val-ifr,1e-2) )\
	val = ifr;\
}
void uiFreqTaperDlg::putToScreen( CallBacker* )
{
    NotifyStopper nsf1( varinpfld_->valuechanged );
    NotifyStopper nsf2( freqrgfld_->valuechanged );

    DrawData& dd = mGetData();
    setToNearestInt( dd.freqrg_.start ); 
    setToNearestInt( dd.freqrg_.stop );
    setPercentsFromFreq(0);
    varinpfld_->setValue( getSlopeFromFreq() );
    freqrgfld_->setValue( dd.freqrg_ );
    
    freqrgfld_->setSensitive( hasmin_ && isminactive_, 0, 0 );
    freqrgfld_->setSensitive( hasmax_ && !isminactive_, 0, 1 );
} 


void uiFreqTaperDlg::setFreqFromPercents( CallBacker* )
{
    DrawData& d = mGetData();

    if ( isminactive_ )
	d.freqrg_.start = d.freqrg_.stop * d.variable_;
    else
	d.freqrg_.stop = ( datasz_-d.freqrg_.start)*d.variable_
		       + d.freqrg_.start;
}


void uiFreqTaperDlg::setPercentsFromFreq( CallBacker* )
{
    NotifyStopper nsf( freqrgfld_->valuechanged );
    dd1_.variable_ = dd1_.freqrg_.start / dd1_.freqrg_.stop;
    dd2_.variable_ = ( dd2_.freqrg_.stop-dd2_.freqrg_.start )
		   / ( datasz_-dd2_.freqrg_.start );
}


#define mDec2Oct 0.301029996 //log(2)
void uiFreqTaperDlg::setFreqFromSlope( float slope )
{
    NotifyStopper nsf( freqrgfld_->valuechanged );
    const float slopeindecade = (float)(slope/mDec2Oct);
    const float slopeinhertz = pow( 10, 1/slopeindecade );
    DrawData& dd = mGetData();

    if ( isminactive_ )
	dd1_.freqrg_.start = dd.freqrg_.stop/slopeinhertz;
    else
	dd2_.freqrg_.stop = dd.freqrg_.start*slopeinhertz;
    mCheckLimitRanges();
}


float uiFreqTaperDlg::getSlopeFromFreq()
{
    DrawData& d = mGetData();
    float slope = fabs( 1/Math::Log10( d.freqrg_.stop / d.freqrg_.start ) );
    slope *= mDec2Oct;
    return slope;
}


void uiFreqTaperDlg::taperChged( CallBacker* cb )
{
    dd1_.window_->setType(  winname , dd1_.variable_ );
    dd2_.window_->setType(  winname , 1-dd2_.variable_ );
    
    TypeSet<float> xvals;
    for ( int idx=0; idx<datasz_; idx++ )
    {
	float val = 1;
	if ( hasmin_ && idx<(int)dd1_.freqrg_.stop )
	    val = 1 - dd1_.window_->getValues()[dd1_.winsz_/2+idx];

	if ( hasmax_ && idx>(int)dd2_.freqrg_.start ) 
	    val = 1- dd2_.window_->getValues()[idx-(int)dd2_.freqrg_.start];
	    
	winvals_->set( idx, val );
	xvals += idx; 
    }
    drawer_->setVals( xvals.arr(), winvals_->getData(), datasz_ );
    putToScreen(0);
}


void uiFreqTaperDlg::freqChoiceChged( CallBacker* )
{
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;
    taperChged(0);
    getFromScreen(0);
}


void uiFreqTaperDlg::setFreqRange( Interval<float> fqrg )
{ 
    dd1_.freqrg_.start = fqrg.start;
    dd2_.freqrg_.stop = fqrg.stop;
    mCheckLimitRanges()
    putToScreen(0); 
    setPercentsFromFreq(0);
    taperChged(0); 
}


Interval<float> uiFreqTaperDlg::getFreqRange() const
{
    return Interval<float> ( dd1_.freqrg_.start, dd2_.freqrg_.stop );
} 

