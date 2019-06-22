/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/


#include "horizonmodifier.h"

#include "trckeysampling.h"
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
}


bool HorizonModifier::setHorizons( const DBKey& dbky1, const DBKey& dbky2 )
{
    EM::ObjectManager& emmgr = EM::getMgr( dbky1 );
    mDynamicCastGet(EM::Horizon*,tophor,emmgr.getObject(dbky1))
    tophor_ = tophor;

    mDynamicCastGet(EM::Horizon*,bothor,emmgr.getObject(dbky2))
    bothor_ = bothor;

    deleteAndZeroPtr( iter_ );

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
	TrcKeySampling hrg;
	StepInterval<int> rrg = tophor_->geometry().rowRange();
	StepInterval<int> crg = tophor_->geometry().colRange();
	hrg.set( rrg, crg );

	rrg = bothor_->geometry().rowRange();
	crg = bothor_->geometry().colRange();
	hrg.include( BinID(rrg.start,crg.start) );
	hrg.include( BinID(rrg.stop,crg.stop) );

	iter_ = new TrcKeySamplingIterator( hrg );
    }

    if ( iter_ )
	bid = iter_->curBinID();

    return iter_ ? iter_->next() : false;
}


bool HorizonModifier::getNextNode2D( BinID& bid )
{
    if ( geomids_.isEmpty() )
    {
	getLines( tophor_ );
	getLines( bothor_ );
    }

    if ( geomids_.isEmpty() ) return false;

    if ( bid.crl() < trcrgs_[bid.inl()].start )
	bid.crl() = trcrgs_[bid.inl()].start;
    else
	bid.crl() += trcrgs_[bid.inl()].step;

    if ( bid.crl() > trcrgs_[bid.inl()].stop )
    {
	bid.inl()++;
	if ( bid.inl() >= geomids_.size() )
	    return false;

	bid.crl() = trcrgs_[bid.inl()].start;
    }

    return true;
}


void HorizonModifier::getLines( const EM::Horizon* hor )
{
    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return;

    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
    {
	const Pos::GeomID geomid = hor2d->geometry().geomID( ldx );
	const Geometry::Horizon2DLine* geom =
				hor2d->geometry().geometryElement();
	if ( !geom ) return;

	const int lidx = geomids_.indexOf( geomid );
	if ( lidx < 0 )
	{
	    geomids_ += geomid;
	    trcrgs_ += geom->colRangeForGeomID( geomid );
	}
	else
	    trcrgs_[lidx].include( geom->colRangeForGeomID(geomid) );
    }
}


void HorizonModifier::doWork()
{
    if ( !tophor_ || !bothor_ )
	return;

    tophor_->ref();
    bothor_->ref();
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
	    const EM::PosID posid = EM::PosID::getFromRowCol(binid);
	    topz = (float) tophor_->getPos( posid ).z_;
	    botz = (float) bothor_->getPos( posid ).z_;
	}

	if ( botz >= topz || mIsUdf(topz) || mIsUdf(botz) ) continue;

	if ( modifymode_ == Shift )
	    shiftNode( binid );
	else if ( modifymode_ == Remove )
	    removeNode( binid );
    }

    tophor_->unRef();
    bothor_->unRef();
}


float HorizonModifier::getDepth2D( const EM::Horizon* hor, const BinID& bid )
{
    if ( !hor ) return mUdf(float);

    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return mUdf(float);

    return (float) hor2d->getPos( geomids_[bid.inl()], bid.crl() ).z_;
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

	float newz = (float) statichor2d->getPos(
					  geomids_[bid.inl()], bid.crl() ).z_;
	if ( !mIsUdf(newz) )
	    newz += (float) extrashift;

	dynamichor2d->setZPos( geomids_[bid.inl()],
			      bid.crl(), newz, false);
    }
}


void HorizonModifier::removeNode( const BinID& bid )
{
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;
    EM::PosID posid;
    if ( !is2d_ )
    {
	posid = EM::PosID::getFromRowCol(bid);
	dynamichor->unSetPos( posid, false );
    }
    else
    {
	mDynamicCastGet(EM::Horizon2D*,dynamichor2d,dynamichor)
	if ( !dynamichor2d ) return;

	dynamichor2d->setZPos( geomids_[bid.inl()],
			      bid.crl(), mUdf(float), false );
    }
}
