/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgcoltabmapping.h"

#include "valseriesimpl.h"

namespace dGBVis
{

ColTabMapping::ColTabMapping()
{}


ColTabMapping::~ColTabMapping()
{}


void ColTabMapping::setData( const float* data, od_int64 sz, TaskRunner* taskr )
{
    data_ = data;
    datasz_ = sz;
    mapper_.setData( data, sz, taskr );
}


void ColTabMapping::update( bool full, TaskRunner* taskr )
{
    mapper_.update( full, taskr );
    indices_.setSize( int(datasz_*nrBands()), 0 );
    histogram_.setSize( nrSteps(), 0.f );
    histogram_.setAll( 0.f );
    if ( data_ && datasz_>0 )
    {
	ArrayValueSeries<float,float> values( const_cast<float*>(data_), false,
					      datasz_ );
	ColTab::MapperTask<unsigned char> task( mapper_, datasz_,
		(unsigned char)nrSteps(), values, indices_.arr(), nrBands(),
		indices_.arr()+1, nrBands() );
	if ( TaskRunner::execute(taskr,task) )
	{
	    const unsigned int* counts = task.getHistogram();
	    unsigned int max = 0;
	    for ( int idx=0; idx<nrSteps(); idx++ )
	    {
		if ( counts[idx] > max )
		    max = counts[idx];
	    }

	    for ( int idx=0; idx<nrSteps() && max>0; idx++ )
		histogram_[idx] = float(counts[idx]) / float(max);
	}
    }
}

} // namespace dGBVis
