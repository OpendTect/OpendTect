/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "impbodyplaneintersect.h"

#include "arrayndimpl.h"
#include "arraytesselator.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "trigonometry.h"
#include "trckeyzsampling.h"


namespace Geometry
{

ImplicitBodyPlaneIntersector::ImplicitBodyPlaneIntersector(
	const Array3D<float>& arr, const TrcKeyZSampling& cs, float threshold,
	char dim, float icz, IndexedShape& ns )
    : output_( ns )
    , arr_( arr )
    , tkzs_( cs )
    , dim_( dim )
    , inlcrlz_( icz )
    , threshold_( threshold )
{}


bool ImplicitBodyPlaneIntersector::compute()
{
    IndexedGeometry* geo = !output_.getGeometry().size() ? 0 :
	const_cast<IndexedGeometry*>(output_.getGeometry()[0]);
    if ( !geo || !output_.coordList() ) return false;

    geo->removeAll( false );

    const int sz0 = dim_ ? tkzs_.nrInl() : tkzs_.nrCrl();
    const int sz1 = dim_==2 ? tkzs_.nrCrl() : tkzs_.nrZ();
    mDeclareAndTryAlloc( PtrMan<Array2D<float> >, data,
	    Array2DImpl<float>(sz0,sz1) );
    if ( !data ) return false;

    const int pidx = !dim_ ?
	tkzs_.hsamp_.inlRange().nearestIndex((int)inlcrlz_) :
	(dim_==1 ? tkzs_.hsamp_.crlRange().nearestIndex((int)inlcrlz_) :
		   tkzs_.zsamp_.nearestIndex(inlcrlz_) );
    const int psz = !dim_ ? tkzs_.nrInl()
			  : (dim_==1 ? tkzs_.nrCrl() : tkzs_.nrZ());
    if ( pidx<0 || pidx>=psz ) return false;

    data->setAll( mUdf(float) );
    Coord3List& crdlist = *output_.coordList();
    const Coord3 normal = !dim_ ? Coord3(1,0,0) :
	( dim_==1 ? Coord3(0,1,0) : Coord3(0,0,1) );
    mDynamicCastGet( ExplicitIndexedShape*, eis, &output_ );
    Coord3List* normallist = 0;
    if ( eis ) normallist = eis->normalCoordList();


    BinID bid( (int)inlcrlz_, (int)inlcrlz_ );
    float z = inlcrlz_;

    for ( int idx=0; idx<sz0; idx++ )
    {
	if ( !dim_ )
	    bid.crl() = tkzs_.hsamp_.crlRange().atIndex(idx);
	else
	    bid.inl() = tkzs_.hsamp_.inlRange().atIndex(idx);

	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( dim_==2 )
		bid.crl() = tkzs_.hsamp_.crlRange().atIndex(idy);
	    else
		z = tkzs_.zsamp_.atIndex(idy);

	    float val;
	    if ( !dim_ )
		val = arr_.get( pidx, idx, idy );
	    else if ( dim_==1 )
		val = arr_.get( idx, pidx, idy );
	    else
		val  = arr_.get( idx, idy, pidx );

	    crdlist.add( Coord3(bid.inl(),bid.crl(),z) ); //Coord isInlCrl
	    if ( val >= threshold_ )
		data->set( idx, idy, val );
	}
    }

    normallist->add( normal );
    const int maxsz = sz0>sz1 ? sz0 : sz1;
    int resolution = maxsz<200 ? 1 : ( maxsz<400 ? 2 : (maxsz<600 ? 3 : 4) );
    if ( mMIN(sz0,sz1)<100 )
	resolution = 1;

    ArrayTesselator tesselator( *data, StepInterval<int>(0,sz0-1,resolution),
	    StepInterval<int>(0,sz1-1,resolution) );
    if ( !tesselator.execute() )
	return false;

    geo->appendCoordIndices( tesselator.arrayIndexes(),false );
    geo->ischanged_ = true;

    return true;
}

} // namespace Geometry
