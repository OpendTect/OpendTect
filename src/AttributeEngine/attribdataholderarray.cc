/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: attribdataholderarray.cc,v 1.3 2007-02-27 15:32:16 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribdataholderarray.h"
#include "attribdataholder.h"
#include "seisinfo.h"

namespace Attrib
{

DataHolderArray::DataHolderArray( const ObjectSet<DataHolder>& dh,
				  bool manageset )
    : dh_(dh)
    , manageset_(manageset)
{
    const int nrdh = dh.size();
    info_.setSize( 0, dh_[0]->nrSeries() );
    info_.setSize( 1, dh_.size() );
    info_.setSize( 2, dh_[0]->nrsamples_ );
}


DataHolderArray::~DataHolderArray()
{
    if ( manageset_ )
	deepErase( dh_ );
}


void DataHolderArray::set( int i0, int i1, int i2, float val )
{
    ValueSeries<float>* vals = dh_[i1]->series( i0 );
    if ( vals )
	vals->setValue( i2, val );
}


float DataHolderArray::get( int i0, int i1, int i2 ) const
{
    const ValueSeries<float>* valseries = dh_[i1]->series( i0 );
    return valseries ? valseries->value( i2 ) : mUdf(float);
}

} // namespace Attrib

