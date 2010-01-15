/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Jan 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emhorizonztransformer.cc,v 1.2 2010-01-15 09:51:19 cvsnanne Exp $";

#include "emhorizonztransformer.h"

#include "emhorizon.h"
#include "emhorizonztransform.h"
#include "horsampling.h"

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
    HorSampling hs;
    hs.set( tarhor_.geometry().rowRange(), tarhor_.geometry().colRange() );
    totalnr_ = hs.totalNr();
    iter_ = new HorSamplingIterator( hs );
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


// TODO: handle multple sections
int HorizonZTransformer::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished();

    int sidx = 0;
    const SubID subid = bid_.getSerialized();
    float z = tarhor_.getPos( tarhor_.sectionID(sidx), subid ).z;
    if ( !mIsUdf(z) && !isforward_ )
	z -= refz_;

    float newz = zat_.transform( BinIDValue(bid_,z) );
    if ( !mIsUdf(newz) && isforward_ )
	newz += refz_;

    SectionID sid = outputhor_->sectionID( sidx );
    outputhor_->setPos( sid, subid, Coord3(0,0,newz), false );
    nrdone_++;
    return Executor::MoreToDo();
}

} // namespace EM 
