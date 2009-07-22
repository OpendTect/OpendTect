/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Data buffers and collections of buffers (trace data)
-*/
static const char* rcsID = "$Id: databuf.cc,v 1.21 2009-07-22 16:01:32 cvsbert Exp $";


#include "tracedata.h"
#include "datachar.h"
#include "scaler.h"
#include "odmemory.h"
#ifdef mac
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif


bool RawDataArray::isZero() const
{
    if ( !data_ || !nelem_ || !bytes_ ) return true;

    register const unsigned char* ptr = data_;
    register const int totbytes = nelem_ * bytes_;
    for ( register int idx=0; idx<totbytes; idx++ )
        if ( *ptr++ ) return false;

    return true;
}


DataBuffer::DataBuffer( int n, int byts, bool doinit )
	: RawDataArray(byts)
{
    if ( n > 0 )
    {
	mTryAlloc( data_, unsigned char [ n * bytes_ ] );
	nelem_ = data_ ? n : 0;
    }
    if ( doinit ) zero();
}


DataBuffer::~DataBuffer()
{
    delete [] data_;
}


DataBuffer& DataBuffer::operator=( const DataBuffer& tb )
{
    if ( &tb != this )
    {
	if ( bytes_*nelem_ != tb.bytes_*tb.nelem_ )
	{
	    if ( bytes_ != tb.bytes_ )
	    {
		delete [] data_; data_ = 0;
		bytes_ = tb.bytes_; nelem_ = 0;
	    }
	    reSize( tb.size() );
	}
	bytes_ = tb.bytes_;
	nelem_ = tb.nelem_;
	if ( data_ ) memcpy( data_, tb.data_, nelem_*bytes_ );
    }

    return *this;
}


void DataBuffer::reSize( int newsz, bool copy )
{
    if ( newsz < 0 )
	newsz = 0;
    if ( newsz == nelem_ )
	return;

    if ( newsz == 0 || !copy || nelem_ < 0 )
    {
	delete [] data_; data_ = 0;
	if ( newsz )
	    { mTryAlloc( data_, unsigned char [ newsz * bytes_ ] ); }
    }
    else
    {
	unsigned char* olddata = data_;
	mTryAlloc( data_, unsigned char [ newsz * bytes_ ] );
	if ( data_ )
	{
	    memcpy( data_, olddata, bytes_*(newsz > nelem_ ? nelem_ : newsz) );
	    if ( nelem_ < newsz )
		memset( data_+(nelem_*bytes_), 0, bytes_ * (newsz - nelem_) );
	}
	delete [] olddata;
    }
    nelem_ = data_ ? newsz : 0;
}


void DataBuffer::reByte( int n, bool copy )
{
    if ( n < 1 ) n = 1;
    if ( n == bytes_ ) return;

    bytes_ = n;
    n = nelem_;
    nelem_ = -1;
    reSize( n, copy );
}


void DataBuffer::zero()
{
    if ( data_ )
    {
	od_int64 sz = nelem_; sz *= bytes_;
	MemSetter<unsigned char> msetter( data_, 0, sz );
	msetter.execute();
    }
}

 
TraceData::~TraceData()
{
    for ( int idx=0; idx<nrcomp_; idx++ )
    {
	delete data_[idx];
	delete interp_[idx];
    }
    delete [] data_;
    delete [] interp_;
}


bool TraceData::allOk() const
{
    for ( int idx=0; idx<nrcomp_; idx++ )
	if ( !data_[idx]->isOk() ) return false;
    return true;
}


void TraceData::copyFrom( const TraceData& td )
{
    while ( nrcomp_ > td.nrcomp_ )
	delComponent( nrcomp_ - 1 );
    while ( nrcomp_ < td.nrcomp_ )
	addComponent( td.size(nrcomp_), td.getInterpreter(nrcomp_)->dataChar(),
		      false );

    for ( int idx=0; idx<nrcomp_; idx++ )
    {
	*data_[idx] = *td.data_[idx];
	*interp_[idx] = *td.interp_[idx];
    }
}


void TraceData::copyFrom( const TraceData& td, int icfrom, int icto )
{
    if ( icfrom < td.nrcomp_ && icto < nrcomp_ )
    {
	*data_[icto] = *td.data_[icfrom];
	*interp_[icto] = *td.interp_[icfrom];
    }
}


void TraceData::addComponent( int ns, const DataCharacteristics& dc,
			      bool cleardata )
{
    DataBuffer** newdata = new DataBuffer* [nrcomp_+1];
    TraceDataInterpreter** newinterp = new TraceDataInterpreter* [nrcomp_+1];
    if ( data_ && nrcomp_ )
    {
	for ( int idx=0; idx<nrcomp_; idx++ )
	{
	    newdata[idx] = data_[idx];
	    newinterp[idx] = interp_[idx];
	}
	delete [] data_;
	delete [] interp_;
    }
    newdata[nrcomp_] = new DataBuffer( ns, (int)dc.nrBytes(), cleardata );
    newinterp[nrcomp_] = new TraceDataInterpreter( dc );
    nrcomp_++;
    data_ = newdata;
    interp_ = newinterp;
}


void TraceData::delComponent( int icomp )
{
    if ( icomp < 0 || icomp >= nrcomp_ ) return;

    DataBuffer** newdata = nrcomp_ > 1 ? new DataBuffer* [nrcomp_-1] : 0;
    TraceDataInterpreter** newinterp = nrcomp_ > 1
		? new TraceDataInterpreter* [nrcomp_-1] : 0;
    int targetidx = 0;
    for ( int idx=0; idx<nrcomp_; idx++ )
    {
	if ( idx != icomp )
	{
	    newdata[targetidx] = data_[idx];
	    newinterp[targetidx] = interp_[idx];
	}
	else
	{
	    delete data_[idx];
	    delete interp_[idx];
	    targetidx--;
	}
	targetidx++;
    }

    nrcomp_--;
    delete [] data_;
    delete [] interp_;
    data_ = newdata;
    interp_ = newinterp;
}


void TraceData::reSize( int n, int compnr, bool copydata )
{
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
    {
	if ( compnr < 0 || compnr == icomp )
	    data_[icomp]->reSize( n, copydata );
    }
}


void TraceData::setComponent( const DataCharacteristics& dc, int icomp )
{
    if ( icomp >= nrcomp_ ) return;

    data_[icomp]->reByte( dc.nrBytes() );
    *interp_[icomp] = dc;
}


void TraceData::scale( const Scaler& sclr, int compnr )
{
    if ( compnr < -1 || compnr >= nrcomp_ ) return;
    const int endcomp = compnr < 0 ? nrcomp_-1 : compnr;
    for ( int icomp=(compnr>=0?compnr:0); icomp<=endcomp; icomp++ )
    {
	const int sz = size(icomp);
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    float val = getValue( isamp, icomp );
	    setValue( isamp, sclr.scale(val), icomp );
	}
    }
}


void TraceData::zero( int icomp )
{
    if ( icomp < -1 || icomp >= nrcomp_ ) return;

    const int endidx = icomp < 0 ? nrcomp_-1 : icomp;
    for ( int idx=(icomp>=0?icomp:0); idx<=endidx; idx++ )
	data_[idx]->zero();
}


void TraceData::handleDataSwapping()
{
    for ( int idx=0; idx<nrcomp_; idx++ )
    {
	if ( interp_[idx]->needSwap() )
	    interp_[idx]->swap( data_[idx]->data(), data_[idx]->size() );
    }
}
