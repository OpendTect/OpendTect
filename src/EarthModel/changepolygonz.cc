/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
________________________________________________________________________

-*/

#include "changepolygonz.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "pickset.h"
#include "picksetmanager.h"
#include "survinfo.h"
#include "taskrunner.h"


ChangePolygonZ::ChangePolygonZ( Pick::Set& ps, DBKey horid )
    : ps_(ps)
    , horid_(horid)
    , constzval_(mUdf(double))
{}

ChangePolygonZ::ChangePolygonZ( Pick::Set& ps, double zval )
    : ps_(ps)
    , constzval_(zval)
{}


ChangePolygonZ::~ChangePolygonZ()
{}


bool ChangePolygonZ::doShift( const TaskRunnerProvider& trprov )
{
    const bool ishor = mIsUdf(constzval_);
    return ishor ? shiftPickToHorizon( trprov ) : shiftPickToConstant();
}


bool ChangePolygonZ::shiftPickToHorizon( const TaskRunnerProvider& trprov )
{
    const EM::Object* obj = EM::MGR().loadIfNotFullyLoaded( horid_, trprov );
    mDynamicCastGet(const EM::Horizon3D*,hor,obj)
    if ( !hor )
	return false;

    hor->ref();
    Pick::SetIter4Edit psiter( ps_ );
    const Geometry::BinIDSurface* geom =
	hor->geometry().geometryElement();
    while ( psiter.next() )
	{
	const BinID bid = psiter.get().binID();
	double zval = hor->getZ( bid );
	if ( mIsUdf(zval) )
	    {
	    if ( geom )
		zval = geom->computePosition( Coord(bid.inl(),bid.crl()) ).z_;
	    }
	if ( mIsUdf(zval) )
	    psiter.removeCurrent();
	else
	    ps_.setZ( psiter.ID(), zval );
	}

    hor->unRef();
    return true;
}


bool ChangePolygonZ::shiftPickToConstant()
{
    Pick::SetIter4Edit psiter( ps_ );
    while ( psiter.next() )
    {
	ps_.setZ( psiter.ID(), constzval_ );
    }
    return true;
}
