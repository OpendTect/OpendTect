/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tracedata.h"
#include "datachar.h"
#include "scaler.h"
#include "odmemory.h"
#include <limits.h>
#ifdef __mac__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif


bool RawDataArray::isZero() const
{
    if ( !data_ || !nelem_ || !bytes_ ) return true;

    const unsigned char* ptr = data_;
    const int totbytes = nelem_ * bytes_;
    for ( int idx=0; idx<totbytes; idx++ )
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
	if ( data_ ) OD::memCopy( data_, tb.data_, nelem_*bytes_ );
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
	    OD::memCopy( data_, olddata,
		    bytes_*(newsz > nelem_ ? nelem_ : newsz) );
	    if ( nelem_ < newsz )
		OD::memZero( data_+(nelem_*bytes_), bytes_ * (newsz - nelem_) );
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
	OD::memZero( data_, sz );
    }
}


bool DataBuffer::fitsInString() const
{
    od_int64 sz = nelem_; sz *= bytes_;
    return sz < INT_MAX;
}


BufferString DataBuffer::getString() const
{
    BufferString ret;
    if ( data_ && fitsInString() )
    {
	const size_t totsz = (size_t)( nelem_ * bytes_ + 1 );
	ret.setBufSize( totsz );
	char* retptr = ret.getCStr();
	OD::memCopy( retptr, data_, totsz-1 );
	retptr[totsz-1] = '\0';
    }
    return ret;
}


TraceData::~TraceData()
{
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
    {
	delete data_[icomp];
	delete interp_[icomp];
    }
    delete [] data_;
    delete [] interp_;
}


bool TraceData::allOk() const
{
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
	if ( !data_[icomp]->isOk() ) return false;
    return true;
}


bool TraceData::isEmpty() const
{
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
	if ( !data_[icomp]->isEmpty() ) return false;
    return true;
}


void TraceData::copyFrom( const TraceData& td )
{
    while ( nrcomp_ > td.nrcomp_ )
	delComponent( nrcomp_ - 1 );
    while ( nrcomp_ < td.nrcomp_ )
	addComponent( td.size(nrcomp_), td.getInterpreter(nrcomp_)->dataChar(),
		      false );

    for ( int icomp=0; icomp<nrcomp_; icomp++ )
    {
	*data_[icomp] = *td.data_[icomp];
	*interp_[icomp] = *td.interp_[icomp];
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


void TraceData::convertTo( const DataCharacteristics& dc, bool pres )
{
    bool allok = true;
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	const TraceDataInterpreter* di = getInterpreter( icomp );
	if ( di && di->dataChar() != dc )
	    { allok = false; break; }
    }
    if ( allok )
	return;

    const int sz = size();
    TraceData oldtd( *this );
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
	setComponent( dc, icomp );
    reSize( sz );

    if ( pres )
    {
	for ( int icomp=0; icomp<nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float val = oldtd.getValue( idx, icomp );
		setValue( idx, val, icomp );
	    }
	}
    }
}


void TraceData::convertToFPs( bool pres )
{
    bool allfloat = true;
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	const TraceDataInterpreter* di = getInterpreter( icomp );
	if ( di && (di->nrBytes() < 4 || di->dataChar().isInteger()) )
	    { allfloat = false; break; }
    }
    if ( allfloat )
	return;

    const int sz = size();
    TraceData oldtd( *this );
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
	setComponent( DataCharacteristics(), icomp );
    reSize( sz );

    if ( pres )
    {
	for ( int icomp=0; icomp<nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float val = oldtd.getValue( idx, icomp );
		setValue( idx, val, icomp );
	    }
	}
    }
}


bool TraceData::isValidComp( int icomp ) const
{
    return ( icomp>=0 && icomp<nrcomp_ && data_[icomp]->isOk() );
}


float TraceData::getValue( int isamp, int icomp ) const
{
#ifdef __debug__
    if ( !isValidComp(icomp) )
    {
	if ( icomp<0 && icomp>=nrcomp_ )
	    { pErrMsg("Non-existing component requested"); }
	else
	    { pErrMsg("Sample requested from empty trace"); }
	return 0.f;
    }
#endif
    return interp_[icomp]->get( data_[icomp]->data(), isamp );
}


void TraceData::setValue( int isamp, float v, int icomp )
{
#ifdef __debug__
    if ( !isValidComp(icomp) )
	{ pErrMsg( BufferString("Invalid comp: ",icomp) ); }
    else
#endif
	interp_[icomp]->put( data_[icomp]->data(), isamp, v );
}


void TraceData::addComponent( int ns, const DataCharacteristics& dc,
			      bool cleardata )
{
    DataBuffer** newdata = new DataBuffer* [nrcomp_+1];
    TraceDataInterpreter** newinterp = new TraceDataInterpreter* [nrcomp_+1];
    if ( data_ && nrcomp_ )
    {
	for ( int icomp=0; icomp<nrcomp_; icomp++ )
	{
	    newdata[icomp] = data_[icomp];
	    newinterp[icomp] = interp_[icomp];
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


void TraceData::delComponent( int todel )
{
    if ( todel < 0 || todel >= nrcomp_ ) return;

    DataBuffer** newdata = nrcomp_ > 1 ? new DataBuffer* [nrcomp_-1] : 0;
    TraceDataInterpreter** newinterp = nrcomp_ > 1
		? new TraceDataInterpreter* [nrcomp_-1] : 0;
    int targetcomp = 0;
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
    {
	if ( icomp != todel )
	{
	    newdata[targetcomp] = data_[icomp];
	    newinterp[targetcomp] = interp_[icomp];
	}
	else
	{
	    delete data_[icomp];
	    delete interp_[icomp];
	    targetcomp--;
	}
	targetcomp++;
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


void TraceData::setNrComponents( int newnrcomps,
				 DataCharacteristics::UserType utin )

{
    const int oldnrcomps = nrComponents();
    const bool isautodatarep = utin == DataCharacteristics::Auto;
    if ( oldnrcomps == newnrcomps )
    {
	if ( isautodatarep )
	    return;

	bool isok = true;
	for ( int icomp=0; icomp<oldnrcomps; icomp++ )
	{
	    DataCharacteristics::UserType ut =
		getInterpreter(icomp)->dataChar().userType();
	    if ( ut != utin )
		{ isok = false; break; }
	}
	if ( isok )
	    return;
    }

    const int sz = size();
    if ( isautodatarep )
    {
	DataCharacteristics::UserType ut =
			getInterpreter(0)->dataChar().userType();
	utin = oldnrcomps > 0 ? ut : DataCharacteristics::F32;
    }

    while ( nrComponents() > 0 )
	delComponent( 0 );

    for ( int icomp=0; icomp<newnrcomps; icomp++ )
	addComponent( sz, DataCharacteristics(utin) );
}


void TraceData::scale( const Scaler& sclr, int compnr )
{
    if ( sclr.isEmpty() || compnr < -1 || compnr >= nrcomp_ )
	return;

    convertToFPs( true );

    const int endcomp = compnr < 0 ? nrcomp_-1 : compnr;
    for ( int icomp=(compnr>=0?compnr:0); icomp<=endcomp; icomp++ )
    {
	const int sz = size(icomp);
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    float val = getValue( isamp, icomp );
	    setValue( isamp, (float)sclr.scale(val), icomp );
	}
    }
}


void TraceData::zero( int targetcomp )
{
    if ( targetcomp < -1 || targetcomp >= nrcomp_ ) return;

    const int endicomp = targetcomp < 0 ? nrcomp_-1 : targetcomp;
    for ( int icomp=(targetcomp>=0?targetcomp:0); icomp<=endicomp; icomp++ )
	data_[icomp]->zero();
}


void TraceData::handleDataSwapping()
{
    for ( int icomp=0; icomp<nrcomp_; icomp++ )
    {
	if ( interp_[icomp]->needSwap() )
	    interp_[icomp]->swap( data_[icomp]->data(), data_[icomp]->size() );
    }
}
