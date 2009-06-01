/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwaveletextraction.cc,v 1.5 2009-06-01 05:57:26 cvsnageswara Exp $";

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
#include "bufstring.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "fft.h"
#include "genericnumer.h"
#include "ioman.h"
#include "iopar.h"
#include "multiid.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "wavelet.h"

#include <algorithm>

static const char* zextractopts[] = { "ZRange", "Surface", 0 };


uiWaveletExtraction::uiWaveletExtraction( uiParent* p )
    : uiDialog( p,Setup("Wavelet Extraction","Specify parameters","104.1.0") )
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , seistrcbuf_(0)
    , wvltsize_(0)
    , zrangefld_(0)
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
	    				 StringListInpSpec( zextractopts ) );
    zextraction_->valuechanged.notify( mCB(this,uiWaveletExtraction,
					   choiceSel) );
    zextraction_->attach( alignedBelow, subselfld_ );

    zrangefld_ = new uiSelZRange( this, false, false, "Z Range" );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface,this,
	    		       uiPosProvGroup::Setup(false,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    BufferString lbl = "Wavelet Length ";
    lbl += SI().getZUnitString();
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, surfacesel_ );

    wvltctio_.ctxt.forread = false;
    outputwvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
    outputwvltfld_->attach( alignedBelow, wtlengthfld_ );

    finaliseDone.notify( mCB(this,uiWaveletExtraction,choiceSel) );
}     


void uiWaveletExtraction::choiceSel( CallBacker* )
{
    zextractval_ = zextraction_->getIntValue();
    zrangefld_->display( !zextractval_ );
    surfacesel_->display( zextractval_ );
}


void uiWaveletExtraction::inputSel( CallBacker* )
{
    CubeSampling cs;
    SeisIOObjInfo si( seisselfld_->ioobj() );
    si.getRanges( cs );
    subselfld_->setInput( *seisselfld_->ioobj() );
    if ( zextractval_ == 0 )
    {
	zrangefld_->setRangeLimits( cs.zrg );
    	zrangefld_->setRange( cs.zrg );
    }
} 


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    if ( !seisselfld_->ioobj() || !outputwvltfld_->ioobj() )
	return false;

    IOPar inputpars;
    seisselfld_->fillPar( inputpars );
    outputwvltfld_->fillPar( inputpars );
    subselfld_->fillPar( inputpars );
    if ( zextractval_ == 1 )
    {
	if ( !surfacesel_->fillPar(inputpars) )
	    return false;
    }
    else
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	inputpars.set(  sKey::ZRange, zrg );
    }

    if ( wtlengthfld_->getfValue()<0 )
    {
	uiMSG().error( "Entered wavelet length is wrong" );
	return false;
    }

    wvltsize_ = mNINT( wtlengthfld_->getfValue() /
	    		      (SI().zStep() * SI().zFactor()) ) + 1 ;

    // TODO: Move this to another class
    return doProcess( inputpars );
}


bool uiWaveletExtraction::doProcess( const IOPar& iopar )
{
    Array1DImpl<float> waveletfd( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	waveletfd.set( idx, 0 );

    Array1DImpl<float> wavelet( wvltsize_ );
    
    bool res = readInputData(iopar) && doFFT(*seistrcbuf_,waveletfd.arr()) &&
	       doIFFT(waveletfd.arr(),wavelet.arr());
    storeWavelet( wavelet.arr() );
    return res;
}


bool uiWaveletExtraction::readInputData( const IOPar& iop )
{
    PtrMan<IOObj> ioobj = IOM().get( seisselfld_->key() );
    PtrMan<SeisTrcReader> seisrdr = new SeisTrcReader( ioobj );

    if ( !seistrcbuf_ )
	seistrcbuf_ = new SeisTrcBuf( true );
    else
	seistrcbuf_->erase();
    
    Seis::SelData* sd = Seis::SelData::get( iop );
    if ( zextractval_ == 0 )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
    	sd->setZRange( zrg );
    }
    else
    {
	sd->usePar( iop );
	uiMSG().message( "Not yet completed" );
	return false;
    }
    seisrdr->setSelData( sd );
    if ( !seisrdr->prepareWork() )
	return false;

    SeisBufReader sbreader( *seisrdr, *seistrcbuf_ );
    uiTaskRunner tr( this );
    return tr.execute( sbreader );
}


bool uiWaveletExtraction::doFFT( const SeisTrcBuf& buf, float* stackedwvlt )
{
    const SeisTrc* firsttrc = buf.size()>0 ? buf.get(0) : 0;
    if ( !firsttrc ) return false;

    const int signalsz = firsttrc->size();
    if ( wvltsize_>signalsz )
    {
	uiMSG().warning("Signal length should be more than wavelet length");
	return false;
    }

    FFT fft;
    fft.setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft.setDir( true );
    fft.init();

    Array1DImpl<float> signal( signalsz );
    ArrayNDWindow window( signal.info(), true, ArrayNDWindow::CosTaper5 );

    const int nrtrcs = buf.size();
    for ( int trcidx=0; trcidx<nrtrcs; trcidx++ )
    {
	bool foundundef = false;
	const SeisTrc& trc = *buf.get( trcidx );
	for ( int sidx=0; sidx<trc.size(); sidx++ )
	{
	    const float val = trc.get( sidx, 0 );
	    if ( !mIsUdf(val) )
		signal.set( sidx, val );
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
    }

    stackedwvlt[0] = 0;
    for ( int idx=1; idx<wvltsize_; idx++ )
	stackedwvlt[idx] = sqrt( stackedwvlt[idx] / nrtrcs );

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


void uiWaveletExtraction::normalisation( Array1DImpl<float>& normalisation )
{
    float maxval = *(std::max_element(normalisation.arr(), 
		     normalisation.arr()+wvltsize_-1) );

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
