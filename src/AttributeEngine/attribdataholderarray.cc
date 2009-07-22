/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: attribdataholderarray.cc,v 1.6 2009-07-22 16:01:29 cvsbert Exp $";

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
    if ( i0<0 || i1<0 || i2<0 ) return mUdf(float);
    
    const ValueSeries<float>* valseries = dh_[i1]->series( i0 );
    return valseries ? valseries->value( i2 ) : mUdf(float);
}

} // namespace Attrib

