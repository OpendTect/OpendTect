/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.1 2002-05-16 14:18:55 kristofer Exp $";

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

    int id = ioobj.key().ID( ioobj.key().nrKeys()-1);

    if ( !strcmp( group, EarthModelWellTranslator::keyword ))
	res = new EarthModel::Well( manager, id );
    else if ( !strcmp( group, EarthModelHorizonTranslator::keyword ))
	res = new EarthModel::Horizon( manager, id );

    return res;
}


EarthModel::EMObject::EMObject( EMManager& emm_, int id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
{}
