/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.40 2009-11-23 15:59:22 cvsbruno Exp $";


#include "uiwindowfuncseldlg.h"
#include "uiamplspectrum.h"
#include "uiaxishandler.h"
#include "uicanvas.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uiseparator.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "cubesampling.h"
#include "uislicesel.h"
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
    
    BufferString tapertxt( "Taper Length (%)" );
    varinpfld_ = new uiGenInput( this, tapertxt, FloatInpSpec() );
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


/*
static const char* winname = "CosTaper";
#define mGetData() isminactive_ ? dd1_ : dd2_; 
#define mCheckLimitRanges()\
    dd1_.freqrg_.limitTo( Interval<float>( 0.05, dd1_.reffreqrg_.stop ) );\
    dd2_.freqrg_.limitTo( Interval<float>(dd2_.reffreqrg_.start+0.05,datasz_));\
    dd1_.freqrg_.stop = dd1_.reffreqrg_.stop;\
    dd2_.freqrg_.start = dd2_.reffreqrg_.start;
uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const Setup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mNoHelpID) )
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , datasz_((int)(0.5/(SI().zStep())))
{
    setCtrlStyle( LeaveOnly );
    setHSpacing( 35 );

    dd1_.freqrg_ = s.minfreqrg_;  dd1_.reffreqrg_ = s.minfreqrg_; 
    dd2_.freqrg_ = s.maxfreqrg_;  dd2_.reffreqrg_ = s.maxfreqrg_;
    mCheckLimitRanges();
    setSlopeFromFreq();
    
    uiFuncTaperDisp::Setup su;
    su.leftrg_ = s.minfreqrg_;    
    su.rightrg_ = s.maxfreqrg_; 
    su.datasz_ = datasz_; 
    su.is2sided_ = true; 

    su.xaxnm_ = "Frequency (Hz)"; 	
    su.yaxnm_ = "Gain (dB)";
    su.noxgridline_ = true;
    su.noygridline_ = true;

    drawer_ = new uiFuncTaperDisp( this, su );

    seisid_ = s.seisid_;
    
    varinpfld_ = new uiGenInput( this, tapertxt_[1], FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, drawer_ );
    varinpfld_->setTitleText ( tapertxt_[1] );
    varinpfld_->setValue( dd1_.variable_ );
    varinpfld_->valuechanged.notify(mCB( this, uiFreqTaperDlg, slopeChanged ));
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperDlg, taperChged) );

    freqrgfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency"),
				    FloatInpSpec().setName("Max frequency") );
    freqrgfld_->valuechanged.notify( mCB( this, uiFreqTaperDlg, freqChanged));
    freqrgfld_->attach( rightOf, varinpfld_ );
    
    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperDlg,freqChoiceChged) );
	freqinpfld_->attach( ensureBelow, freqrgfld_ );
	freqinpfld_->attach( centeredBelow, drawer_ );
    }
 
    bool withpreview_ = true; bool is2d = false; bool isinl = true;
    if ( withpreview_ )
    {
	uiSeparator* sep = new uiSeparator( this, "Seismic2Log Sep" );
	sep->attach( stretchedBelow, freqinpfld_ ? freqinpfld_ : freqrgfld_  );

	CallBack cbview = mCB(this,uiFreqTaperDlg,previewPushed);
	previewfld_ = new uiPushButton( this, "&Preview spectrum...", cbview, true);
	previewfld_->attach( ensureBelow, sep );
	previewfld_->attach( centeredBelow, drawer_ );

	ZDomain::Info info;
	uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD
				   : (isinl ? uiSliceSel::Inl 
					    : uiSliceSel::Crl);
	CubeSampling cs;
	CallBack dummycb;

	posdlg_ = new uiSliceSelDlg( this, cs, cs, dummycb, tp, info );
	posdlg_->grp()->enableApplyButton( false );
	posdlg_->grp()->enableScrollButton( false );
	posdlg_->setModal( true );
    }

    setPercentsFromFreq();
    finaliseDone.notify( mCB(this, uiFreqTaperDlg, taperChged ) );
}


uiFreqTaperDlg::~uiFreqTaperDlg()
{
    delete drawer_;
}


void uiFreqTaperDlg::freqChanged( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.freqrg_ = freqrgfld_->getFInterval();
    mCheckLimitRanges();
    setPercentsFromFreq();
    setSlopeFromFreq();
    
    taperChged(0);
}


void uiFreqTaperDlg::slopeChanged( CallBacker* )
{
    DrawData& dd = mGetData();
    dd.slope_ = varinpfld_->getfValue();
    setFreqFromSlope( dd.slope_ );
    setPercentsFromFreq();
    taperChged(0);
}


#define setToNearestInt(val)\
{\
    int ifr = mNINT( val  );\
    if ( mIsZero(val-ifr,1e-2) )\
	val = ifr;\
}
#define setTo1Decimal(val)\
{\
    val*=10;\
    val = (int)val;\
    val = (float)val/10;\
}
void uiFreqTaperDlg::putToScreen( CallBacker* )
{
    NotifyStopper nsf1( varinpfld_->valuechanged );
    NotifyStopper nsf2( freqrgfld_->valuechanged );

    DrawData& dd = mGetData();

    float freq1 = dd.freqrg_.start;
    float freq2 = dd.freqrg_.stop;

    setTo1Decimal( freq1 );
    setToNearestInt( freq1 ); 
    setTo1Decimal( freq2 );
    setToNearestInt( freq2 );
    
    freqrgfld_->setValue( Interval<float>( freq1, freq2 ) );

    float slope = dd.slope_;
    setTo1Decimal( slope );
    setToNearestInt( slope ); 
    varinpfld_->setValue( slope );
    
    freqrgfld_->setSensitive( hasmin_ && isminactive_, 0, 0 );
    freqrgfld_->setSensitive( hasmax_ && !isminactive_, 0, 1 );
} 


void uiFreqTaperDlg::setPercentsFromFreq()
{
    NotifyStopper nsf( freqrgfld_->valuechanged );
    dd1_.variable_ = hasmin_ ? dd1_.freqrg_.start / dd1_.freqrg_.stop : 0;
    dd2_.variable_ = hasmax_ ? ( dd2_.freqrg_.stop - dd2_.freqrg_.start )
			       / ( datasz_ - dd2_.freqrg_.start ) : 0;

    drawer_->setWinVariable( dd1_.variable_, dd2_.variable_ );
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


void uiFreqTaperDlg::setSlopeFromFreq()
{
    DrawData& d = mGetData();
    float slope = fabs( 1/Math::Log10( d.freqrg_.stop / d.freqrg_.start ) );
    d.slope_ = slope*mDec2Oct;
}


void uiFreqTaperDlg::taperChged( CallBacker* cb )
{
    drawer_->taperChged(0);
    putToScreen(0);
}


void uiFreqTaperDlg::freqChoiceChged( CallBacker* )
{
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;
    setSlopeFromFreq();
    putToScreen(0);
}


void uiFreqTaperDlg::setFreqRange( Interval<float> fqrg )
{ 
    dd1_.freqrg_.start = fqrg.start;
    dd2_.freqrg_.stop = fqrg.stop;
    mCheckLimitRanges()
}


Interval<float> uiFreqTaperDlg::getFreqRange() const
{
    return Interval<float> ( dd1_.freqrg_.start, dd2_.freqrg_.stop );
} 


void uiFreqTaperDlg::previewPushed(CallBacker*)
{
    posdlg_->go();

    uiAmplSpectrum spec( this );

    DataPack dp();
    DPM(DataPackMgr::FlatID()).add( fddatapack );
    dp.id_ = seisid_;
    spec.setData(   );
}



uiFuncTaperDisp::uiFuncTaperDisp( uiParent* p, const Setup& s )
    : uiFunctionDisplay( p, s )
    , is2sided_(s.is2sided_)  
    , window_(0)				
    , funcvals_(0)
    , orgfuncvals_(0)
{
    if ( is2sided_  )
    {
	datasz_ = s.datasz_; 
	leftd_.rg_ = s.leftrg_;   
	leftd_.winsz_ = 2*( (int)s.leftrg_.stop );
	leftd_.window_ = new ArrayNDWindow( 
			    Array1DInfoImpl(leftd_.winsz_),false,winname,0 );

	rightd_.rg_ = s.rightrg_;
	rightd_.winsz_ = 2*( datasz_ - (int)s.rightrg_.start );
	rightd_.window_ = new ArrayNDWindow( 
			    Array1DInfoImpl(rightd_.winsz_),false,winname,0 );
    }
    window_ = new ArrayNDWindow( Array1DInfoImpl(datasz_),false,winname,0);

    xAxis()->setName( s.xaxnm_ ); 	
    yAxis(false)->setName( s.yaxnm_ );
}


uiFuncTaperDisp::~uiFuncTaperDisp()
{
    delete window_;
    delete leftd_.window_; 
    delete rightd_.window_;
    delete orgfuncvals_; 
}


void uiFuncTaperDisp::setFunction( Array1DImpl<float>& data, Interval<float> rg)
{
    orgfuncvals_ = new Array1DImpl<float>( data );
    funcvals_ = &data;
    funcrg_ = rg;

    taperChged(0);
}



void uiFuncTaperDisp::setWinVariable( float leftvar, float rightvar )
{
    if ( leftvar == 1 ) leftvar -= 0.01; 

    if ( is2sided_ )
    {
	delete leftd_.window_; leftd_.window_ = 0;
	if ( leftvar )	
	    leftd_.window_ = new ArrayNDWindow( Array1DInfoImpl(leftd_.winsz_), 
						false, winname, leftvar );

	delete rightd_.window_; rightd_.window_ = 0;
	if ( rightvar )	
	    rightd_.window_ = new ArrayNDWindow(Array1DInfoImpl(rightd_.winsz_),
						false, winname, 1-rightvar );
    }
    else
	window_->setType( winname, leftvar );

    taperChged(0);
}


void uiFuncTaperDisp::taperChged( CallBacker* cb )
{
    TypeSet<float> xvals;
    if ( is2sided_ )
    {
	for ( int idx=0; idx<datasz_; idx++ )
	{
	    float val = 0;
	    if ( leftd_.window_ && idx < (int)leftd_.rg_.stop )
		val = leftd_.window_->getValues()[leftd_.winsz_/2+idx];

	    if ( rightd_.window_ && idx > (int)rightd_.rg_.start ) 
		val= rightd_.window_->getValues()[idx-(int)rightd_.rg_.start];
	    xvals += idx; 
	    window_->setValue( idx, 1-val );
	}
    }

    if ( funcvals_ )
    {
	window_->apply( orgfuncvals_, funcvals_ );
	setVals( funcrg_, funcvals_->getData(), datasz_ );
	setY2Vals( funcrg_, window_->getValues(), datasz_ );
    }
    else
	setVals( xvals.arr(), window_->getValues(), datasz_ );
}
*/
