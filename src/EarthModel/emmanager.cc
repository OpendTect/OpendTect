/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.15 2003-07-29 13:12:22 nanne Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emfaulttransl.h"
#include "emhistory.h"
#include "emhorizontransl.h"
#include "emobject.h"
#include "emwelltransl.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ptrman.h"
#include "uidobjset.h"


EM::EMManager& EM::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EM::EMManager;
    return *emm;
}

EM::EMManager::EMManager()
    : history_( *new EM::History(*this) )
{
    init();
}


EM::EMManager::~EMManager()
{
    deepErase( objects );
    delete &history_;
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


void EM::EMManager::init()
{ } 


MultiID EM::EMManager::add( EM::EMManager::Type type, const char* name )
{
    if ( type==EM::EMManager::Hor )
    {
	CtxtIOObj* ctio = new CtxtIOObj(EMHorizonTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();

	if ( !ctio->ioobj ) return -1;
	MultiID key = ctio->ioobj->key();

	Horizon* hor = new Horizon( *this, key );
	PtrMan<Executor> exec = hor->saver();
	exec->execute();
	objects += hor;
	refcounts += 0;

	return key;
    }


    if ( type==EM::EMManager::Fault )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EMFaultTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();

	if ( !ctio->ioobj ) return -1;
	MultiID key = ctio->ioobj->key();

	EM::Fault* fault = new EM::Fault( *this, key );
	PtrMan<Executor> exec = fault->saver();
	exec->execute();
	objects += fault;
	refcounts += 0;

	return key;
    }


    if ( type==EM::EMManager::Well )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EMWellTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();
	if ( !ctio->ioobj ) return -1;

	MultiID key = ctio->ioobj->key();

	EM::Well* well = new EM::Well( *this, key );
	PtrMan<Executor> exec = well->saver();
	exec->execute();

	objects += well;
	refcounts += 0;

	return key;
    }

    return -1;
}


EM::EMObject* EM::EMManager::getObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const MultiID& id ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getEMObject( int idx ) const
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


EM::EMObject* EM::EMManager::getEMObject( int idx )
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


void EM::EMManager::removeObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    delete objects[idx];
	    objects.remove( idx );
	    refcounts.remove( idx );
	    return;
	}
    }
}


void EM::EMManager::ref( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    refcounts[idx]++;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRef( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    if ( !refcounts[idx] )
		removeObject( id );

	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRefNoDel( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


bool EM::EMManager::createObject( const MultiID& id )
{
    EMObject* obj = getObject( id );
    if ( obj ) return true;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return false;

    const char* grpname = ioobj->group();
    if ( !strcmp(grpname,EMWellTranslator::keyword) )
    {
	EM::Well* well = new EM::Well( *this, id );
	objects += well;
	refcounts += 0;
	return true;
    }

    if ( !strcmp(grpname,EMHorizonTranslator::keyword) )
    {
	EM::Horizon* hor = new EM::Horizon( *this, id );
	objects += hor;
	refcounts += 0;
	return true;
    }

    if ( !strcmp(grpname,EMFaultTranslator::keyword) )
    {
	EM::Fault* fault = new EM::Fault( *this, id );
	objects += fault;
	refcounts += 0;
	return true;
    }

    return false;
}


Executor* EM::EMManager::load( const MultiID& id )
{
    EMObject* obj = getObject( id );
    if ( obj ) return obj->loader();
    
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return 0;

    const char* grpname = ioobj->group();
    if ( !strcmp( grpname, EMWellTranslator::keyword ))
    {
	EM::Well* well = new EM::Well( *this, id );
	objects += well;
	refcounts += 0;
	return well->loader();
    }

    if ( !strcmp( grpname, EMHorizonTranslator::keyword ))
    {
	EM::Horizon* hor = new EM::Horizon( *this, id );
	objects += hor;
	refcounts += 0;
	return hor->loader();
    }


    if ( !strcmp( grpname, EMFaultTranslator::keyword ))
    {
	EM::Fault* fault = new EM::Fault( *this, id );
	objects += fault;
	refcounts += 0;
	return fault->loader();
    }

    return 0;
}


bool EM::EMManager::isLoaded( const MultiID& id ) const
{
    const EMObject* obj = getObject( id );
    return obj ? obj->isLoaded() : false;
}


void EM::EMManager::getSurfaceData( const MultiID& id, EM::SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;

    const char* grpname = ioobj->group();
    if ( !strcmp( grpname, EMHorizonTranslator::keyword ))
    {
	EM::Horizon* hor = new EM::Horizon( *this, id );
	dgbEMHorizonTranslator tr;
	if ( !tr.startRead( *ioobj, *hor ) )
	    return;

	delete hor;
	const EM::SurfaceIOData& newsd = tr.selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.patches, newsd.patches );
	deepCopy( sd.valnames, newsd.valnames );
    }
}
