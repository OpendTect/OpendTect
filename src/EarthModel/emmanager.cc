/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.3 2002-05-22 06:17:03 kristofer Exp $";

#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "uidobjset.h"
#include "iodir.h"
#include "ptrman.h"
#include "ctxtioobj.h"
#include "emhorizontransl.h"
#include "emwelltransl.h"
#include "executor.h"

EarthModel::EMManager::EMManager()
{
    init();
}


EarthModel::EMManager::~EMManager()
{
    deepErase( objects );
}

void EarthModel::EMManager::init()
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Mdl)->id) );
    const UserIDObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	EMObject* emo = EMObject::create( *ioobjs[idx], false, *this, errmsg );
	if ( emo )
	    objects += emo;
	else
	    cerr << (const char*) errmsg;
    }
}


MultiID EarthModel::EMManager::add( EarthModel::EMManager::Type type,
				const char* name )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Mdl)->id) );

    if ( type==EarthModel::EMManager::Hor )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EarthModelHorizonTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();

	if ( !ctio->ioobj ) return -1;
	MultiID key = ctio->ioobj->key();

	Horizon* hor = new Horizon( *this, key );
	PtrMan<Executor> exec = hor->saver();
	exec->execute();

	return key;
    }

    if ( type==EarthModel::EMManager::Well )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EarthModelWellTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();
	if ( !ctio->ioobj ) return -1;

	MultiID key = ctio->ioobj->key();

	EarthModel::Well* well = new EarthModel::Well( *this, key );
	PtrMan<Executor> exec = well->saver();
	exec->execute();

	return key;
    }

    return -1;
}


EarthModel::EMObject* EarthModel::EMManager::getObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EarthModel::EMObject* EarthModel::EMManager::getObject( const MultiID& id ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}
