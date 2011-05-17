/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: horizonmerger.cc,v 1.1 2011-05-17 11:57:09 cvsnanne Exp $";

#include "horizonmerger.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"


namespace EM
{

Horizon3DMerger::Horizon3DMerger( const TypeSet<ObjectID>& ids )
    : outputhor_(0)
    , ownsarray_(true)
    , hs_(false)
{
    SurfaceIOData sd;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	mDynamicCastGet(Horizon3D*,hor,EMM().getObject(ids[idx]))
	inputhors_ += hor;
	if ( hor && !EM::EMM().getSurfaceData(hor->multiID(),sd) )
	    hs_.include( sd.rg );
    }

    deepRef( inputhors_ );
    depths_ = new Array2DImpl<float>( hs_.nrInl(), hs_.nrCrl() );
    depths_->setAll( mUdf(float) );
}


Horizon3DMerger::~Horizon3DMerger()
{
    deepUnRef( inputhors_ );
    if ( ownsarray_ )
	delete depths_;
}


Horizon3D* Horizon3DMerger::getOutputHorizon()
{
    return outputhor_;
}


od_int64 Horizon3DMerger::nrIterations() const
{ return hs_.totalNr(); }


bool Horizon3DMerger::doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	float sum = 0;
	int count = 0;
	const BinID bid = hs_.atIndex( idx );
	for ( int hidx=0; hidx<inputhors_.size(); hidx++ )
	{
	    const float zval = inputhors_[hidx]->getZ( bid );
	    if ( mIsUdf(zval) )
		continue;

	    sum += zval;
	    count++;
	}

	if ( count > 0 )
	    depths_->getData()[idx] = sum / count; 

	addToNrDone( 1 );
    }

    return true;
}


bool Horizon3DMerger::doFinish( bool success )
{
    if ( !success )
	return success;

    EM::SectionID sid = outputhor_->sectionID( 0 );
    Geometry::BinIDSurface* geom = outputhor_->geometry().sectionGeometry( sid);
    if ( !geom )
	return false;

    geom->setArray( hs_.start, hs_.step, depths_, true );
    ownsarray_ = false;
    return success;
}

} // namespace EM
