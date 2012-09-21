/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "attribdataholderarray.h"
#include "attribdataholder.h"
#include "seisinfo.h"

namespace Attrib
{

DataHolderArray::DataHolderArray( const ObjectSet<DataHolder>& dh,
				  bool manageset )
    : dh_(dh)
    , manageset_(manageset)
    , type_(1)
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


DataHolderArray::DataHolderArray( const ObjectSet<DataHolder>& dh, int sidx,
	int dim0sz, int dim1sz, bool manageset )
    : dh_(dh)
    , seriesidx_(sidx)
    , manageset_(manageset)
    , type_(0)
{
    const int nrdh = dh_.size();
    info_.setSize( 0, dim0sz );
    info_.setSize( 1, dim1sz );
    info_.setSize( 2, nrdh ? dh_[0]->nrsamples_ : 0 );
}


DataHolderArray::~DataHolderArray()
{
    if ( manageset_ )
	deepErase( dh_ );
}


void DataHolderArray::set( int i0, int i1, int i2, float val )
{
    if ( type_==1 )
    {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	const int sidx = valididxs[i0];
	ValueSeries<float>* vals = dh_[i1]->series( sidx );
	if ( vals )
	    vals->setValue( i2, val );
    }
    else
    {
	const int idx = (i0*info_.getSize(1)) + i1;
	ValueSeries<float>* vals = dh_[idx]->series( seriesidx_ );
	if ( vals )
	    vals->setValue( i2, val );
    }
}


float DataHolderArray::get( int i0, int i1, int i2 ) const
{
    if ( i0<0 || i1<0 || i2<0 ) return mUdf(float);
   
   if ( type_==1 )
   {
	TypeSet<int> valididxs = dh_[0]->validSeriesIdx();
	const int sidx = valididxs[i0];
	const ValueSeries<float>* valseries = dh_[i1]->series( sidx );
	return valseries ? valseries->value( i2 ) : mUdf(float);
   }
   else
   {
	const int idx = (i0*info_.getSize(1)) + i1;
	const ValueSeries<float>* vals = dh_[idx]->series( seriesidx_ );
	return vals ? vals->value(i2) : mUdf(float);
   }
}

} // namespace Attrib
