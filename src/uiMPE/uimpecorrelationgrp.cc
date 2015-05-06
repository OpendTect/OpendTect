/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

#include "uimpecorrelationgrp.h"

#include "draw.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "horizonadjuster.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "od_helpids.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

uiCorrelationGroup::uiCorrelationGroup( uiParent* p )
    : uiGroup(p,"")
    , sectiontracker_(0)
    , adjuster_(0)
    , changed_(this)
    , seedpos_(Coord3::udf())
{
    usecorrfld_ = new uiGenInput( this, tr("Use Correlation"),
				  BoolInpSpec(true));
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,selUseCorrelation) );
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );

    const StepInterval<int> intv( -10000, 10000, 1 );
    IntInpSpec iis; iis.setLimits( intv );
    BufferString compwindtxt( "Compare window ", SI().getZUnitString() );
    compwinfld_ = new uiGenInput( this, compwindtxt, iis, iis );
    compwinfld_->attach( alignedBelow, usecorrfld_ );
    compwinfld_->valuechanging.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );

    iis.setLimits( StepInterval<int>(0,100,1) );
    corrthresholdfld_ =
	new uiGenInput( this, tr("Correlation threshold (0-100)"), iis );
    corrthresholdfld_->attach( alignedBelow, compwinfld_ );
    corrthresholdfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );

    correlationvwr_ = new uiFlatViewer( this );
    correlationvwr_->attach( rightOf, usecorrfld_ );
    correlationvwr_->setPrefWidthInChar( 30 );
    correlationvwr_->setPrefHeightInChar( 25 );
    correlationvwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);

    FlatView::AuxData* ad = correlationvwr_->createAuxData( "Mid line" );
    ad->linestyle_.color_ = Color( 255, 0, 255 );
    correlationvwr_->addAuxData( ad );

    setHAlignObj( usecorrfld_ );
}


uiCorrelationGroup::~uiCorrelationGroup()
{
}


void uiCorrelationGroup::selUseCorrelation( CallBacker* )
{
    const bool usecorr = usecorrfld_->getBoolValue();
    compwinfld_->setSensitive( usecorr );
    corrthresholdfld_->setSensitive( usecorr );
    correlationvwr_->setSensitive( usecorr );
}


void uiCorrelationGroup::correlationChangeCB( CallBacker* )
{
    updateViewer();
    changed_.trigger();
}


void uiCorrelationGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    adjuster_ = horadj;
    if ( !adjuster_ ) return;

    init();
}


void uiCorrelationGroup::init()
{
    usecorrfld_->setValue( !adjuster_->trackByValue() );

    const Interval<int> corrintv(
	    mCast(int,adjuster_->similarityWindow().start *
		      SI().zDomain().userFactor()),
	    mCast(int,adjuster_->similarityWindow().stop *
		      SI().zDomain().userFactor()) );

    compwinfld_->setValue( corrintv );
    corrthresholdfld_->setValue( adjuster_->similarityThreshold()*100 );
}


void uiCorrelationGroup::setSeedPos( const Coord3& crd )
{
    seedpos_ = crd;
    updateViewer();
}


void uiCorrelationGroup::updateViewer()
{
    if ( seedpos_.isUdf() )
	return;

    const BinID& bid = SI().transform( seedpos_.coord() );
    const float z = (float)seedpos_.z;
    const int nrtrcs = 5;

    Interval<int> intval = compwinfld_->getIInterval();
    const StepInterval<int> zintv( intval.start/4, intval.stop/4, 1 );
    const DataPack::ID dpid =
	MPE::engine().getSeedPosDataPack( bid, z, nrtrcs, zintv );

    correlationvwr_->setPack( true, dpid );
    correlationvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
    correlationvwr_->setViewToBoundingBox();
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

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

    const Interval<int> intval = compwinfld_->getIInterval();
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

    const int gate = corrthresholdfld_->getIntValue();
    if ( gate > 100 || gate < 0)
	mErrRet( tr("Correlation threshold must be between 0 to 100") );
    const float newthreshold = (float)gate/100.f;
    if ( !mIsEqual(adjuster_->similarityThreshold(),newthreshold,mDefEps) )
    {
	fieldchange = true;
	adjuster_->setSimilarityThreshold( newthreshold );
    }

    return true;
}

} //namespace MPE
