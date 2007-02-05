/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: uihorizontracksetup.cc,v 1.10 2007-02-05 14:32:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihorizontracksetup.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "emhorizon.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitabstack.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

void uiHorizonSetupGroup::initClass()
{
    uiMPE().setupgrpfact.addFactory( uiHorizonSetupGroup::create );
}


uiSetupGroup* uiHorizonSetupGroup::create( uiParent* p, const char* typestr,
					   const Attrib::DescSet* ads )
{
    if ( strcmp(typestr,EM::Horizon::typeStr()) )
	return 0;

    return new uiHorizonSetupGroup( p, ads );
}


const char** uiHorizonSetupGroup::sKeyEventNames()
{
    static const char* event_names[] = { "Min", "Max", "0+-", "0-+", 0 };
    return event_names;
}


const VSEvent::Type* uiHorizonSetupGroup::cEventTypes()
{
    static const VSEvent::Type event_types[] = { VSEvent::Min, VSEvent::Max,
					 VSEvent::ZCPosNeg, VSEvent::ZCNegPos };

    return event_types;
}


uiHorizonSetupGroup::uiHorizonSetupGroup( uiParent* p,
					  const Attrib::DescSet* ads )
    : uiSetupGroup(p,"")
    , sectiontracker_(0)
    , attrset_(ads)
    , inpfld(0)
{
    tabgrp_ = new uiTabStack( this, "TabStack" );
    uiGroup* eventgrp = createEventGroup();
    tabgrp_->addTab( eventgrp, "Event" );

    uiGroup* simigrp = createSimiGroup();
    tabgrp_->addTab( simigrp, "Similarity" );
}


uiGroup* uiHorizonSetupGroup::createEventGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Event" );

    inpfld = new uiAttrSel( grp, attrset_, attrset_ ? attrset_->is2D() : false,
	    		    "Seismic data" );
    grp->setHAlignObj( inpfld );

    evfld = new uiGenInput( grp, "Event type",
	    		    StringListInpSpec(sKeyEventNames()) );
    evfld->attach( alignedBelow, inpfld );
    evfld->valuechanged.notify( mCB(this,uiHorizonSetupGroup,selEventType) );

    BufferString srchwindtxt( "Search window " );
    srchwindtxt += SI().getZUnit();
    srchgatefld = new uiGenInput( grp, srchwindtxt, IntInpIntervalSpec() );
    srchgatefld->attach( alignedBelow, evfld );

    thresholdtypefld = new uiGenInput( grp, "Threshold type",
		BoolInpSpec(true,"Cut-off amplitude","Relative difference") );
    thresholdtypefld->valuechanged.notify(
			mCB(this,uiHorizonSetupGroup,selAmpThresholdType) );
    thresholdtypefld->attach( alignedBelow, srchgatefld );

    ampthresholdfld = new uiGenInput ( grp, "XXXXXXXXXXXXXX",
	    			       FloatInpSpec() );
    ampthresholdfld->attach( alignedBelow, thresholdtypefld );

    extriffailfld = new uiGenInput( grp, "If tracking fails",
				    BoolInpSpec(true,"Extrapolate","Stop") );
    extriffailfld->attach( alignedBelow, ampthresholdfld );

    return grp;
}


uiGroup* uiHorizonSetupGroup::createSimiGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Similarity" );

    usesimifld = new uiGenInput( grp, "Use similarity", BoolInpSpec(true) );
    usesimifld->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selUseSimilarity) );

    BufferString compwindtxt( "Compare window " );
    compwindtxt += SI().getZUnit();
    compwinfld = new uiGenInput( grp, compwindtxt, IntInpIntervalSpec() );
    compwinfld->attach( alignedBelow, usesimifld );

    simithresholdfld = new uiGenInput( grp, "Similarity threshold(0-1)",
				       FloatInpSpec() );
    simithresholdfld->attach( alignedBelow, compwinfld );
    grp->setHAlignObj( usesimifld );
    return grp;
}


uiHorizonSetupGroup::~uiHorizonSetupGroup()
{
}


void uiHorizonSetupGroup::selUseSimilarity( CallBacker* )
{
    const bool usesimi = usesimifld->getBoolValue();
    compwinfld->display( usesimi );
    simithresholdfld->display( usesimi );
}


void uiHorizonSetupGroup::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld->getBoolValue();
    ampthresholdfld->setTitleText( absthreshold ? "Amplitude value"
						: "Allowed difference (%)" );
    ampthresholdfld->setValue( absthreshold ? horadj_->amplitudeThreshold()
	    				    : horadj_->allowedVariance()*100 );
}


void uiHorizonSetupGroup::selEventType( CallBacker* )
{
    const VSEvent::Type ev = cEventTypes()[ evfld->getIntValue() ];
    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld->setSensitive( thresholdneeded );
    ampthresholdfld->setSensitive( thresholdneeded );
}
    

void uiHorizonSetupGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    horadj_ = horadj;
    if ( !horadj_ ) return;

    initEventGroup();
    selEventType(0);
    selAmpThresholdType(0);
    initSimiGroup();
    selUseSimilarity(0);
}


void uiHorizonSetupGroup::initEventGroup()
{
    Attrib::DescID curid = horadj_->getAttributeSel(0) ?
	horadj_->getAttributeSel(0)->id() : Attrib::DescID::undef();
    if ( attrset_->getDesc(curid) )
	inpfld->setDesc( attrset_->getDesc(curid) );

    VSEvent::Type ev = horadj_->trackEvent();
    const int fldidx = ev == VSEvent::Min ? 0
			    : (ev == VSEvent::Max ? 1
			    : (ev == VSEvent::ZCPosNeg ? 2 : 3) );
    evfld->setValue( fldidx );

    Interval<int> srchintv(
	    mNINT(horadj_->permittedZRange().start * SI().zFactor()),
	    mNINT(horadj_->permittedZRange().stop * SI().zFactor()) );
    srchgatefld->setValue( srchintv );

    thresholdtypefld->setValue( horadj_->useAbsThreshold() );
    extriffailfld->setValue( !horadj_->removesOnFailure() );
}


void uiHorizonSetupGroup::initSimiGroup()
{
    usesimifld->setValue( !horadj_->trackByValue() );

    Interval<int> simiintv(
	    mNINT(horadj_->similarityWindow().start * SI().zFactor()),
	    mNINT(horadj_->similarityWindow().stop * SI().zFactor()) );
    compwinfld->setValue( simiintv );

    simithresholdfld->setValue( horadj_->similarityThreshold() );
}


bool uiHorizonSetupGroup::commitToTracker() const
{
    if ( !horadj_ || horadj_->getNrAttributes()<1 )
    {   uiMSG().warning( "Unable to apply tracking setup" ); 
	return true;
    }
	
    if ( !inpfld ) return true;

    const bool usesimi = usesimifld->getBoolValue();
    
    inpfld->processInput();
    Attrib::SelSpec as;
    inpfld->fillSelSpec( as );
    if ( as.id() < 0 )
	mErrRet( "Please select the seismic data to track on" );

    horadj_->setAttributeSel( 0, as );

    horadj_->setTrackEvent( cEventTypes()[evfld->getIntValue()] );
    Interval<int> intv = srchgatefld->getIInterval();
    horadj_->setPermittedZRange(
		Interval<float>( (float)intv.start/SI().zFactor(),
				 (float)intv.stop/SI().zFactor()));

    horadj_->setTrackByValue( !usesimi );
    if ( usesimi )
    {
	Interval<float> intval = compwinfld->getFInterval();
	if ( intval.start > 0 || intval.stop < 0 || intv.start == intv.stop )
	    mErrRet( "Compare window should be minus to positive, ex. -20, 20");
	horadj_->setSimilarityWindow(
		Interval<float>((float)intval.start/SI().zFactor(),
				(float)intval.stop/SI().zFactor()) );
	    
	float mgate = simithresholdfld->getfValue();
	if ( mgate > 1 || mgate <= 0)
	    mErrRet( "Similarity threshold must be within 0 to 1" );
	horadj_->setSimilarityThreshold(mgate);
    }
	    
    bool useabs = thresholdtypefld->getBoolValue();
    horadj_->setUseAbsThreshold(useabs);
    if ( useabs )
    {
	float vgate = ampthresholdfld->getfValue();
	if ( Values::isUdf(vgate) )
	    mErrRet( "Value threshold not set" );
	horadj_->setAmplitudeThreshold(vgate);
    }
    else
    {
	float var = ampthresholdfld->getfValue();
	if ( var <= 0 || var >= 100 )
	    mErrRet( "Allowed variance must be between 0-100" );
	horadj_->setAllowedVariance(var/100);
    }
	
    horadj_->removeOnFailure( !extriffailfld->getBoolValue() );
    return true;
}


} //namespace MPE
