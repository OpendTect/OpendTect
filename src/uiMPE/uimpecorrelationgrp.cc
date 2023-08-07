/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpecorrelationgrp.h"

#include "draw.h"
#include "horizonadjuster.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimpepreviewgrp.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"

#define mErrRet(s) { uiMSG().error( s ); return false; }

static int sNrZDecimals = 0;

namespace MPE
{

uiCorrelationGroup::uiCorrelationGroup( uiParent* p, bool is2d )
    : uiDlgGroup(p,tr("Correlation"))
    , sectiontracker_(0)
    , adjuster_(0)
    , changed_(this)
    , seedpos_(TrcKeyValue::udf())
{
    sNrZDecimals = SI().nrZDecimals();

    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    usecorrfld_ = new uiGenInput( leftgrp, tr("Use Correlation"),
				  BoolInpSpec(true));
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,selUseCorrelation) );
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );
    leftgrp->setHAlignObj( usecorrfld_ );

    uiString compwindtxt = tr("Compare window %1").arg(SI().getUiZUnitString());
    if ( sNrZDecimals==0 )
    {
	IntInpIntervalSpec iis;
	iis.setSymmetric( true );
	StepInterval<int> swin( -10000, 10000, 1 );
	iis.setLimits( swin, 0 );
	iis.setLimits( swin, 1 );
	compwinfld_ = new uiGenInput( leftgrp, compwindtxt, iis );
	compwinfld_->valueChanging.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );
    }
    else
    {
	FloatInpIntervalSpec iis;
	compwinfld_ = new uiGenInput( leftgrp, compwindtxt, iis );
	compwinfld_->valueChanged.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );
    }

    compwinfld_->attach( alignedBelow, usecorrfld_ );

    corrthresholdfld_ =
	new uiLabeledSpinBox( leftgrp, tr("Correlation threshold (%)"), 1 );
    corrthresholdfld_->box()->setInterval( 0.f, 100.f, 0.1f );
    corrthresholdfld_->attach( alignedBelow, compwinfld_ );
    corrthresholdfld_->box()->valueChanging.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    uiString nostr = uiStrings::sEmptyString();
    snapfld_ = new uiGenInput( leftgrp, nostr,
			BoolInpSpec(true,tr("Snap to Event"),nostr) );
    snapfld_->valuechanged.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );
    snapfld_->attach( rightTo, corrthresholdfld_ );

    uiSeparator* sep = new uiSeparator( leftgrp, "Sep" );
    sep->attach( stretchedBelow, corrthresholdfld_ );

    uiString disptxt=tr("Data Display window %1").arg(SI().getUiZUnitString());
    if ( sNrZDecimals==0 )
    {
	const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
	StepInterval<int> intv( -10000, 10000, step );
	IntInpIntervalSpec diis;
	diis.setSymmetric( true );
	diis.setLimits( intv, 0 );
	diis.setLimits( intv, 1 );
	nrzfld_ = new uiGenInput( leftgrp, disptxt, diis );
	nrzfld_->valueChanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );
    }
    else
    {
	FloatInpIntervalSpec iis;
	nrzfld_ = new uiGenInput( leftgrp, disptxt, iis );
	nrzfld_->valueChanged.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );
    }

    nrzfld_->attach( alignedBelow, corrthresholdfld_ );
    nrzfld_->attach( ensureBelow, sep );

    IntInpSpec tiis;
    tiis.setLimits( StepInterval<int>(3,99,2) );
    nrtrcsfld_ = new uiGenInput( leftgrp, tr("Nr Traces"), tiis );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    previewgrp_ = new uiPreviewGroup( this );
    previewgrp_->attach( rightTo, leftgrp );
    previewgrp_->windowChanged_.notify(
		mCB(this,uiCorrelationGroup,previewChgCB) );

}


uiCorrelationGroup::~uiCorrelationGroup()
{
}


void uiCorrelationGroup::selUseCorrelation( CallBacker* )
{
    const bool usecorr = usecorrfld_->getBoolValue();
    compwinfld_->setSensitive( usecorr );
    corrthresholdfld_->setSensitive( usecorr );
    snapfld_->setSensitive( usecorr );
    previewgrp_->setSensitive( usecorr );

    nrzfld_->setSensitive( usecorr );
    nrtrcsfld_->setSensitive( usecorr );
}


void uiCorrelationGroup::visibleDataChangeCB( CallBacker* )
{
    previewgrp_->setWindow( compwinfld_->getFInterval() );
    previewgrp_->setDisplaySize( nrtrcsfld_->getIntValue(),
				 nrzfld_->getFInterval() );
}


void uiCorrelationGroup::correlationChangeCB( CallBacker* )
{
    previewgrp_->setWindow( compwinfld_->getFInterval() );
    changed_.trigger();
}


void uiCorrelationGroup::previewChgCB( CallBacker* )
{
    const Interval<float> intv = previewgrp_->getManipWindowF();
    if ( mIsUdf(intv.start) )
    {
	compwinfld_->setValue( intv.stop, 1 );
	compwinfld_->setNrDecimals( sNrZDecimals+1, 1 );
    }
    if ( mIsUdf(intv.stop) )
    {
	compwinfld_->setValue( intv.start, 0 );
	compwinfld_->setNrDecimals( sNrZDecimals+1, 0 );
    }

    visibleDataChangeCB( nullptr );
}


void uiCorrelationGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,st ? st->adjuster() : 0)
    adjuster_ = horadj;
    if ( !adjuster_ ) return;

    init();
}


void uiCorrelationGroup::init()
{
    NotifyStopper ns1( usecorrfld_->valuechanged );
    usecorrfld_->setValue( !adjuster_->trackByValue() );

    const float zfac = SI().zDomain().userFactor();
    const Interval<float> corrintv(
		adjuster_->similarityWindow().start * zfac,
		adjuster_->similarityWindow().stop * zfac );

    NotifyStopper ns2( compwinfld_->valueChanging );
    compwinfld_->setValue( corrintv );

    NotifyStopper ns3( corrthresholdfld_->box()->valueChanging );
    corrthresholdfld_->box()->setValue(
				adjuster_->similarityThreshold()*100.f );

    snapfld_->setValue( adjuster_->snapToEvent() );

    const float sample = SI().zStep()*zfac;
    const Interval<float> dataintv =
			corrintv + Interval<float>(-2*sample,2*sample);
    nrzfld_->setValue( dataintv );
    nrtrcsfld_->setValue( 5 );

    selUseCorrelation( nullptr );
    visibleDataChangeCB( nullptr );
}


void uiCorrelationGroup::setSeedPos( const TrcKeyValue& tkv )
{
    seedpos_ = tkv;
    previewgrp_->setSeedPos( tkv );
}


bool uiCorrelationGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;

    const bool usecorr = usecorrfld_->getBoolValue();
    if ( adjuster_->trackByValue() == usecorr )
    {
	fieldchange = true;
	adjuster_->setTrackByValue( !usecorr );
    }

    if ( !usecorr )
	return true;

    const Interval<float> intval = compwinfld_->getFInterval();
    if ( intval.width()==0 || intval.isRev() )
	mErrRet( tr("Correlation window's start value must be less than"
		    " the stop value") );

    Interval<float> relintval(
	    (float)intval.start/SI().zDomain().userFactor(),
	    (float)intval.stop/SI().zDomain().userFactor() );
    if ( adjuster_->similarityWindow() != relintval )
    {
	fieldchange = true;
	adjuster_->setSimilarityWindow( relintval );
    }

    const float thr = corrthresholdfld_->box()->getFValue();
    if ( thr > 100 || thr < 0)
	mErrRet( tr("Correlation threshold must be between 0 to 100") );

    const float newthreshold = thr/100.f;
    if ( !mIsEqual(adjuster_->similarityThreshold(),newthreshold,mDefEps) )
    {
	fieldchange = true;
	adjuster_->setSimilarityThreshold( newthreshold );
    }

    adjuster_->setSnapToEvent( snapfld_->getBoolValue() );

    return true;
}

} // namespace MPE
