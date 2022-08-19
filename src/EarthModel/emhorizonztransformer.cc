/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emhorizonztransformer.h"

#include "emhorizon2d.h"
#include "emhorizonztransform.h"
#include "binidvalue.h"

namespace EM
{

HorizonZTransformer::HorizonZTransformer( const ZAxisTransform& zat,
					  const Horizon& tarhor,
					  bool isforward )
    : Executor("Transforming horizon")
    , tarhor_(tarhor)
    , zat_(zat)
    , isforward_(isforward)
    , refz_(0)
    , nrdone_(0)
    , outputhor_(0)
{
    iter_ = tarhor_.createIterator();
    totalnr_ = iter_->maximumSize();
}


HorizonZTransformer::~HorizonZTransformer()
{
    delete iter_;
    if ( outputhor_ ) outputhor_->unRef();
}


void HorizonZTransformer::setOutputHorizon( Horizon& outhor )
{
    outputhor_ = &outhor;
    outputhor_->ref();
}


void HorizonZTransformer::setReferenceZ( float z )
{ refz_ = z; }


int HorizonZTransformer::nextStep()
{
    PosID posid = iter_->next();
    if ( posid.isUdf() )
	return Executor::Finished();

    float z = (float) tarhor_.getPos( posid ).z;
    if ( !isforward_ && !mIsUdf(z) )
	z -= refz_;

    const Pos::IdxPair bid = BinID::fromInt64( posid.subID() );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,&tarhor_)
    const TrcKey tk( bid, hor2d );

    float newz = zat_.transformTrc( tk, z );
    if ( isforward_ && !mIsUdf(newz) )
	newz += refz_;

    outputhor_->setPos( posid.subID(), Coord3(0,0,newz), false );
    nrdone_++;
    return Executor::MoreToDo();
}

} // namespace EM
