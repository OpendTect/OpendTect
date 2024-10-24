/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon2dextender.h"

#include "horizon2dtracker.h"
#include "math2.h"
#include "survinfo.h"


// MPE::Horizon2DExtenderBase

mImplFactory1Param( MPE::Horizon2DExtenderBase, EM::Horizon2D&,
		    MPE::Horizon2DExtenderBase::factory );

MPE::Horizon2DExtenderBase::Horizon2DExtenderBase( EM::Horizon2D& hor )
    : SectionExtender()
    , hor2d_(hor)
{}


MPE::Horizon2DExtenderBase::~Horizon2DExtenderBase()
{}


MPE::Horizon2DExtenderBase*
	MPE::Horizon2DExtenderBase::createInstance( EM::Horizon2D& hor )
{
    const auto& horextfact = factory();
    BufferString typestr = horextfact.getDefaultName();
    if ( !horextfact.hasName(typestr.buf()) && !horextfact.isEmpty() )
	typestr = horextfact.getNames().last()->buf();

    return horextfact.create( typestr.buf(), hor );
}


void MPE::Horizon2DExtenderBase::setAngleThreshold( float rad )
{
    anglethreshold_ = cos( rad );
}


float MPE::Horizon2DExtenderBase::getAngleThreshold() const
{ return Math::ACos(anglethreshold_); }


void MPE::Horizon2DExtenderBase::setDirection( const TrcKeyValue& dir )
{
    direction_ = dir;
    xydirection_ =
	SI().transform(BinID::noStepout()) - SI().transform(dir.tk_.position());
    const double abs = xydirection_.abs();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


const TrcKeyValue* MPE::Horizon2DExtenderBase::getDirection() const
{
    return &direction_;
}


void MPE::Horizon2DExtenderBase::setGeomID( const Pos::GeomID& geomid )
{
    geomid_ = geomid;
}


Pos::GeomID MPE::Horizon2DExtenderBase::geomID() const
{
    return geomid_;
}


int MPE::Horizon2DExtenderBase::nextStep()
{
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	addNeighbor( false, startpos_[idx] );
	addNeighbor( true, startpos_[idx] );
    }

    return Finished();
}


void MPE::Horizon2DExtenderBase::addNeighbor( bool upwards, const TrcKey& src )
{
    const StepInterval<int> colrange = hor2d_.geometry().colRange( geomid_ );
    TrcKey neighbor = src;
    const TrcKey& cstneighbor = const_cast<const TrcKey&>( neighbor );
    neighbor.setTrcNr( cstneighbor.trcNr() +
		       (upwards ? colrange.step_ : -colrange.step_) );
    if ( !colrange.includes(cstneighbor.trcNr(),false) )
	return;

    const TrcKeyZSampling& boundary = getExtBoundary();
    if ( !boundary.isEmpty() && !boundary.hsamp_.includes(cstneighbor) )
	return;

    const bool hasz = hor2d_.hasZ( cstneighbor );
    const bool isintersection = hor2d_.isAttrib(
			cstneighbor, EM::EMObject::sIntersectionNode() );
    if ( hasz && !isintersection )
	return;

    hor2d_.setZAndNodeSourceType(
	cstneighbor, hor2d_.getZ(src), true, EM::EMObject::Auto );
    addTarget( cstneighbor, src );
}


float MPE::Horizon2DExtenderBase::getDepth( const TrcKey& src,
					    const TrcKey& /* dest */) const
{
    return hor2d_.getZ( src );
}


// MPE::Horizon2DExtender

MPE::Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor )
    : Horizon2DExtenderBase(hor)
{}


MPE::Horizon2DExtender::~Horizon2DExtender()
{}
