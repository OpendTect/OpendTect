/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
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

mDefineEnumUtils(EM::HorizonMerger,Mode,"Merge mode")
{ "Take average", "Use top", "Use base", 0 };

template<>
void EnumDefImpl<EM::HorizonMerger::Mode>::init()
{
    uistrings_ += mEnumTr("Take Average",0);
    uistrings_ += mEnumTr("Use Top","Top of a geologic model");
    uistrings_ += mEnumTr("Use Base","Base of a geologic model");
}

namespace EM
{

Horizon3DMerger::Horizon3DMerger( const DBKeySet& ids )
    : outputhor_(0)
    , ownsarray_(true)
    , hs_(false)
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const DBKey& objid( ids[idx] );
	mDynamicCastGet(Horizon3D*,hor,Hor3DMan().getObject(objid))
	if ( !hor )
	    continue;
	inputhors_ += hor;
	IOObjInfo oi( objid );
	SurfaceIOData sd;
	uiString errmsg;
	if ( oi.getSurfaceData(sd,errmsg) )
	    { hs_.include( sd.rg ); hs_.step_ = sd.rg.step_; }
    }

    deepRef( inputhors_ );
    depths_ = new Array2DImpl<float>( hs_.nrInl(), hs_.nrCrl() );
    depths_->setAll( mUdf(float) );

    Object* emobj = Hor3DMan().createTempObject( Horizon3D::typeStr() );
    if ( emobj )
	emobj->ref();
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

    Geometry::BinIDSurface* geom = outputhor_->geometry().geometryElement();
    if ( !geom )
	return false;

    geom->setArray( hs_.start_, hs_.step_, depths_, true );
    ownsarray_ = false;
    return success;
}

} // namespace EM
