/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Kristofer Tingdahl
 DATE:		May 2011
_______________________________________________________________________________

 -*/
static const char* rcsID = "$Id: uivoxelconnectivityfilter.cc,v 1.1 2011-08-11 09:46:19 cvskris Exp $";

#include "uivoxelconnectivityfilter.h"

#include "datainpspec.h"
#include "voxelconnectivityfilter.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uivolprocchain.h"

namespace VolProc
{

void uiVoxelConnectivityFilter::initClass()
{
    uiChain::factory().addCreator( createInstance,
	    VoxelConnectivityFilter::sFactoryKeyword(),
	    VoxelConnectivityFilter::sFactoryDisplayName() );
}


uiStepDialog* uiVoxelConnectivityFilter::createInstance( uiParent* p,
							 Step* step )
{
    mDynamicCastGet( VoxelConnectivityFilter*, vcf, step );
    return vcf 
	?  new uiVoxelConnectivityFilter( p, vcf )
	: 0; 
}


uiVoxelConnectivityFilter::uiVoxelConnectivityFilter( uiParent* p,
	VoxelConnectivityFilter* step )
    : uiStepDialog( p, VoxelConnectivityFilter::sFactoryDisplayName(), step )
{
    rangefld_ = new uiGenInput( this, "Clip-range",
	    			FloatInpIntervalSpec(false) );
    rangefld_->setValue( step->getAcceptRange() );

    connectivityfld_ = new uiGenInput( this, "Connectivity",
	    StringListInpSpec( VoxelConnectivityFilter::ConnectivityNames() ) );
    connectivityfld_->setValue( (int) step->getConnectivity() );
    connectivityfld_->attach( alignedBelow, rangefld_ );

    minbodysizefld_ = new uiGenInput( this, "Minimum size",
	    IntInpSpec( step->getMinimumBodySize() ) );
    minbodysizefld_->attach( alignedBelow, connectivityfld_ );

    acceptoutputfld_ = new uiGenInput( this, "Accept output",
	    StringListInpSpec( VoxelConnectivityFilter::AcceptOutputNames() ) );
    acceptoutputfld_->setValue( (int) step->getAcceptOutput() );
    acceptoutputfld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );
    acceptoutputfld_->attach( alignedBelow, minbodysizefld_ );

    acceptvaluefld_ = new uiGenInput( this, "Accept value",
	    FloatInpSpec( step->getAcceptValue() ) );
    acceptvaluefld_->attach( alignedBelow, acceptoutputfld_ );

    rejectoutputudffld_ = new uiGenInput( this, "Reject output",
	    BoolInpSpec(mIsUdf(step->getRejectValue() ),
			"Undefined value", "Value" ) );
    rejectoutputudffld_->attach( alignedBelow, acceptvaluefld_ );
    rejectoutputudffld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );

    rejectoutputvalfld_ = new uiGenInput( this, "Reject output value",
	    FloatInpSpec(step->getRejectValue() ) );
    rejectoutputvalfld_->attach( alignedBelow, rejectoutputudffld_ );

    updateFieldsCB( 0 );
}


void uiVoxelConnectivityFilter::updateFieldsCB( CallBacker* )
{
    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
	     					    output );
    acceptvaluefld_->display( output==VoxelConnectivityFilter::Value );
    rejectoutputvalfld_->display( !rejectoutputudffld_->getBoolValue() );
}


bool uiVoxelConnectivityFilter::acceptOK( CallBacker* )
{
    const Interval<float> range = rangefld_->getFInterval();
    if ( mIsUdf(range.start) && mIsUdf(range.stop) )
    {
	uiMSG().error("Undefined range");
	return false;
    }

    if ( mIsUdf(minbodysizefld_->getIntValue() ) ||
         minbodysizefld_->getIntValue()<1 )
    {
	uiMSG().error("Minimum size is not set or is less than 1.");
	return false;
    }

    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
	     					    output );

    if ( output==VoxelConnectivityFilter::Value &&
	 mIsUdf(acceptvaluefld_->getfValue() ) )
    {
	uiMSG().error("Accept-value not set");
    }

    if ( !rejectoutputudffld_->getBoolValue() &&
	    mIsUdf(rejectoutputvalfld_->getfValue() ) )
    {
	uiMSG().error("Rejection value is not set");
	return false;
    }

    mDynamicCastGet( VoxelConnectivityFilter*, step, step_ );
    step->setAcceptRange(range);
    step->setConnectivity( (VoxelConnectivityFilter::Connectivity)
			    connectivityfld_->getIntValue() );
    step->setMinimumBodySize( minbodysizefld_->getIntValue() );
    step->setAcceptOutput( output );

    if ( output==VoxelConnectivityFilter::Value )
	 step->setAcceptValue( acceptvaluefld_->getfValue() );

    step->setRejectValue( rejectoutputudffld_->getBoolValue()
	   ? mUdf(float)
	   : rejectoutputvalfld_->getfValue() );

    return true;
}


}; //Namespace
