/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizonmodifier.cc,v 1.4 2008-11-25 15:35:22 cvsbert Exp $";


#include "horizonmodifier.h"

#include "cubesampling.h"
#include "emhorizon.h"
#include "emmanager.h"


HorizonModifier::HorizonModifier()
    : tophor_(0)
    , bothor_(0)
    , topisstatic_(true)
{
}


HorizonModifier::~HorizonModifier()
{
    if ( tophor_ ) tophor_->unRef();
    if ( bothor_ ) bothor_->unRef();
}


bool HorizonModifier::setHorizons( const MultiID& mid1, const MultiID& mid2 )
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid1 );
    mDynamicCastGet(EM::Horizon*,tophor,EM::EMM().getObject(objid))
    tophor_ = tophor;

    objid = EM::EMM().getObjectID( mid2 );
    mDynamicCastGet(EM::Horizon*,bothor,EM::EMM().getObject(objid))
    bothor_ = bothor;

    if ( tophor_ && bothor_ )
    {
	tophor_->ref();
	bothor_->ref();
    }

    return tophor_ && bothor_;
}


void HorizonModifier::setMode( ModifyMode mode )
{
    modifymode_ = mode;
}


void HorizonModifier::setStaticHorizon( bool top )
{
    topisstatic_ = top;
}


void HorizonModifier::getHorSampling( HorSampling& hrg )
{   
    StepInterval<int> rrg = tophor_->geometry().rowRange();
    StepInterval<int> crg = tophor_->geometry().colRange();
    hrg.set( rrg, crg );

    rrg = bothor_->geometry().rowRange();
    crg = bothor_->geometry().colRange();
    hrg.include( BinID(rrg.start,crg.start) );
    hrg.include( BinID(rrg.stop,crg.stop) );
}


void HorizonModifier::doWork()
{
    HorSampling hrg;
    getHorSampling( hrg );
    HorSamplingIterator iterator( hrg );

    BinID binid;
    while ( iterator.next(binid) )
    {
	const EM::SubID subid = binid.getSerialized();
	const float topz = tophor_->getPos( tophor_->sectionID(0), subid ).z;
	const float botz = bothor_->getPos( bothor_->sectionID(0), subid ).z;
	if ( botz >= topz || mIsUdf(topz) || mIsUdf(botz) ) continue;

	if ( modifymode_ == Shift )
	    shiftNode( subid );
	else if ( modifymode_ == Remove )
	    removeNode( subid );
    }
}


void HorizonModifier::shiftNode( const EM::SubID& subid )
{
    const EM::Horizon* statichor = topisstatic_ ? tophor_ : bothor_;
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;

    const float extrashift = topisstatic_ ? 0.001 : -0.001;
    const float newz = statichor->getPos( statichor->sectionID(0), subid ).z;
    dynamichor->setPos( dynamichor->sectionID(0), subid,
	    		Coord3(0,0,newz+extrashift), false );
}


void HorizonModifier::removeNode( const EM::SubID& subid )
{
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;
    dynamichor->unSetPos( dynamichor->sectionID(0), subid, false );
}
