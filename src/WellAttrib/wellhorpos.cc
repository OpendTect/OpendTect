/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellhorpos.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "welltrack.h"


WellHorIntersectFinder::WellHorIntersectFinder( const Well::Track& tr,
						const Well::D2TModel* d2t )
    : track_(tr)
    , d2t_(d2t)
{
}


WellHorIntersectFinder::~WellHorIntersectFinder()
{
}


void WellHorIntersectFinder::setHorizon( const EM::ObjectID& emid )
{
    hor2d_ = nullptr; hor3d_ = nullptr;
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj)
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj)
    hor3d_ = hor3d; hor2d_ = hor2d;
}


float WellHorIntersectFinder::findZIntersection() const
{
    const float zstep = SI().zStep();
    const Interval<float>& dahrg = track_.dahRange();
    float zstart = d2t_ ? d2t_->getTime( dahrg.start_, track_ ) : dahrg.start_;
    float zstop = d2t_ ? d2t_->getTime( dahrg.stop_, track_ ) : dahrg.stop_;
    zstart = mMAX( SI().zRange(true).start_, zstart );
    zstop = mMIN( SI().zRange(true).stop_, zstop );

    float zval = zstart;
    bool isabove = true;
    bool firstvalidzfound = false;

    while ( zval < zstop )
    {
	const float dah = d2t_ ? d2t_->getDah( zval, track_ ) : zval;
	const Coord3& crd = track_.getPos( dah );
	const float horz = intersectPosHor( crd );

	if ( mIsUdf( horz ) )
	{
	    zval += zstep;
	    continue;
	}

	if ( !firstvalidzfound )
	{
	    isabove = zval >= horz;
	    firstvalidzfound = true;
	}

	if ( ( isabove && horz >= zval ) || ( !isabove && horz <= zval ) )
	    return horz;

	zval += zstep;
    }
    return mUdf( float );
}


float WellHorIntersectFinder::intersectPosHor( const Coord3& pos ) const
{
    const BinID& bid = SI().transform( pos );
    if ( !SI().isInside( bid, true ) )
       return mUdf( float );

    if ( hor3d_ )
    {
	const EM::SubID subid = bid.toInt64();
	const Coord3& horpos = hor3d_->getPos( subid );
	const BinID horbid = SI().transform( horpos );
	if ( bid == horbid )
            return (float)horpos.z_;
    }
    else if ( hor2d_ )
	return hor2d_->getZValue( pos );

    return  mUdf( float );
}
