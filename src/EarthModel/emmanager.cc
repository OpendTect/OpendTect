/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.1 2002-05-16 14:18:55 kristofer Exp $";

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
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Misc)->id) );
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


int EarthModel::EMManager::add( EarthModel::EMManager::Type type,
				const char* name )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Misc)->id) );

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
	int id = key.ID(key.nrKeys()-1);

	Horizon* hor = new Horizon( *this, id );
	PtrMan<Executor> exec = hor->saver();
	exec->execute();

	return id;
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
	int id = key.ID(key.nrKeys()-1);

	EarthModel::Well* well = new EarthModel::Well( *this, id );
	PtrMan<Executor> exec = well->saver();
	exec->execute();

	return id;
    }

    return -1;
}


EarthModel::EMObject* EarthModel::EMManager::getObject( int id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EarthModel::EMObject* EarthModel::EMManager::getObject( int id ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}
