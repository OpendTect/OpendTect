/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwaveletextraction.cc,v 1.7 2009-07-02 12:39:04 cvsnageswara Exp $";

#include "uiwaveletextraction.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiposprovgroup.h"
#include "uiposprovgroupstd.h"
#include "uiseissel.h"
#include "uiselsurvranges.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "binidvalset.h"
#include "bufstring.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceposprov.h"
#include "executor.h"
#include "fft.h"
#include "genericnumer.h"
#include "hilberttransform.h"
#include "horsampling.h"
#include "ioman.h"
#include "iopar.h"
#include "multiid.h"
#include "position.h"
#include "posprovider.h"
#include "ptrman.h"
#include "ranges.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "wavelet.h"

#include <algorithm>
#include <iostream>

uiWaveletExtraction::uiWaveletExtraction( uiParent* p )
    : uiDialog( p,Setup("Wavelet Extraction","Specify parameters","104.1.0") )
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , seistrcbuf_(0)
    , wvltsize_(0)
    , zrangefld_(0)
    , isdouble_(false)
{
    seisctio_.ctxt.forread = true;
    seisctio_.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    seisctio_.ctxt.includeconstraints = false;
    seisctio_.ctxt.allowcnstrsabsent = true;

    seisselfld_ = new uiSeisSel( this, seisctio_
	    			     , uiSeisSel::Setup(false,false) );
    seisselfld_->selectiondone.notify( mCB(this,uiWaveletExtraction
					       ,inputSel) );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup( false,false)
	    				   .onlyrange(false)
					   .withstep(true)
	   				   .withoutz(true) );
    subselfld_->attach( alignedBelow, seisselfld_ );

    zextraction_ = new uiGenInput( this, "Vertical Extraction",
	    			   BoolInpSpec(true,"Z range","Horizons") );
    zextraction_->valuechanged.notify(
				mCB(this,uiWaveletExtraction,choiceSel) );
    zextraction_->attach( alignedBelow, subselfld_ );

    BufferString rangelbl = "Z Range ";
    rangelbl += SI().getZUnitString();
    zrangefld_ = new uiSelZRange( this, false, false, rangelbl );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface, this,
	   		 uiPosProvGroup::Setup(false,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    wvltphasefld_ = new uiGenInput( this, "Phase (Degrees)", IntInpSpec(0) );
    wvltphasefld_->attach( alignedBelow, surfacesel_ );

    BufferString lbl = "Wavelet Length ";
    lbl += SI().getZUnitString();
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, wvltphasefld_ );

    wvltctio_.ctxt.forread = false;
    outputwvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
    outputwvltfld_->attach( alignedBelow, wtlengthfld_ );

    finaliseDone.notify( mCB(this,uiWaveletExtraction,choiceSel) );
}     


void uiWaveletExtraction::choiceSel( CallBacker* )
{
    zrangefld_->display( zextraction_->getBoolValue() );
    surfacesel_->display( !zextraction_->getBoolValue() );
}


void uiWaveletExtraction::inputSel( CallBacker* )
{
    CubeSampling cs;
    SeisIOObjInfo si( seisselfld_->ioobj() );
    si.getRanges( cs );
    subselfld_->setInput( *seisselfld_->ioobj() );
    if ( zextraction_->getBoolValue() )
    {
	zrangefld_->setRangeLimits( cs.zrg );
    	zrangefld_->setRange( cs.zrg );
    }
} 


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    if ( !seisselfld_->ioobj() || !outputwvltfld_->ioobj() )
	return false;

    IOPar inputpars, surfacepars;
    seisselfld_->fillPar( inputpars );
    outputwvltfld_->fillPar( inputpars );
    subselfld_->fillPar( inputpars );

    if ( zextraction_->getBoolValue() )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	inputpars.set( sKey::ZRange, zrg );
    }

    else if ( !surfacesel_->fillPar(surfacepars) )
	    return false;

    if ( wtlengthfld_->getfValue() < 0 )
    {
	uiMSG().error( "Please enter correct Wavelet length" );
	wtlengthfld_->setValue( 120 );
	return false;
    }

    wvltsize_ = mNINT( wtlengthfld_->getfValue() /
	    		      (SI().zStep() * SI().zFactor()) ) + 1 ;

    if ( wvltphasefld_->getfValue()<0 || wvltphasefld_->getfValue() >360 )
    {
	uiMSG().error( "Please enter Phase between 0-360" );
	wvltphasefld_->setValue( 0 );
	return false;
    }

    // TODO: Move this to another class
    return doProcess( inputpars, surfacepars );
}


bool uiWaveletExtraction::doProcess( const IOPar& rangepar
				     , const IOPar& surfacepar )
{
    Array1DImpl<float> waveletfd( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	waveletfd.set( idx, 0 );

    Array1DImpl<float> wavelet( wvltsize_ );
    Array1DImpl<float> waveletwithphase( wvltsize_ );
    
    Seis::SelData* sd = 0;
    bool res = readInputData(rangepar, surfacepar,sd) &&
	       doFFT(*seistrcbuf_, sd, waveletfd.arr()) &&
	       doIFFT(waveletfd.arr(),wavelet.arr()) && 
	       calcWvltPhase( wavelet.arr(), waveletwithphase.arr() );
    storeWavelet( waveletwithphase.arr() );

    return res;
}


bool uiWaveletExtraction::fillHorizonSelData( const IOPar& rangepar
					      , const IOPar& surfacepar
					      , Seis::TableSelData& tsd )
{
    Pos::Provider3D* prov = Pos::Provider3D::make( rangepar );
    BufferString surfkey = IOPar::compKey( sKey::Surface,
	    				   Pos::EMSurfaceProvider::id1Key() );
    MultiID surf1mid, surf2mid;
    if ( !surfacepar.get(surfkey.buf(),surf1mid) )
	return false;

    surfkey = IOPar::compKey( sKey::Surface,
	    			  Pos::EMSurfaceProvider::id2Key() );
    const bool isdouble = surfacepar.get( surfkey.buf(), surf2mid );
    isdouble_ = isdouble;

    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( surf1mid );
    mDynamicCastGet(EM::Horizon3D*,horizon1,emobj)
    if ( !horizon1 )
    {
	uiMSG().error( "Error loading horizon" );
	return false;
    }

    if ( isdouble )
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	EM::EMObject* emobj2 = EM::EMM().loadIfNotFullyLoaded( surf2mid );
	mDynamicCastGet( EM::Horizon3D*,horizon2,emobj2 )
	if ( !horizon2 )
	{
	    uiMSG().error( "Error loading second horizon" );
	    return false;
	}
	EM::SectionID sid2 = horizon2->sectionID( 0 );

	BinIDValueSet& bvs = tsd.binidValueSet();
	bvs.allowDuplicateBids( true );
	horizon1->geometry().fillBinIDValueSet( sid, bvs, prov );
	horizon2->geometry().fillBinIDValueSet( sid2, bvs, prov );
    }
    else
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	horizon1->geometry().fillBinIDValueSet( sid,tsd.binidValueSet(),prov );
    }

    const char* extrazkey = IOPar::compKey( sKey::Surface,
	    			  	  Pos::EMSurfaceProvider::extraZKey() );
    Interval<float> extz( 0, 0 );
    if ( surfacepar.get(extrazkey,extz) )
	tsd.extendZ( extz );
    
    return true;
}


bool uiWaveletExtraction::readInputData( const IOPar& rangepar,
					 const IOPar& surfacepar,
       					 Seis::SelData*& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( seisselfld_->key() );
    PtrMan<SeisTrcReader> seisrdr = new SeisTrcReader( ioobj );

    if ( !seistrcbuf_ )
	seistrcbuf_ = new SeisTrcBuf( true );
    else
	seistrcbuf_->erase();
    
    if ( zextraction_->getBoolValue() )
    {
	sd = Seis::SelData::get( rangepar );
	if ( !sd ) return false;

	StepInterval<float> zrg = zrangefld_->getRange();
    	sd->setZRange( zrg );
    }

    else
    {
	Seis::TableSelData* tsd = new Seis::TableSelData;
	fillHorizonSelData( rangepar, surfacepar, *tsd );
	sd = tsd;
    }

    seisrdr->setSelData( sd->clone() );
    if ( !seisrdr->prepareWork() )
	return false;

    SeisBufReader sbreader( *seisrdr, *seistrcbuf_ );
    uiTaskRunner tr( this );
    return tr.execute( sbreader );
}


bool uiWaveletExtraction::doFFT( const SeisTrcBuf& buf, const Seis::SelData* sd
						      , float* stackedwvlt )
{
    if ( buf.size() == 0 ) return false;

    FFT fft;
    fft.setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft.setDir( true );
    fft.init();

    const int nrtrcs = buf.size();
    int nrgoodtrcs = 0;
    const Seis::TableSelData* tsd = 0;
    if ( sd )
	mDynamicCast(const Seis::TableSelData*,tsd,sd)

    for ( int trcidx=0; trcidx<nrtrcs; trcidx++ )
    {
	bool foundundef = false;
	const SeisTrc& trc = *buf.get( trcidx );
	int signalsz = trc.size();

	if ( trc.isNull() ) continue;

	int start = 0;
	int stop = trc.size() - 1;
	if ( tsd )
	{
	    const BinIDValueSet& bvis = tsd->binidValueSet();
	    Interval<float> extz = tsd->extraZ();
	    BinID bid = trc.info().binid;
	    float z1(mUdf(float)), z2(mUdf(float));
	    BinID duplicatebid;
	    BinIDValueSet::Pos pos = bvis.findFirst( bid );
	    bvis.get( pos, bid, z1 );
	    if ( !isdouble_ )
		z2 = z1;
	    else
	    {
		bvis.next( pos );
		bvis.get( pos, duplicatebid, z2 );
		if ( duplicatebid != bid || mIsUdf(z2) )
		    continue;
	    }

	    start = trc.nearestSample( z1 < z2 ? z1 + extz.start
		    			       : z2 + extz.start );
	    stop = trc.nearestSample( z1 < z2 ? z2 + extz.stop
		    			      : z1 + extz.stop );
	    signalsz = 1 + stop - start;
	    if ( signalsz < wvltsize_ )
		continue;
	}

	Array1DImpl<float> signal( signalsz );
	ArrayNDWindow window( signal.info(), true, ArrayNDWindow::CosTaper5 );

	for ( int sidx=start; sidx<=stop; sidx++ )
	{
	    const float val = trc.get( sidx, 0 );
	    if ( !mIsUdf(val) )
		signal.set( sidx-start, val );
	    else
	    {
		foundundef = true;
		break;
	    }
	}

	if ( foundundef ) continue;

	//Cos Taper
	window.apply( &signal );

	Array1DImpl<float> acarr( signalsz );
	float* acarrptr = acarr.arr();
	genericCrossCorrelation( signalsz, 0, signal.arr(),
				 signalsz, 0, signal.arr(),
				 signalsz,  -signalsz/2, acarrptr );

	Array1DImpl<float> temp( wvltsize_ );
	int startidx = (signalsz/2) - ((wvltsize_-1)/2);
	int endidx = (signalsz/2) + ((wvltsize_-1)/2);
	for ( int idx=0; idx<wvltsize_; idx++ )
	    temp.set( idx, acarr.get( startidx+idx ) );

	removeBias( &temp );
	normalisation( temp );

	Array1DImpl<float_complex> timedomsignal( wvltsize_ );
	Array1DImpl<float_complex> freqdomsignal( wvltsize_ );
	for ( int idx=0; idx<wvltsize_; idx++ )
	    timedomsignal.set( idx, temp.arr()[idx] );

	fft.transform( timedomsignal, freqdomsignal );

    	for ( int idx=0; idx<wvltsize_; idx++ )
	{
	    const float val = std::abs( freqdomsignal.arr()[idx] );
	    stackedwvlt[idx] += val;
	}

	nrgoodtrcs++;
    }

    if ( nrgoodtrcs == 0 )
    {
	uiMSG().error( "No valid traces for computation. ",
		      "Please change selection" );
	return false;
    }
    stackedwvlt[0] = 0;
    for ( int idx=1; idx<wvltsize_; idx++ )
	stackedwvlt[idx] = sqrt( stackedwvlt[idx] / nrgoodtrcs );

    return true;
}


bool uiWaveletExtraction::doIFFT( const float* in, float* out )
{
    FFT ifft;
    ifft.setInputInfo( Array1DInfoImpl(wvltsize_) );
    ifft.setDir( false );
    ifft.init();

    Array1DImpl<float_complex> complexsig( wvltsize_ );
    Array1DImpl<float_complex> ifftsig( wvltsize_ );
    
    for ( int idx=0; idx<wvltsize_; idx++ )
	complexsig.set( idx, in[idx] );

    ifft.transform( complexsig, ifftsig );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	if ( idx>=wvltsize_/2 )
    	    out[idx] = ifftsig.get( idx - wvltsize_/2 ).real();
	else
	    out[idx] = ifftsig.get( wvltsize_/2 - idx ).real();
    }

    return true;
}


bool uiWaveletExtraction::calcWvltPhase( const float* vals,
					 float* valswithphase )
{
    Array1DImpl<float> wvltzerophase( wvltsize_ );
    Array1DImpl<float_complex> wvltincalcphase( wvltsize_ );
    HilbertTransform ht;
    ht.init();
    ht.setCalcRange( 0, wvltsize_, 0 );
    for ( int idx=0; idx<wvltsize_; idx++ )
	wvltzerophase.set( idx, vals[idx] );
    bool isht = ht.transform( wvltzerophase, wvltincalcphase );
    if ( !isht )
    {
	uiMSG().error( "Problem in Hilbert Transform" );
	return false;
    }


    int phase = mNINT(wvltphasefld_->getfValue());
    float angle = (float)phase * M_PI/180;
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float realval = wvltzerophase.arr()[idx];
	const float imagval = -wvltincalcphase.arr()[idx].imag();
	valswithphase[idx] = realval*cos( angle ) - imagval*sin( angle );
    }

    return true;
}


void uiWaveletExtraction::normalisation( Array1DImpl<float>& normalisation )
{
    float maxval = fabs( normalisation.arr()[0] );
    for ( int idx=1; idx<wvltsize_; idx++ )
    {
	float val = fabs( normalisation.arr()[idx] );
	if( val > maxval )
	    maxval = val;
    }

    for( int idx=0; idx<wvltsize_; idx++ )
	normalisation.arr()[idx] = (normalisation.arr()[idx])/(maxval);
}


void uiWaveletExtraction::storeWavelet( const float* vals )
{
    Wavelet wvlt( outputwvltfld_->getInput(), -wvltsize_/2, SI().zStep() );
    wvlt.reSize( wvltsize_ );
    for( int idx=0; idx<wvltsize_; idx++ )
	wvlt.samples()[idx] = vals[idx];
    wvlt.put( wvltctio_.ioobj );
}


MultiID uiWaveletExtraction::storeKey() const
{
    return wvltctio_.ioobj ? wvltctio_.ioobj->key() : MultiID("");
}
