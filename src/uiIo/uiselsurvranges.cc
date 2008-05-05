/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiselsurvranges.cc,v 1.10 2008-05-05 05:42:29 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiselsurvranges.h"
#include "survinfo.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uilabel.h"

static const int cUnLim = 1000000;


uiSelZRange::uiSelZRange( uiParent* p, bool wstep, bool isrel,
			  const char* lbltxt )
	: uiGroup(p,"Z range selection")
	, stepfld_(0)
{
    StepInterval<float> limitrg( -cUnLim, cUnLim, 1 );
    if ( !isrel )
	limitrg = SI().zRange( false );

    makeInpFields( lbltxt, wstep, limitrg );
    if ( isrel )
	setRange( StepInterval<float>(0,0,1) );
    else
	setRange( SI().zRange(true) );
}


uiSelZRange::uiSelZRange( uiParent* p, StepInterval<float> limitrg, bool wstep,
			  const char* lbltxt )
	: uiGroup(p,"Z range selection")
	, stepfld_(0)
{
    makeInpFields( lbltxt, wstep, limitrg );
    setRange( limitrg );
}


void uiSelZRange::makeInpFields( const char* lbltxt, bool wstep,
				 StepInterval<float> limitrg )
{
    limitrg.scale( SI().zFactor() );
    StepInterval<int> zrg( mNINT(limitrg.start), mNINT(limitrg.stop),
			   mNINT(limitrg.step) );
    startfld_ = new uiSpinBox( this, 0, "Z start" );
    startfld_->setInterval( zrg );
    startfld_->doSnap( true );
    if ( !lbltxt ) lbltxt = "Z Range";
    uiLabel* lbl = new uiLabel( this, lbltxt, startfld_ );
    stopfld_ = new uiSpinBox( this, 0, "Z stop" );
    stopfld_->setInterval( zrg );
    stopfld_->doSnap( true );
    stopfld_->attach( rightOf, startfld_ );

    if ( wstep )
    {
	stepfld_ = new uiSpinBox( this, 0, "Z step" );
	stepfld_->setInterval( StepInterval<int>(zrg.step,zrg.width(),zrg.step) );
	stepfld_->doSnap( true );
	lbl = new uiLabel( this, "step", stepfld_ );
	lbl->attach( rightOf, stopfld_ );
    }

    const CallBack cb( mCB(this,uiSelZRange,valChg) );
    startfld_->valueChanging.notify( cb );
    stopfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


StepInterval<float> uiSelZRange::getRange() const
{
    StepInterval<float> zrg( startfld_->getValue(), stopfld_->getValue(), 
	    		     stepfld_ ? stepfld_->getValue() : 1 );
    zrg.scale( 1 / SI().zFactor() );
    if ( !stepfld_ )
	zrg.step = SI().zRange(true).step;
    return zrg;
}


void uiSelZRange::setRange( const StepInterval<float>& inpzrg )
{
    StepInterval<float> zrg( inpzrg );
    zrg.scale( SI().zFactor() );
    StepInterval<int> irg( mNINT(zrg.start), mNINT(zrg.stop), mNINT(zrg.step) );

    startfld_->setValue( mNINT(zrg.start) );
    stopfld_->setValue( mNINT(zrg.stop) );
    if ( stepfld_ )
	stepfld_->setValue( mNINT(zrg.step) );
}


void uiSelZRange::valChg( CallBacker* cb )
{
    if ( startfld_->getValue() > stopfld_->getValue() )
    {
	const bool chgstart = cb == stopfld_;
	if ( chgstart )
	    startfld_->setValue( stopfld_->getValue() );
	else
	    stopfld_->setValue( startfld_->getValue() );
    }
}


uiSelNrRange::uiSelNrRange( uiParent* p, uiSelNrRange::Type typ, bool wstep )
	: uiGroup(p,typ == Inl ? "Inline range selection"
		 : (typ == Crl ? "Crossline range selection"
			       : "Number range selection"))
	, stepfld_(0)
	, defstep_(1)
	, icstopfld_(0)
	, nrstopfld_(0)
{
    StepInterval<int> rg( 1, mUdf(int), 1 );
    StepInterval<int> wrg( rg );
    const char* nm = "Number";
    if ( typ != Gen )
    {
	const HorSampling& hs( SI().sampling(false).hrg );
	rg = typ == Inl ? hs.inlRange() : hs.crlRange();
	const HorSampling& whs( SI().sampling(true).hrg );
	wrg = typ == Inl ? whs.inlRange() : whs.crlRange();
	nm = typ == Inl ? "Inline" : "Crossline";
	defstep_ = wrg.step;
    }

    makeInpFields( nm, rg, wstep, typ==Gen );
    setRange( wrg );
}


uiSelNrRange::uiSelNrRange( uiParent* p, StepInterval<int> limitrg, bool wstep,
			    const char* lbltxt )
	: uiGroup(p,BufferString(lbltxt," range selection"))
	, stepfld_(0)
	, defstep_(1)
	, icstopfld_(0)
	, nrstopfld_(0)
{
    makeInpFields( lbltxt, limitrg, wstep, false );
    setRange( limitrg );
}


void uiSelNrRange::makeInpFields( const char* lbltxt, StepInterval<int> limitrg,
				  bool wstep, bool isgen )
{
    const CallBack cb( mCB(this,uiSelNrRange,valChg) );
    startfld_ = new uiSpinBox( this, 0, BufferString(lbltxt," start") );
    startfld_->setInterval( limitrg );
    startfld_->doSnap( true );
    uiLabel* lbl = new uiLabel( this, BufferString(lbltxt," range"), startfld_);
    uiObject* stopfld;
    if ( isgen )
    {
	stopfld = nrstopfld_ = new uiLineEdit( this,
					       BufferString(lbltxt," stop") );
	nrstopfld_->setHSzPol( uiObject::Small );
	nrstopfld_->editingFinished.notify( cb );
    }
    else
    {
	stopfld = icstopfld_ = new uiSpinBox( this, 0,
					      BufferString(lbltxt," stop") );
	icstopfld_->setInterval( limitrg );
	icstopfld_->doSnap( true );
	icstopfld_->valueChanging.notify( cb );
    }
    stopfld->attach( rightOf, startfld_ );

    if ( wstep )
    {
	stepfld_ = new uiSpinBox( this, 0, BufferString(lbltxt," step") );
	stepfld_->setInterval( StepInterval<int>(limitrg.step,limitrg.width(),
		    				 limitrg.step) );
	stepfld_->doSnap( true );
	lbl = new uiLabel( this, "step", stepfld_ );
	if ( stopfld )
	    lbl->attach( rightOf, stopfld );
    }

    startfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


int uiSelNrRange::getStopVal() const
{
    if ( icstopfld_ )
	return icstopfld_->getValue();

    const char* txt = nrstopfld_->text();
    return txt && *txt ? atoi(txt) : mUdf(int);
}


void uiSelNrRange::setStopVal( int nr )
{
    if ( icstopfld_ )
	icstopfld_->setValue( nr );
    else
    {
	if ( mIsUdf(nr) )
	    nrstopfld_->setText( "" );
	else
	    nrstopfld_->setValue( nr );
    }
}


void uiSelNrRange::valChg( CallBacker* cb )
{
    if ( startfld_->getValue() > getStopVal() )
    {
	const bool chgstop = cb == startfld_;
	if ( chgstop )
	    setStopVal( startfld_->getValue() );
	else
	    startfld_->setValue( getStopVal() );
    }
}


StepInterval<int> uiSelNrRange::getRange() const
{
    return StepInterval<int>( startfld_->getValue(), getStopVal(),
	    		      stepfld_ ? stepfld_->getValue() : defstep_ );
}


void uiSelNrRange::setRange( const StepInterval<int>& rg )
{
    startfld_->setValue( rg.start );
    setStopVal( rg.stop );
    if ( stepfld_ )
	stepfld_->setValue( rg.step );
}


uiSelSteps::uiSelSteps( uiParent* p, bool is2d )
	: uiGroup(p,"Step selection")
	, inlfld_(0)
{
    BinID stp( 0, 1 );
    const char* lbl = "Trace number step";
    uiSpinBox* firstbox = 0;
    if ( !is2d )
    {
	stp = SI().sampling(true).hrg.step;
	firstbox = inlfld_ = new uiSpinBox( this, 0, "inline step" );
	inlfld_->setInterval( StepInterval<int>(stp.inl,cUnLim,stp.inl) );
	inlfld_->doSnap( true );
	lbl = "Inline/Crossline steps";
    }
    crlfld_ = new uiSpinBox( this, 0, "crossline step" );
    crlfld_->setInterval( StepInterval<int>(stp.crl,cUnLim,stp.crl) );
    crlfld_->doSnap( true );
    if ( inlfld_ )
	crlfld_->attach( rightOf, inlfld_ );
    else
	firstbox = crlfld_;

    new uiLabel( this, lbl, firstbox );
    setHAlignObj( firstbox );
}


BinID uiSelSteps::getSteps() const
{
    return BinID( inlfld_ ? inlfld_->getValue() : 0, crlfld_->getValue() );
}


void uiSelSteps::setSteps( const BinID& bid )
{
    crlfld_->setValue( bid.crl );
    if ( inlfld_ )
	inlfld_->setValue( bid.inl );
}


uiSelHRange::uiSelHRange( uiParent* p, bool wstep )
    : uiGroup(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,uiSelNrRange::Inl,wstep))
    , crlfld_(new uiSelNrRange(this,uiSelNrRange::Crl,wstep))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    setHAlignObj( inlfld_ );
}


uiSelHRange::uiSelHRange( uiParent* p, const HorSampling& hslimit, bool wstep )
    : uiGroup(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,hslimit.inlRange(),wstep,"Inline"))
    , crlfld_(new uiSelNrRange(this,hslimit.crlRange(),wstep,"Crossline"))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    setHAlignObj( inlfld_ );
}


HorSampling uiSelHRange::getSampling() const
{
    HorSampling hs( false );
    hs.set( inlfld_->getRange(), crlfld_->getRange() );
    return hs;
}


void uiSelHRange::setSampling( const HorSampling& hs )
{
    inlfld_->setRange( hs.inlRange() );
    crlfld_->setRange( hs.crlRange() );
}


uiSelSubvol::uiSelSubvol( uiParent* p, bool wstep )
    : uiGroup(p,"Sub vol selection")
    , hfld_(new uiSelHRange(this,wstep))
    , zfld_(new uiSelZRange(this,wstep))
{
    zfld_->attach( alignedBelow, hfld_ );
    setHAlignObj( hfld_ );
}


CubeSampling uiSelSubvol::getSampling() const
{
    CubeSampling cs( false );
    cs.hrg = hfld_->getSampling();
    cs.zrg = zfld_->getRange();
    return cs;
}


void uiSelSubvol::setSampling( const CubeSampling& cs )
{
    hfld_->setSampling( cs.hrg );
    zfld_->setRange( cs.zrg );
}


uiSelSubline::uiSelSubline( uiParent* p, bool wstep )
    : uiGroup(p,"Sub vol selection")
    , nrfld_(new uiSelNrRange(this,uiSelNrRange::Gen,wstep))
    , zfld_(new uiSelZRange(this,wstep))
{
    zfld_->attach( alignedBelow, nrfld_ );
    setHAlignObj( nrfld_ );
}
