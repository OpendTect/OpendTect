/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: attribdataholderarray.cc,v 1.8 2012/06/29 19:13:19 cvsyuancheng Exp $";

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
    const int nrdh = dh_.size();
    int nrseries = 0;
    int nrsamples = 0;
    if ( nrdh>0 )
    {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	nrseries = valididxs.size();
	nrsamples = dh_[0]->nrsamples_;
    }
    
    info_.setSize( 0, nrseries );
    info_.setSize( 1, nrdh );
    info_.setSize( 2, nrsamples );
}


DataHolderArray::~DataHolderArray()
{
    if ( manageset_ )
	deepErase( dh_ );
}


void DataHolderArray::set( int i0, int i1, int i2, float val )
{
    TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
    const int sidx = valididxs[i0];
    ValueSeries<float>* vals = dh_[i1]->series( sidx );
    if ( vals )
	vals->setValue( i2, val );
}


float DataHolderArray::get( int i0, int i1, int i2 ) const
{
    if ( i0<0 || i1<0 || i2<0 ) return mUdf(float);
   
    TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
    const int sidx = valididxs[i0];
    const ValueSeries<float>* valseries = dh_[i1]->series( sidx );
    return valseries ? valseries->value( i2 ) : mUdf(float);
}

} // namespace Attrib
