/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

mDefineEnumUtils(HorizonMerger,Mode,"Merge mode")
{ "Take average", "Use top", "Use base", nullptr };


HorizonMerger::HorizonMerger()
{}


HorizonMerger::~HorizonMerger()
{}


Horizon3DMerger::Horizon3DMerger( const TypeSet<ObjectID>& ids )
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const ObjectID& objid( ids[idx] );
	mDynamicCastGet(Horizon3D*,hor,EMM().getObject(objid))
	if ( !hor ) continue;
	inputhors_ += hor;
	IOObjInfo oi( EMM().getMultiID(objid) );
	SurfaceIOData sd;
	uiString errmsg;
	if ( oi.getSurfaceData(sd,errmsg) )
	{
	    if( idx == 0 )
		hs_ = sd.rg;
	    else
		hs_.include( sd.rg );

	    hs_.step_ = sd.rg.step_;
	}
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
{
    return hs_.totalNr();
}


uiString Horizon3DMerger::uiMessage() const
{
    return tr("Merging 3D horizons");
}


uiString Horizon3DMerger::uiNrDoneText() const
{
    return sPosFinished();
}


bool Horizon3DMerger::doWork( od_int64 start, od_int64 stop, int threadid )
{
    Stats::CalcSetup rcs;
    rcs.require( mode_ == Average ? Stats::Average : Stats::Extreme );
    Stats::RunCalc<float> rc( rcs );
    TrcKeySampling hs( hs_ );
    TrcKeySamplingIterator iter( hs );
    iter.setNextPos( hs.trcKeyAt(start) );
    BinID bid;
    for ( od_int64 idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	iter.next( bid );
	rc.clear();
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
    }

    return true;
}


bool Horizon3DMerger::doFinish( bool success )
{
    if ( !success )
	return success;

    Geometry::BinIDSurface* geom = outputhor_->geometry().geometryElement();
    if ( !geom )
	return false;

    geom->setArray( hs_.start_, hs_.step_, depths_, true );
    ownsarray_ = false;
    return success;
}

} // namespace EM
