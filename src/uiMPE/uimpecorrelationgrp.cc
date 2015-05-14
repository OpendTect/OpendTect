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
#include "horizonadjuster.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimsg.h"
#include "od_helpids.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

uiCorrelationGroup::uiCorrelationGroup( uiParent* p, bool is2d )
    : uiDlgGroup(p,tr("Correlation"))
    , sectiontracker_(0)
    , adjuster_(0)
    , changed_(this)
    , seedpos_(Coord3::udf())
{
    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    usecorrfld_ = new uiGenInput( leftgrp, tr("Use Correlation"),
				  BoolInpSpec(true));
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,selUseCorrelation) );
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    StepInterval<int> intv( -10000, 10000, step );
    IntInpSpec iis; iis.setLimits( intv );

    BufferString disptxt( "Data Display window ", SI().getZUnitString() );
    nrzfld_ = new uiGenInput( leftgrp, disptxt, iis, iis );
    nrzfld_->attach( alignedBelow, usecorrfld_ );
    nrzfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    IntInpSpec tiis; tiis.setLimits( StepInterval<int>(3,99,2) );
    nrtrcsfld_ = new uiGenInput( leftgrp, "Nr Traces", tiis );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    intv.step = 1; iis.setLimits( intv );
    BufferString compwindtxt( "Compare window ", SI().getZUnitString() );
    compwinfld_ = new uiGenInput( leftgrp, compwindtxt, iis, iis );
    compwinfld_->attach( alignedBelow, nrtrcsfld_ );
    compwinfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    tiis.setLimits( StepInterval<int>(0,100,1) );
    corrthresholdfld_ =
	new uiGenInput( leftgrp, tr("Correlation threshold (%)"), tiis );
    corrthresholdfld_->attach( alignedBelow, compwinfld_ );
    corrthresholdfld_->valuechanged.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    uiGroup* rightgrp = new uiGroup( this, "Right Group" );
    rightgrp->attach( rightTo, leftgrp );
    wvafld_ = new uiCheckList( rightgrp, uiCheckList::OneMinimum,
			       OD::Horizontal );
    wvafld_->addItem( "WVA" ).addItem( "VD" );
    wvafld_->changed.notify( mCB(this,uiCorrelationGroup,wvavdChgCB) );

    previewvwr_ = new uiFlatViewer( rightgrp );
    previewvwr_->attach( alignedBelow, wvafld_ );
    previewvwr_->setPrefWidth( 150 );
    previewvwr_->setPrefHeight( 200 );
    previewvwr_->setStretch( 0, 0 );
    previewvwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);
    previewvwr_->appearance().setGeoDefaults( true );

    LineStyle ls( LineStyle::Solid, 3, Color(0,255,0) );
    minitm_ = previewvwr_->createAuxData( "Min line" );
    minitm_->cursor_.shape_ = MouseCursor::SizeVer;
    minitm_->linestyle_ = ls;
    minitm_->poly_ += FlatView::Point(0,0);
    minitm_->poly_ += FlatView::Point(0,0);
    previewvwr_->addAuxData( minitm_ );

    maxitm_ = previewvwr_->createAuxData( "Max line" );
    maxitm_->cursor_.shape_ = MouseCursor::SizeVer;
    maxitm_->linestyle_ = ls;
    maxitm_->poly_ += FlatView::Point(0,0);
    maxitm_->poly_ += FlatView::Point(0,0);
    previewvwr_->addAuxData( maxitm_ );

    seeditm_ = previewvwr_->createAuxData( "Seed" );
    seeditm_->poly_ += FlatView::Point(0,0);
    seeditm_->markerstyles_ += MarkerStyle2D(MarkerStyle2D::Square,3);
    seeditm_->markerstyles_[0].color_ = Color(0,255,0);
    previewvwr_->addAuxData( seeditm_ );

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
    previewvwr_->setSensitive( usecorr );

    nrzfld_->setSensitive( usecorr );
    nrtrcsfld_->setSensitive( usecorr );
}


void uiCorrelationGroup::wvavdChgCB( CallBacker* )
{
    previewvwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
					    wvafld_->isChecked(1) );
    previewvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiCorrelationGroup::visibleDataChangeCB( CallBacker* )
{
    updateViewer();
    updateWindowLines();
}


void uiCorrelationGroup::correlationChangeCB( CallBacker* )
{
    updateWindowLines();
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
    wvafld_->setChecked( 0, true );
    wvafld_->setChecked( 1, false );
}


void uiCorrelationGroup::setSeedPos( const Coord3& crd )
{
    seedpos_ = crd;
    updateViewer();
    updateWindowLines();
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

    previewvwr_->setPack( true, dpid );
    previewvwr_->setPack( false, dpid );
    previewvwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
					    wvafld_->isChecked(1) );
    previewvwr_->setViewToBoundingBox();

    FlatView::Point& pt = seeditm_->poly_[0];
    pt = FlatView::Point( bid.crl(), z );

    previewvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiCorrelationGroup::updateWindowLines()
{
    if ( seedpos_.isUdf() )
	return;

    const BinID& bid = SI().transform( seedpos_.coord() );
    const float z = (float)seedpos_.z;
    const int nrtrcs = nrtrcsfld_->getIntValue();

    StepInterval<float> zintv; zintv.setFrom( compwinfld_->getIInterval() );
    zintv.scale( 1.f/SI().zDomain().userFactor() );
    zintv.step = SI().zStep();

    minitm_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs/2, z+zintv.start );
    minitm_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs/2, z+zintv.start );
    maxitm_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs/2, z+zintv.stop );
    maxitm_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs/2, z+zintv.stop );

    previewvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
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
