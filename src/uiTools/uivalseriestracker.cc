/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivalseriestracker.h"

#include "survinfo.h"
#include "valseriestracker.h"
#include "uigeninput.h"
#include "uimsg.h"


#define mErrRet(s) { if ( domsg ) uiMSG().error( s ); return false; }

uiEventTracker::uiEventTracker( uiParent* p, EventTracker& tracker,
       				bool hideeventtype, bool immediateupdate )
    : uiDlgGroup( p , tr("Event tracker"))
    , tracker_( tracker )
    , immediateupdate_( immediateupdate )
{
    tracker_.fillPar( restorepars_ );

    if ( hideeventtype )
	evfld_ = 0;
    else
    {
	evfld_ = new uiGenInput( this, tr("Event type"),
				StringListInpSpec(EventTracker::sEventNames()));
	evfld_->setValue( EventTracker::getEventTypeIdx(tracker_.trackEvent()));
	evfld_->valueChanged.notify( mCB(this,uiEventTracker,selEventType) );
	evfld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    }

    uiString srchwindtxt( tr("Search window %1").arg(SI().getUiZUnitString()) );
    srchgatefld_ =
	new uiGenInput( this, srchwindtxt, FloatInpIntervalSpec());
    const Interval<float> srchintv(
	    tracker_.permittedRange().start * SI().zDomain().userFactor(),
	    tracker_.permittedRange().stop * SI().zDomain().userFactor() );
    srchgatefld_->setValue( srchintv );
    srchgatefld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    if ( evfld_ ) srchgatefld_->attach( alignedBelow, evfld_ );

    thresholdtypefld_ = new uiGenInput( this, tr("Threshold type"),
		                        BoolInpSpec(true,
                                        tr("Cut-off amplitude"),
                                        tr("Relative difference")) );
    thresholdtypefld_->setValue( tracker_.useAbsThreshold() );
    thresholdtypefld_->valueChanged.notify(
			mCB(this,uiEventTracker,selAmpThresholdType) );
    thresholdtypefld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    thresholdtypefld_->attach( alignedBelow, srchgatefld_ );

    ampthresholdfld_ = new uiGenInput ( this, tr("Amplitude value"),
                                        FloatInpSpec());
    ampthresholdfld_->setValue( tracker_.amplitudeThreshold() );
    ampthresholdfld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    ampthresholdfld_->attach( alignedBelow, thresholdtypefld_ );

    alloweddifffld_ = new uiGenInput ( this, tr("Allowed difference (%)"),
	    			       FloatInpSpec());
    alloweddifffld_->setValue( tracker_.allowedVariance()*100 );
    alloweddifffld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    alloweddifffld_->attach( alignedBelow, thresholdtypefld_ );

    usesimifld_ = new uiGenInput( this, tr("Compare waveforms"),
                                  BoolInpSpec(true) );
    usesimifld_->setValue( tracker_.usesSimilarity() );
    usesimifld_->valueChanged.notify(
	    mCB(this,uiEventTracker,selUseSimilarity) );
    usesimifld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    usesimifld_->attach( alignedBelow, ampthresholdfld_ );

    uiString compwindtxt =
		tr("Compare window %1").arg(SI().getUiZUnitString());
    compwinfld_ = new uiGenInput( this, compwindtxt, FloatInpIntervalSpec() );
    compwinfld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    compwinfld_->attach( alignedBelow, usesimifld_ );

    const Interval<float> simiintv(
	    tracker_.similarityWindow().start * SI().zDomain().userFactor(),
	    tracker_.similarityWindow().stop * SI().zDomain().userFactor() );
    compwinfld_->setValue( simiintv );


    simithresholdfld_ = new uiGenInput( this, tr("Similarity threshold(0-1)"),
				       FloatInpSpec() );
    simithresholdfld_->attach( alignedBelow, compwinfld_ );
    simithresholdfld_->valueChanged.notify( mCB(this,uiEventTracker,changeCB) );
    simithresholdfld_->setValue( tracker_.similarityThreshold() );

    selEventType( 0 );
    selAmpThresholdType( 0 );
    selUseSimilarity( 0 );

    setHAlignObj( simithresholdfld_ );
}


uiEventTracker::~uiEventTracker()
{}


void uiEventTracker::changeCB( CallBacker* )
{
    if ( immediateupdate_ )
	updateTracker( false );
}


void uiEventTracker::selUseSimilarity( CallBacker* cb )
{
    const bool usesimi = usesimifld_->getBoolValue();
    compwinfld_->display( usesimi );
    simithresholdfld_->display( usesimi );
}


void uiEventTracker::selAmpThresholdType( CallBacker* cb )
{
    const bool absthreshold = thresholdtypefld_->getBoolValue();
    ampthresholdfld_->display( absthreshold );
    alloweddifffld_->display( !absthreshold );
}


void uiEventTracker::selEventType( CallBacker* cb )
{
    const VSEvent::Type ev = evfld_
	? EventTracker::cEventTypes()[evfld_->getIntValue()]
	: tracker_.trackEvent();

    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld_->setSensitive( thresholdneeded );
    ampthresholdfld_->setSensitive( thresholdneeded );
}


bool uiEventTracker::rejectOK()
{
    tracker_.usePar( restorepars_ );
    return true;
}


bool uiEventTracker::acceptOK()
{
    return updateTracker( true );
}


bool uiEventTracker::updateTracker( bool domsg )
{
    if ( evfld_ )
    {
	VSEvent::Type evtyp= EventTracker::cEventTypes()[evfld_->getIntValue()];
	tracker_.setTrackEvent( evtyp );
    }

    const Interval<float> intv = srchgatefld_->getFInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( tr("Search window should be minus to positive, ex. -20, 20"));
    const Interval<float> relintv( intv.start/SI().zDomain().userFactor(),
			           intv.stop/SI().zDomain().userFactor() );
    tracker_.setPermittedRange( relintv );

    const bool usesimi = usesimifld_->getBoolValue();
    tracker_.useSimilarity( usesimi );

    const Interval<float> intval = compwinfld_->getFInterval();
    if ( intval.start>0 || intval.stop<0 || intval.start==intval.stop )
	mErrRet( tr("Compare window should be minus to positive, ex. -20, 20"));
    const Interval<float> relintval( intval.start/SI().zDomain().userFactor(),
				     intval.stop/SI().zDomain().userFactor() );
    tracker_.setSimilarityWindow( relintval );

    const float mgate = simithresholdfld_->getFValue();
    if ( mgate > 1 || mgate <= 0)
	mErrRet( tr("Similarity threshold must be within 0 to 1") );
    tracker_.setSimilarityThreshold( mgate );

    const bool useabs = thresholdtypefld_->getBoolValue();
    tracker_.setUseAbsThreshold( useabs );

    const float vgate = ampthresholdfld_->getFValue();
    tracker_.setAmplitudeThreshold( vgate );
    const float var = alloweddifffld_->getFValue() / 100;
    if ( var<=0.0 || var>=1.0 )
	    mErrRet( tr("Allowed difference must be between 0-100") );
    tracker_.setAllowedVariance( var );
    return true;
}
