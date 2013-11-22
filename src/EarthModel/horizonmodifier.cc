/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "horizonmodifier.h"

#include "cubesampling.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "survinfo.h"


HorizonModifier::HorizonModifier( bool is2d )
    : tophor_(0)
    , bothor_(0)
    , topisstatic_(true)
    , is2d_(is2d)
    , iter_(0)
{
}


HorizonModifier::~HorizonModifier()
{
    delete iter_;
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


bool HorizonModifier::getNextNode( BinID& bid )
{
    return is2d_ ? getNextNode2D(bid) : getNextNode3D(bid);
}


bool HorizonModifier::getNextNode3D( BinID& bid )
{   
    if ( !iter_ )
    {
	HorSampling hrg;
	StepInterval<int> rrg = tophor_->geometry().rowRange();
	StepInterval<int> crg = tophor_->geometry().colRange();
	hrg.set( rrg, crg );

	rrg = bothor_->geometry().rowRange();
	crg = bothor_->geometry().colRange();
	hrg.include( BinID(rrg.start,crg.start) );
	hrg.include( BinID(rrg.stop,crg.stop) );

	iter_ = new HorSamplingIterator( hrg );
    }

    return iter_ ? iter_->next(bid) : false;
}


bool HorizonModifier::getNextNode2D( BinID& bid )
{
    if ( l2dkeys_.isEmpty() )
    {
	getLines( tophor_ );
	getLines( bothor_ );
    }

    if ( l2dkeys_.isEmpty() ) return false;

    if ( bid.crl() < trcrgs_[bid.inl()].start )
	bid.crl() = trcrgs_[bid.inl()].start;
    else
	bid.crl() += trcrgs_[bid.inl()].step;

    if ( bid.crl() > trcrgs_[bid.inl()].stop )
    {
	bid.inl()++;
	if ( bid.inl() >= l2dkeys_.size() )
	    return false;

	bid.crl() = trcrgs_[bid.inl()].start;
    }

    return true;
}


void HorizonModifier::getLines( const EM::Horizon* hor )
{
    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return;

    const EM::SectionID sid = hor2d->sectionID( 0 );
    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
    {
	const PosInfo::Line2DKey& l2dkey = hor2d->geometry().lineKey( ldx );
	const Geometry::Horizon2DLine* geom =
	    	hor2d->geometry().sectionGeometry(sid);
	if ( !geom ) return;

	const int lidx = l2dkeys_.indexOf( l2dkey );
	if ( lidx < 0 )
	{
	    l2dkeys_ += l2dkey;
	    trcrgs_ += geom->colRange( l2dkey );
	}
	else
	    trcrgs_[lidx].include( geom->colRange(l2dkey) );
    }
}


void HorizonModifier::doWork()
{
    BinID binid;
    while ( getNextNode(binid) )
    {
	float topz = mUdf(float), botz = mUdf(float);
	if ( is2d_ )
	{
	    topz = getDepth2D( tophor_, binid );
	    botz = getDepth2D( bothor_, binid );
	}
	else
	{
	    const EM::SubID subid = binid.toInt64();
	    topz = (float) tophor_->getPos( tophor_->sectionID(0), subid ).z;
	    botz = (float) bothor_->getPos( bothor_->sectionID(0), subid ).z;
	}

	if ( botz >= topz || mIsUdf(topz) || mIsUdf(botz) ) continue;

	if ( modifymode_ == Shift )
	    shiftNode( binid );
	else if ( modifymode_ == Remove )
	    removeNode( binid );
    }
}


float HorizonModifier::getDepth2D( const EM::Horizon* hor, const BinID& bid )
{
    if ( !hor ) return mUdf(float);

    const EM::SectionID sid = hor->sectionID(0);
    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return mUdf(float);

    return (float) hor2d->getPos( sid, l2dkeys_[bid.inl()], bid.crl() ).z;
}


void HorizonModifier::shiftNode( const BinID& bid )
{
    const EM::Horizon* statichor = topisstatic_ ? tophor_ : bothor_;
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;
    const float extrashift = SI().zStep() / (topisstatic_ ? 4.f : -4.f);
    
    if ( !is2d_ )
    {
	mDynamicCastGet(const EM::Horizon3D*,statichor3d,statichor)
	mDynamicCastGet(EM::Horizon3D*,dynamichor3d,dynamichor)
	if ( !statichor3d || !dynamichor3d ) return;

	float newz = statichor3d->getZ( bid );
	if ( !mIsUdf(newz) )
	    newz += extrashift;

	dynamichor3d->setZ( bid, newz, false );
    }
    else
    { 
	mDynamicCastGet(const EM::Horizon2D*,statichor2d,statichor)
	mDynamicCastGet(EM::Horizon2D*,dynamichor2d,dynamichor)
	if ( !statichor2d || !dynamichor2d ) return;

	float newz = (float) statichor2d->getPos( statichor->sectionID(0),
					  l2dkeys_[bid.inl()], bid.crl() ).z;
	if ( !mIsUdf(newz) )
	    newz += (float) extrashift;

	dynamichor2d->setPos( dynamichor->sectionID(0), l2dkeys_[bid.inl()],
			      bid.crl(), newz, false);
    }
}


void HorizonModifier::removeNode( const BinID& bid )
{
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;
    EM::SubID subid;
    if ( !is2d_ )
    {
	subid = bid.toInt64();
	dynamichor->unSetPos( dynamichor->sectionID(0), subid, false );
    }
    else
    { 
	mDynamicCastGet(EM::Horizon2D*,dynamichor2d,dynamichor)
	if ( !dynamichor2d ) return;

	dynamichor2d->setPos( dynamichor->sectionID(0), l2dkeys_[bid.inl()],
			      bid.crl(), mUdf(float), false );
    }
}
