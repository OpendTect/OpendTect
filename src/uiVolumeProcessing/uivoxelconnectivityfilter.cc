/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivoxelconnectivityfilter.h"

#include "datainpspec.h"
#include "voxelconnectivityfilter.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uivolprocchain.h"
#include "od_helpids.h"

namespace VolProc
{

uiStepDialog* uiVoxelConnectivityFilter::createInstance( uiParent* p,
							 Step* step, bool is2d )
{
    mDynamicCastGet( VoxelConnectivityFilter*, vcf, step );
    return vcf
	?  new uiVoxelConnectivityFilter( p, vcf, is2d )
	: 0;
}


#define mCutBelow 0
#define mCutAbove 1
#define mCutOutside 2
#define mCutInside 3

#define mMinBodySize 1


uiVoxelConnectivityFilter::uiVoxelConnectivityFilter( uiParent* p,
	VoxelConnectivityFilter* step, bool is2d )
    : uiStepDialog(p,VoxelConnectivityFilter::sFactoryDisplayName(),step,is2d)
{
    setHelpKey( mODHelpKey(mVoxelConnectivityFilterHelpID) );
    const char* cutofftypes[] = { "Values larger than", "Values less than",
				  "Values between", "Values outside", 0 };
    cutofftypefld_ = new uiGenInput( this, tr("Keep"),
                              StringListInpSpec( cutofftypes ) );
    const Interval<float>& acceptrange = step->getAcceptRange();
    Interval<float> displayacceptrange = acceptrange;
    displayacceptrange.sort( true );

    cutoffvalfld_ = new uiGenInput( this, uiStrings::sValue(), FloatInpSpec() );
    cutoffvalfld_->attach( alignedBelow, cutofftypefld_ );
    cutoffrangefld_ = new uiGenInput( this, tr("Range"),
	    FloatInpIntervalSpec(false) );
    cutoffrangefld_->attach( alignedBelow, cutofftypefld_ );
    cutoffrangefld_->setValue( displayacceptrange );

    if ( !mIsUdf(acceptrange.start) && !mIsUdf(acceptrange.stop) )
    {
	if ( acceptrange.isRev() )
	    cutofftypefld_->setValue( mCutInside );
	else
	    cutofftypefld_->setValue( mCutOutside );
    }
    else if ( !mIsUdf(acceptrange.start) )
    {
	cutofftypefld_->setValue( mCutBelow );
	cutoffvalfld_->setValue( acceptrange.start );
    }
    else
    {
	cutofftypefld_->setValue( mCutAbove );
	cutoffvalfld_->setValue( acceptrange.stop );
    }

    cutofftypefld_->valuechanged.notify(
          mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, cutoffrangefld_ );

    connectivityfld_ = new uiGenInput( this, tr("Connectivity"),
	StringListInpSpec( VoxelConnectivityFilter::ConnectivityDef() ) );
    connectivityfld_->setValue( (int) step->getConnectivity() );
    connectivityfld_->attach( alignedBelow, cutoffrangefld_ );
    connectivityfld_->attach( ensureBelow, sep );

    int minsz = mCast(int,step->getMinimumBodySize());
    if ( mIsUdf(minsz) ) minsz = mMinBodySize;
    minbodysizefld_ = new uiGenInput( this,
                                      tr("Keep bodies larger than [voxels]"),
				      IntInpSpec(minsz) );
    minbodysizefld_->attach( alignedBelow, connectivityfld_ );

    acceptoutputfld_ = new uiGenInput( this, tr("Kept output"),
	StringListInpSpec( VoxelConnectivityFilter::AcceptOutputDef() ) );
    acceptoutputfld_->setValue( (int) step->getAcceptOutput() );
    acceptoutputfld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );
    acceptoutputfld_->attach( alignedBelow, minbodysizefld_ );

    acceptvaluefld_ = new uiGenInput( this, tr("Kept value"),
	    FloatInpSpec( step->getAcceptValue() ) );
    acceptvaluefld_->attach( alignedBelow, acceptoutputfld_ );

    rejectoutputudffld_ = new uiGenInput( this, tr("Rejected output"),
	    BoolInpSpec(mIsUdf(step->getRejectValue() ),
			tr("Undefined value"), uiStrings::sValue() ) );
    rejectoutputudffld_->attach( alignedBelow, acceptvaluefld_ );
    rejectoutputudffld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );

    rejectoutputvalfld_ = new uiGenInput( this, tr("Rejected value"),
	    FloatInpSpec(step->getRejectValue() ) );
    rejectoutputvalfld_->attach( alignedBelow, rejectoutputudffld_ );

    updateFieldsCB( 0 );

    addNameFld( rejectoutputvalfld_ );
}


uiVoxelConnectivityFilter::~uiVoxelConnectivityFilter()
{}


void uiVoxelConnectivityFilter::updateFieldsCB( CallBacker* )
{
    cutoffvalfld_->display( cutofftypefld_->getIntValue()!=mCutOutside &&
                          cutofftypefld_->getIntValue()!=mCutInside );
    cutoffrangefld_->display( cutofftypefld_->getIntValue()==mCutOutside ||
                            cutofftypefld_->getIntValue()==mCutInside );

    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
						    output );
    acceptvaluefld_->display( output==VoxelConnectivityFilter::Value );
    rejectoutputvalfld_->display( !rejectoutputudffld_->getBoolValue() );
}


bool uiVoxelConnectivityFilter::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    Interval<float> range;
    if ( cutofftypefld_->getIntValue()==mCutOutside ||
         cutofftypefld_->getIntValue()==mCutInside )
    {
	range = cutoffrangefld_->getFInterval();
	if ( mIsUdf(range.start) || mIsUdf(range.stop) )
	{
	    uiMSG().error(tr("Cut range not set"));
	    return false;
	}

	if ( range.isRev() )
	{
	    uiMSG().error(tr("Cut range is reversed"));
	    return false;
	}

	if ( cutofftypefld_->getIntValue()==mCutInside )
	    range.sort( false );
    }
    else
    {
	const float cutoffval = cutoffvalfld_->getFValue();
	if ( mIsUdf(cutoffval) )
	{
	    uiMSG().error( tr("Cut value value not set") );
	    return false;
	}

	if ( cutofftypefld_->getIntValue()==mCutBelow )
	{
	    range.start = cutoffval;
	    range.stop = mUdf(float);
	}
	else
	{
	    range.start = mUdf(float);
	    range.stop = cutoffval;
	}
    }

    if ( mIsUdf(minbodysizefld_->getIntValue() ) ||
         minbodysizefld_->getIntValue()<1 )
    {
	uiMSG().error(tr("Minimum size is not set or is less than 1."));
	return false;
    }

    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
						    output );

    if ( output==VoxelConnectivityFilter::Value &&
	 mIsUdf(acceptvaluefld_->getFValue() ) )
    {
	uiMSG().error(tr("Accept-value not set"));
    }

    if ( !rejectoutputudffld_->getBoolValue() &&
	    mIsUdf(rejectoutputvalfld_->getFValue() ) )
    {
	uiMSG().error(tr("Rejection value is not set"));
	return false;
    }

    mDynamicCastGet( VoxelConnectivityFilter*, step, step_ );
    step->setAcceptRange(range);
    step->setConnectivity( (VoxelConnectivityFilter::Connectivity)
			    connectivityfld_->getIntValue() );
    step->setMinimumBodySize( minbodysizefld_->getIntValue() );
    step->setAcceptOutput( output );

    if ( output==VoxelConnectivityFilter::Value )
	 step->setAcceptValue( acceptvaluefld_->getFValue() );

    step->setRejectValue( rejectoutputudffld_->getBoolValue()
	   ? mUdf(float)
	   : rejectoutputvalfld_->getFValue() );

    return true;
}


} // namespace VolProc
