/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
-*/


static const char* rcsID = "$Id: attribdataholder.cc,v 1.2 2006-05-03 17:57:52 cvskris Exp $";

#include "attribdataholder.h"

#include "seisinfo.h"

namespace Attrib
{


DataHolder::DataHolder( int z0, int nrsamples )
    : z0_(z0), nrsamples_(nrsamples)
{ data_.allowNull(true); }


DataHolder::~DataHolder()
{ deepErase(data_); }


DataHolder* DataHolder::clone() const
{
    DataHolder* dh = new DataHolder( z0_, nrsamples_ );
    dh->data_.allowNull(true);

    for ( int idx=0; idx<nrSeries(); idx++ )
    {
	if ( !data_[idx] ) dh->add( true );
	else
	{
	    dh->add();
	    memcpy( dh->data_[idx]->arr(), data_[idx]->arr(),
	    nrsamples_*sizeof(float) );
	}
    }

    return dh;
}


ValueSeries<float>* DataHolder::add( bool addnull )
{
    ValueSeries<float>* res = addnull ? 0
	: new ArrayValueSeries<float>( new float[nrsamples_], true );
    data_ += res;
    return res;
}


bool DataHolder::dataPresent( int samplenr ) const
{
    if ( samplenr >= z0_ && samplenr < (z0_ + nrsamples_) )
	return true;

    return false;
}


TypeSet<int> DataHolder::validSeriesIdx() const
{
    TypeSet<int> seriesidset;
    for( int idx=0; idx<nrSeries(); idx++ )
    {
	if ( data_[idx] )
	    seriesidset += idx;
    }

    return seriesidset;
}


void DataHolder::replace( int idx, ValueSeries<float>* valseries )
{
    ValueSeries<float>* ptr = data_.replace( idx, valseries );
    if ( ptr ) delete ptr;
}


Data2DHolder::~Data2DHolder()
{
    deepErase( dataset_ );
    deepErase( trcinfoset_ );
}

}; // namespace Attrib
