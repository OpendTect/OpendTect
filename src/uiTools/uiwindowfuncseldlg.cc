/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.23 2009-10-14 14:37:32 cvsbruno Exp $";


#include "uiwindowfuncseldlg.h"

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
#include "windowfunction.h"

#define mTransHeight    250
#define mTransWidth     500


uiFunctionDrawer::uiFunctionDrawer( uiParent* p, const Setup& su )
    : uiFunctionDisplay( p, uiFunctionDisplay::Setup() )
    , transform_(new uiWorld2Ui())
    , polyitemgrp_(0)
    , borderrectitem_(0)
    , funcrg_(su.funcrg_)
{
    setPrefHeight( mTransHeight );
    setPrefWidth( mTransWidth );
    setStretch(0,0);

    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect( su.xaxrg_.start, su.yaxrg_.stop, 
				  su.xaxrg_.stop, su.yaxrg_.start ) );
    uiBorder border(10,10,10,10);
    
    uiAxisHandler::Setup asu( uiRect::Bottom, width(), height() );
    asu.style( LineStyle::None );

    asu.maxnumberdigitsprecision_ = 2;
    asu.epsaroundzero_ = 1e-3;
    asu.border_ = border;

    xax_ = new uiAxisHandler( &scene(), asu );
    float annotstart = -1;
    xax_->setRange( su.xaxrg_, &annotstart );

    asu.side( uiRect::Left ); asu.islog_ = false;
    yax_ = new uiAxisHandler( &scene(), asu );
    yax_->setRange( su.yaxrg_, 0 );

    xax_->setName( su.xaxname_ ); 	yax_->setName( su.yaxname_ );
    xax_->setBegin( yax_ ); 		yax_->setBegin( xax_ );
    xax_->setBounds( su.xaxrg_ ); 	yax_->setBounds( su.yaxrg_ );
}


uiFunctionDrawer::~uiFunctionDrawer()
{
    delete transform_;
    pointlistset_.erase();
    linesetcolor_.erase();
}


void uiFunctionDrawer::draw( TypeSet<int>& selecteditems )
{
    xax_->setNewDevSize( mTransWidth, mTransHeight );
    yax_->setNewDevSize( mTransHeight , mTransWidth );
    xax_->plotAxis();
    yax_->plotAxis();
    uiRect borderrect( xax_->pixBefore(), 5, mTransWidth - 5,
	    	       mTransHeight - yax_->pixBefore() );
    if ( !borderrectitem_ )
	borderrectitem_ = scene().addRect(
		borderrect.left(), borderrect.top(), borderrect.width(),
		borderrect.height() );
    else
	borderrectitem_->setRect( borderrect.left(), borderrect.top(),
				  borderrect.width(), borderrect.height() );
    borderrectitem_->setPenStyle( LineStyle() );
    const int selsz = pointlistset_.size();
    if ( !polyitemgrp_ )
    {
	polyitemgrp_ = new uiGraphicsItemGroup();
	scene().addItemGrp( polyitemgrp_ );
    }
    else
	polyitemgrp_->removeAll( true );
    for ( int idx=0; idx<pointlistset_.size(); idx++ )
    {
	uiPolyLineItem* polyitem = new uiPolyLineItem();
	polyitem->setPolyLine( pointlistset_[idx] );
	LineStyle ls;
	ls.width_ = 2;
	ls.color_ = linesetcolor_[ selecteditems[idx] ];
	polyitem->setPenStyle( ls );
	polyitemgrp_->add( polyitem );
    }
}


void uiFunctionDrawer::createLine( const FloatMathFunction* mathfunc )
{
    if ( !mathfunc ) return;
    TypeSet<uiPoint> pointlist;

    uiRect borderrect( xax_->pixBefore(), 10, mTransWidth - 10,
	    	       mTransHeight - yax_->pixBefore() );
    transform_->resetUiRect( borderrect );

    StepInterval<float> xrg( funcrg_ );
    xrg.step = 0.0001;

    LinScaler scaler( funcrg_.start, xax_->range().start,
		      funcrg_.stop, xax_->range().stop );

    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	float x = xrg.atIndex( idx );
	const float y = mathfunc->getValue( x );
	x = scaler.scale( x );
	pointlist += uiPoint( transform_->transform(uiWorldPoint(x,y)) );
    }

    pointlistset_ += pointlist;
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


void uiFuncSelDraw::addToList( const char* fcname, bool withcolor )
{
    const int curidx = funclistfld_->size();
    const Color& col = withcolor? Color::stdDrawColor( curidx ):Color::Black();
    view_->addColor( col );
    funclistfld_->addItem( fcname, col );
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
    return curidx;
}


void uiFuncSelDraw::removeItem( int idx )
{
    funclistfld_->removeItem( idx );
    mathfunc_.remove( idx );
}


void uiFuncSelDraw::funcSelChg( CallBacker* )
{
    funclistselChged.trigger();
    view_->erasePoints();
    for ( int idx=0; idx<funclistfld_->size(); idx++ )
    {
	if ( !funclistfld_->isSelected(idx) )
	    continue;
	view_->createLine( mathfunc_[idx] );
    }

    TypeSet<int> selecteditems;
    funclistfld_->getSelectedItems( selecteditems );
    view_->draw( selecteditems );
}


void uiFuncSelDraw::addFunction( FloatMathFunction* mfunc )
{ 
    if (!mfunc ) return; 
    mathfunc_ += mfunc; 
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
	funcdrawer_->addToList( funcnames_[idx]->buf());
	funcdrawer_->addFunction( winfunc_[idx]  );
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
    //canvas_->update();
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


#define mGetData() isminactive_ ? dd1_ : dd2_; 
uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const Setup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mNoHelpID) )
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
{
    setCtrlStyle( LeaveOnly );
    setHSpacing( 35 );
  
    winfunc_ = WinFuncs().create( "CosTaper" );

    uiFunctionDrawer::Setup su;
    dd1_.funcrg_.set( -1.2, 0 ); 
    dd1_.xaxrg_ = s.minfreqrg_; 
    su.xaxrg_ = dd1_.xaxrg_;
    dd1_.freqrg_ = s.minfreqrg_; 
    drawers_ += new uiFunctionDrawer( this, su );

    dd2_.funcrg_.set( 0, 1.2 );
    dd2_.xaxrg_ = s.maxfreqrg_;
    su.xaxrg_ = dd2_.xaxrg_;
    dd2_.freqrg_ = s.maxfreqrg_; 
    drawers_ += new uiFunctionDrawer( this, su );

    su.xaxname_= "Frequency (Hz)"; 
    su.yaxname_ = "Gain (dB)";

    varinpfld_ = new uiGenInput( this, tapertxt_[1], FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, drawers_[0] );
    varinpfld_->attach( ensureBelow, drawers_[1] );
    varinpfld_->setTitleText ( tapertxt_[1] );
    varinpfld_->valuechanged.notify( 
	    		mCB( this, uiFreqTaperDlg, setFreqFromPercents ) );
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperDlg, taperChged) );

    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperDlg,freqChoiceChged) );
	freqinpfld_->valuechanged.notify( mCB(this,uiFreqTaperDlg,taperChged) );
	freqinpfld_->attach( leftAlignedAbove, drawers_[0] );
	freqinpfld_->attach( leftAlignedAbove, drawers_[1] );
    }

    freqrgfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency"),
				    FloatInpSpec().setName("Max frequency") );
    freqrgfld_->valuechanged.notify( mCB( 
			    this, uiFreqTaperDlg, setPercentsFromFreq ) );
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, taperChged  ) );
    freqrgfld_->attach( rightOf, varinpfld_ );
    
    const Color& col = Color::DgbColor();
    drawers_[0]->addColor( col );	  drawers_[1]->addColor( col );
    drawers_[0]->display( isminactive_ ); drawers_[1]->display( !isminactive_ );

    initAll(0);
}

void uiFreqTaperDlg::initAll( CallBacker* )
{
    getFromScreen(0);
    setFreqFromPercents(0);;
    setViewRanges();
    taperChged(0);
}


void uiFreqTaperDlg::getFromScreen( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.variable_ = varinpfld_->getfValue();
    dd.freqrg_ = freqrgfld_->getFInterval();
}


void uiFreqTaperDlg::putToScreen( CallBacker* )
{
    NotifyStopper nsf1( varinpfld_->valuechanged );
    NotifyStopper nsf2( freqrgfld_->valuechanged );

    DrawData& dd = mGetData();
    varinpfld_->setValue( dd.variable_ );
    freqrgfld_->setValue( dd.freqrg_ );
} 


void uiFreqTaperDlg::setFreqFromPercents(CallBacker*)
{
    getFromScreen(0);
    setViewRanges();
    LinScaler scaler;
    DrawData& d = mGetData();
    scaler.set( d.funcrg_.start, d.xaxrg_.start, d.funcrg_.stop, d.xaxrg_.stop);

    float freqstart, freqstop;
    if ( isminactive_ )
    {
	freqstart = scaler.scale( -1 );
	freqstop = d.xaxrg_.stop;
    }
    else
    {
	freqstart = d.xaxrg_.start;
	freqstop = scaler.scale( 1 );
    }
    d.freqrg_.set( freqstart, freqstop );
}


void uiFreqTaperDlg::setPercentsFromFreq(CallBacker*)
{
    getFromScreen(0);
    NotifyStopper nsf( freqrgfld_->valuechanged );

    DrawData& d = mGetData();
    float v = 0;
    if ( isminactive_ )
	v = ( d.xaxrg_.stop-d.freqrg_.start )/( d.xaxrg_.stop-d.xaxrg_.start );
    else
	v = 1.2-(d.freqrg_.stop-d.xaxrg_.start)/(d.xaxrg_.stop-d.xaxrg_.start);

    d.variable_ = ( 1 - v )*100* isminactive_? -1:1;
}


float uiFreqTaperDlg::getPercentsFromSlope( float slope )
{
    DrawData& dd = mGetData();
    if ( isminactive_ )
	dd1_.freqrg_.start = dd.freqrg_.stop/pow( 10,(float)(slope/(Math::Log10(2.0) )));
    else
	dd1_.freqrg_.stop = pow( 10,(float)(slope/(Math::Log10(2.0) )))*dd.freqrg_.start;
    setPercentsFromFreq(0);
    return isminactive_ ? dd.variable_ : dd.variable_; 
}


float uiFreqTaperDlg::getSlope()
{
    DrawData& dd = mGetData();
    float slope = Math::Log10( (float)(dd.freqrg_.stop/dd.freqrg_.start) );
    slope *= Math::Log10( 2.0 );
    return slope;
}


void uiFreqTaperDlg::taperChged( CallBacker* )
{
    setViewRanges();
    DrawData& dd = mGetData();
    winfunc_->setVariable( 1.0 - dd.variable_/100 );
    uiFunctionDrawer* drawer = isminactive_ ? drawers_[0] : drawers_[1];
    for ( int idx=0; idx<drawers_.size(); idx++ )
    {
	drawer->setFunctionRange( dd.funcrg_ );
	drawer->erasePoints();
	drawer->createLine( winfunc_ );
	TypeSet<int> intset; intset += 0;
	drawer->draw( intset );
    }
    putToScreen(0);
}


void uiFreqTaperDlg::freqChoiceChged( CallBacker* )
{
    if ( !freqinpfld_ ) return;
    isminactive_ = freqinpfld_->getBoolValue();
    freqrgfld_->setSensitive( hasmin_ && isminactive_, 0, 0 );
    freqrgfld_->setSensitive( hasmax_ && !isminactive_, 0, 1 );
    drawers_[0]->display( isminactive_ );
    drawers_[1]->display( !isminactive_ );
    taperChged(0);
}


void uiFreqTaperDlg::setViewRanges()
{
    if ( isminactive_ )
	dd1_.funcrg_.set( -1.2, dd1_.variable_/100-1 );
    else
	dd2_.funcrg_.set( 1 - dd2_.variable_/100, 1.2 );
} 


void uiFreqTaperDlg::setVariable(float var)
{ 
    DrawData& dd = mGetData();
    dd.variable_ = var * 100;
    putToScreen(0); 
    taperChged(0); 
}

float uiFreqTaperDlg::getVariable( bool min )
{
    return min ? dd1_.variable_ : dd2_.variable_;
} 


