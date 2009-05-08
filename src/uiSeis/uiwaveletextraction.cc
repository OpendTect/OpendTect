/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/

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
    , wvletsize_(0)
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
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(60) );
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

    wvletsize_ = mNINT( wtlengthfld_->getfValue() /
	    		      (SI().zStep() * SI().zFactor()) );

    // TODO: Move this to another class
    return doProcess( inputpars );
}


bool uiWaveletExtraction::doProcess( const IOPar& iopar )
{
    Array1DImpl<float> wavelet( wvletsize_ );
    bool res = readInputData(iopar) && doFFT(*seistrcbuf_, wavelet.arr());
    storeWavelet( iopar, wavelet.arr() );
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

    FFT fft;
    const int signalsz = firsttrc->size();
    Array1DImpl<float> signal( signalsz );
    ArrayNDWindow window( Array1DInfoImpl(signalsz), true, 
			  ArrayNDWindow::CosTaper5 );
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

	Array1DImpl<float> acarr( wvletsize_ );
	if ( wvletsize_>signalsz )
	{
	    uiMSG().warning("Signal length should be more than wavelet length");
	    return false;
	}

	float* acarrptr = acarr.arr();

	genericCrossCorrelation( signalsz, 0, signal.arr(),
				 signalsz, 0, signal.arr(),
				 wvletsize_, -wvletsize_/2, acarrptr );
	removeBias( &acarr );
	detrend( acarr );

	Array1DImpl<float_complex> freqdomsignal( wvletsize_ );
	fft.transform( acarr, freqdomsignal );
	for ( int idx=0; idx<wvletsize_; idx++ )
	{
	    if ( idx>=wvletsize_/2 )
    		stackedwvlt[idx] += 
		    	std::abs( freqdomsignal.get(idx-wvletsize_/2) );
	    else
		stackedwvlt[idx] +=
		    	std::abs( freqdomsignal.get(wvletsize_/2-idx) );
	}
    }

    for ( int idx=0; idx<wvletsize_; idx++ )
	stackedwvlt[idx] = stackedwvlt[idx] / nrtrcs;

    return true;
}


void uiWaveletExtraction::detrend( Array1DImpl<float>& normalisation )
{

    float maxval = *(std::max_element(normalisation.arr(), 
		     normalisation.arr()+wvletsize_-1) );

    for( int idx=0; idx<wvletsize_; idx++ )
	normalisation.arr()[idx] = fabs(normalisation.arr()[idx])/fabs(maxval);
}


void uiWaveletExtraction::storeWavelet( const IOPar& iopar, float* vals )
{
    Wavelet wvlt( outputwvltfld_->getInput(), -wvletsize_/2, SI().zStep() );
    wvlt.reSize( wvletsize_ );
    for( int idx=0; idx<wvletsize_; idx++ )
	wvlt.samples()[idx] = vals[idx];
    wvlt.put( wvltctio_.ioobj );
}
