/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiselsurvranges.h"

#include "math.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uistrings.h"

static const int cUnLim = 1000000;
static const float cMaxUnsnappedZStep = 0.999f;


#define mDefConstrList(isrel) \
    uiGroup(p,"Z range selection") \
    , stepfld_(0) \
    , isrel_(isrel) \
    , zddef_(ZDomain::Def::get(domky)) \
    , rangeChanged(this) \
    , othdom_(&zddef_ != &ZDomain::SI()) \
    , cansnap_( !othdom_ \
	&& SI().zRange(false).step > cMaxUnsnappedZStep / zddef_.userFactor() )


uiSelZRange::uiSelZRange( uiParent* p, bool wstep, bool isrel,
			  const char* lbltxt, const char* domky )
	: mDefConstrList(isrel)
{
    const StepInterval<float> limitrg( SI().zRange(false) );
    makeInpFields( mToUiStringTodo(lbltxt), wstep, !othdom_ && 
						       !isrel_ ? &limitrg : 0 );
    if ( isrel_ )
	setRange( StepInterval<float>(0,0,1) );
    else if ( !othdom_ )
	setRange( SI().zRange(true) );
}


uiSelZRange::uiSelZRange( uiParent* p, StepInterval<float> limitrg, bool wstep,
			  const char* lbltxt, const char* domky )
	: mDefConstrList(false)
{
    makeInpFields( mToUiStringTodo(lbltxt), wstep, &limitrg );
    setRange( limitrg );
}


void uiSelZRange::makeInpFields( const uiString& lbltxt, bool wstep,
				 const StepInterval<float>* inplimitrg )
{
    const float zfac = mCast( float, zddef_.userFactor() );
    const StepInterval<float>& sizrg( SI().zRange(false) );

    StepInterval<float> limitrg( -mCast(float,cUnLim), mCast(float,cUnLim), 1 );
    if ( inplimitrg )
	limitrg = *inplimitrg;
    if ( !othdom_ && limitrg.step > sizrg.step )
	limitrg.step = sizrg.step;

    if ( mIsZero(limitrg.step,mDefEpsF) )
	limitrg.step = 1.0f;
    limitrg.scale( zfac );

    const int nrdecimals = cansnap_ ? 0 : 2;
    const StepInterval<int> izrg( mNINT32(limitrg.start),
				  mNINT32(limitrg.stop), mNINT32(limitrg.step));

    startfld_ = new uiSpinBox( this, nrdecimals, "Z start" );
    uiString ltxt( mToUiStringTodo(lbltxt) );
    if ( ltxt.isEmpty() )
	ltxt = zddef_.getRange();

    uiLabel* lbl = new uiLabel( this,
		uiStrings::phrJoinStrings( ltxt, zddef_.uiUnitStr(true)),
		startfld_ );

    stopfld_ = new uiSpinBox( this, nrdecimals, "Z stop" );
    stopfld_->attach( rightOf, startfld_ );
    if ( !cansnap_ )
	{ startfld_->setInterval( limitrg ); stopfld_->setInterval( limitrg ); }
    else
	{ startfld_->setInterval( izrg ); stopfld_->setInterval( izrg ); }
    startfld_->doSnap( cansnap_ ); stopfld_->doSnap( cansnap_ );

    if ( wstep )
    {
	stepfld_ = new uiSpinBox( this, nrdecimals, "Z step" );
	if ( cansnap_ )
	    stepfld_->setInterval(
		StepInterval<int>(izrg.step,izrg.width(),izrg.step) );
	else
	    stepfld_->setInterval(
	    StepInterval<int>(mNINT32(limitrg.step),mNINT32(limitrg.width()),
				  mNINT32(limitrg.step)) );
	stepfld_->doSnap( cansnap_ );
	lbl = new uiLabel( this, uiStrings::sStep(), stepfld_ );
	lbl->attach( rightOf, stopfld_ );
    }

    const CallBack cb( mCB(this,uiSelZRange,valChg) );
    startfld_->valueChanging.notify( cb );
    stopfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


StepInterval<float> uiSelZRange::getRange() const
{
    StepInterval<float> zrg( startfld_->getFValue(), stopfld_->getFValue(),
			     stepfld_ ? stepfld_->getFValue() : 1 );
    zrg.scale( 1.f / zddef_.userFactor() );
    if ( !stepfld_ )
	zrg.step = SI().zRange(true).step;
    return zrg;
}


#define mAdaptRangeToLimits( rg, limit, newrg ) \
if ( mIsUdf(rg.start) || mIsUdf(rg.stop) ) \
    newrg = rg; \
else \
{ \
    const double eps = 1e-4; \
    if ( mIsZero(limit.step,eps) || mIsUdf(limit.step) ) \
	limit.step = 1; \
\
    const double realstartdif = double(rg.start) - double(limit.start); \
    const double realstartidx = realstartdif / double(limit.step); \
    const double realstepfac = double(rg.step) / double(limit.step); \
    const bool useoldstep = !mIsZero(realstartidx-mNINT32(realstartidx),eps) ||\
			    !mIsZero(realstepfac-mNINT32(realstepfac),eps); \
    int stepfac = useoldstep ? 1 : mNINT32(realstepfac); \
    if ( stepfac == 0 ) \
	stepfac = 1; \
\
    int startidx = mNINT32( ceil(realstartidx-eps) ); \
    if ( startidx < 0 ) \
	startidx = (startidx*(1-stepfac)) % stepfac; \
\
    const double width = mMIN(rg.stop,limit.stop) - limit.atIndex(startidx); \
    const double realnrsteps = width / (stepfac*limit.step); \
    const int stopidx = startidx \
			+ stepfac * mNINT32( Math::Floor(realnrsteps+eps) );\
\
    if ( startidx <= stopidx ) \
    { \
	newrg.start = limit.atIndex( startidx ); \
	newrg.stop = limit.atIndex( stopidx ); \
	newrg.step = stepfac * limit.step; \
    } \
    else \
	newrg = limit; \
}


void uiSelZRange::setRange( const StepInterval<float>& inpzrg )
{
    StepInterval<float> zrg( inpzrg );
    zrg.scale( mCast(float,zddef_.userFactor()) );

    StepInterval<float> limitrg = startfld_->getFInterval();
    StepInterval<float> newzrg;
    mAdaptRangeToLimits( zrg, limitrg, newzrg );

    if ( cansnap_ )
    {
	startfld_->setValue( mNINT32(newzrg.start) );
	stopfld_->setValue( mNINT32(newzrg.stop) );
	if ( stepfld_ )
	    stepfld_->setValue( mNINT32(newzrg.step) );
    }
    else
    {
	startfld_->setValue( newzrg.start );
	stopfld_->setValue( newzrg.stop );
	if ( stepfld_ )
	    stepfld_->setValue( newzrg.step );
    }
}


void uiSelZRange::valChg( CallBacker* cb )
{
    if ( !isrel_ && startfld_->getValue() > stopfld_->getValue() )
    {
	const bool chgstart = cb == stopfld_;
	if ( chgstart )
	    startfld_->setValue( stopfld_->getValue() );
	else
	    stopfld_->setValue( startfld_->getValue() );
    }

    rangeChanged.trigger();
}

void uiSelZRange::setRangeLimits( const StepInterval<float>& zlimits )
{
    StepInterval<float> zrg( zlimits );
    zrg.scale( mCast(float,zddef_.userFactor()) );
    startfld_->setInterval( zrg );
    stopfld_->setInterval( zrg );
    if ( stepfld_ )
    {
	stepfld_->setMinValue( zrg.step );
	stepfld_->setMaxValue( mMAX(zrg.step, zrg.stop-zrg.start) );
	stepfld_->setStep( zrg.step );
    }
}


uiSelNrRange::uiSelNrRange( uiParent* p, uiSelNrRange::Type typ, bool wstep )
	: uiGroup(p,typ == Inl ? "In-line range selection"
		 : (typ == Crl ? "Cross-line range selection"
			       : "Number range selection"))
	, stepfld_(0)
	, icstopfld_(0)
	, nrstopfld_(0)
	, defstep_(1)
	, checked(this)
	, rangeChanged(this)
	, finalised_(false)
	, checked_(false)
	, cbox_(0), withchk_(false)
	, lbltxt_("")
{
    StepInterval<int> rg( 1, mUdf(int), 1 );
    StepInterval<int> wrg( rg );
    const char* nm = "Number";
    if ( typ != Gen )
    {
	const TrcKeySampling& hs( SI().sampling(false).hsamp_ );
	rg = typ == Inl ? hs.inlRange() : hs.crlRange();
	const TrcKeySampling& whs( SI().sampling(true).hsamp_ );
	wrg = typ == Inl ? whs.inlRange() : whs.crlRange();
	nm = typ == Inl ? sKey::Inline() : sKey::Crossline();
	defstep_ = typ == Inl ? SI().inlStep() : SI().crlStep();
    }
    lbltxt_ = nm;
    makeInpFields( rg, wstep, typ==Gen );
    setRange( wrg );
    preFinalise().notify( mCB(this,uiSelNrRange,doFinalise) );
}


uiSelNrRange::uiSelNrRange( uiParent* p, StepInterval<int> limitrg, bool wstep,
			    const char* lbltxt )
	: uiGroup(p,BufferString(lbltxt," range selection"))
	, stepfld_(0)
	, defstep_(limitrg.step)
	, icstopfld_(0)
	, nrstopfld_(0)
	, checked(this)
	, rangeChanged(this)
	, finalised_(false)
	, withchk_(false)
	, checked_(false)
	, cbox_(0)
	, lbltxt_(lbltxt)
{
    makeInpFields( limitrg, wstep, false );
    setRange( limitrg );
    preFinalise().notify( mCB(this,uiSelNrRange,doFinalise) );
}


void uiSelNrRange::makeInpFields( StepInterval<int> limitrg, bool wstep,
				  bool isgen )
{
    const CallBack cb( mCB(this,uiSelNrRange,valChg) );
    startfld_ = new uiSpinBox( this, 0, BufferString(lbltxt_," start") );
    startfld_->setInterval( limitrg );
    startfld_->doSnap( true );
    uiObject* stopfld;
    if ( isgen )
    {
	stopfld = nrstopfld_ = new uiLineEdit( this,
					       BufferString(lbltxt_," stop") );
	nrstopfld_->setHSzPol( uiObject::Small );
	nrstopfld_->editingFinished.notify( cb );
    }
    else
    {
	stopfld = icstopfld_ = new uiSpinBox( this, 0,
					      BufferString(lbltxt_," stop") );
	icstopfld_->setInterval( limitrg );
	icstopfld_->doSnap( true );
	icstopfld_->valueChanging.notify( cb );
    }
    stopfld->attach( rightOf, startfld_ );

    if ( wstep )
    {
	stepfld_ = new uiSpinBox( this, 0, BufferString(lbltxt_," step") );
	stepfld_->setInterval( StepInterval<int>(limitrg.step,
			    limitrg.width() ? limitrg.width() : limitrg.step,
			    limitrg.step) );
	stepfld_->doSnap( true );
	stepfld_->valueChanging.notify( cb );
	uiLabel* lbl = new uiLabel( this, uiStrings::sStep(), stepfld_ );
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
    return txt && *txt ? toInt(txt) : mUdf(int);
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

    rangeChanged.trigger();
}


void uiSelNrRange::doFinalise( CallBacker* )
{
    if ( finalised_ ) return;

    if ( withchk_ )
    {
	cbox_ = new uiCheckBox( this, toUiString("%1 %2")
		      .arg(mToUiStringTodo(lbltxt_)).arg(uiStrings::sRange()) );
	cbox_->attach( leftTo, startfld_ );
	cbox_->activated.notify( mCB(this,uiSelNrRange,checkBoxSel) );
	setChecked( checked_ );
	checkBoxSel(0);
    }
    else
	new uiLabel( this,  toUiString("%1 %2").arg(mToUiStringTodo(lbltxt_))
					.arg(uiStrings::sRange()), startfld_ );

    finalised_ = true;
}


StepInterval<int> uiSelNrRange::getRange() const
{
    return StepInterval<int>( startfld_->getValue(), getStopVal(),
			      stepfld_ ? stepfld_->getValue() : defstep_ );
}


void uiSelNrRange::setRange( const StepInterval<int>& rg )
{
    StepInterval<int> limitrg = startfld_->getInterval();
    StepInterval<int> newrg;
    mAdaptRangeToLimits( rg, limitrg, newrg );

    startfld_->setValue( newrg.start );
    setStopVal( newrg.stop );
    if ( stepfld_ )
	stepfld_->setValue( newrg.step );
}


void uiSelNrRange::setLimitRange( const StepInterval<int>& limitrg )
{
    startfld_->setInterval( limitrg );
    if ( icstopfld_ )
	icstopfld_->setInterval( limitrg );
    if ( stepfld_ )
    {
	stepfld_->setMinValue( limitrg.step );
	stepfld_->setMaxValue( mMAX(limitrg.step, limitrg.stop-limitrg.start) );
	stepfld_->setStep( limitrg.step );
    }
}


bool uiSelNrRange::isChecked()
{ return checked_; }


void uiSelNrRange::setChecked( bool yn )
{
    checked_ = yn;
    if ( cbox_ ) cbox_->setChecked( yn );
}


void uiSelNrRange::checkBoxSel( CallBacker* cb )
{
    if ( !cbox_ ) return;

    checked_ = cbox_->isChecked();
    const bool elemsens = cbox_->sensitive() && cbox_->isChecked();

    if ( startfld_ )
	startfld_->setSensitive( elemsens );
    if ( icstopfld_ )
	icstopfld_->setSensitive( elemsens );
    if ( stepfld_ )
	stepfld_->setSensitive( elemsens );
    if ( nrstopfld_ )
	nrstopfld_->setSensitive( elemsens );

    checked.trigger(this);
}

uiSelSteps::uiSelSteps( uiParent* p, bool is2d )
	: uiGroup(p,"Step selection")
	, inlfld_(0)
{
    BinID stp( 0, 1 );
    uiString lbl = tr("Trace number step");
    uiSpinBox* firstbox = 0;
    if ( !is2d )
    {
	stp = SI().sampling(true).hsamp_.step_;
	firstbox = inlfld_ = new uiSpinBox( this, 0, "inline step" );
	inlfld_->setInterval( StepInterval<int>(stp.inl(),cUnLim,stp.inl()) );
	inlfld_->doSnap( true );
	lbl = toUiString("%1/%2 %3")
	      .arg(uiStrings::sInline()).arg(uiStrings::sCrossline())
	      .arg(uiStrings::sSteps());
    }
    crlfld_ = new uiSpinBox( this, 0, "crossline step" );
    crlfld_->setInterval( StepInterval<int>(stp.crl(),cUnLim,stp.crl()) );
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
    crlfld_->setValue( bid.crl() );
    if ( inlfld_ )
	inlfld_->setValue( bid.inl() );
}


uiSelHRange::uiSelHRange( uiParent* p, bool wstep )
    : uiGroup(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,uiSelNrRange::Inl,wstep))
    , crlfld_(new uiSelNrRange(this,uiSelNrRange::Crl,wstep))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    setHAlignObj( inlfld_ );
}


uiSelHRange::uiSelHRange( uiParent* p, const TrcKeySampling& hslimit,
			  bool wstep )
    : uiGroup(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,hslimit.inlRange(),wstep,sKey::Inline()))
    , crlfld_(new uiSelNrRange(this,hslimit.crlRange(),wstep,sKey::Crossline()))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    setHAlignObj( inlfld_ );
}


TrcKeySampling uiSelHRange::getSampling() const
{
    TrcKeySampling hs( true );
    hs.set( inlfld_->getRange(), crlfld_->getRange() );
    return hs;
}


void uiSelHRange::setSampling( const TrcKeySampling& hs )
{
    inlfld_->setRange( hs.inlRange() );
    crlfld_->setRange( hs.crlRange() );
}


void uiSelHRange::setLimits( const TrcKeySampling& hs )
{
    inlfld_->setLimitRange( hs.inlRange() );
    crlfld_->setLimitRange( hs.crlRange() );
}


uiSelSubvol::uiSelSubvol( uiParent* p, bool wstep, const char* zdomkey )
    : uiGroup(p,"Sub vol selection")
    , hfld_(new uiSelHRange(this,wstep))
    , zfld_(new uiSelZRange(this,wstep,false,0,zdomkey))
{
    zfld_->attach( alignedBelow, hfld_ );
    setHAlignObj( hfld_ );
}


TrcKeyZSampling uiSelSubvol::getSampling() const
{
    TrcKeyZSampling cs( false );
    cs.hsamp_ = hfld_->getSampling();
    cs.zsamp_ = zfld_->getRange();
    return cs;
}


void uiSelSubvol::setSampling( const TrcKeyZSampling& cs )
{
    hfld_->setSampling( cs.hsamp_ );
    zfld_->setRange( cs.zsamp_ );
}


uiSelSubline::uiSelSubline( uiParent* p, bool wstep )
    : uiGroup(p,"Sub vol selection")
    , nrfld_(new uiSelNrRange(this,uiSelNrRange::Gen,wstep))
    , zfld_(new uiSelZRange(this,wstep))
{
    zfld_->attach( alignedBelow, nrfld_ );
    setHAlignObj( nrfld_ );
}
