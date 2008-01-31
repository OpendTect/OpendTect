/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: uivalseriestracker.cc,v 1.2 2008-01-31 22:05:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uivalseriestracker.h"

#include "survinfo.h"
#include "valseriestracker.h"
#include "uigeninput.h"
#include "uimsg.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

uiEventTracker::uiEventTracker( uiParent* p, EventTracker& tracker,
       				bool hideeventtype )
    : uiDlgGroup( p , "Event tracker")
    , tracker_( tracker )
{
    if ( hideeventtype )
	evfld_ = 0;
    else
    {
	evfld_ = new uiGenInput( this, "Event type",
				StringListInpSpec(EventTracker::sEventNames()) );
	evfld_->setValue( EventTracker::getEventTypeIdx( tracker_.trackEvent() ) );
	evfld_->valuechanged.notify( mCB(this,uiEventTracker,selEventType) );
    }

    BufferString srchwindtxt( "Search window " );
    srchwindtxt += SI().getZUnit();
    srchgatefld_ =
	new uiGenInput( this, srchwindtxt.buf(), IntInpIntervalSpec());
    Interval<int> srchintv(
	    mNINT(tracker_.permittedZRange().start * SI().zFactor()),
	    mNINT(tracker_.permittedZRange().stop * SI().zFactor()) );
    srchgatefld_->setValue( srchintv );
    if ( evfld_ ) srchgatefld_->attach( alignedBelow, evfld_ );

    thresholdtypefld_ = new uiGenInput( this, "Threshold type",
		BoolInpSpec(true,"Cut-off amplitude","Relative difference") );
    thresholdtypefld_->setValue( tracker_.useAbsThreshold() );
    thresholdtypefld_->valuechanged.notify(
			mCB(this,uiEventTracker,selAmpThresholdType) );
    thresholdtypefld_->attach( alignedBelow, srchgatefld_ );

    ampthresholdfld_ = new uiGenInput ( this, "Amplitude value",FloatInpSpec());
    ampthresholdfld_->setValue( tracker_.amplitudeThreshold() );
    ampthresholdfld_->attach( alignedBelow, thresholdtypefld_ );

    alloweddifffld_ = new uiGenInput ( this, "Allowed difference (%)",
	    			       FloatInpSpec());
    alloweddifffld_->setValue( tracker_.allowedVariance()*100 );
    alloweddifffld_->attach( alignedBelow, thresholdtypefld_ );

    usesimifld_ = new uiGenInput( this, "Compare wafeforms",BoolInpSpec(true) );
    usesimifld_->setValue( tracker_.usesSimilarity() );
    usesimifld_->valuechanged.notify(
	    mCB(this,uiEventTracker,selUseSimilarity) );
    usesimifld_->attach( alignedBelow, ampthresholdfld_ );

    BufferString compwindtxt( "Compare window " );
    compwindtxt += SI().getZUnit();
    compwinfld_ = new uiGenInput( this, compwindtxt, IntInpIntervalSpec() );
    compwinfld_->attach( alignedBelow, usesimifld_ );

    Interval<int> simiintv(
	    mNINT(tracker_.similarityWindow().start * SI().zFactor()),
	    mNINT(tracker_.similarityWindow().stop * SI().zFactor()) );
    compwinfld_->setValue( simiintv );


    simithresholdfld_ = new uiGenInput( this, "Similarity threshold(0-1)",
				       FloatInpSpec() );
    simithresholdfld_->attach( alignedBelow, compwinfld_ );
    simithresholdfld_->setValue( tracker_.similarityThreshold() );

    selEventType( 0 );
    selAmpThresholdType( 0 );
    selUseSimilarity( 0 );

    setHAlignObj( simithresholdfld_ );
}


uiEventTracker::~uiEventTracker()
{
}


void uiEventTracker::selUseSimilarity( CallBacker* )
{
    const bool usesimi = usesimifld_->getBoolValue();
    compwinfld_->display( usesimi );
    simithresholdfld_->display( usesimi );
}


void uiEventTracker::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld_->getBoolValue();
    ampthresholdfld_->display( absthreshold );
    alloweddifffld_->display( !absthreshold );
}


void uiEventTracker::selEventType( CallBacker* )
{
    const VSEvent::Type ev = evfld_
	? EventTracker::cEventTypes()[evfld_->getIntValue()]
	: tracker_.trackEvent();

    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld_->setSensitive( thresholdneeded );
    ampthresholdfld_->setSensitive( thresholdneeded );
}
    

bool uiEventTracker::acceptOK()
{
    if ( evfld_ )
    {
	VSEvent::Type evtyp= EventTracker::cEventTypes()[evfld_->getIntValue()];
	tracker_.setTrackEvent( evtyp );
    }

    const Interval<int> intv = srchgatefld_->getIInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( "Search window should be minus to positive, ex. -20, 20");
    const Interval<int> relintv( mNINT(intv.start/SI().zFactor()),
			     mNINT( intv.stop/SI().zFactor() ) );
    tracker_.setPermittedZRange( relintv );

    const bool usesimi = usesimifld_->getBoolValue();
    tracker_.useSimilarity( usesimi );

    const Interval<float> intval = compwinfld_->getFInterval();
    if ( intval.start>0 || intval.stop<0 || intval.start==intval.stop )
	mErrRet( "Compare window should be minus to positive, ex. -20, 20");
    const Interval<int> relintval( mNINT(intval.start/SI().zFactor() ),
				   mNINT( intval.stop/SI().zFactor() ) );
    tracker_.setSimilarityWindow( relintval );
	
    const float mgate = simithresholdfld_->getfValue();
    if ( mgate > 1 || mgate <= 0)
	mErrRet( "Similarity threshold must be within 0 to 1" );
    tracker_.setSimilarityThreshold( mgate );
	    
    const bool useabs = thresholdtypefld_->getBoolValue();
    tracker_.setUseAbsThreshold( useabs );

    float vgate = ampthresholdfld_->getfValue();
    tracker_.setAmplitudeThreshold( vgate );
    float var = alloweddifffld_->getfValue() / 100;
    if ( var<=0.0 || var>=1.0 )
	    mErrRet( "Allowed variance must be between 0-100" );
    tracker_.setAllowedVariance( var );
    return true;
}
