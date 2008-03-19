/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: uihorizontracksetup.cc,v 1.18 2008-03-19 11:22:51 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uihorizontracksetup.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
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
    if ( strcmp(typestr,EM::Horizon3D::typeStr()) && 
	 strcmp(typestr,EM::Horizon2D::typeStr()) )
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

    autogrp_ = createAutoGroup();
    tabgrp_->addTab( autogrp_, "Autotrack" );
    inwizard_ = p && !strncmp(p->name(),"Page",4);
}


uiGroup* uiHorizonSetupGroup::createEventGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Event" );

    inpfld = new uiAttrSel( grp, attrset_, attrset_ ? attrset_->is2D() : false,
	    		    "Input data" );
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

    ampthresholdfld = new uiGenInput ( grp, "Allowed difference (%)",
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


uiGroup* uiHorizonSetupGroup::createAutoGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Autotrack" );
    
    startpropfld = new uiGenInput( grp, "Start propagation from",
		   BoolInpSpec(true,"all boundary nodes","seed nodes only") );
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
    ampthresholdfld->setTitleText( absthreshold ? "       Amplitude value"
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
    initAutoGroup();
}


void uiHorizonSetupGroup::setAttribSet( const Attrib::DescSet* ads )
{ 
    attrset_ = ads; 
    if ( inpfld )
	inpfld->setDescSet( ads );
}


void uiHorizonSetupGroup::initEventGroup()
{
    Attrib::DescID curid = horadj_->getAttributeSel(0) ?
	horadj_->getAttributeSel(0)->id() : Attrib::DescID::undef();
    if ( attrset_ && attrset_->getDesc(curid) )
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


void uiHorizonSetupGroup::initAutoGroup()
{
    startpropfld->setValue( !sectiontracker_->propagatingFromSeedOnly() );
    
    const bool is2d = sectiontracker_ && sectiontracker_->adjuster() &&
		      sectiontracker_->adjuster()->is2D();
    tabgrp_->setTabEnabled( autogrp_, !is2d && !inwizard_ ); 
}


bool uiHorizonSetupGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;

    if ( !horadj_ || horadj_->getNrAttributes()<1 )
    {   uiMSG().warning( "Unable to apply tracking setup" ); 
	return true;
    }
    if ( !inpfld ) return true;

    
    inpfld->processInput();
    Attrib::SelSpec as;
    inpfld->fillSelSpec( as );
    if ( as.id() < 0 )
	mErrRet( "Please select the seismic data to track on" );
    if ( !horadj_->getAttributeSel(0) || *horadj_->getAttributeSel(0)!=as )
    {
	fieldchange = true;
	horadj_->setAttributeSel( 0, as );
    }

    VSEvent::Type evtyp = cEventTypes()[ evfld->getIntValue() ];
    if ( horadj_->trackEvent() != evtyp )
    {
	fieldchange = true;
	horadj_->setTrackEvent( evtyp );
    }

    Interval<int> intv = srchgatefld->getIInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( "Search window should be minus to positive, ex. -20, 20");
    Interval<float> relintv( (float)intv.start/SI().zFactor(),
			     (float)intv.stop/SI().zFactor() );
    if ( horadj_->permittedZRange() != relintv )
    {
	fieldchange = true;
	horadj_->setPermittedZRange( relintv );
    }

    const bool usesimi = usesimifld->getBoolValue();
    if ( horadj_->trackByValue() == usesimi )
    {
	fieldchange = true;
	horadj_->setTrackByValue( !usesimi );
    }

    if ( usesimi )
    {
	Interval<float> intval = compwinfld->getFInterval();
	if ( intval.start>0 || intval.stop<0 || intval.start==intval.stop )
	    mErrRet( "Compare window should be minus to positive, ex. -20, 20");
	Interval<float> relintval( (float)intval.start/SI().zFactor(),
				   (float)intval.stop/SI().zFactor() );
	if ( horadj_->similarityWindow() != relintval )
	{
	    fieldchange = true;
	    horadj_->setSimilarityWindow( relintval );
	}
	    
	float mgate = simithresholdfld->getfValue();
	if ( mgate > 1 || mgate <= 0)
	    mErrRet( "Similarity threshold must be within 0 to 1" );
	if ( horadj_->similarityThreshold() != mgate )
	{
	    fieldchange = true;
	    horadj_->setSimilarityThreshold( mgate );
	}
    }
	    
    const bool useabs = thresholdtypefld->getBoolValue();
    if ( horadj_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	horadj_->setUseAbsThreshold( useabs );
    }

    if ( useabs )
    {
	float vgate = ampthresholdfld->getfValue();
	if ( Values::isUdf(vgate) )
	    mErrRet( "Value threshold not set" );
	if ( horadj_->amplitudeThreshold() != vgate )
	{
	    fieldchange = true;
	    horadj_->setAmplitudeThreshold( vgate );
	}
    }
    else
    {
	float var = ampthresholdfld->getfValue() / 100;
	if ( var<=0.0 || var>=1.0 )
	    mErrRet( "Allowed variance must be between 0-100" );
	if ( horadj_->allowedVariance() != var )
	{
	    fieldchange = true;
	    horadj_->setAllowedVariance( var );
	}
    }

    const bool rmonfail = !extriffailfld->getBoolValue();
    if ( horadj_->removesOnFailure() != rmonfail )    
    {
	fieldchange = true;
	horadj_->removeOnFailure( rmonfail );
    }

    const bool seedonlyprop = !startpropfld->getBoolValue();
    if ( sectiontracker_->propagatingFromSeedOnly() != seedonlyprop )    
    {
	fieldchange = true;
	sectiontracker_->setSeedOnlyPropagation( seedonlyprop );
    }

    return true;
}


} //namespace MPE
