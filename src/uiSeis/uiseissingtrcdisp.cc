/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseissingtrcdisp.cc,v 1.1 2012-05-01 11:39:45 cvsbert Exp $";


#include "uiseissingtrcdisp.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "wavelet.h"
#include "seistrc.h"
#include "survinfo.h"


uiSeisSingleTraceDisplay::uiSeisSingleTraceDisplay( uiParent* p, bool annot )
    : uiFlatViewer(p)
    , compnr_(0)
    , curid_(DataPack::cNoID())
{
    FlatView::Appearance& app = appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ = SI().zIsTime() ? "Time" : "Depth";
    app.annot_.setAxesAnnot( annot );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.mappersetup_.cliprate_ = Interval<float>(0,0);
    app.ddpars_.wva_.left_ = Color::NoColor();
    app.ddpars_.wva_.right_ = Color::Black();
    app.ddpars_.wva_.mid_ = Color::Black();
    app.ddpars_.wva_.mappersetup_.symmidval_ = mUdf(float);
    app.setDarkBG( false );

    setExtraBorders( uiRect(2,5,2,5) );
}


void uiSeisSingleTraceDisplay::setData( const Wavelet* wvlt )
{
    removePack( curid_ ); curid_ = DataPack::cNoID();
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
    handleChange( All );
}


void uiSeisSingleTraceDisplay::setData( const SeisTrc* trc, const char* nm )
{
    removePack( curid_ ); curid_ = DataPack::cNoID();
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
    handleChange( All );
}
