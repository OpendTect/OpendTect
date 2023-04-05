/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseissingtrcdisp.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "seistrc.h"
#include "survinfo.h"
#include "wavelet.h"


uiSeisSingleTraceDisplay::uiSeisSingleTraceDisplay( uiParent* p )
    : uiFlatViewer(p)
    , compnr_(0)
{
    FlatView::Appearance& app = appearance();
    app.annot_.x1_.name_ = " ";
    app.annot_.x2_.name_ = " ";
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.mappersetup_.cliprate_ = Interval<float>(0,0);
    app.ddpars_.wva_.refline_ = OD::Color::Black();
    app.ddpars_.wva_.mappersetup_.symmidval_ = mUdf(float);
    app.setDarkBG( false );

    setExtraBorders( uiSize(-10,5), uiSize(2,5) );
}


uiSeisSingleTraceDisplay::~uiSeisSingleTraceDisplay()
{}


void uiSeisSingleTraceDisplay::cleanUp()
{
    removeAllAuxData();
    fdp_ = nullptr;
}


void uiSeisSingleTraceDisplay::setData( const Wavelet* wvlt )
{
    cleanUp();

    RefMan<FlatDataPack> dp;
    if ( wvlt )
    {
	const int wvltsz = wvlt->size();
	const float zfac = mCast( float, SI().zDomain().userFactor() );

	auto* fva2d = new Array2DImpl<float>( 1, wvltsz );
	dp = new FlatDataPack( "Wavelet", fva2d );
	OD::memCopy( fva2d->getData(), wvlt->samples(), wvltsz*sizeof(float) );
	dp->setName( wvlt->name() );
	StepInterval<double> posns;
	posns.setFrom( wvlt->samplePositions() );
	if ( SI().zIsTime() )
	    posns.scale( zfac );

	dp->posData().setRange( false, posns );
    }

    fdp_ = dp;
    setPack( FlatView::Viewer::WVA, dp, false );
    addRefZ( 0 );

    handleChange( sCast(od_uint32,FlatView::Viewer::All) );
    setViewToBoundingBox();
}


void uiSeisSingleTraceDisplay::setData( const SeisTrc* trc, const char* nm )
{
    cleanUp();

    RefMan<FlatDataPack> dp;
    if ( trc )
    {
	const int trcsz = trc->size();
	const float zfac = mCast( float, SI().zDomain().userFactor() );

	auto* fva2d = new Array2DImpl<float>( 1, trcsz );
	dp = new FlatDataPack( "Wavelet", fva2d );
	float* ptr = fva2d->getData();
	for ( int idx=0; idx<trcsz; idx++ )
	    *ptr++ = trc->get( idx, compnr_ );
	dp->setName( nm );
	StepInterval<double> posns( trc->samplePos(0), trc->samplePos(trcsz-1),
				    trc->info().sampling.step );
	if ( SI().zIsTime() ) posns.scale( zfac );
	dp->posData().setRange( false, posns );
    }

    fdp_ = dp;
    setPack( FlatView::Viewer::WVA, dp, false );

    if ( trc )
    {
	float refz = trc->info().zref;
	if ( mIsZero(refz,1e-8) || mIsUdf(refz) )
	    refz = trc->info().pick;
	if ( !mIsZero(refz,1e-8) && !mIsUdf(refz) )
	    addRefZ( refz );
    }

    handleChange( sCast(od_uint32,FlatView::Viewer::All) );
    setViewToBoundingBox();
}


void uiSeisSingleTraceDisplay::addRefZ( float zref )
{
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    if ( SI().zIsTime() )
	zref *= zfac;

    const int curnraux = nrAuxData();
    FlatView::AuxData* ad = createAuxData(
				BufferString("Ref Z ",curnraux) );
    ad->poly_ += FlatView::Point( 0, zref );
    ad->markerstyles_ += MarkerStyle2D( MarkerStyle2D::HLine, 20,
				OD::Color::stdDrawColor(curnraux) );
    ad->zvalue_ = 100;
    addAuxData( ad );

    handleChange( sCast(od_uint32,FlatView::Viewer::Annot) );
}
