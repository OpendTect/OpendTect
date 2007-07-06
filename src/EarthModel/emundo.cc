/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emundo.cc,v 1.1 2007-07-06 14:11:05 cvskris Exp $
________________________________________________________________________

-*/

#include "emundo.h"

#include "emmanager.h"
#include "emsurface.h"
#include "emsurfacerelations.h"
#include "errh.h"
#include "iopar.h"


const char* EM::SetPosUndoEvent::savedposstr = "Pos";


EM::SetPosUndoEvent::SetPosUndoEvent( const Coord3& oldpos_,
					    const EM::PosID& posid_ )
    : posid( posid_ )
    , savedpos( oldpos_ )
{}


const char* EM::SetPosUndoEvent::getStandardDesc() const
{ return "Set/Changed positon"; }


bool EM::SetPosUndoEvent::unDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid.objectID());
    if ( !emobject ) return false;

    const bool haschecks = emobject->enableGeometryChecks( false );
    
    bool res = false;
    const Coord3 proxy = emobject->getPos( posid );
    if ( emobject->setPos( posid, savedpos, false ) )
    {
	savedpos = proxy;
	res = true;
    }

    emobject->enableGeometryChecks( haschecks );
    return res;
}


bool EM::SetPosUndoEvent::reDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid.objectID());
    if ( !emobject ) return false;

    bool res = false;
    const bool haschecks = emobject->enableGeometryChecks(false);

    const Coord3 proxy = emobject->getPos( posid );
    if ( emobject->setPos( posid, savedpos, false ) )
    {
	savedpos = proxy;
	res = true;
    }

    emobject->enableGeometryChecks( haschecks );

    return res;
}


EM::SetPosAttribUndoEvent::SetPosAttribUndoEvent( const EM::PosID& pid,
							int attr, bool yesno )
    : posid( pid )
    , attrib( attr )
    , yn( yesno )
{}


const char* EM::SetPosAttribUndoEvent::getStandardDesc() const
{ return "Set/Changed positon attribute"; }

#define mSetPosAttribUndoEvenUndoRedo( arg ) \
    EMManager& manager = EM::EMM(); \
 \
    EMObject* emobject = manager.getObject(posid.objectID()); \
    if ( !emobject ) return true; \
 \
    emobject->setPosAttrib( posid, attrib, arg, false ); \
    return true

bool EM::SetPosAttribUndoEvent::unDo()
{
    mSetPosAttribUndoEvenUndoRedo( !yn );
}


bool EM::SetPosAttribUndoEvent::reDo()
{
    mSetPosAttribUndoEvenUndoRedo( yn );
}


EM::SurfaceRelationEvent::SurfaceRelationEvent( char prevrelation_,
    const EM::ObjectID& cuttedobject_, const EM::SectionID& cuttedsection_,
    const EM::ObjectID& cuttingobject_, const EM::SectionID& cuttingsection_ )
    : prevrelation( prevrelation_ )
    , cuttedobject( cuttedobject_ )
    , cuttedsection( cuttedsection_ )
    , cuttingobject( cuttingobject_ )
    , cuttingsection( cuttingsection_ )
{}


const char* EM::SurfaceRelationEvent::getStandardDesc() const
{
    return "Modified surface relation";
}


bool EM::SurfaceRelationEvent::unDo()
{
    return restoreRelation();
}


bool EM::SurfaceRelationEvent::reDo()
{
    return restoreRelation();
}


bool EM::SurfaceRelationEvent::restoreRelation()
{
    EM::EMManager& emm = EM::EMM();
    mDynamicCastGet( EM::Surface*, cuttedsurface, emm.getObject(cuttedobject));
    if ( !cuttedsurface ) return false;

    SurfaceRelations& relations = cuttedsurface->relations;

    const char currentrelation =
	relations.getRelation( cuttedsection, cuttingobject, cuttingsection );

    if ( !prevrelation )
	relations.removeRelation( cuttedsection, cuttingobject, cuttingsection,
				  false );
    else
	relations.setRelation( cuttedsection, cuttingobject, cuttingsection,
			       prevrelation>0, false );
    prevrelation = currentrelation;
    return true;
}



EM::PosIDChangeEvent::PosIDChangeEvent( const EM::PosID& from_,
				        const EM::PosID& to_,
				        const Coord3& tosprevpos_ )
    : from( from_ )
    , to( to_ )
    , savedpos( tosprevpos_ )
{ }


const char* EM::PosIDChangeEvent::getStandardDesc() const
{
    return "Changed posid";
}


bool EM::PosIDChangeEvent::unDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 frompos = emobject->getPos( from );
    emobject->changePosID( to, from, false );
    emobject->setPos( to, savedpos, false );
    savedpos = frompos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}


bool EM::PosIDChangeEvent::reDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 topos = emobject->getPos( to );
    emobject->changePosID( from, to, false );
    emobject->setPos( from, savedpos, false );
    savedpos = topos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}
