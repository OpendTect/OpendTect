/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Data buffers and collections of buffers (trace data)
-*/


#include "tracedata.h"
#include "datachar.h"
#include <malloc.h>


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
	nelem_ = n;
	data_ = new unsigned char [ nelem_ * bytes_ ];
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


void DataBuffer::reSize( int n, bool copy )
{
    if ( n < 0 ) n = 0;
    if ( n == nelem_ )
	return;

    if ( !n || !copy || nelem_ < 0 )
    {
	delete [] data_;
	data_ = n ? new unsigned char [ n * bytes_ ] : 0;
    }
    else
    {
	unsigned char* olddata = data_;
	data_ = new unsigned char [ n * bytes_ ];
	if ( data_ )
	    memcpy( data_, olddata, n > nelem_ ? nelem_ : n );
	delete [] olddata;
    }
    nelem_ = data_ ? n : 0;
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
	memset( (char*)data_, 0, nelem_*bytes_ );
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


void TraceData::reSize( int n, int icomp, bool copydata )
{
    if ( icomp < nrcomp_ )
	data_[icomp]->reSize( n, copydata );
}


void TraceData::setComponent( const DataCharacteristics& dc, int icomp )
{
    if ( icomp >= nrcomp_ ) return;

    data_[icomp]->reByte( dc.nrBytes() );
    *interp_[icomp] = dc;
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
