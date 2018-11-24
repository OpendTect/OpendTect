/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Jan 2010
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
    if ( posid.isInvalid() )
	return Executor::Finished();

    float z = (float) tarhor_.getPos( posid ).z_;
    if ( !isforward_ && !mIsUdf(z) )
	z -= refz_;

    const auto gs = tarhor_.geomSystem();
    const BinID bid( posid.getBinID() );
    TrcKey tk( gs, bid );
    if ( is2D(gs) )
	tk.setLineNr( bid.lineNr() );

    float newz = zat_.transformTrc( tk, z );
    if ( isforward_ && !mIsUdf(newz) )
	newz += refz_;

    outputhor_->setPos( posid, Coord3(0,0,newz), false );
    nrdone_++;
    return Executor::MoreToDo();
}

} // namespace EM
