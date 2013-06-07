/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "uiseissingtrcdisp.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "wavelet.h"
#include "seistrc.h"
#include "survinfo.h"


uiSeisSingleTraceDisplay::uiSeisSingleTraceDisplay( uiParent* p )
    : uiFlatViewer(p)
    , compnr_(0)
    , curid_(DataPack::cNoID())
{
    FlatView::Appearance& app = appearance();
    app.annot_.x1_.name_ = " ";
    app.annot_.x2_.name_ = " ";
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.mappersetup_.cliprate_ = Interval<float>(0,0);
    app.ddpars_.wva_.left_ = Color::NoColor();
    app.ddpars_.wva_.right_ = Color::Black();
    app.ddpars_.wva_.mid_ = Color::Black();
    app.ddpars_.wva_.mappersetup_.symmidval_ = mUdf(float);
    app.setDarkBG( false );

    setExtraBorders( uiRect(-10,5,2,5) );
}


void uiSeisSingleTraceDisplay::cleanUp()
{
    removeRefs();
    removePack( curid_ ); curid_ = DataPack::cNoID();
}


void uiSeisSingleTraceDisplay::setData( const Wavelet* wvlt )
{
    cleanUp();

    if ( wvlt )
    {
	const int wvltsz = wvlt->size();
	const float zfac = SI().zDomain().userFactor();

	Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
	FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
	memcpy( fva2d->getData(), wvlt->samples(), wvltsz * sizeof(float) );
	dp->setName( wvlt->name() );
	DPM( DataPackMgr::FlatID() ).add( dp );
	curid_ = dp->id();
	StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
	if ( SI().zIsTime() ) posns.scale( zfac );
	dp->posData().setRange( false, posns );
    }

    setPack( true, curid_, false );
    addRefZ( 0 );

    handleChange( All );
}


void uiSeisSingleTraceDisplay::setData( const SeisTrc* trc, const char* nm )
{
    cleanUp();

    if ( trc )
    {
	const int trcsz = trc->size();
	const float zfac = SI().zDomain().userFactor();

	Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, trcsz );
	FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
	float* ptr = fva2d->getData();
	for ( int idx=0; idx<trcsz; idx++ )
	    *ptr++ = trc->get( idx, compnr_ );
	dp->setName( nm );
	DPM( DataPackMgr::FlatID() ).add( dp );
	curid_ = dp->id();
	StepInterval<double> posns( trc->samplePos(0), trc->samplePos(trcsz-1),
				    trc->info().sampling.step );
	if ( SI().zIsTime() ) posns.scale( zfac );
	dp->posData().setRange( false, posns );
    }

    setPack( true, curid_, false );

    if ( trc )
    {
	float refz = trc->info().zref;
	if ( mIsZero(refz,1e-8) || mIsUdf(refz) )
	    refz = trc->info().pick;
	if ( !mIsZero(refz,1e-8) && !mIsUdf(refz) )
	    addRefZ( refz );
    }

    handleChange( All );
}


void uiSeisSingleTraceDisplay::removeRefs()
{
    deepErase( appearance().annot_.auxdata_ );
    handleChange( Annot );
}


void uiSeisSingleTraceDisplay::addRefZ( float zref )
{
    const float zfac = SI().zDomain().userFactor();
    if ( SI().zIsTime() ) 
	zref *= zfac;

    FlatView::Annotation& ann = appearance().annot_;
    const int curnraux = ann.auxdata_.size();

    FlatView::Annotation::AuxData* ad = new FlatView::Annotation::AuxData( 
	    			BufferString("Ref Z ",curnraux) );
    ad->poly_ += FlatView::Point( 0, zref );
    ad->markerstyles_ += MarkerStyle2D( MarkerStyle2D::HLine, 20,
				Color::stdDrawColor(curnraux) );
    ad->zvalue_ = 100;
    ann.auxdata_ += ad;

    handleChange( Annot );
}
