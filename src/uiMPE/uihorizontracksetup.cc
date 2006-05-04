/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: uihorizontracksetup.cc,v 1.5 2006-05-04 21:18:52 cvskris Exp $
________________________________________________________________________

-*/

#include "uihorizontracksetup.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"

#define mErrRet(s) \
{\
    uiMSG().error( s );\
    return false;\
}


namespace MPE
{


void uiHorizonSetupDialog::initClass()
{
    uiMPE().setupdlgfact.addFactory( uiHorizonSetupDialog::create );
}


uiSetupDialog* uiHorizonSetupDialog::create( uiParent* p, SectionTracker* st,
					     const Attrib::DescSet* ds )
{
    mDynamicCastGet(HorizonAdjuster*,horadj,st->adjuster());
    if ( !horadj ) return 0;

    return new uiHorizonSetupDialog( p, st, ds );
}


uiHorizonSetupDialog::uiHorizonSetupDialog( uiParent* p, SectionTracker* trkr,
					    const Attrib::DescSet* ads )
    : uiSetupDialog(p,"108.0.2")
{
    grp = new uiHorizonSetupGroup( this, trkr, ads );
}


bool uiHorizonSetupDialog::acceptOK( CallBacker* )
{
    return grp->commitToTracker();
}


void  uiHorizonSetupDialog::enableApplyButton( bool yn )
{ grp->enableApplyButton( yn ); }


NotifierAccess* uiHorizonSetupDialog::applyButtonPressed()
{ return grp->applyButtonPressed(); }


bool uiHorizonSetupDialog::commitToTracker() const
{ return grp->commitToTracker(); }



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


uiHorizonSetupGroup::uiHorizonSetupGroup( uiParent* p, SectionTracker* tracker,
					  const Attrib::DescSet* ads )
    : uiGroup(p,"Horizon Setup Group")
    , sectiontracker_(tracker)
    , attrset_(ads)
    , inpfld(0)
{
    SectionAdjuster* adjuster = sectiontracker_->adjuster();
    mDynamicCastGet(HorizonAdjuster*,horadj,adjuster)
    horadj_ = horadj;

    Attrib::DescID curid = adjuster->getAttributeSel(0) ?
	adjuster->getAttributeSel(0)->id() : Attrib::DescID::undef(); 

    maingrp = new uiGroup( this, "main group" );

    inpfld = new uiAttrSel( maingrp, attrset_, "Seismic data", curid );
    inpfld->set2D( adjuster->is2D() );
    maingrp->setHAlignObj( inpfld );

    uiSeparator* hseptop = new uiSeparator( maingrp, "top sep" );
    hseptop->attach( stretchedBelow, inpfld );

    uiGroup* trackgrp = new uiGroup( maingrp, "track group" );
    uiGroup* simigrp = new uiGroup( trackgrp, "simi group" );
    uiGroup* snapgrp = new uiGroup( trackgrp, "snap group" );
    uiSeparator* vsep = new uiSeparator( trackgrp, "vert sep", false );
    vsep->attach ( rightOf, simigrp );
    vsep->attach( heightSameAs, simigrp );
    snapgrp->attach( rightOf, vsep );
    trackgrp->setHAlignObj( vsep );
    trackgrp->attach( alignedBelow, inpfld );
    trackgrp->attach( ensureBelow, hseptop );
    
    usesimifld = new uiGenInput( simigrp, "Use similarity", BoolInpSpec() );
    usesimifld->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selUseSimilarity) );
    usesimifld->setValue( !horadj_->trackByValue() );
    BufferString compwindtxt( "Compare window " );
    compwindtxt += SI().getZUnit();
    Interval<int> simiintv(
	    mNINT(horadj_->similarityWindow().start * SI().zFactor()),
	    mNINT(horadj_->similarityWindow().stop * SI().zFactor()) );
    compwinfld = new uiGenInput( simigrp, compwindtxt,
				 IntInpIntervalSpec(simiintv) );
    compwinfld->attach( alignedBelow, usesimifld );
    simithresholdfld = new uiGenInput( simigrp, "Similarity threshold(0-1)",
				FloatInpSpec(horadj_->similarityThreshold()) );
    simithresholdfld->attach( alignedBelow, compwinfld );
    simigrp->setHAlignObj( usesimifld );

    evfld = new uiGenInput( snapgrp, "Event type",
	    		    StringListInpSpec(sKeyEventNames()));
    VSEvent::Type ev = horadj_->trackEvent();
    int fldidx = ev == VSEvent::Min ? 0
	      : (ev == VSEvent::Max ? 1
              : (ev == VSEvent::ZCPosNeg ? 2 : 3) );
    evfld->setValue( fldidx );
    BufferString srchwindtxt( "Search window " );
    srchwindtxt += SI().getZUnit();
    Interval<int> srchintv(
	    mNINT(horadj_->permittedZRange().start * SI().zFactor()),
	    mNINT(horadj_->permittedZRange().stop * SI().zFactor()) );
    srchgatefld = new uiGenInput( snapgrp, srchwindtxt,
	    			    IntInpIntervalSpec(srchintv) );
    srchgatefld->attach( alignedBelow, evfld );

    uiGroup* thresholdgrp = new uiGroup( snapgrp, "Amplitude threshold group" );
    thresholdtypefld = new uiGenInput( thresholdgrp, "Threshold type",
		       BoolInpSpec("Cut-off amplitude","Relative difference") );
    thresholdtypefld->valuechanged.notify(
	    		mCB(this,uiHorizonSetupGroup,selAmpThresholdType) );
    ampthresholdfld = new uiGenInput ( thresholdgrp, "Value", FloatInpSpec() );
    ampthresholdfld->attach( alignedBelow, thresholdtypefld );
    thresholdtypefld->setValue( horadj_->useAbsThreshold() );
    thresholdgrp->setHAlignObj( thresholdtypefld );
    thresholdgrp->attach( alignedBelow, srchgatefld );

    uiSeparator* hsepbot = new uiSeparator( maingrp, "bot sep" );
    hsepbot->attach( stretchedBelow, trackgrp );

    extriffailfld = new uiGenInput( maingrp, "If tracking fails",
				    BoolInpSpec("Extrapolate","Stop") );
    extriffailfld->attach( alignedBelow, trackgrp );
    extriffailfld->attach( ensureBelow, hsepbot );
    extriffailfld->setValue( !horadj_->removesOnFailure() );

    applybut = new uiPushButton( this, "&Apply", true );
    applybut->attach( alignedBelow, maingrp );

    initWin( 0 );
}


uiHorizonSetupGroup::~uiHorizonSetupGroup()
{
}


void  uiHorizonSetupGroup::enableApplyButton( bool yn )
{ applybut->setSensitive( yn ); }


NotifierAccess* uiHorizonSetupGroup::applyButtonPressed()
{ return &applybut->activated; }


void uiHorizonSetupGroup::initWin( CallBacker* cb )
{
    selUseSimilarity( cb );
    selAmpThresholdType( cb );
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
    ampthresholdfld->setValue( absthreshold ? horadj_->amplitudeTreshold()
	    				    : horadj_->allowedVariance()*100 );
}


bool uiHorizonSetupGroup::commitToTracker() const
{
    if ( horadj_->getNrAttributes()<1 )
	return false;
	
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
	horadj_->setSimiliarityThreshold(mgate);
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
