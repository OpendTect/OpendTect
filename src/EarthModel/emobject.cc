/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.2 2002-05-22 06:17:03 kristofer Exp $";

#include "emobject.h"
#include "emhorizontransl.h"
#include "emwelltransl.h"
#include "ioobj.h"

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

    return res;
}


EarthModel::EMObject::EMObject( EMManager& emm_, const MultiID& id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
{}
