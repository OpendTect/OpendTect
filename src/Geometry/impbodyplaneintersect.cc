/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : December 2011
-*/

static const char* rcsID = "$Id: impbodyplaneintersect.cc,v 1.1 2011-12-09 22:41:58 cvsyuancheng Exp $";

#include "impbodyplaneintersect.h"

#include "arrayndimpl.h"
#include "arraytesselator.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"


namespace Geometry
{

ImplicitBodyPlaneIntersector::ImplicitBodyPlaneIntersector( 
	const Array3D<float>& arr, const CubeSampling& cs, float threshold,
	char dim, float icz, Coord3List& cl )
    : arr_( arr )
    , cs_( cs )  
    , crdlist_( cl )
    , output_( 0 )
    , dim_( dim )
    , inlcrlz_( icz )		
    , threshold_( threshold )			       	
{}


void ImplicitBodyPlaneIntersector::setShape( const IndexedShape& ns )
{
    delete output_;
    output_ = &ns;
}


const IndexedShape* ImplicitBodyPlaneIntersector::getShape( bool takeover ) 
{ return takeover ? output_ : new IndexedShape(*output_); }


bool ImplicitBodyPlaneIntersector::compute()
{
    IndexedGeometry* geo = !output_ || !output_->getGeometry().size() ? 0 :
	const_cast<IndexedGeometry*>(output_->getGeometry()[0]);
    if ( !geo ) 
	return false;
    geo->removeAll( true );

    const int sz0 = dim_ ? cs_.nrInl() : cs_.nrCrl();
    const int sz1 = dim_==2 ? cs_.nrCrl() : cs_.nrZ();
    mDeclareAndTryAlloc( PtrMan<Array2D<float> >, data,
	    Array2DImpl<float>(sz0,sz1) );
    if ( !data ) return false;
    data->setAll( mUdf(float) );

    const int pidx = !dim_ ? cs_.hrg.inlRange().nearestIndex((int)inlcrlz_) : 
	(dim_==1 ? cs_.hrg.crlRange().nearestIndex((int)inlcrlz_) : 
	 	   cs_.zrg.nearestIndex(inlcrlz_) );
    if ( pidx<0 ) return false;

    const float zscale = SI().zScale();
    BinID bid( (int)inlcrlz_, (int)inlcrlz_ );
    float z = inlcrlz_;

    for ( int idx=0; idx<sz0; idx++ )
    {
	if ( !dim_ )
	    bid.crl = cs_.hrg.crlRange().atIndex(idx);
	else
	    bid.inl = cs_.hrg.inlRange().atIndex(idx);

	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( dim_==2 )
		bid.crl = cs_.hrg.crlRange().atIndex(idy);
	    else
		z = cs_.zrg.atIndex(idy);

	    float val;
	    if ( !dim_ )
		val = arr_.get( pidx, idx, idy );
	    else if ( dim_==1 )
		val = arr_.get( idx, pidx, idy );
	    else
		val  = arr_.get( idx, idy, pidx );

	    crdlist_.add( Coord3(SI().transform(bid),z) );
	    if ( val<threshold_ )
    		data->set( idx, idy, val );
	}
    }

    ArrayTesselator tesselator( *data, StepInterval<int>(0,sz0-1,1),
	    StepInterval<int>(0,sz1-1,1) );
    if ( !tesselator.execute() )
	return false;

    geo->coordindices_ = tesselator.getIndices();
    geo->ischanged_ = true;

    return true;
}


};

