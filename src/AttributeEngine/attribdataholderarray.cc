/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: attribdataholderarray.cc,v 1.1 2007-02-02 15:42:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribdataholderarray.h"
#include "attribdataholder.h"
#include "seisinfo.h"

namespace Attrib
{

Data2DHolderArray::Data2DHolderArray( Data2DHolder& dh )
    : dh_(dh)
{
    dh_.ref();

    const CubeSampling cs = dh_.getCubeSampling();
    info_.setSize( 0, dh_.dataset_[0]->nrSeries() );
    info_.setSize( 1, cs.nrCrl() );
    info_.setSize( 2, cs.nrZ() );

    trcidxs_.setSize( cs.nrCrl(), -1 );
    for ( int idx=0; idx<dh_.trcinfoset_.size(); idx++ )
    {
	const int trcnr = dh_.trcinfoset_[idx]->nr;
	trcidxs_[ cs.crlIdx(trcnr) ] = idx;
    }
}


Data2DHolderArray::~Data2DHolderArray()
{
    dh_.unRef();
}


void Data2DHolderArray::set( int i0, int i1, int i2, float val )
{
    const int trcidx = trcidxs_[i1];
    if ( trcidx < 0 )
	return;

    ValueSeries<float>* vals = dh_.dataset_[trcidx]->series( i0 );
    if ( vals )
	vals->setValue( i2, val );
}


float Data2DHolderArray::get( int i0, int i1, int i2 ) const
{
    const int trcidx = trcidxs_[i1];
    if ( trcidx < 0 )
	return mUdf(float);

    ValueSeries<float>* valseries = dh_.dataset_[trcidx]->series( i0 );
    return valseries ? valseries->value( i2 ) : mUdf(float);
}

} // namespace Attrib

