/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.6 2003-04-22 11:01:52 kristofer Exp $";

#include "emfault.h"

#include "emfaulttransl.h"
#include "emhistoryimpl.h"
#include "geomgridsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

EarthModel::Fault::Fault(EarthModel::EMManager & emm_, const MultiID& mid_)
    : EMObject( emm_, mid_ )
    , surface ( 0 )
{ }


EarthModel::Fault::~Fault()
{
    delete surface;
}


EarthModel::PosID EarthModel::Fault::setPos(const RowCol& node,
					    const Coord3& pos,
       					    bool addtohistory	)
{
    if ( !surface ) surface = new Geometry::GridSurfaceImpl;
    const Coord3 oldpos = surface->getGridPos( node );
    if ( oldpos!=pos )
    {
	surface->setGridPos( node, pos );
	if ( addtohistory )
	{
	    const Geometry::PosID posid = surface->getPosId(node);
	    HistoryEvent* history = new SetPosHistoryEvent( oldpos, pos,
		    			EarthModel::PosID(id(),0,posid) );

	}
    }
	
    return EarthModel::PosID( id(), 0, surface->getPosId(node));    
}    


bool EarthModel::Fault::setPos( const EarthModel::PosID& posid,
				const Coord3& newpos, bool addtohistory )
{
    if ( posid.emObject()!=id() ) return false;
    setPos( Geometry::GridSurface::getGridNode(posid.subID()), newpos,
	    addtohistory );
    return true;
}


Coord3 EarthModel::Fault::getPos(const EarthModel::PosID& posid) const
{
    return surface->getPos( posid.subID() );
}	


Executor* EarthModel::Fault::loader()
{
    if ( surface ) delete surface;
    surface = new Geometry::GridSurfaceImpl;

    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
Executor* EarthModel::Fault::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
