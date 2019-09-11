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
#include "picksetmanager.h"
#include "survinfo.h"
#include "taskrunner.h"
#include "uistrings.h"


EM::PolygonZChanger::PolygonZChanger( Pick::Set& ps, DBKey horid )
    : ps_(ps)
    , horid_(horid)
    , zval_(mUdf(float))
{}

EM::PolygonZChanger::PolygonZChanger( Pick::Set& ps, float zval )
    : ps_(ps)
    , zval_(zval)
{}


EM::PolygonZChanger::~PolygonZChanger()
{}


uiRetVal EM::PolygonZChanger::doWork( const TaskRunnerProvider& trprov ) const
{
    uiRetVal uiretval;
    const bool ishor = mIsUdf(zval_);

    if ( ishor )
    {
	const EM::Object* obj = EM::MGR().loadIfNotFullyLoaded( horid_, trprov);
	mDynamicCastGet(const EM::Horizon3D*,hor,obj)
	if ( !hor )
	    return uiretval.add( uiStrings::sNoValidData() );

	hor->ref();
	Pick::SetIter4Edit psiter( ps_ );
	const Geometry::BinIDSurface* geom = hor->geometry().geometryElement();
	while ( psiter.next() )
	{
	    const BinID bid = psiter.get().binID();
	    float zval = hor->getZ( bid );
	    if ( mIsUdf(zval) )
	    {
		if ( geom )
		    zval = geom->computePosition( Coord( bid.inl(),
						  bid.crl() ) ).z_;
	    }
	    if ( mIsUdf(zval) )
		psiter.removeCurrent();
	    else
		ps_.setZ( psiter.ID(), zval );
	}

	hor->unRef();
    }
    else
    {
	Pick::SetIter4Edit psiter( ps_ );
	psiter.get().setZ( zval_ );
    }

    return uiretval;
}
