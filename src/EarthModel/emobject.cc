/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.7 2003-05-26 09:16:58 kristofer Exp $";

#include "emobject.h"

#include "emhorizontransl.h"
#include "emwelltransl.h"
#include "emfaulttransl.h"
#include "emmanager.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ioman.h"

EarthModel::EMObject* EarthModel::EMObject::create( const IOObj& ioobj,
				    bool load, EarthModel::EMManager& manager,
				    BufferString& errmsg )
{
    EarthModel::EMObject* res = 0;
    const char* group = ioobj.group();

    MultiID id = ioobj.key();

    if ( !strcmp( group, EarthModelWellTranslator::keyword ))
	res = new EarthModel::Well( manager, id );
    else if ( !strcmp( group, EarthModelHorizonTranslator::keyword ))
	res = new EarthModel::Horizon( manager, id );
    else if ( !strcmp( group, EarthModelFaultTranslator::keyword ))
	res = new EarthModel::Fault( manager, id );

    return res;
}


void EarthModel::EMObject::ref() const
{
    const_cast<EarthModel::EMObject*>(this)->manager.ref(id());
}


void EarthModel::EMObject::unRef() const
{
    const_cast<EarthModel::EMObject*>(this)->manager.unRef(id());
}


void EarthModel::EMObject::unRefNoDel() const
{
    const_cast<EarthModel::EMObject*>(this)->manager.unRefNoDel(id());
}


EarthModel::EMObject::EMObject( EMManager& emm_, const MultiID& id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
{}


EarthModel::EMObject::~EMObject()
{
    deepErase( posattribs );
}


BufferString EarthModel::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( id_ );
    return ioobj ? ioobj->name() : BufferString("");
}


void EarthModel::EMObject::setPosAttrib( EarthModel::PosID& pid, int attr,
					 bool yn )
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( !yn ) return;
	attribs += attr;
	posattribs += new TypeSet<EarthModel::PosID>(1,pid);
    }
    else
    {
	TypeSet<EarthModel::PosID>& posids = *posattribs[idx];
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


bool EarthModel::EMObject::isPosAttrib( EarthModel::PosID& pid, int attr) const
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return false;

    TypeSet<EarthModel::PosID>& posids = *posattribs[idx];
    const int idy=posids.indexOf(pid);

    if ( idy==-1 )
	return false;

    return true;
}
