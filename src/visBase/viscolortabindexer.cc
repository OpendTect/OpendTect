/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscolortabindexer.cc,v 1.5 2007-10-30 16:53:36 cvskris Exp $";

#include "viscolortabindexer.h"

#include "thread.h"
#include "valseries.h"
#include "varlenarray.h"
#include "viscolortab.h"


#define mUndefColIdx	255

namespace visBase
{

ColorTabIndexer::ColorTabIndexer( const ValueSeries<float>& inp,
				  unsigned char* outp, int nsz,
				  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( &inp )
    , datacacheptr_( inp.arr() )
    , histogrammutex_( *new Threads::Mutex )
    , globalhistogram_( 0 )
    , nrhistogramsteps_( ct->nrSteps() )
{
    histogrammutex_.lock();

    globalhistogram_ = new unsigned int[nrhistogramsteps_];

    memset( globalhistogram_, 0, sizeof(int)*nrhistogramsteps_ );
    histogrammutex_.unlock();
}


ColorTabIndexer::ColorTabIndexer( const float* inp,
				  unsigned char* outp, int nsz,
				  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( 0 )
    , datacacheptr_( inp )
    , histogrammutex_( *new Threads::Mutex )
    , globalhistogram_( 0 )
    , nrhistogramsteps_( ct->nrSteps() )
{
    histogrammutex_.lock();

    globalhistogram_ = new unsigned int[nrhistogramsteps_];

    memset( globalhistogram_, 0, sizeof(int)*nrhistogramsteps_ );
    histogrammutex_.unlock();
}


ColorTabIndexer::~ColorTabIndexer()
{
    delete &histogrammutex_;
    delete [] globalhistogram_;
}


const unsigned int* ColorTabIndexer::getHistogram() const
{ return globalhistogram_; }


int ColorTabIndexer::nrHistogramSteps() const
{ return nrhistogramsteps_; }


int ColorTabIndexer::totalNr() const
{ return sz_; }



bool ColorTabIndexer::doWork( int start, int stop, int threadid )
{
    mVariableLengthArr( unsigned int, histogram, nrhistogramsteps_ );
    memset( histogram, 0, sizeof(int)*nrhistogramsteps_ );

    if ( datacacheptr_ )
    {
	for ( int idx=start; idx<=stop; idx++, reportNrDone() )
	{
	    const int colorindex = colortab_->colIndex( datacacheptr_[idx] );
	    indexcache_[idx] = colorindex;
	    histogram[colorindex]++;
	}
    }
    else
    {
	for ( int idx=start; idx<=stop; idx++ )
	{
	    const int colorindex = colortab_->colIndex(datacache_->value(idx));
	    indexcache_[idx] = colorindex;
	    histogram[colorindex]++;
	}
    }

    histogrammutex_.lock();
    for ( int idx=nrhistogramsteps_-1; idx>=0; idx-- )
	globalhistogram_[idx] += histogram[idx];
    histogrammutex_.unlock();

    return true;
}



}; // namespace visBase
