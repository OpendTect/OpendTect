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
#include "uigraphicsview.h"
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

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const StepInterval<int> intv( -10000, 10000, step );
    IntInpSpec iis; iis.setLimits( intv );

    BufferString disptxt( "Data Display window ", SI().getZUnitString() );
    nrzfld_ = new uiGenInput( this, disptxt, iis, iis );
    nrzfld_->attach( alignedBelow, usecorrfld_ );
    nrzfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    nrtrcsfld_ = new uiGenInput( this, "Nr Traces", IntInpSpec(5) );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    BufferString compwindtxt( "Compare window ", SI().getZUnitString() );
    compwinfld_ = new uiGenInput( this, compwindtxt, iis, iis );
    compwinfld_->attach( alignedBelow, nrtrcsfld_ );
    compwinfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    IntInpSpec tiis; tiis.setLimits( StepInterval<int>(0,100,1) );
    corrthresholdfld_ =
	new uiGenInput( this, tr("Correlation threshold (0-100)"), tiis );
    corrthresholdfld_->attach( alignedBelow, compwinfld_ );
    corrthresholdfld_->valuechanged.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    correlationvwr_ = new uiFlatViewer( this );
    correlationvwr_->attach( rightOf, usecorrfld_ );
    correlationvwr_->setPrefWidthInChar( 30 );
    correlationvwr_->setPrefHeightInChar( 25 );
    correlationvwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);
    correlationvwr_->appearance().setGeoDefaults( true );
    correlationvwr_->rgbCanvas();

    minitm_ = correlationvwr_->createAuxData( "Min line" );
    minitm_->cursor_.shape_ = MouseCursor::SizeVer;
    minitm_->linestyle_.color_ = Color(0,255,0);
    minitm_->poly_ += FlatView::Point(0,0);
    minitm_->poly_ += FlatView::Point(0,0);
    correlationvwr_->addAuxData( minitm_ );

    maxitm_ = correlationvwr_->createAuxData( "Max line" );
    maxitm_->cursor_.shape_ = MouseCursor::SizeVer;
    maxitm_->linestyle_.color_ = Color(0,255,0);
    maxitm_->poly_ += FlatView::Point(0,0);
    maxitm_->poly_ += FlatView::Point(0,0);
    correlationvwr_->addAuxData( maxitm_ );

    seeditm_ = correlationvwr_->createAuxData( "Seed" );
    seeditm_->poly_ += FlatView::Point(0,0);
    seeditm_->markerstyles_ += MarkerStyle2D();
    seeditm_->markerstyles_[0].color_ = Color(0,255,0);
    correlationvwr_->addAuxData( seeditm_ );

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


void uiCorrelationGroup::visibleDataChangeCB( CallBacker* )
{
    updateViewer();
}


void uiCorrelationGroup::correlationChangeCB( CallBacker* )
{
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

    const int sample = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const Interval<int> dataintv = corrintv + Interval<int>(-sample,sample);
    nrzfld_->setValue( dataintv );

    nrtrcsfld_->setValue( 5 );
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
    const int nrtrcs = nrtrcsfld_->getIntValue();

    StepInterval<float> zintv; zintv.setFrom( nrzfld_->getIInterval() );
    zintv.scale( 1.f/SI().zDomain().userFactor() );
    zintv.step = SI().zStep();
    const DataPack::ID dpid =
	MPE::engine().getSeedPosDataPack( bid, z, nrtrcs, zintv );

    correlationvwr_->setPack( true, dpid );
    correlationvwr_->setViewToBoundingBox();

    FlatView::Point& pt = seeditm_->poly_[0];
    pt = FlatView::Point( bid.inl(), z );

    minitm_->poly_[0] = FlatView::Point( bid.inl()-nrtrcs/2, z+zintv.start );
    minitm_->poly_[1] = FlatView::Point( bid.inl()+nrtrcs/2, z+zintv.start );
    maxitm_->poly_[0] = FlatView::Point( bid.inl()-nrtrcs/2, z+zintv.stop );
    maxitm_->poly_[1] = FlatView::Point( bid.inl()+nrtrcs/2, z+zintv.stop );
    correlationvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
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

    const int thr = corrthresholdfld_->getIntValue();
    if ( thr > 100 || thr < 0)
	mErrRet( tr("Correlation threshold must be between 0 to 100") );

    const float newthreshold = (float)thr/100.f;
    if ( !mIsEqual(adjuster_->similarityThreshold(),newthreshold,mDefEps) )
    {
	fieldchange = true;
	adjuster_->setSimilarityThreshold( newthreshold );
    }

    return true;
}

} //namespace MPE
