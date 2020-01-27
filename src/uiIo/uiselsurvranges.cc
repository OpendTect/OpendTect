/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiselsurvranges.h"

#include "cubesubsel.h"
#include "linesubsel.h"
#include "keystrs.h"
#include "math.h"
#include "survgeommgr.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uistrings.h"

static const int cUnLim = 1000000;
static const float cMaxUnsnappedZStep = 0.999f;


uiSelRangeBase::uiSelRangeBase( uiParent* p, const char* grpnm )
    : uiGroup(p,grpnm)
    , rangeChanged(this)
{
}


uiSelRangeBase::~uiSelRangeBase()
{
}


void uiSelRangeBase::valChg( CallBacker* )
{
    rangeChanged.trigger( this );
}



#define mDefConstrList(isrel) \
    uiSelRangeBase(p,"Z range selection") \
    , stepfld_(0) \
    , isrel_(isrel) \
    , zddef_(ZDomain::Def::get(domky)) \
    , othdom_(&zddef_ != &ZDomain::SI()) \
    , cansnap_( !othdom_ \
	&& SI().zStep() > cMaxUnsnappedZStep / zddef_.userFactor() )


uiSelZRange::uiSelZRange( uiParent* p, bool wstep, bool isrel,
			  const uiString& lbltxt, const char* domky )
    : mDefConstrList(isrel)
{
    const ZSampling limitrg( SI().zRange() );
    makeInpFields( lbltxt, wstep, !othdom_ && !isrel_ ? &limitrg : 0 );
    if ( isrel_ )
	setRange( ZSampling(0,0,1) );
    else if ( !othdom_ )
	setRange( SI().zRange(OD::UsrWork) );
}


uiSelZRange::uiSelZRange( uiParent* p, ZSampling limitrg, bool wstep,
			  const uiString& lbltxt, const char* domky )
	: mDefConstrList(false)
{
    makeInpFields( lbltxt, wstep, &limitrg );
    setRange( limitrg );
}


uiSelZRange::~uiSelZRange()
{
    detachAllNotifiers();
}


void uiSelZRange::displayStep( bool yn )
{
    if ( stepfld_ )
	stepfld_->display( yn );
}


void uiSelZRange::makeInpFields( const uiString& lbltxt, bool wstep,
				 const ZSampling* inplimitrg )
{
    const float zfac = mCast( float, zddef_.userFactor() );
    const ZSampling& sizrg( SI().zRange() );

    ZSampling limitrg( -mCast(float,cUnLim), mCast(float,cUnLim), 1 );
    if ( inplimitrg )
	limitrg = *inplimitrg;
    else if ( !othdom_ && limitrg.step > sizrg.step )
	limitrg.step = sizrg.step;

    if ( mIsZero(limitrg.step,mDefEpsF) )
    {
	limitrg.step = sizrg.step;
	if ( mIsZero(limitrg.step,mDefEpsF) )
	    limitrg.step = 1.0f;
    }
    limitrg.scale( zfac );

    const int nrdecimals = cansnap_ ? 0 : 2;
    const trcnr_steprg_type izrg( mNINT32(limitrg.start),
				  mNINT32(limitrg.stop), mNINT32(limitrg.step));

    startfld_ = new uiSpinBox( this, nrdecimals, "Z start" );
    uiString ltxt( lbltxt );
    if ( ltxt.isEmpty() )
	ltxt = zddef_.getRange();

    new uiLabel( this, ltxt.withUnit(zddef_.unitStr()),
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
	stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep(), nrdecimals,
					 "Z step" );
	if ( cansnap_ )
	    stepfld_->box()->setInterval(
		trcnr_steprg_type(izrg.step,izrg.width(),izrg.step) );
	else
	    stepfld_->box()->setInterval(
	    trcnr_steprg_type(mNINT32(limitrg.step),mNINT32(limitrg.width()),
				  mNINT32(limitrg.step)) );
	stepfld_->box()->doSnap( cansnap_ );
	stepfld_->attach( rightOf, stopfld_ );
    }

    const CallBack cb( mCB(this,uiSelZRange,valChg) );
    startfld_->valueChanging.notify( cb );
    stopfld_->valueChanging.notify( cb );
    setHAlignObj( startfld_ );
}


ZSampling uiSelZRange::getRange() const
{
    ZSampling zrg( startfld_->getFValue(), stopfld_->getFValue(),
			     stepfld_ ? stepfld_->box()->getFValue() : 1 );
    zrg.scale( 1.f / zddef_.userFactor() );
    if ( !stepfld_ )
	zrg.step = SI().zStep( OD::UsrWork );
    return zrg;
}


template <class T>
static void adaptRangeToLimits( const StepInterval<T>& rg,
				StepInterval<T>& limit,
				StepInterval<T>& newrg )
{
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
    {
	newrg = rg;
	return;
    }

    const double eps = 1e-4;
    if ( mIsZero(limit.step,eps) || mIsUdf(limit.step) )
	limit.step = 1;

    const double realstartdif = double(rg.start) - double(limit.start);
    const double realstartidx = realstartdif / double(limit.step);
    const double realstepfac = double(rg.step) / double(limit.step);
    const bool useoldstep = !mIsZero(realstartidx-mNINT32(realstartidx),eps) ||
			    !mIsZero(realstepfac-mNINT32(realstepfac),eps);
    int stepfac = useoldstep ? 1 : mNINT32(realstepfac);
    if ( stepfac == 0 )
	stepfac = 1;

    int startidx = mNINT32( ceil(realstartidx-eps) );
    if ( startidx < 0 )
	startidx = (startidx*(1-stepfac)) % stepfac;

    const double width = mMIN(rg.stop,limit.stop) - limit.atIndex(startidx);
    const double realnrsteps = width / (stepfac*limit.step);
    const int stopidx = startidx
			+ stepfac * mNINT32( Math::Floor(realnrsteps+eps) );

    if ( startidx <= stopidx )
    {
	newrg.start = limit.atIndex( startidx );
	newrg.stop = limit.atIndex( stopidx );
	newrg.step = stepfac * limit.step;
    }
    else
	newrg = limit;
}


void uiSelZRange::setRange( const ZSampling& inpzrg )
{
    ZSampling zrg( inpzrg );
    zrg.scale( mCast(float,zddef_.userFactor()) );

    ZSampling limitrg = startfld_->getFInterval();
    ZSampling newzrg;
    adaptRangeToLimits<float>( zrg, limitrg, newzrg );

    if ( cansnap_ )
    {
	startfld_->setValue( mNINT32(newzrg.start) );
	stopfld_->setValue( mNINT32(newzrg.stop) );
	if ( stepfld_ )
	    stepfld_->box()->setValue( mNINT32(newzrg.step) );
    }
    else
    {
	startfld_->setValue( newzrg.start );
	stopfld_->setValue( newzrg.stop );
	if ( stepfld_ )
	    stepfld_->box()->setValue( newzrg.step );
    }
}


void uiSelZRange::valChg( CallBacker* cb )
{
    if ( !isrel_ && startfld_->getIntValue() > stopfld_->getIntValue() )
    {
	const bool chgstart = cb == stopfld_;
	if ( chgstart )
	    startfld_->setValue( stopfld_->getIntValue() );
	else
	    stopfld_->setValue( startfld_->getIntValue() );
    }

    uiSelRangeBase::valChg( cb );
}

void uiSelZRange::setRangeLimits( const ZSampling& zlimits )
{
    ZSampling zrg( zlimits );
    zrg.scale( mCast(float,zddef_.userFactor()) );
    startfld_->setInterval( zrg );
    stopfld_->setInterval( zrg );
    if ( stepfld_ )
    {
	stepfld_->box()->setMinValue( zrg.step );
	stepfld_->box()->setMaxValue( mMAX(zrg.step, zrg.stop-zrg.start) );
	stepfld_->box()->setStep( zrg.step );
    }
}


uiSelNrRange::uiSelNrRange( uiParent* p, uiSelNrRange::Type typ, bool wstep )
	: uiSelRangeBase(p,typ == Inl ? "In-line range selection"
			 : (typ == Crl ? "Cross-line range selection"
				       : (typ == Trc ? "Trace range selection"
						    :"Number range selection")))
	, defstep_(1)
	, checked(this)
	, finalised_(false)
	, checked_(false)
	, withchk_(false)
{
    trcnr_steprg_type rg( 1, mUdf(int), 1 );
    trcnr_steprg_type wrg( rg );
    const char* nm = "Number";
    lbltxt_ = uiStrings::sTraceRange();
    if ( typ < Trc )
    {
	const CubeSubSel css( OD::UsrWork );
	const CubeSubSel whs( OD::FullSurvey );
	rg = typ == Inl ? css.inlRange() : css.crlRange();
	wrg = typ == Inl ? whs.inlRange() : whs.crlRange();
	nm = typ == Inl ? sKey::Inline() : sKey::Crossline();
	defstep_ = typ == Inl ? SI().inlStep() : SI().crlStep();
	lbltxt_ = typ == Inl ? uiStrings::sInlineRange()
			     : uiStrings::sCrosslineRange();
    }
    fldnm_ = nm;
    makeInpFields( rg, wstep, typ==Gen );
    setRange( wrg );
    mAttachCB( preFinalise(), uiSelNrRange::doFinalise );
}


uiSelNrRange::uiSelNrRange( uiParent* p, trcnr_steprg_type limitrg, bool wstep,
			    const char* fldnm )
	: uiSelRangeBase(p,BufferString(fldnm," range selection"))
	, defstep_(limitrg.step)
	, checked(this)
	, finalised_(false)
	, withchk_(false)
	, checked_(false)
	, fldnm_(fldnm)
{
    if ( fldnm_.isEqual( sKey::Inline() ) ) // TODO Better way is to check
	lbltxt_ = uiStrings::sInlineRange(); // with uiSelNrRange::Type
    else if ( fldnm_.isEqual( sKey::Crossline() ) )
	lbltxt_ = uiStrings::sCrosslineRange();
    else if ( fldnm_.isEqual( sKey::Trace() ) )
	lbltxt_ = uiStrings::sTraceRange();
    else
	lbltxt_ = toUiString( BufferString(fldnm,sKey::Range()) );

    makeInpFields( limitrg, wstep, false );
    setRange( limitrg );
    mAttachCB( preFinalise(), uiSelNrRange::doFinalise );
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


void uiSelNrRange::makeInpFields( trcnr_steprg_type limitrg, bool wstep,
				  bool isgen )
{
    const CallBack cb( mCB(this,uiSelNrRange,valChg) );
    startfld_ = new uiSpinBox( this, 0, BufferString(fldnm_," start") );
    startfld_->setInterval( limitrg );
    startfld_->doSnap( true );
    uiObject* stopfld;
    if ( isgen )
    {
	stopfld = nrstopfld_ = new uiLineEdit( this,
					       BufferString(fldnm_," stop") );
	nrstopfld_->setHSzPol( uiObject::Small );
	nrstopfld_->editingFinished.notify( cb );
    }
    else
    {
	stopfld = icstopfld_ = new uiSpinBox( this, 0,
					      BufferString(fldnm_," stop") );
	icstopfld_->setInterval( limitrg );
	icstopfld_->doSnap( true );
	icstopfld_->valueChanging.notify( cb );
    }
    stopfld->attach( rightOf, startfld_ );

    if ( wstep )
    {
	stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep(), 0,
					 lbltxt_.getString().add(" step") );
	stepfld_->box()->setInterval( trcnr_steprg_type(limitrg.step,
			    limitrg.width() ? limitrg.width() : limitrg.step,
			    limitrg.step) );
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

    uiSelRangeBase::valChg( cb );
}


void uiSelNrRange::doFinalise( CallBacker* )
{
    if ( finalised_ ) return;

    if ( withchk_ )
    {
	cbox_ = new uiCheckBox( this, lbltxt_ );
	cbox_->attach( leftTo, startfld_ );
	cbox_->activated.notify( mCB(this,uiSelNrRange,checkBoxSel) );
	setChecked( checked_ );
	checkBoxSel(0);
    }
    else
	new uiLabel( this,  lbltxt_, startfld_ );

    finalised_ = true;
}


uiSelNrRange::trcnr_steprg_type uiSelNrRange::getRange() const
{
    return trcnr_steprg_type( startfld_->getIntValue(), getStopVal(),
			stepfld_ ? stepfld_->box()->getIntValue() : defstep_ );
}


void uiSelNrRange::setRange( const trcnr_steprg_type& rg )
{
    trcnr_steprg_type limitrg = startfld_->getInterval();
    trcnr_steprg_type newrg;
    adaptRangeToLimits<int>( rg, limitrg, newrg );

    startfld_->setValue( newrg.start );
    setStopVal( newrg.stop );
    if ( stepfld_ )
	stepfld_->box()->setValue( newrg.step );
}


void uiSelNrRange::setLimitRange( const trcnr_steprg_type& limitrg )
{
    startfld_->setInterval( limitrg );
    if ( icstopfld_ )
	icstopfld_->setInterval( limitrg );
    if ( stepfld_ )
    {
	stepfld_->box()->setMinValue( limitrg.step );
	stepfld_->box()->setMaxValue( mMAX(limitrg.step,
					   limitrg.stop-limitrg.start) );
	stepfld_->box()->setStep( limitrg.step );
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
    const bool elemsens = cbox_->isSensitive() && cbox_->isChecked();

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
{
    BinID stp( 0, 1 );
    uiString lbl = tr("Trace Number Step");
    uiSpinBox* firstbox = 0;
    if ( !is2d )
    {
	stp = BinID( SI().inlStep(), SI().crlStep() );
	firstbox = inlfld_ = new uiSpinBox( this, 0, "inline step" );
	inlfld_->setInterval( uiSelRangeBase::trcnr_steprg_type(stp.inl(),
							 cUnLim,stp.inl()) );
	inlfld_->doSnap( true );
	lbl = toUiString("%1/%2 %3")
	      .arg(uiStrings::sInline()).arg(uiStrings::sCrossline())
	      .arg(uiStrings::sSteps());
    }
    crlfld_ = new uiSpinBox( this, 0, "crossline step" );
    crlfld_->setInterval( uiSelRangeBase::trcnr_steprg_type(stp.crl(),cUnLim,
							    stp.crl()) );
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
    return BinID( inlfld_ ? inlfld_->getIntValue() : 0,
		  crlfld_->getIntValue() );
}


void uiSelSteps::setSteps( const BinID& bid )
{
    crlfld_->setValue( bid.crl() );
    if ( inlfld_ )
	inlfld_->setValue( bid.inl() );
}



uiSelHRange::uiSelHRange( uiParent* p, bool wstep )
    : uiSelRangeBase(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,uiSelNrRange::Inl,wstep))
    , crlfld_(new uiSelNrRange(this,uiSelNrRange::Crl,wstep))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    mAttachCB( inlfld_->rangeChanged, uiSelHRange::valChg );
    mAttachCB( crlfld_->rangeChanged, uiSelHRange::valChg );
    setHAlignObj( inlfld_ );
}


uiSelHRange::uiSelHRange( uiParent* p, const CubeHorSubSel& hslimit, bool wstep)
    : uiSelRangeBase(p,"Hor range selection")
    , inlfld_(new uiSelNrRange(this,hslimit.inlRange(),wstep,sKey::Inline()))
    , crlfld_(new uiSelNrRange(this,hslimit.crlRange(),wstep,sKey::Crossline()))
{
    crlfld_->attach( alignedBelow, inlfld_ );
    mAttachCB( inlfld_->rangeChanged, uiSelHRange::valChg );
    mAttachCB( crlfld_->rangeChanged, uiSelHRange::valChg );
    setHAlignObj( inlfld_ );
}


uiSelHRange::~uiSelHRange()
{
    detachAllNotifiers();
}


void uiSelHRange::displayStep( bool yn )
{
    inlfld_->displayStep( yn );
    crlfld_->displayStep( yn );
}


CubeHorSubSel uiSelHRange::getSampling() const
{
    CubeHorSubSel chss( OD::UsrWork );
    chss.setInlRange( inlfld_->getRange() );
    chss.setCrlRange( crlfld_->getRange() );
    return chss;
}


void uiSelHRange::setSampling( const CubeHorSubSel& chss )
{
    inlfld_->setRange( chss.inlRange() );
    crlfld_->setRange( chss.crlRange() );
}


void uiSelHRange::setLimits( const CubeHorSubSel& chss )
{
    inlfld_->setLimitRange( chss.inlRange() );
    crlfld_->setLimitRange( chss.crlRange() );
}



uiSelSubvol::uiSelSubvol( uiParent* p, bool wstep, bool withz,
			  const char* zdomkey, const CubeSubSel* css )
    : uiSelRangeBase(p,"Sub vol selection")
{
    hfld_ = new uiSelHRange( this, wstep );
    mAttachCB( hfld_->rangeChanged, uiSelSubvol::valChg );

    if ( withz )
    {
	zfld_ = new uiSelZRange( this, wstep, false, uiString(), zdomkey );
	mAttachCB( zfld_->rangeChanged, uiSelSubvol::valChg );
	zfld_->attach( alignedBelow, hfld_ );
    }

    if ( css )
	setSampling( *css );

    setHAlignObj( hfld_ );
}


uiSelSubvol::~uiSelSubvol()
{
    detachAllNotifiers();
}


CubeSubSel uiSelSubvol::getSampling() const
{
    CubeSubSel css;
    css.clearSubSel();
    css.cubeHorSubSel() = hfld_->getSampling();
    if ( hasZ() )
	css.setZRange( zfld_->getRange() );
    return css;
}


ZSampling uiSelSubvol::getZRange() const
{
    ZSampling zrg;
    if ( hasZ() )
	zrg = zfld_->getRange();
    return zrg;
}


void uiSelSubvol::setSampling( const CubeSubSel& css )
{
    hfld_->setSampling( css.cubeHorSubSel() );
    if ( hasZ() )
	zfld_->setRange( css.zRange() );
}


void uiSelSubvol::displayStep( bool yn )
{
    hfld_->displayStep( yn );
    if ( hasZ() )
	zfld_->displayStep( yn );
}



uiSelSubline::uiSelSubline( uiParent* p, Pos::GeomID gid, bool wstep,
			    bool withz )
    : uiSelRangeBase(p,"Sub line selection")
    , gid_(gid)
{
    const LineSubSel lss( gid );
    nrfld_ = new uiSelNrRange( this, uiSelNrRange::Trc, wstep );
    nrfld_->setLimitRange( lss.trcNrRange() );
    mAttachCB( nrfld_->rangeChanged, uiSelSubline::valChg );

    if ( withz )
    {
	zfld_ = new uiSelZRange( this, wstep );
	zfld_->attach( alignedBelow, nrfld_ );
	mAttachCB( zfld_->rangeChanged, uiSelSubline::valChg );
    }

    setHAlignObj( nrfld_ );
}


uiSelSubline::~uiSelSubline()
{
    detachAllNotifiers();
}


LineSubSel uiSelSubline::getSampling() const
{
    LineSubSel lss( gid_ );
    lss.setTrcNrRange( nrfld_->getRange() );
    if ( hasZ() )
	lss.setZRange( zfld_->getRange() );
    return lss;
}


uiSelSubline::trcnr_steprg_type uiSelSubline::getTrcNrRange() const
{
    return nrfld_->getRange();
}


ZSampling uiSelSubline::getZRange() const
{
    ZSampling zrg;
    if ( hasZ() )
	zrg = zfld_->getRange();
    return zrg;
}


void uiSelSubline::setSampling( const LineSubSel& lss )
{
    gid_ = lss.geomID();
    nrfld_->setRange( lss.trcNrRange() );
    if ( hasZ() )
	zfld_->setRange( lss.zRange() );
}


void uiSelSubline::displayStep( bool yn )
{
    nrfld_->displayStep( yn );
    if ( hasZ() )
	zfld_->displayStep( yn );
}



uiSelSublineSet::uiSelSublineSet( uiParent* p, bool withstep, bool withz,
				  const LineSubSelSet* lsss )
    : uiSelRangeBase(p,"Sub Geometries selection")
    , withstep_(withstep)
    , withz_(withz)
    , geomChanged(this)
{
    BufferStringSet linenms;
    if ( lsss )
	getLineNames( *lsss, linenms );
    linenmsfld_ = new uiGenInput( this,  tr("LineSel"),
				  StringListInpSpec(linenms) );
    mAttachCB( linenmsfld_->valuechanged, uiSelSublineSet::lineChgCB );
    if ( !lsss )
	return;

    for ( const auto lss : *lsss )
    {
	auto linergfld = new uiSelSubline( this, lss->geomID(), withstep,
					   withz );
	linergfld->setSampling( *lss );
	linergfld->attach( alignedBelow, linenmsfld_ );
	mAttachCB( linergfld->rangeChanged, uiSelSublineSet::valChg );
	linergsfld_.add( linergfld );
    }

    setHAlignObj( linenmsfld_->attachObj() );
    mAttachCB( postFinalise(), uiSelSublineSet::initGrp );
}


uiSelSublineSet::~uiSelSublineSet()
{
    detachAllNotifiers();
}


void uiSelSublineSet::initGrp( CallBacker* cb )
{
    lineChgCB(cb);
}


void uiSelSublineSet::lineChgCB( CallBacker* cb )
{
    if ( linergsfld_.isEmpty() )
	return;

    const BufferString curlinm( linenmsfld_->text() );
    uiSelSubline* activefld = nullptr;
    for ( auto linergfld : linergsfld_ )
    {
	const Pos::GeomID gid = linergfld->getGeomID();
	const BufferString linenm( Survey::GM().getName(gid) );
	const bool dodisplay = linenm == curlinm;
	linergfld->display( dodisplay );
	if ( dodisplay )
	    activefld = linergfld;
    }

    geomChanged.trigger( activefld ? activefld->getGeomID() : mUdfGeomID );
}


void uiSelSublineSet::adaptLineSelIfNeeded( const LineSubSelSet& lsss )
{
    mDynamicCastGet(const StringListInpSpec*,stringspec,
		    linenmsfld_->dataInpSpec() )
    BufferStringSet curlinnms;
    if ( stringspec )
    {
	for ( auto linenm : stringspec->strings() )
	    curlinnms.add( linenm->getOriginalString() );
    }

    BufferStringSet linenms;
    getLineNames( lsss, linenms );
    if ( linenms == curlinnms )
	return;

    const BufferString curlinnm( linenmsfld_->text() );
    const int defidx = linenms.isPresent(curlinnm) ? linenms.indexOf(curlinnm)
						   : 0;
    if ( finalised() )
	linenmsfld_->setEmpty();
    linenmsfld_->newSpec( StringListInpSpec(linenms), defidx );
    if ( linenms.isPresent(curlinnm) )
	linenmsfld_->setText( curlinnm );

    const int prevnrlines = linergsfld_.size();
    if ( prevnrlines < linenms.size() )
    {
	for ( int idx=prevnrlines; idx<linenms.size(); idx++ )
	{
	    auto linergfld = new uiSelSubline( this, lsss[idx]->geomID(),
					       withstep_, withz_ );
	    linergfld->attach( alignedBelow, linenmsfld_ );
	    mAttachCB( linergfld->rangeChanged, uiSelSublineSet::valChg );
	    linergsfld_.add( linergfld );
	}
    }
    else if ( prevnrlines > linenms.size() )
	linergsfld_.removeRange( linenms.size(), prevnrlines );
}


void uiSelSublineSet::displayStep( bool yn )
{
    for ( auto linergfld : linergsfld_ )
	linergfld->displayStep( yn );
}


LineSubSelSet uiSelSublineSet::getSampling() const
{
    LineSubSelSet lsss;
    for ( const auto linergfld : linergsfld_ )
	lsss.add( new LineSubSel(linergfld->getSampling()) );
    return lsss;
}


void uiSelSublineSet::setSampling( const LineSubSelSet& lsss )
{
    adaptLineSelIfNeeded( lsss );
    for ( int idx=0; idx<lsss.size(); idx++ )
	linergsfld_.get(idx)->setSampling( *lsss[idx] );
    lineChgCB( nullptr );
}


void uiSelSublineSet::getLineNames( const LineSubSelSet& lsss,
				 BufferStringSet& linenms )
{
    linenms.setEmpty();
    for ( const auto lss : lsss )
	linenms.add( Survey::GM().getName( lss->geomID() ) );
}
