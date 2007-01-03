/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscolortabindexer.cc,v 1.1 2007-01-03 18:22:19 cvskris Exp $";

#include "viscolortabindexer.h"

#include "thread.h"
#include "viscolortab.h"


#define mUndefColIdx	255

namespace visBase
{

TextureColorTabIndexer::TextureColorTabIndexer( const float* inp,
					  unsigned char* outp, int nsz,
					  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( inp )
    , histogrammutex_( *new Threads::Mutex )
    , globalhistogram_( 0 )
    , nrhistogramsteps_( ct->nrSteps() )
{
    histogrammutex_.lock();

    globalhistogram_ = new unsigned int[nrhistogramsteps_];

    memset( globalhistogram_, 0, sizeof(int)*nrhistogramsteps_ );
    histogrammutex_.unlock();
}


TextureColorTabIndexer::~TextureColorTabIndexer()
{
    delete &histogrammutex_;
    delete [] globalhistogram_;
}


const unsigned int* TextureColorTabIndexer::getHistogram() const
{ return globalhistogram_; }


int TextureColorTabIndexer::nrHistogramSteps() const
{ return nrhistogramsteps_; }


int TextureColorTabIndexer::nrTimes() const
{ return sz_; }



bool TextureColorTabIndexer::doWork( int start, int stop, int threadid )
{
    unsigned int histogram[nrhistogramsteps_];
    memset( histogram, 0, sizeof(int)*nrhistogramsteps_ );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int colorindex = colortab_->colIndex(datacache_[idx]);
	indexcache_[idx] = colorindex;
	histogram[colorindex]++;
    }

    histogrammutex_.lock();
    for ( int idx=nrhistogramsteps_-1; idx>=0; idx-- )
	globalhistogram_[idx] += histogram[idx];
    histogrammutex_.unlock();

    return true;
}




}; // namespace visBase
