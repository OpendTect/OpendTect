/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.5 2002-09-20 08:48:51 nanne Exp $";

#include "emobject.h"
#include "emhorizontransl.h"
#include "emwelltransl.h"
#include "emfaulttransl.h"
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


EarthModel::EMObject::EMObject( EMManager& emm_, const MultiID& id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
{}


BufferString EarthModel::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( id_ );
    return ioobj ? ioobj->name() : BufferString("");
}
