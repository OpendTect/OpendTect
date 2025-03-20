/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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


// uiSelZRange

uiSelZRange::uiSelZRange( uiParent* p, bool wstep, const ZDomain::Info& zinfo )
    : uiGroup(p,"Z range selection")
    , zinfo_(&zinfo)
    , rangeChanged(this)
{
    makeInpFields( wstep );
}


uiSelZRange::uiSelZRange( uiParent* p, bool wstep, const char* domky,
			  const char* zunitstr )
    : uiSelZRange(p,wstep,ZDomain::Info::getFrom( domky,zunitstr ))
{}


uiSelZRange::uiSelZRange( uiParent* p, bool wstep, bool isrel,
			  const char* lbltxt, const char* domky,
			  const char* zunitstr )
    : uiSelZRange(p,wstep,domky,zunitstr)
{
    if ( lbltxt && *lbltxt )
	setLabel( toUiString(lbltxt) );

    setIsRelative( isrel );
}


uiSelZRange::uiSelZRange( uiParent* p, const ZSampling& limitrg, bool wstep,
			  const char* lbltxt, const char* domky,
			  const char* zunitstr )
    : uiSelZRange(p,wstep,domky,zunitstr)
{
    if ( lbltxt && *lbltxt )
	setLabel( toUiString(lbltxt) );

    setRangeLimits( limitrg );
    setRange( limitrg );
}


uiSelZRange::~uiSelZRange()
{
    detachAllNotifiers();
}


void uiSelZRange::setLabel( const uiString& lbl )
{
    lblfld_->setText( lbl );
}


bool uiSelZRange::isSIDomain() const
{
    return zDomain().isCompatibleWith( SI().zDomainInfo() );
}


bool uiSelZRange::canSnap() const
{
    return isSIDomain() &&
	SI().zRange(false).step_ > cMaxUnsnappedZStep / zDomain().userFactor();
}


void uiSelZRange::displayStep( bool yn )
{
    if ( stepfld_ )
	stepfld_->display( yn );
}


void uiSelZRange::setIsRelative( bool yn )
{
    if ( yn == isrel_ )
	return;

    isrel_ = yn;
    if ( isrel_ )
	setRange( ZSampling(0.f,0.f,1.f) );
    else if ( isSIDomain() )
	setRange( SI().zRange(true) );
}


void uiSelZRange::makeInpFields( bool wstep )
{
    lblfld_ = new uiLabel( this, zDomain().getRange() );

    const float zfac = sCast( float, zDomain().userFactor() );
    ZSampling limitrg;
    if ( isSIDomain() )
	limitrg = SI().zRange( false );
    else
    {
	limitrg.set( zDomain().getReasonableZRange(true), 1.f );
	if ( limitrg.isUdf() )
	    limitrg.set( -sCast(float,cUnLim), sCast(float,cUnLim), 1.f );
    }

    limitrg.scale( zfac );

    const bool cansnap = canSnap();
    const int nrdecimals = cansnap ? 0
			 : zDomain().nrDecimals( limitrg.step_, false );
    const StepInterval<int> izrg( mNINT32(limitrg.start_),
				  mNINT32(limitrg.stop_),
				  mNINT32(limitrg.step_) );

    startfld_ = new uiSpinBox( this, nrdecimals, "Z start" );
    startfld_->attach( rightOf, lblfld_ );

    stopfld_ = new uiSpinBox( this, nrdecimals, "Z stop" );
    stopfld_->attach( rightOf, startfld_ );
    if ( nrdecimals==0 )
    {
	startfld_->setInterval( izrg );
	stopfld_->setInterval( izrg );
    }
    else
    {
	startfld_->setInterval( limitrg );
	stopfld_->setInterval( limitrg );
    }

    startfld_->doSnap( cansnap );
    stopfld_->doSnap( cansnap );

    if ( wstep )
    {
	stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep(), nrdecimals,
					 "Z step" );
	if ( nrdecimals==0 )
	    stepfld_->box()->setInterval(
		StepInterval<int>(izrg.step_,izrg.width(),izrg.step_) );
	else
	    stepfld_->box()->setInterval(
	     StepInterval<float>(limitrg.step_,limitrg.width(),limitrg.step_));

	stepfld_->box()->doSnap( cansnap );
	stepfld_->attach( rightOf, stopfld_ );
    }

    if ( isSIDomain() )
    {
	const ZSampling workzrg = SI().zRange( true );
        startfld_->setValue( workzrg.start_ );
        stopfld_->setValue( workzrg.stop_ );
	if ( wstep )
            stepfld_->box()->setValue( workzrg.step_ );
    }

    const CallBack cb( mCB(this,uiSelZRange,valChg) );
    startfld_->valueChanging.notify( cb );
    stopfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


ZSampling uiSelZRange::getRange() const
{
    ZSampling zrg( startfld_->getFValue(), stopfld_->getFValue(),
		   stepfld_ ? stepfld_->box()->getFValue() : 1.f );
    zrg.scale( 1.f / zDomain().userFactor() );
    if ( !stepfld_ && isSIDomain() )
        zrg.step_ = SI().zRange(true).step_;

    return zrg;
}


template <class T>
static void adaptRangeToLimits( const StepInterval<T>& rg,
				StepInterval<T>& limit,
				StepInterval<T>& newrg )
{
    if ( mIsUdf(rg.start_) || mIsUdf(rg.stop_) )
    {
	newrg = rg;
	return;
    }

    const double eps = 1e-4;
    if ( mIsZero(limit.step_,eps) || mIsUdf(limit.step_) )
        limit.step_ = 1;

    const double realstartdif = double(rg.start_) - double(limit.start_);
    const double realstartidx = realstartdif / double(limit.step_);
    const double realstepfac = double(rg.step_) / double(limit.step_);
    const bool useoldstep = !mIsZero(realstartidx-mNINT32(realstartidx),eps) ||
			    !mIsZero(realstepfac-mNINT32(realstepfac),eps);
    int stepfac = useoldstep ? 1 : mNINT32(realstepfac);
    if ( stepfac == 0 )
	stepfac = 1;

    int startidx = mNINT32( ceil(realstartidx-eps) );
    if ( startidx < 0 )
	startidx = (startidx*(1-stepfac)) % stepfac;

    const double width = mMIN(rg.stop_,limit.stop_) - limit.atIndex(startidx);
    const double realnrsteps = width / (stepfac*limit.step_);
    const int stopidx = startidx
			+ stepfac * mNINT32( Math::Floor(realnrsteps+eps) );

    if ( startidx <= stopidx )
    {
        newrg.start_ = limit.atIndex( startidx );
        newrg.stop_ = limit.atIndex( stopidx );
        newrg.step_ = stepfac * limit.step_;
    }
    else
	newrg = limit;
}


void uiSelZRange::setRange( const ZSampling& inpzrg )
{
    ZSampling zrg( inpzrg );
    zrg.scale( sCast(float,zDomain().userFactor()) );

    ZSampling limitrg = startfld_->getFInterval();
    ZSampling newzrg;
    adaptRangeToLimits<float>( zrg, limitrg, newzrg );

    const int nrdecimals = canSnap()
			 ? 0 : zDomain().nrDecimals( limitrg.step_, false );
    if ( nrdecimals==0 )
    {
        startfld_->setValue( mNINT32(newzrg.start_) );
        stopfld_->setValue( mNINT32(newzrg.stop_) );
	if ( stepfld_ )
            stepfld_->box()->setValue( mNINT32(newzrg.step_) );
    }
    else
    {
        startfld_->setValue( newzrg.start_ );
        stopfld_->setValue( newzrg.stop_ );
	if ( stepfld_ )
            stepfld_->box()->setValue( newzrg.step_ );
    }
}


void uiSelZRange::setRangeLimits( const ZSampling& zlimits )
{
    ZSampling zrg( zlimits );
    zrg.scale( sCast(float,zDomain().userFactor()) );
    startfld_->setInterval( zrg );
    stopfld_->setInterval( zrg );
    if ( stepfld_ )
    {
        stepfld_->box()->setMinValue( zrg.step_ );
        stepfld_->box()->setMaxValue( mMAX(zrg.step_, zrg.stop_-zrg.start_) );
        stepfld_->box()->setStep( zrg.step_ );
    }
}


void uiSelZRange::valChg( CallBacker* cb )
{
    if ( !isrel_ && startfld_->getFValue() > stopfld_->getFValue() )
    {
	const bool chgstart = cb == stopfld_;
	if ( chgstart )
	    startfld_->setValue( stopfld_->getFValue() );
	else
	    stopfld_->setValue( startfld_->getFValue() );
    }

    rangeChanged.trigger();
}



// uiSelNrRange

uiSelNrRange::uiSelNrRange( uiParent* p, uiSelNrRange::Type typ, bool wstep )
    : uiGroup(p,typ == Inl ? "In-line range selection"
	     : (typ == Crl ? "Cross-line range selection"
			   : "Number range selection"))
    , checked(this)
    , rangeChanged(this)
    , defstep_(1)
{
    StepInterval<int> rg( 1, mUdf(int), 1 );
    StepInterval<int> wrg( rg );
    lbltxt_ = tr("Number");
    if ( typ != Gen )
    {
	const TrcKeySampling& hs( SI().sampling(false).hsamp_ );
	rg = typ == Inl ? hs.inlRange() : hs.crlRange();
	const TrcKeySampling& whs( SI().sampling(true).hsamp_ );
	wrg = typ == Inl ? whs.inlRange() : whs.crlRange();
	lbltxt_ = typ == Inl ? uiStrings::sInline() : uiStrings::sCrossline();
	defstep_ = typ == Inl ? SI().inlStep() : SI().crlStep();
    }

    makeInpFields( rg, wstep, typ==Gen );
    setRange( wrg );
    mAttachCB( preFinalize(), uiSelNrRange::doFinalize );
}


uiSelNrRange::uiSelNrRange( uiParent* p, bool wstep, const uiString& lbltxt,
			    const StepInterval<int>* limitrg )
    : uiGroup(p,BufferString(toString(lbltxt)," range selection"))
    , lbltxt_(lbltxt)
    , defstep_(limitrg ? limitrg->step_ : 1)
    , checked(this)
    , rangeChanged(this)
{
    StepInterval<int> rg( 1, mUdf(int), 1 );
    if ( limitrg )
	rg = *limitrg;

    makeInpFields( rg, wstep, false );
    mAttachCB( preFinalize(), uiSelNrRange::doFinalize );
}


uiSelNrRange::uiSelNrRange( uiParent* p, const StepInterval<int>& limitrg,
			    bool wstep, const char* lbltxt )
    : uiSelNrRange(p,wstep,toUiString(lbltxt),&limitrg)
{
}


uiSelNrRange::~uiSelNrRange()
{
    detachAllNotifiers();
}


void uiSelNrRange::displayStep( bool yn )
{
    if ( stepfld_ )
	stepfld_->display( yn );
}


void uiSelNrRange::makeInpFields( StepInterval<int> limitrg, bool wstep,
				  bool isgen )
{
    const CallBack cb( mCB(this,uiSelNrRange,valChg) );
    const BufferString lbltxt = toString( lbltxt_ );
    startfld_ = new uiSpinBox( this, 0, BufferString(lbltxt," start") );
    startfld_->setInterval( limitrg );
    startfld_->doSnap( true );
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
	stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep(), 0,
					 BufferString(lbltxt," step") );
	stepfld_->box()->setInterval( StepInterval<int>(
			limitrg.step_,
			limitrg.width() ? limitrg.width() : limitrg.step_,
			limitrg.step_) );
	stepfld_->box()->doSnap( true );
	stepfld_->box()->valueChanging.notify( cb );
	if ( stopfld )
	    stepfld_->attach( rightOf, stopfld );
    }

    startfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


int uiSelNrRange::getStopVal() const
{
    if ( icstopfld_ )
	return icstopfld_->getIntValue();

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
    if ( startfld_->getIntValue() > getStopVal() )
    {
	const bool chgstop = cb == startfld_;
	if ( chgstop )
	    setStopVal( startfld_->getIntValue() );
	else
	    startfld_->setValue( getStopVal() );
    }

    rangeChanged.trigger();
}


void uiSelNrRange::doFinalize( CallBacker* )
{
    if ( finalized_ )
	return;

    const uiString lbl = uiStrings::phrJoinStrings( lbltxt_,
				uiStrings::sRange().toLower() );
    if ( withchk_ )
    {
	cbox_ = new uiCheckBox( this, lbl );
	cbox_->attach( leftTo, startfld_ );
	mAttachCB( cbox_->activated, uiSelNrRange::checkBoxSel );
	setChecked( checked_ );
	checkBoxSel( nullptr );
    }
    else
	new uiLabel( this, lbl, startfld_ );

    finalized_ = true;
}


StepInterval<int> uiSelNrRange::getRange() const
{
    return StepInterval<int>( startfld_->getIntValue(), getStopVal(),
			stepfld_ ? stepfld_->box()->getIntValue() : defstep_ );
}


void uiSelNrRange::setRange( const StepInterval<int>& rg )
{
    StepInterval<int> limitrg = startfld_->getInterval();
    StepInterval<int> newrg;
    adaptRangeToLimits<int>( rg, limitrg, newrg );

    startfld_->setValue( newrg.start_ );
    setStopVal( newrg.stop_ );
    if ( stepfld_ )
        stepfld_->box()->setValue( newrg.step_ );
}


void uiSelNrRange::setLimitRange( const StepInterval<int>& limitrg )
{
    startfld_->setInterval( limitrg );
    if ( icstopfld_ )
	icstopfld_->setInterval( limitrg );
    if ( stepfld_ )
    {
        stepfld_->box()->setMinValue( limitrg.step_ );
        stepfld_->box()->setMaxValue( mMAX(limitrg.step_,
                                           limitrg.stop_-limitrg.start_) );
        stepfld_->box()->setStep( limitrg.step_ );
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



// uiSelSteps

uiSelSteps::uiSelSteps( uiParent* p, bool is2d )
    : uiGroup(p,"Step selection")
{
    BinID stp( 0, 1 );
    uiString lbl = mJoinUiStrs( sTraceNumber(), sStep() );
    uiSpinBox* firstbox = nullptr;
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


uiSelSteps::~uiSelSteps()
{}


BinID uiSelSteps::getSteps() const
{
    return BinID( inlfld_ ? inlfld_->getIntValue() : 0,
		  crlfld_->getIntValue() );
}


void uiSelSteps::setSteps( const BinID& bid )
{
    crlfld_->setValue( bid.crl() );
    if ( inlfld_ )
	inlfld_->setValue( bid.inl() );
}



// uiSelHRange

uiSelHRange::uiSelHRange( uiParent* p, bool wstep,
			  const TrcKeySampling* hslimit )
    : uiGroup(p,"Hor range selection")
{
    inlfld_ = new uiSelNrRange( this, uiSelNrRange::Inl, wstep );
    crlfld_ = new uiSelNrRange( this, uiSelNrRange::Crl, wstep );
    crlfld_->attach( alignedBelow, inlfld_ );
    if ( hslimit )
    {
	const StepInterval<int> inlrg = hslimit->inlRange();
	const StepInterval<int> crlrg = hslimit->crlRange();
	inlfld_->setLimitRange( inlrg );
	crlfld_->setLimitRange( crlrg );
    }

    setHAlignObj( inlfld_ );
}


uiSelHRange::~uiSelHRange()
{
}


void uiSelHRange::displayStep( bool yn )
{
    inlfld_->displayStep( yn );
    crlfld_->displayStep( yn );
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



// uiSelSubvol

uiSelSubvol::uiSelSubvol( uiParent* p, bool wstep, const ZDomain::Info& zinfo )
    : uiGroup(p,"Sub vol selection")
    , hfld_(new uiSelHRange(this,wstep))
    , zfld_(new uiSelZRange(this,wstep,zinfo))
{
    zfld_->attach( alignedBelow, hfld_ );
    setHAlignObj( hfld_ );
}


uiSelSubvol::uiSelSubvol( uiParent* p, bool wstep, const char* zdomkey,
			  const char* zunitstr )
    : uiSelSubvol(p,wstep,ZDomain::Info::getFrom(zdomkey,zunitstr))
{}


uiSelSubvol::~uiSelSubvol()
{}


TrcKeyZSampling uiSelSubvol::getSampling() const
{
    TrcKeyZSampling tkzs( false );
    tkzs.hsamp_ = hfld_->getSampling();
    tkzs.zsamp_ = zfld_->getRange();
    return tkzs;
}


void uiSelSubvol::setSampling( const TrcKeyZSampling& tkzs )
{
    hfld_->setSampling( tkzs.hsamp_ );
    zfld_->setRange( tkzs.zsamp_ );
}


void uiSelSubvol::setLimits( const TrcKeyZSampling& tkzs )
{
    hfld_->setLimits( tkzs.hsamp_ );
    zfld_->setRangeLimits( tkzs.zsamp_ );
}
