/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.14 2003-11-24 08:39:52 kristofer Exp $";

#include "emobject.h"

#include "emhorizontransl.h"
#include "emfaulttransl.h"
#include "emsticksettransl.h"
#include "emmanager.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ioman.h"

EM::EMObject* EM::EMObject::create( const IOObj& ioobj, EM::EMManager& manager )
{
    EM::EMObject* res = 0;
    const char* group = ioobj.group();

    const EM::ObjectID id = EM::EMManager::multiID2ObjectID(ioobj.key());

    if ( !strcmp( group, EMHorizonTranslatorGroup::keyword ))
	res = new EM::Horizon( manager, id );
    else if ( !strcmp( group, EMFaultTranslatorGroup::keyword ))
	res = new EM::Fault( manager, id );
    else if ( !strcmp( group, EMStickSetTranslatorGroup::keyword ))
	res = new EM::StickSet( manager, id );

    return res;
}


void EM::EMObject::ref() const
{
    const_cast<EM::EMObject*>(this)->manager.ref(id());
}


void EM::EMObject::unRef() const
{
    const_cast<EM::EMObject*>(this)->manager.unRef(id());
}


void EM::EMObject::unRefNoDel() const
{
    const_cast<EM::EMObject*>(this)->manager.unRefNoDel(id());
}


EM::EMObject::EMObject( EMManager& emm_, const EM::ObjectID& id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
{}


EM::EMObject::~EMObject()
{
    deepErase( posattribs );
}


BufferString EM::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return ioobj ? ioobj->name() : BufferString("");
}


MultiID EM::EMObject::multiID() const
{
    MultiID res = getIOObjContext().stdSelKey();
    res.add(id());
    return res;
}


void EM::EMObject::setPosAttrib( EM::PosID& pid, int attr, bool yn )
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( !yn ) return;
	attribs += attr;
	posattribs += new TypeSet<EM::PosID>(1,pid);
    }
    else
    {
	TypeSet<EM::PosID>& posids = *posattribs[idx];
	const int idy=posids.indexOf(pid);

	if ( idy==-1 )
	{
	    if ( !yn ) return;
	    posids += pid;
	}
	else if ( !yn )
	{
	    posids.remove(idy);
	    if ( !posids.size() )
	    {
		delete posattribs[idx];
		posattribs.remove( idx );
		attribs.remove( idx );
	    }
	}
    }
}


bool EM::EMObject::isPosAttrib( EM::PosID& pid, int attr ) const
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return false;

    TypeSet<EM::PosID>& posids = *posattribs[idx];
    const int idy=posids.indexOf(pid);

    if ( idy==-1 )
	return false;

    return true;
}
