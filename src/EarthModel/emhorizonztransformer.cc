/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Jan 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emhorizonztransformer.cc,v 1.1 2010-01-15 09:33:47 cvsranojay Exp $";

#include "emhorizonztransformer.h"

#include "emhorizon.h"
#include "emhorizonztransform.h"
#include "horsampling.h"

namespace EM
{

HorizonZTransformer::HorizonZTransformer( const ZAxisTransform& zat,
					  const EM::Horizon& tarhor,
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


void HorizonZTransformer::setOutputHorizon( EM::Horizon& outhor )
{
    outputhor_ = &outhor;
    outputhor_->ref();
}


void HorizonZTransformer::setReferenceZ( float z )
{ refz_ = z; }


int HorizonZTransformer::nextStep()
{
    // TODO: handle multple sections

    if ( !iter_->next(bid_) )
	return Executor::Finished();

    EM::PosID posid( outputhor_->id() );
    int sidx = 0;
    const EM::SubID subid = bid_.getSerialized();
    float z = tarhor_.getPos( tarhor_.sectionID(sidx), subid ).z;
    if ( !mIsUdf(z) && !isforward_ )
	z -= refz_;

    float newz = zat_.transform( BinIDValue(bid_,z) );
    if ( !mIsUdf(newz) && isforward_ )
	newz += refz_;

    posid.setSectionID( EM::SectionID(sidx) );
    posid.setSubID( subid );
    outputhor_->setPos( posid, Coord3(0,0,newz), false );
    nrdone_++;
    return Executor::MoreToDo();
}

} 
