/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : September 2018
-*/


#include "seisconvolve.h"

#include "genericnumer.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "waveletattrib.h"
#include "waveletmanager.h"
#include "uistrings.h"


Seis::TrcConvolver::TrcConvolver( const SeisTrcBuf::TrcSet& bufin,
				  const Wavelet& wvlt,
				  SeisTrcBuf::TrcSet* bufout )
    : ParallelTask("Trace convolver")
    , bufin_(bufin)
    , wavelet_(&wvlt)
    , bufout_(bufout ? *bufout : const_cast<SeisTrcBuf::TrcSet& >( bufin ) )
    , totalnr_(bufin.size())
{
    msg_ = tr("Convolving %1").arg( uiStrings::sSeismicData() );
}


Seis::TrcConvolver::~TrcConvolver()
{
}


uiString Seis::TrcConvolver::nrDoneText() const
{
    return Task::sTracesDone();
}


bool Seis::TrcConvolver::doPrepare( int )
{
    SeisTrcBuf::ensureCompatible( bufin_, bufout_ );

    return true;
}


bool Seis::TrcConvolver::doWork( od_int64 start, od_int64 stop, int )
{
    TypeSet<float> wvltsamps;
    wavelet_->getSamples( wvltsamps );
    const float* wvltarr = wvltsamps.arr();
    const int wvltsz = wvltsamps.size();
    const int maxtrcsz = SeisTrcBuf::maxTrcSize( bufin_ );

    const bool inpisout = inpIsOut();
    float* workbuf = 0;
    if ( inpisout )
    {
	mTryAlloc(workbuf,float[maxtrcsz])
	if ( !workbuf )
	{
	    msg_ = uiStrings::phrCannotAllocateMemory( maxtrcsz );
	    return false;
	}
    }

    for ( od_int64 lidx=start; lidx<=stop; lidx++ )
    {
	const int idx = (int)lidx;
	const SeisTrc& trcin = *bufin_.get( idx );
	SeisTrc& trcout = *bufout_.get( idx );
	for ( int icomp=0; icomp<trcin.nrComponents(); icomp++ )
	{
	    const float* arrin = trcin.arr( icomp );
	    float* arrout = inpisout ? workbuf : trcout.arr( icomp );

	    const int sz = trcin.size();
	    const od_int64 nrbytes = mCast(od_int64,sz) * sizeof(float);
	    OD::sysMemZero( arrout, nrbytes );
	    GenericConvolve( sz, 0, arrin, wvltsz, 0, wvltarr, sz, 0, arrout );
	    if ( inpisout )
		OD::sysMemCopy( const_cast<float*>( arrin ), arrout, nrbytes );
	}
    }

    delete [] workbuf;

    return true;
}


bool Seis::reWavelet( const SeisTrcBuf::TrcSet& bufin, const DBKey& refobjid,
		      const DBKey& tarobjid, SeisTrcBuf::TrcSet* bufout,
		      TaskRunnerProvider& trprov, uiString* msg )
{
    if ( refobjid == tarobjid )
	return true;

    ConstRefMan<Wavelet> ref = WaveletMGR().fetch( refobjid );
    ConstRefMan<Wavelet> target = WaveletMGR().fetch( tarobjid );
    if ( !ref || !target )
    {
	if ( msg )
	    msg->set( uiStrings::phrCannotRead( uiStrings::sWavelet() ) );
	return false;
    }

    ConstRefMan<Wavelet> filter = getMatchFilter( *ref, *target, trprov );
    if ( !filter )
	return true;

    TrcConvolver filterapplier( bufin, *filter, bufout );
    const bool success = trprov.execute( filterapplier );
    if ( !success && msg )
	msg->set( filterapplier.message() );

    return success;
}
