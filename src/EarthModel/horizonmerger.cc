/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "horizonmerger.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "statruncalc.h"


namespace EM
{

DefineEnumNames(HorizonMerger,Mode,1,"Merge mode")
{ "Take average", "Use top", "Use base", 0 };

Horizon3DMerger::Horizon3DMerger( const TypeSet<ObjectID>& ids )
    : outputhor_(0)
    , ownsarray_(true)
    , hs_(false)
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const ObjectID& objid( ids[idx] );
	mDynamicCastGet(Horizon3D*,hor,EMM().getObject(objid))
	if ( !hor ) continue;
	inputhors_ += hor;
	IOObjInfo oi( EMM().getMultiID(objid) );
	SurfaceIOData sd;
	if ( oi.getSurfaceData(sd) )
	    { hs_.include( sd.rg ); hs_.step = sd.rg.step; }
    }

    deepRef( inputhors_ );
    depths_ = new Array2DImpl<float>( hs_.nrInl(), hs_.nrCrl() );
    depths_->setAll( mUdf(float) );

    EMObject* emobj = EMM().createTempObject( Horizon3D::typeStr() );
    if ( emobj ) emobj->ref();
    mDynamicCast(Horizon3D*,outputhor_,emobj)
    if ( emobj && !outputhor_ )
	emobj->unRef();
}


Horizon3DMerger::~Horizon3DMerger()
{
    deepUnRef( inputhors_ );
    if ( ownsarray_ )
	delete depths_;

    if ( outputhor_ )
	outputhor_->unRef();
}


Horizon3D* Horizon3DMerger::getOutputHorizon()
{
    return outputhor_;
}


od_int64 Horizon3DMerger::nrIterations() const
{ return hs_.totalNr(); }


bool Horizon3DMerger::doWork( od_int64 start, od_int64 stop, int threadid )
{
    Stats::CalcSetup rcs;
    rcs.require( Stats::Extreme ).require( Stats::Average );
    Stats::RunCalc<float> rc( rcs );
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	rc.clear();
	const BinID bid = hs_.atIndex( idx );
	for ( int hidx=0; hidx<inputhors_.size(); hidx++ )
	{
	    const float zval = inputhors_[hidx]->getZ( bid );
	    if ( mIsUdf(zval) )
		continue;

	    rc += zval;
	}

	if ( rc.isEmpty() )
	    continue;

	if ( mode_ == Average )
	    depths_->getData()[idx] = (float) rc.average();
	else if ( mode_ == Top )
	    depths_->getData()[idx] = rc.min();
	else if ( mode_ == Base )
	    depths_->getData()[idx] = rc.max();

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
