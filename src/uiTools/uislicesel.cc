/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uislicesel.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitoolbutton.h"

#include "flatview.h"
#include "posinfo2dsurv.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "thread.h"
#include "timer.h"
#include "od_helpids.h"
#include "zdomain.h"

class uiSliderSettings : public uiDialog
{ mODTextTranslationClass(uiSliderSettings)
public:
uiSliderSettings( uiParent* p, int step, float dt,
		  uiSliceSel::AutoScrollType astype )
    : uiDialog( p, uiDialog::Setup(tr("Slider settings"),mNoHelpKey) )
    , timebwupdates_( dt )
    , astype_( astype )
{
    auto* lblspb = new uiLabeledSpinBox( this, uiStrings::sStep(), 0 );
    stepfld_ = lblspb->box();
    stepfld_->setValue( step );
    if ( mIsUdf(step_) )
	stepfld_->setMinValue( step );

    dtfld_ = new uiGenInput( this, tr("Time between updates (s)"),
			     FloatInpSpec(2) );
    dtfld_->setValue( timebwupdates_ );
    dtfld_->attach( alignedBelow, lblspb->attachObj() );

    uiStringSet cbstrs;
    cbstrs.add( tr("Stop") ).add( tr("Continue reversed") )
			    .add( tr("Wrap around") );
    auto* lblcb = new uiLabeledComboBox( this, cbstrs,
					 tr("Upon hitting a boundary") );
    lblcb->attach( alignedBelow, dtfld_ );
    boundaryfld_ = lblcb->box();
    boundaryfld_->setCurrentItem( (int)astype_ );
}


~uiSliderSettings()
{}


int getStep() const
{
    return step_;
}


float getDT() const
{
    return timebwupdates_;
}


uiSliceSel::AutoScrollType getBoundaryBehaviour() const
{
    return astype_;
}


void disableAutoScrollSettings()
{
    dtfld_->setSensitive( false );
}


private:

bool acceptOK( CallBacker* ) override
{
    step_ = stepfld_->getIntValue();
    if ( mIsUdf(step_) || step_<=0 )
    {
	uiMSG().error( tr("Please ensure that the specified \"Step\" "
			  "value is positive") );
	return false;
    }

    timebwupdates_ = dtfld_->getFValue();
    const int astypsel = boundaryfld_->currentItem();
    astype_ = sCast( uiSliceSel::AutoScrollType, astypsel );
    return true;
}

    uiSpinBox*	    stepfld_;
    uiGenInput*     dtfld_;
    uiComboBox*     boundaryfld_;

    int		    step_		    = mUdf(int);
    float	    timebwupdates_	    = 2.f;
    uiSliceSel::AutoScrollType astype_	    = uiSliceSel::AutoScrollType::Stop;

};


// uiSliceSel
uiSliceSel::uiSliceSel( uiParent* p, Type type, const ZDomain::Info& zi,
			const Pos::GeomID& gid, bool withscroll,
			const ZDomain::Info* dispzi )
    : uiGroup(p,"Slice Selection")
    , type_(type)
    , dogeomcheck_(gid.is3D())
    , zdominfo_(zi)
    , dispzdominfo_(dispzi?*dispzi:(zi.isDepth()?ZDomain::DefaultDepth():zi))
    , sliderMoved(this)
{
    tkzs_.init( gid );
    maxcs_.init( gid );

    if ( (isInl() || isCrl() || isZSlice()) && withscroll )
    {
	timer_ = new Timer( "uiSliceSel timer" );
	mAttachCB( timer_->tick, uiSliceSel::timerTickCB );
	scrollgrp_ = new uiGroup( this, "Scroll group" );

	auto* mvgrp = new uiGroup( scrollgrp_, "Movement group" );
	uiString prevttip = tr( "Previous position \n\nShortcut: \"z\"" );
	uiString nextttip = tr( "Next position \n\nShortcut: \"x\"" );
	auto* prevbut = new uiToolButton( mvgrp, "prevpos", prevttip,
				     mCB(this,uiSliceSel,prevCB) );
	slider_ = new uiSlider( mvgrp, uiSlider::Setup(), "Slice slider" );
	slider_->attach( rightOf, prevbut );
	mAttachCB( slider_->sliderMoved, uiSliceSel::sliderMovedCB );
	mAttachCB( slider_->sliderReleased, uiSliceSel::sliderReleasedCB );
	auto* nextbut = new uiToolButton( mvgrp, "nextpos", nextttip,
				     mCB(this,uiSliceSel,nextCB) );
	nextbut->attach( rightOf, slider_ );
	auto* settingsbut_ = new uiToolButton( mvgrp, "settings",
				     tr("Movement settings"),
				     mCB(this,uiSliceSel,settingsCB) );
	settingsbut_->attach( rightOf, nextbut );

	auto* playgrp = new uiButtonGroup( scrollgrp_,
					   "Auto-play group", OD::Horizontal );
	playgrp->attach( centeredBelow, mvgrp );
	uiToolButtonSetup revsu( "reverse", tr("Auto-scroll backwards"),
				 mCB(this,uiSliceSel,playRevCB) );
	revsu.istoggle( true );
	playrevbut_ = new uiToolButton( playgrp, revsu );
	uiToolButtonSetup psu( "resume", tr("Start auto-scroll"),
			       mCB(this,uiSliceSel,playPauseCB) );
	playpausebut_ = new uiToolButton( playgrp, psu );
	uiToolButtonSetup fsu( "forward", tr("Auto-scroll forwards"),
			       mCB(this,uiSliceSel,playForwardCB) );
	fsu.istoggle( true );
	playforwardbut_ = new uiToolButton( playgrp, fsu );

	auto* sep = new uiSeparator( scrollgrp_ );
	sep->attach( stretchedBelow, playgrp );
    }

    posgrp_ = new uiGroup( this, "Position group" );
    if ( scrollgrp_ )
	posgrp_->attach( centeredBelow, scrollgrp_ );

    if ( is3DSlice() )
	createInlFld();

    createCrlFld();
    createZFld();

    if ( inl0fld_ )
	mainObject()->setTabOrder( (uiObject*)inl0fld_, (uiObject*)crl0fld_ );

    mainObject()->setTabOrder( (uiObject*)crl0fld_, (uiObject*)z0fld_ );

    if ( is3DSlice() )
    {
	applybut_ = uiButton::getStd( posgrp_, OD::Apply,
				    mCB(this,uiSliceSel,applyPush), true );
	mainObject()->setTabOrder( (uiObject*)z0fld_, (uiObject*)applybut_ );
	applybut_->attach( alignedBelow, z0fld_ );
	applybut_->display( false );
    }

    if ( isVol() )
    {
	auto* fullbut = new uiToolButton( posgrp_, "exttofullsurv",
					tr("Set ranges to full survey"),
					mCB(this,uiSliceSel,fullPush) );
	fullbut->attach( rightTo, inl1fld_ );
    }

    setHAlignObj( crl0fld_ );
    mAttachCB( postFinalize(), uiSliceSel::initGrp );
}


uiSliceSel::~uiSliceSel()
{
    detachAllNotifiers();
    delete applycb_;
    delete timer_;
}


void uiSliceSel::initGrp( CallBacker* )
{
    updateUI();
}


bool uiSliceSel::useTrcNr() const
{
    return isInl() || is2DSlice();
}


bool uiSliceSel::is2DSlice() const
{
    return is2D() || isSynth();
}


bool uiSliceSel::is3DSlice() const
{
    return isInl() || isCrl() || isZSlice() || isVol();
}


bool uiSliceSel::is2DSlice( Type typ )
{
    return typ == TwoD || typ == Synth;
}


bool uiSliceSel::is3DSlice( Type typ )
{
    return typ == Inl || typ == Crl || typ == Tsl || typ == Vol;
}


bool uiSliceSel::isSliderActive() const
{
    return slideractive_;
}


uiSliceSel::Type uiSliceSel::getType( const TrcKeyZSampling& tkzs )
{
    if ( tkzs.is2D() )
	return TwoD;
    if ( tkzs.isSynthetic() )
	return Synth;
    if ( !tkzs.isFlat() )
	return Vol;

    const TrcKeyZSampling::Dir prefdir = tkzs.defaultDir();
    return prefdir == TrcKeyZSampling::Inl ? Inl
					   : (prefdir == TrcKeyZSampling::Crl
						   ? Crl : Tsl);
}


uiString uiSliceSel::sButTxtAdvance()
{
    return tr("Advance >>");
}


uiString uiSliceSel::sButTxtPause()
{
    return tr("Pause");
}


void uiSliceSel::setApplyCB( const CallBack& acb )
{
    delete applycb_;
    applycb_ = new CallBack( acb );
    if ( applybut_ )
	applybut_->display( true );
}


void uiSliceSel::prevCB( CallBacker* cb )
{
    doPrevious( slider_->step() );
}


void uiSliceSel::doPrevious( int step )
{
    doMove( -step );
}


void uiSliceSel::nextCB( CallBacker* cb )
{
    doNext( slider_->step() );
}


void uiSliceSel::doNext( int step )
{
    doMove( step );
}


void uiSliceSel::doMove( int step )
{
    StepInterval<float> sliderintv;
    slider_->getInterval( sliderintv );
    const float currval = slider_->getFValue();
    float nextval = currval + step;
    if ( !sliderintv.includes(nextval,true) && asprops_.autoon_ )
    {
	if ( asprops_.astype_ == AutoScrollType::Stop )
	{
	    asprops_.autoon_ = false;
	    playpausebut_->setIcon( "resume" );
	    stopAuto();
	}
	else if ( asprops_.astype_ == AutoScrollType::ContRev )
	{
	    playPauseCB( nullptr );
	    asprops_.isforward_ = !asprops_.isforward_;
	    playPauseCB( nullptr );
	}
	else
	    nextval = asprops_.isforward_ ? sliderintv.start_
					  : sliderintv.stop_;
    }

    slider_->setValue( nextval );
    sliderReleasedCB( nullptr );
}


void uiSliceSel::sliderMovedCB( CallBacker* cb )
{
    slideractive_ = true;
    sliderValChanged();
    sliderMoved.trigger();
}


void uiSliceSel::sliderReleasedCB( CallBacker* cb )
{
    slideractive_ = false;
    sliderValChanged();
    applyPush( nullptr );
}


void uiSliceSel::sliderValChanged()
{
    if ( isInl() )
    {
	const int inlnr = slider_->getIntValue();
	inl0fld_->box()->setValue( inlnr );
	tkzs_.hsamp_.start_.inl() = tkzs_.hsamp_.stop_.inl() = inlnr;
    }
    else if ( isCrl() )
    {
	const int crlnr = slider_->getIntValue();
	crl0fld_->box()->setValue( crlnr );
	tkzs_.hsamp_.start_.crl() = tkzs_.hsamp_.stop_.crl() = crlnr;
    }
    else if ( isZSlice() )
    {
	const float zval = slider_->getFValue();
	z0fld_->box()->setValue( zval );
	const float zfac = userFactor();
	tkzs_.zsamp_.start_ = tkzs_.zsamp_.stop_ = zval / zfac;
    }
}


void uiSliceSel::settingsCB( CallBacker* cb )
{
    uiSliderSettings dlg( this, asprops_.step_,
			  asprops_.dt_, asprops_.astype_ );
    if ( !asprops_.enabled_ )
	dlg.disableAutoScrollSettings();

    if ( dlg.go() == uiDialog::Accepted )
    {
	asprops_.step_ = dlg.getStep();
	asprops_.dt_ = dlg.getDT();
	asprops_.astype_ = dlg.getBoundaryBehaviour();
    }
}


void uiSliceSel::playRevCB( CallBacker* cb )
{
    const bool ison = playrevbut_->isOn();
    if ( ison )
    {
	if ( playforwardbut_->isOn() )
	{
	    NotifyStopper revns( playforwardbut_->activated );
	    playforwardbut_->setOn( false );
	}

	asprops_.isforward_ = false;
	asprops_.autoon_ = true;
	playpausebut_->setIcon( "pause" );
	doAuto();
    }
    else
    {
	asprops_.autoon_ = false;
	playpausebut_->setIcon( "resume" );
	stopAuto();
    }
}


void uiSliceSel::playPauseCB( CallBacker* cb )
{
    if ( asprops_.autoon_ )
    {
	asprops_.autoon_ = false;
	playpausebut_->setIcon( "resume" );
	stopAuto();
    }
    else
    {
	asprops_.autoon_ = true;
	playpausebut_->setIcon( "pause" );
	if ( asprops_.isforward_ )
	{
	    playforwardbut_->setOn( true );
	    playForwardCB( nullptr );
	}
	else
	{
	    playrevbut_->setOn( true );
	    playRevCB( nullptr );
	}
    }
}


void uiSliceSel::playForwardCB( CallBacker* cb )
{
    const bool ison = playforwardbut_->isOn();
    if ( ison )
    {
	if ( playrevbut_->isOn()  )
	{
	    NotifyStopper revns( playrevbut_->activated );
	    playrevbut_->setOn( false );
	}

	asprops_.isforward_ = true;
	asprops_.autoon_ = true;
	playpausebut_->setIcon( "pause" );
	doAuto();
    }
    else
    {
	asprops_.autoon_ = false;
	playpausebut_->setIcon( "resume" );
	stopAuto();
    }

}


void uiSliceSel::doAuto()
{
    asprops_.isforward_ ? doNext( asprops_.step_ )
			: doPrevious( asprops_.step_ );
    setTimer();
}


void uiSliceSel::stopAuto()
{
    timer_->stop();
    asprops_.autoon_ = false;
}


void uiSliceSel::disableAutoScroll( bool yn )
{
    playrevbut_->setSensitive( !yn );
    playpausebut_->setSensitive( !yn );
    playforwardbut_->setSensitive( !yn );
    asprops_.enabled_ = !yn;
}


void uiSliceSel::timerTickCB( CallBacker* cb )
{
    if ( !asprops_.autoon_ )
	return;

    doAuto();
}


void uiSliceSel::setTimer()
{
    if ( !timer_ )
	return;

    float val = asprops_.dt_;
    if ( mIsUdf(val) || val < 0.2 )
	val = 200;
    else
	val *= 1000;

    timer_->start( mNINT32(val), true );
}


void uiSliceSel::createInlFld()
{
    const bool isinl = isInl();
    const uiString label = isinl ? uiStrings::sInline()
				 : uiStrings::sInlineRange();
    inl0fld_ = new uiLabeledSpinBox( posgrp_, label, 0,
			BufferString(isinl ? "Inl nr" : "Inl Start") );
    inl1fld_ = new uiSpinBox( posgrp_, 0, "Inl Stop" );
    inl1fld_->attach( rightTo, inl0fld_ );
    inl1fld_->display( !isinl );
}


void uiSliceSel::createCrlFld()
{
    const bool iscrl = isCrl();
    const uiString label = is2DSlice() ? uiStrings::sTraceRange()
			   : (iscrl ? uiStrings::sCrossline()
				    : uiStrings::sCrosslineRange());
    crl0fld_ = new uiLabeledSpinBox( posgrp_, label, 0,
			 BufferString( iscrl ? "Crl nr" : "Crl Start ") );
    crl1fld_ = new uiSpinBox( posgrp_, 0, "Crl Stop" );
    crl1fld_->attach( rightTo, crl0fld_ );
    crl1fld_->display( !iscrl );
    if ( inl0fld_ )
	crl0fld_->attach( alignedBelow, inl0fld_ );
}


void uiSliceSel::createZFld()
{
    const bool iszslice = isZSlice();
    uiString label = tr("%1 %2")
		.arg(iszslice ? uiStrings::sZ() : uiStrings::sZRange())
		.arg(zDomain(true).uiUnitStr(true));
    z0fld_ = new uiLabeledSpinBox( posgrp_, label, 0,
				   iszslice ? "Z" : "Z Start" );
    z1fld_ = new uiSpinBox( posgrp_, 0, "Z Stop" );
    z1fld_->attach( rightTo, z0fld_ );
    z1fld_->display( !iszslice );
    z0fld_->attach( alignedBelow, crl0fld_ );
}


void uiSliceSel::setBoxValues( uiSpinBox* box, const StepInterval<int>& intv,
			       int curval )
{
    box->setInterval( intv.start_, intv.stop_ );
    box->setStep( intv.step_, true );
    box->setValue( curval );
}


void uiSliceSel::applyPush( CallBacker* )
{
    Threads::Locker lckr( updatelock_, Threads::Locker::DontWaitForLock );
    if ( !lckr.isLocked() )
	return;

    readInput();
    if ( applycb_ )
	applycb_->doCall(this);
}


void uiSliceSel::fullPush( CallBacker* )
{
    setTrcKeyZSampling( maxcs_ );
}


void uiSliceSel::readInput()
{
    const TrcKeySampling& hs = maxcs_.hsamp_;
    Interval<int> inlrg, crlrg;
    hs.get( inlrg, crlrg );
    if ( inl0fld_ )
    {
	inlrg.start_ = inl0fld_->box()->getIntValue();
	inlrg.stop_ = isInl() ? inlrg.start_ : inl1fld_->getIntValue();
	if ( !isInl() && inlrg.start_ == inlrg.stop_ )
	    inlrg.stop_ += hs.step_.inl();
    }

    crlrg.start_ = crl0fld_->box()->getIntValue();
    crlrg.stop_ = isCrl() ? crlrg.start_ : crl1fld_->getIntValue();
    if ( !isCrl() && crlrg.start_ == crlrg.stop_ )
	crlrg.stop_ += hs.step_.crl();

    const float zfac = userFactor();
    Interval<float> zrg;
    zrg.start_ = z0fld_->box()->getFValue() / zfac;
    zrg.start_ = maxcs_.zsamp_.snap( zrg.start_ );
    if ( isZSlice() )
	zrg.stop_ = zrg.start_;
    else
    {
	zrg.stop_ = z1fld_->getFValue() / zfac;
	zrg.sort();
	zrg.stop_ = maxcs_.zsamp_.snap( zrg.stop_ );
	if ( mIsEqual(zrg.start_,zrg.stop_,mDefEps) )
	    zrg.stop_ += maxcs_.zsamp_.step_;
    }

    if ( is3DSlice() )
	tkzs_.hsamp_.setLineRange( inlrg );

    tkzs_.hsamp_.setTrcRange(  crlrg );
    tkzs_.zsamp_.setInterval( zrg );

    if ( dogeomcheck_ && is3DSlice() )
    {
	SI().snap( tkzs_.hsamp_.start_ );
	SI().snap( tkzs_.hsamp_.stop_ );
    }
}


void uiSliceSel::updateUI()
{
    if ( inl0fld_ )
    {
	Interval<int> inlrg( tkzs_.hsamp_.start_.inl(),
			     tkzs_.hsamp_.stop_.inl() );
	StepInterval<int> maxinlrg( maxcs_.hsamp_.start_.inl(),
				    maxcs_.hsamp_.stop_.inl(),
				    maxcs_.hsamp_.step_.inl() );
	setBoxValues( inl0fld_->box(), maxinlrg, inlrg.start_ );
	setBoxValues( inl1fld_, maxinlrg, inlrg.stop_ );
	if ( scrollgrp_ && isInl() )
	{
	    slider_->setInterval( maxinlrg );
	    slider_->setValue( inlrg.start_ );
	    if ( mIsUdf(asprops_.step_) )
		asprops_.step_ = maxinlrg.step_;
	}
    }

    Interval<int> crlrg( tkzs_.hsamp_.start_.crl(), tkzs_.hsamp_.stop_.crl() );
    StepInterval<int> maxcrlrg( maxcs_.hsamp_.start_.crl(),
				maxcs_.hsamp_.stop_.crl(),
				maxcs_.hsamp_.step_.crl() );
    setBoxValues( crl0fld_->box(), maxcrlrg, crlrg.start_ );
    setBoxValues( crl1fld_, maxcrlrg, crlrg.stop_ );
    if ( scrollgrp_ && isCrl() )
    {
	slider_->setInterval( maxcrlrg );
	slider_->setValue( crlrg.stop_ );
	if ( mIsUdf(asprops_.step_) )
	    asprops_.step_ = maxcrlrg.step_;
    }

    const float zfac = userFactor();
    const int nrdec = nrDec();
    if ( nrdec==0 )
    {
	Interval<int> zrg( mNINT32(tkzs_.zsamp_.start_*zfac),
			   mNINT32(tkzs_.zsamp_.stop_*zfac) );
	StepInterval<int> maxzrg =
		StepInterval<int>( mNINT32(maxcs_.zsamp_.start_*zfac),
				   mNINT32(maxcs_.zsamp_.stop_*zfac),
				   mNINT32(maxcs_.zsamp_.step_*zfac) );
	setBoxValues( z0fld_->box(), maxzrg, zrg.start_ );
	setBoxValues( z1fld_, maxzrg, zrg.stop_ );
    }
    else
    {
	StepInterval<float> zrg = tkzs_.zsamp_;
	zrg.scale( zfac );
	StepInterval<float> maxzrg = maxcs_.zsamp_;
	maxzrg.scale( zfac );

	z0fld_->box()->setInterval( maxzrg );
	z0fld_->box()->setValue( tkzs_.zsamp_.start_*zfac );

	z1fld_->setInterval( maxzrg );
	z1fld_->setValue( tkzs_.zsamp_.stop_*zfac );
	if ( scrollgrp_ && isZSlice() )
	{
	    slider_->setInterval( maxzrg );
	    slider_->setValue( tkzs_.zsamp_.start_*zfac );
	    if ( mIsUdf(asprops_.step_) )
		asprops_.step_ = maxzrg.step_;
	}
    }

    z0fld_->box()->setNrDecimals( nrdec );
    z1fld_->setNrDecimals( nrdec );
}


void uiSliceSel::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    if ( cs.hsamp_.getGeomID() != tkzs_.hsamp_.getGeomID() )
	{ pErrMsg("Invalid geomID"); }

    tkzs_ = cs;
    updateUI();
}


void uiSliceSel::setMaxTrcKeyZSampling( const TrcKeyZSampling& maxcs )
{
    if ( maxcs.hsamp_.getGeomID() != maxcs_.hsamp_.getGeomID() )
	{ pErrMsg("Invalid geomID"); }

    maxcs_ = maxcs;
    updateUI();
}


bool uiSliceSel::acceptOK()
{
#ifdef __mac__
    crl0fld_->setFocus();
    crl1fld_->setFocus(); // Hack
#endif
    readInput();
    return true;
}


void uiSliceSel::enableApplyButton( bool yn )
{
    if ( !applybut_ )
	return;

    applybut_->display( yn );
}


void uiSliceSel::fillPar( IOPar& iop )
{
    TrcKeyZSampling cs;
    if ( is3DSlice() )
    {
	cs.init( Survey::default3DGeomID() );
	cs.hsamp_.start_.inl() = inl0fld_->box()->getIntValue();
	cs.hsamp_.stop_.inl() = isInl () ? inl0fld_->box()->getIntValue()
					 : inl1fld_->getIntValue();
    }
    else if ( is2DSlice() )
    {
	if ( isSynth() )
	    cs = TrcKeyZSampling::getSynth();
	else
	    cs.init( Survey::getDefault2DGeomID() );
    }

    cs.hsamp_.start_.crl() = crl0fld_->box()->getIntValue();
    cs.hsamp_.stop_.crl() = isCrl() ? crl0fld_->box()->getIntValue()
				    : crl1fld_->getIntValue();

    cs.zsamp_.start_ = float( z0fld_->box()->getIntValue() );
    cs.zsamp_.stop_ = float( isZSlice() ? z0fld_->box()->getIntValue()
				       : z1fld_->getIntValue() );
    cs.fillPar( iop );
}


void uiSliceSel::usePar( const IOPar& par )
{
    if ( is3DSlice() )
    {
	int inlnr = mUdf(int);
	if ( par.get(sKey::FirstInl(),inlnr) && !mIsUdf(inlnr) )
	    inl0fld_->box()->setValue( inlnr );

	if ( inl1fld_->isDisplayed() )
	{
	    int inl1 = mUdf(int);
	    if ( par.get(sKey::LastInl(),inl1) && !mIsUdf(inl1) )
		inl1fld_->setValue( inl1 );
	}
    }

    int crl0 = mUdf(int);
    if ( par.get(sKey::FirstCrl(),crl0) && !mIsUdf(crl0) )
	crl0fld_->box()->setValue( crl0 );

    if ( crl1fld_->isDisplayed() )
    {
	int crl1 = mUdf(int);
	if ( par.get(sKey::LastCrl(),crl1) && !mIsUdf(crl1) )
	    crl1fld_->setValue( crl1 );
    }

    ZSampling zrg = ZSampling::udf();
    if ( par.get(sKey::ZRange(),zrg)  && !zrg.isUdf() )
    {
	z0fld_->box()->setValue( zrg.start_ );
	if ( z1fld_->isDisplayed() )
	    z1fld_->setValue( zrg.stop_ );
    }
}


const ZDomain::Info& uiSliceSel::zDomain( bool fordisplay ) const
{
    return fordisplay ? dispzdominfo_ : zdominfo_;
}


float uiSliceSel::userFactor()
{
    return FlatView::Viewer::userFactor( zdominfo_, &dispzdominfo_ );
}


int uiSliceSel::nrDec()
{
    return FlatView::Viewer::nrDec( dispzdominfo_ );
}

uiString dlgTitle( uiSliceSel::Type type )
{
    uiString ret = ::toUiString( "Positioning" );
    if ( type == uiSliceSel::Inl )
	ret.append( ": ").append( uiStrings::sInline() );
    else if ( type == uiSliceSel::Crl )
	ret.append( ": ").append( uiStrings::sCrossline() );
    else if ( type == uiSliceSel::Tsl )
	ret.append( ": ").append( uiStrings::sZSlice() );

    return ret;
}

//uiSliceSelDlg

uiSliceSelDlg::uiSliceSelDlg( uiParent* p, const TrcKeyZSampling& curcs,
			const TrcKeyZSampling& maxcs,
			const CallBack& acb, uiSliceSel::Type type,
			const ZDomain::Info& zdominfo, bool withscroll )
    : uiDialog(p,Setup( dlgTitle(type), tr("Specify the element's position"),
		       mODHelpKey(mSliceSelHelpID))
		    .modal(type==uiSliceSel::Vol || type==uiSliceSel::TwoD ||
			   type==uiSliceSel::Synth))
{
    slicesel_ = new uiSliceSel( this, type, zdominfo,
				curcs.hsamp_.getGeomID(), withscroll );
    slicesel_->setMaxTrcKeyZSampling( maxcs );
    slicesel_->setTrcKeyZSampling( curcs );
    slicesel_->setApplyCB( acb );
}


uiSliceSelDlg::~uiSliceSelDlg()
{}


bool uiSliceSelDlg::acceptOK( CallBacker* )
{
    slicesel_->stopAuto();
    return slicesel_->acceptOK();
}


bool uiSliceSelDlg::rejectOK( CallBacker* )
{
    slicesel_->stopAuto();
    return uiDialog::rejectOK( nullptr );
}


// uiLinePosSelDlg

uiLinePosSelDlg::uiLinePosSelDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Select line position"),mNoHelpKey))
    , tkzs_(Survey::getDefault2DGeomID())
{
    BufferStringSet linenames;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( linenames, geomids, true );
    if ( !geomids.isEmpty() )
	tkzs_.hsamp_.init( geomids.first() );

    linesfld_ = new uiGenInput( this, tr("Compute on line:"),
				StringListInpSpec(linenames) );
    setOkText( uiStrings::sNext() );
}


uiLinePosSelDlg::uiLinePosSelDlg( uiParent* p, const TrcKeyZSampling& tkzs )
    : uiDialog(p,Setup(tr("Select line position"),mNoHelpKey))
    , tkzs_(tkzs)
{
    inlcrlfld_ = new uiGenInput( this, tr("Compute on:"),
			BoolInpSpec(true,uiStrings::sInline(),
			uiStrings::sCrossline()));
    setOkText( uiStrings::sNext() );
}


uiLinePosSelDlg::~uiLinePosSelDlg()
{
    delete posdlg_;
}


bool uiLinePosSelDlg::acceptOK( CallBacker* )
{
    return linesfld_ ? selectPos2D() : selectPos3D();
}


bool uiLinePosSelDlg::selectPos2D()
{
    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(linesfld_->text()) );
    if ( !geom2d )
	return false;

    TrcKeyZSampling inputcs = tkzs_;
    if ( prefcs_ )
	inputcs = *prefcs_;
    else
    {
	inputcs.hsamp_.setTrcRange( geom2d->data().trcNrRange() );
	inputcs.zsamp_ = geom2d->data().zRange();
    }

    const ZDomain::Info& info = SI().zDomainInfo();
    const uiSliceSel::Type tp = uiSliceSel::TwoD;
    posdlg_ = new uiSliceSelDlg( this, inputcs, tkzs_, CallBack(), tp, info );
    posdlg_->grp()->enableApplyButton( false );
    posdlg_->setModal( true );
    if ( !prevpar_.isEmpty() )
	posdlg_->grp()->usePar( prevpar_ );

    return posdlg_->go();
}


bool uiLinePosSelDlg::selectPos3D()
{
    CallBack dummycb;
    const bool isinl = inlcrlfld_->getBoolValue();

    TrcKeyZSampling inputcs = tkzs_;
    if ( prefcs_ )
	inputcs = *prefcs_;
    else
    {
	if ( isinl )
	    inputcs.hsamp_.stop_.inl() = inputcs.hsamp_.start_.inl()
				   = inputcs.hsamp_.inlRange().snappedCenter();
	else
	    inputcs.hsamp_.stop_.crl() = inputcs.hsamp_.start_.crl()
				   = inputcs.hsamp_.crlRange().snappedCenter();

	inputcs.zsamp_.start_ = 0;
    }

    const ZDomain::Info& info = SI().zDomainInfo();
    const uiSliceSel::Type tp = isinl ? uiSliceSel::Inl : uiSliceSel::Crl;
    posdlg_ = new uiSliceSelDlg( this, inputcs, tkzs_, dummycb, tp, info );
    posdlg_->grp()->enableApplyButton( false );
    posdlg_->setModal( true );
    if ( !prevpar_.isEmpty() )
	posdlg_->grp()->usePar( prevpar_ );

    return posdlg_->go();
}


const TrcKeyZSampling& uiLinePosSelDlg::getTrcKeyZSampling() const
{
    return posdlg_ ? posdlg_->getTrcKeyZSampling() : tkzs_;
}


const char* uiLinePosSelDlg::getLineName() const
{
    return linesfld_ ? linesfld_->text() : "";
}
