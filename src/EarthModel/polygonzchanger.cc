/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
________________________________________________________________________

-*/

#include "polygonzchanger.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "pickset.h"
#include "picklocation.h"
#include "picksetmgr.h"
#include "uistrings.h"


EM::PolygonZChanger::PolygonZChanger( Pick::Set& ps, const MultiID& horid )
    : ps_(ps)
    , horid_(horid)
    , zval_(mUdf(float))
{}

EM::PolygonZChanger::PolygonZChanger( Pick::Set& ps, float zval )
    : ps_(ps)
    , horid_(MultiID::udf())
    , zval_(zval)
{}


EM::PolygonZChanger::~PolygonZChanger()
{}


uiRetVal EM::PolygonZChanger::doWork( TaskRunner& trprov )
{
    uiRetVal uiretval;
    const bool ishor = mIsUdf(zval_);
    ObjectSet<Pick::Location> positions;
    ps_.getLocations( positions, 0 );

    if ( ishor )
    {
	ConstRefMan<EM::EMObject> obj =
			EM::EMManager().loadIfNotFullyLoaded( horid_, &trprov );
	mDynamicCastGet(const EM::Horizon3D*,hor,obj.ptr())
	if ( !hor )
	    return uiretval.add( uiStrings::sNoValidData() );

	const Geometry::BinIDSurface* geom =
			hor->geometry().sectionGeometry( hor->sectionID(0) );
	for ( int idy=0; idy<positions.size(); idy++ )
	{
	    const BinID bid = positions[idy]->binID();
	    float zval = hor->getZ( bid );

	    if ( mIsUdf(zval) )
	    {
		if ( geom )
		    zval =
			geom->computePosition( Coord(bid.inl(),bid.crl()) ).z;
	    }
	    if ( !mIsUdf(zval) )
	    {
		positions[idy]->setZ( zval );
		reportChange( Pick::SetMgr::ChangeData::Changed, idy );
	    }
	    else
	    {
		ps_.removeSingle( idy );
		reportChange( Pick::SetMgr::ChangeData::ToBeRemoved, idy );
		idy--;
	    }
	}

    }
    else
    {
	for ( int idy=0; idy<positions.size(); idy++ )
	{
	    positions[idy]->setZ( zval_ );
	    reportChange( Pick::SetMgr::ChangeData::Changed, idy );
	}
    }

    return uiretval;
}


void EM::PolygonZChanger::reportChange( Pick::SetMgr::ChangeData::Ev ev,
					int idy )
{
    Pick::SetMgr::ChangeData cd( ev, &ps_, idy );
    Pick::Mgr().reportChange( 0, cd );
}
