/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.33 2009-11-04 16:39:05 cvsbruno Exp $";


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


#define mGetData() isminactive_ ? dd1_ : dd2_; 
#define mCheckLimitRanges()\
    dd1_.freqrg_.limitTo( Interval<float>( 0.01, dd1_.orgfreqrg_.stop ) );\
    dd2_.freqrg_.limitTo( dd2_.orgfreqrg_ );\
    if ( mIsZero(dd2_.freqrg_.start-dd2_.freqrg_.stop,0.5))\
	dd2_.freqrg_.stop += 0.5;\
    if ( mIsZero(dd2_.freqrg_.start-dd2_.freqrg_.stop,0.5))\
	dd2_.freqrg_.stop += 0.5;
uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const Setup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mNoHelpID) )
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , dispfac_(s.displayfac_)
    , winfsize_(s.winfreqsize_)		     
{
    setCtrlStyle( LeaveOnly );
    setHSpacing( 35 );
  
    winfunc_ = WinFuncs().create( "CosTaper" );

    uiFunctionDrawer::Setup su;
    su.xaxname_= "Frequency (Hz)"; 	su.yaxname_ = "Gain (dB)";

    dd1_.funcrg_.set( -dispfac_, 0 ); 	dd2_.funcrg_.set( 0, dispfac_ );
    dd1_.xaxrg_ = s.minfreqrg_; 	dd2_.xaxrg_ = s.maxfreqrg_;
    dd1_.freqrg_ = s.minfreqrg_;  	dd2_.freqrg_ = s.maxfreqrg_; 
    dd1_.orgfreqrg_ = s.minfreqrg_;   	dd2_.orgfreqrg_ = s.maxfreqrg_; 
    dd1_.orgfreqrg_.start = s.orgfreqrg_.start; 
    dd2_.orgfreqrg_.stop = s.orgfreqrg_.stop; 

    mCheckLimitRanges(); 

    su.xaxrg_ = dd1_.xaxrg_;		
    drawers_ += new uiFunctionDrawer( this, su );

    su.xaxrg_ = dd2_.xaxrg_;
    drawers_ += new uiFunctionDrawer( this, su );
    
    TypeSet<int> selecteditems;

    varinpfld_ = new uiGenInput( this, tapertxt_[1], FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, drawers_[0] );
    varinpfld_->attach( ensureBelow, drawers_[1] );
    varinpfld_->setTitleText ( tapertxt_[1] );
    varinpfld_->setValue( dd1_.variable_ );
    varinpfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, getFromScreen));
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
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, getFromScreen));
    freqrgfld_->valuechanged.notify( mCB( 
			    this, uiFreqTaperDlg, setPercentsFromFreq ) );
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, taperChged  ) );
    freqrgfld_->attach( rightOf, varinpfld_ );
    
    freqChoiceChged(0);
}


void uiFreqTaperDlg::getFromScreen( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.variable_ = getPercentsFromSlope( varinpfld_->getfValue() );
    dd.freqrg_ = freqrgfld_->getFInterval();
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
    varinpfld_->setValue( getSlope() );
    freqrgfld_->setValue( dd.freqrg_ );
} 


void uiFreqTaperDlg::setFreqFromPercents( CallBacker* )
{
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


void uiFreqTaperDlg::setPercentsFromFreq( CallBacker* )
{
    NotifyStopper nsf( freqrgfld_->valuechanged );

    DrawData& d = mGetData();
    float v = 0; 
    const float denom = ( d.xaxrg_.stop -d.xaxrg_.start );
    if ( isminactive_ )
    {
	float f =( d.freqrg_.start - d.xaxrg_.start ) / denom;
	v = ( d.funcrg_.start )*( 1 - 1/f ) - 1/f;
    }
    else
    {
	float f =( d.freqrg_.stop-d.xaxrg_.start ) / denom;
	v = ( 1 - d.funcrg_.stop*f )/ ( 1 - f );
    }
    float factor = isminactive_? -1:1;
    d.variable_ = ( 1 - v*factor  )*100;
}


#define mDec2Oct 0.301029996
float uiFreqTaperDlg::getPercentsFromSlope( float slope )
{
    NotifyStopper nsf( freqrgfld_->valuechanged );
    const float slopeindecade = (float)(slope/mDec2Oct);
    const float slopeinhertz = pow( 10, slopeindecade );
    DrawData& dd = mGetData();

    if ( isminactive_ )
    {
	dd1_.freqrg_.start = dd.freqrg_.stop/slopeinhertz;
	float& val = dd1_.freqrg_.start;
    }
    else
    {
	dd2_.freqrg_.stop = dd.freqrg_.start*slopeinhertz;
	float& val = dd2_.freqrg_.start;
    }
    mCheckLimitRanges()
    setPercentsFromFreq(0);
    return isminactive_ ? dd.variable_ : dd.variable_; 
}


float uiFreqTaperDlg::getSlope()
{
    DrawData& d = mGetData();
    float slope = fabs( Math::Log10( d.freqrg_.start/d.freqrg_.stop ) );
    slope *= mDec2Oct;
    return slope;
}


void uiFreqTaperDlg::taperChged( CallBacker* cb )
{
    setViewRanges();
    DrawData& dd = mGetData();
    winfunc_->setVariable( 1.0 - dd.variable_/100 );
    uiFunctionDrawer* drawer = isminactive_ ? drawers_[0] : drawers_[1];
    uiFunctionDrawer::DrawFunction* drawfunction = 
			new uiFunctionDrawer::DrawFunction( winfunc_ );
    drawer->addFunction( drawfunction );

    for ( int idx=0; idx<drawers_.size(); idx++ )
    {
	drawer->setFunctionRange( dd.funcrg_ );
	drawer->draw(0);
	TypeSet<int> intset; intset += 0;
    }
    putToScreen(0);
}


void uiFreqTaperDlg::freqChoiceChged( CallBacker* )
{
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;
    drawers_[0]->display( isminactive_ );
    drawers_[1]->display( !isminactive_ );
    freqrgfld_->setSensitive( hasmin_ && isminactive_, 0, 0 );
    freqrgfld_->setSensitive( hasmax_ && !isminactive_, 0, 1 );
    taperChged(0);
    getFromScreen(0);
}


void uiFreqTaperDlg::setViewRanges()
{
    if ( isminactive_ )
	dd1_.funcrg_.set( -dispfac_, dd1_.variable_/100-1 );
    else
	dd2_.funcrg_.set( 1 - dd2_.variable_/100, dispfac_ );
} 


void uiFreqTaperDlg::setVariables( Interval<float> vars )
{ 
    dd1_.freqrg_.start = -( vars.start ) + dd1_.freqrg_.stop;
    dd2_.freqrg_.stop = ( vars.stop ) + dd2_.freqrg_.start;
    mCheckLimitRanges()
    putToScreen(0); 
    setPercentsFromFreq(0);
    taperChged(0); 
}


Interval<float> uiFreqTaperDlg::getVariables() const
{
    float var1 = ( dd1_.freqrg_.stop - dd1_.freqrg_.start )*winfsize_/100;
    float var2 = ( dd2_.freqrg_.stop - dd2_.freqrg_.start )*winfsize_/100;
    return Interval<float> ( var1, var2 );
} 

