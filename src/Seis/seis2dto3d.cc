/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/



static const char* rcsID = "$Id: seis2dto3d.cc,v 1.1 2011-02-21 14:18:30 cvsbruno Exp $";

#include "seis2dto3d.h"

#include "arrayndutils.h"
#include "bufstring.h"
#include "ioobj.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "seiswrite.h"


Seis2DTo3D::Seis2DTo3D()
    : Executor("Generating 3D pseudo cube")
    , seisbuf_(*new SeisTrcBuf(false))
    , wrr_(0)
    , cs_(false)
    , nriter_(100)	
    , nrdone_(0)
    , max_(0)
    , ls_(0)
    , outioobj_(0)	    
{}


Seis2DTo3D::~Seis2DTo3D()
{
    clear();
}


void Seis2DTo3D::clear()
{
    errmsg_.setEmpty();
    delete wrr_; wrr_ = 0;
    nriter_ = 1;
    totnr_ = -1; nrdone_ = 0;
}


const CubeSampling& Seis2DTo3D::setInput( const IOObj& obj, const char* attrnm )
{
    clear();

    ls_ = new Seis2DLineSet( obj ); 
    attrnm_ = attrnm;

    if ( read() )
    {
	for ( int idx=0; idx<seisbuf_.size(); idx++ )
	{
	    const SeisTrc& trc = *seisbuf_.get( idx );
	    cs_.hrg.include( trc.info().binid );
	}
	StepInterval<float> si;
	const SeisTrc& trc = *seisbuf_.get( 0 );
	si.start = trc.info().sampling.start;
	si.step = trc.info().sampling.step;
	si.stop = trc.size()*trc.info().sampling.step + si.start;
	cs_.zrg = si;
    }
    return cs_;
}


void Seis2DTo3D::setNrIter( int nriter )
{
    nriter_ = nriter;
}


void Seis2DTo3D::setOutput( IOObj& obj, const CubeSampling* outcs )
{
    outioobj_ = &obj;
    if ( outcs )
	cs_ = *outcs;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool Seis2DTo3D::read()
{
    if ( ls_->nrLines() < 1 )
	mErrRet( "Empty LineSet" )
    BufferStringSet lnms;
    ls_->getLineNamesWithAttrib( lnms, attrnm_.buf() );
    if ( lnms.isEmpty() )
	mErrRet( "No lines for this attribute" )

    SeisTrcBuf tmpbuf(false);
    seisbuf_.erase();
    for ( int idx=0; idx<lnms.size(); idx++)
    {
	const LineKey lk( lnms.get( idx ), attrnm_.buf() );
	const int lsidx = ls_->indexOf( lk );
	if ( lsidx < 0 )
	    continue;
	tmpbuf.erase();
	Executor* lf = ls_->lineFetcher( lsidx, tmpbuf );
	lf->execute();
	seisbuf_.add( tmpbuf );
    }
    if ( seisbuf_.isEmpty() )
	mErrRet("No trace present in dataset")

    return true;
}


bool Seis2DTo3D::write()
{
    if ( !wrr_ )
	wrr_ = new SeisTrcWriter( outioobj_ );
    Seis::RangeSelData* sd = new Seis::RangeSelData( cs_ );
    wrr_->setSelData( sd );
    return true;
}


void Seis2DTo3D::doPrepare()
{
    fft_ = Fourier::CC::createDefault();
    szx_ = fft_->getFastSize( cs_.hrg.inlRange().nrSteps() ); 
    szy_ = fft_->getFastSize( cs_.hrg.crlRange().nrSteps() );
    szz_ = fft_->getFastSize( cs_.zrg.nrSteps() );

    setUpData();
}




#define mDefThreshold ( (float)(nriter_-nrdone_-10)/ (float)nriter_ )
#define mDoTransform(tf,isstraight,inp,outp,onedim) \
{   \
    if ( onedim )\
	tf.setInputInfo( Array1DInfoImpl(szz_) );\
    else\
	tf.setInputInfo( Array2DInfoImpl(szx_,szy_) );\
    tf.setDir(isstraight);\
    tf.setNormalization(!isstraight);\
    tf.setInput(inp.getData());\
    tf.setOutput(outp.getData());\
    tf.run(true);\
}
int Seis2DTo3D::nextStep()
{
    if ( nrdone_ == nriter_ ) 
	{ setFinalTrcs(); return Executor::Finished(); }

    if ( nrdone_ == 0 )
	doPrepare();

    Array2DImpl<float_complex> fftsignal( szx_, szy_ );

    for ( int idz=0; idz<szz_; idz++ )
    {
	if ( nrdone_ == 0 )
	{
	    signals2d_ += new Array2DImpl<float_complex>( szx_, szy_ );
	    signals2d_[idz]->setAll( 0 ); 
	}

	for ( int idtrc=0; idtrc<posidxs_.size(); idtrc++ )
	{
	    TrcPosTrl& trpos = posidxs_[idtrc];
	    const Array1DImpl<float_complex>& signals1d = *fftsignals1d_[idtrc];
	    signals2d_[idz]->set( trpos.idx_, trpos.idy_, signals1d.get(idz) );
	}

	mDoTransform( (*fft_), true, (*signals2d_[idz]), fftsignal, false );

	float initreal = fftsignal.get(0,0).real();
	float initimag = fftsignal.get(0,0).imag();
	if ( nrdone_ == 0 )
	{
	    max_ = initreal*initreal + initimag*initimag; 
	    for ( int idx=0; idx<szx_; idx++ )
	    {
		for ( int idy=0; idy<szy_; idy++ )
		{
		    float real = fftsignal.get(idx,idy).real();
		    float imag = fftsignal.get(idx,idy).imag();
		    float mod = real*real + imag*imag;
		    if ( mod > max_ ) max_ = mod;
		}
	    }
	}
	for ( int idx=0; idx<szx_; idx++ )
	{
	    for ( int idy=0; idy<szy_; idy++ )
	    {
		float real = fftsignal.get(idx,idy).real();
		float imag = fftsignal.get(idx,idy).imag();
		float mod = real*real + imag*imag;
		if ( mod < max_*mDefThreshold )
		   fftsignal.set(idx,idy,0);
	    }
	}
	mDoTransform( (*fft_), false, fftsignal, (*signals2d_[idz]) , false);
    }
    
    nrdone_++;
    return Executor::MoreToDo();
}


const BinID Seis2DTo3D::convertToBID( int idx, int idy ) const
{
    return BinID( cs_.hrg.inlRange().atIndex(idx), 
	    	  cs_.hrg.crlRange().atIndex(idy) );
}


const SeisTrc* Seis2DTo3D::getTrcInSet( const BinID& bin ) const
{
    for ( int idx=0; idx<seisbuf_.size(); idx++ )
    {
	const SeisTrc* trc = seisbuf_.get( idx );
	if ( trc->info().binid == bin )
	   return trc; 
    }
    return 0;
}


void Seis2DTo3D::setUpData()
{
    int trcpos = -1;
    deepErase( fftsignals1d_ );
    deepErase( signals2d_ );

    Array1DImpl<float_complex> carr( szz_ );
    for ( int idx=0; idx<szx_; idx++ )
    {
	for ( int idy=0; idy<szy_; idy++ )
	{
	    trcpos ++;
	    const BinID bid = convertToBID( idx, idy );
	    const SeisTrc* trc = getTrcInSet( bid );
	    if ( !trc ) 
		continue;

	    Array1DImpl<float_complex>* fftarr
				= new Array1DImpl<float_complex>(szz_);
	    posidxs_ += TrcPosTrl( idx, idy, trcpos );
	    for ( int idz=0; idz<szz_; idz ++ )
	    {
		const float val = trc->get( idz, 0 ); 
		carr.set( idz, val );
	    }

	    mDoTransform( (*fft_), true, carr, (*fftarr), true);
	    fftsignals1d_ += fftarr;
	}
    }
}
	    

void Seis2DTo3D::setFinalTrcs()
{
    deepErase( fftsignals1d_ );
    SeisTrc trc( szz_ );
    write();
    for ( int idx=0; idx<szx_; idx++ )
    {
	for ( int idy=0; idy<szy_; idy++ )
	{
	    Array1DImpl<float_complex> fftarr( szz_ );
	    for ( int idz=0; idz<szz_; idz ++ )
	    {
		float_complex val = signals2d_[idz]->get( idx, idy );
		fftarr.set( idz, val );
	    }

	    Array1DImpl<float_complex> carr( szz_ );
	    mDoTransform( (*fft_), false, fftarr, carr, true);
	    
	    const BinID bid = convertToBID( idx, idy );
	    if ( bid.inl <0 || bid.crl < 0 ) continue; 
	    trc.info().binid = bid; 
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		float val = (carr.get( idz )).real();
		if ( mIsUdf(val) ) val = 0;
		trc.set( idz, val, 0 );
	    }
	    wrr_->put( trc );
	}
    }
    deepErase( signals2d_ );
}
